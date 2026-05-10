/*
Copyright 2020-2026 Dario Mambro

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include "Attachments.h"
#include "WrappedBoolParameter.h"
#include "oversimple/Oversampling.hpp"
#include <vector>

// APVTS-driven oversampling parameters and attachments.
//
// Oversampling order and linear-phase mode are exposed as a single pair of
// parameters that drive N independent oversimple::TOversampling streams kept
// in lockstep (e.g. wet+dry, or wet+dry+sidechain). The attachment owns the
// cross-thread synchronisation: when the user changes either parameter it
// takes the caller-supplied mutex, suspends the processor, pushes the new
// settings into every managed stream, reports the resulting latency back to
// JUCE, then resumes the processor.

struct OversamplingParameters
{
  juce::RangedAudioParameter* order;
  WrappedBoolParameter linearPhase;
};

template<class Mutex>
class OversamplingAttachments
{
  std::unique_ptr<FloatAttachment> orderAttachment;
  std::unique_ptr<BoolAttachment> linearPhaseAttachment;
  std::vector<oversimple::TOversampling<double>*> streams;

public:
  OversamplingAttachments(
    OversamplingParameters& parameters,
    juce::AudioProcessorValueTreeState& apvts,
    juce::AudioProcessor* processor,
    oversimple::OversamplingSettings* oversamplingSettings,
    Mutex* oversamplingMutex,
    std::vector<oversimple::TOversampling<double>*> streamsToManage)
    : streams(std::move(streamsToManage))
  {
    auto const apply =
      [this, processor, oversamplingSettings, oversamplingMutex] {
        auto const guard = std::lock_guard<Mutex>(*oversamplingMutex);

        processor->suspendProcessing(true);

        for (auto* s : streams) {
          s->setUseLinearPhase(oversamplingSettings->isUsingLinearPhase);
          s->setOrder(oversamplingSettings->order);
        }

        processor->setLatencySamples(
          static_cast<int>(streams.front()->getLatency()));

        processor->suspendProcessing(false);
      };

    linearPhaseAttachment = std::make_unique<BoolAttachment>(
      apvts,
      parameters.linearPhase.getID(),
      [this, oversamplingSettings, apply] {
        if (!linearPhaseAttachment) {
          return;
        }
        oversamplingSettings->isUsingLinearPhase =
          linearPhaseAttachment->getValue();
        apply();
      });

    orderAttachment = std::make_unique<FloatAttachment>(
      apvts,
      parameters.order->paramID,
      [this, oversamplingSettings, apply] {
        if (!orderAttachment) {
          return;
        }
        oversamplingSettings->order =
          static_cast<uint32_t>(orderAttachment->getValue());
        apply();
      },
      juce::NormalisableRange<float>(0.f, 5.f, 1.f));
  }
};
