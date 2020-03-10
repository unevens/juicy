/*
Copyright 2020 Dario Mambro

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

/**
 * Parameters and attachments to use the Oversimple oversampling wrappers
 * https://github.com/unevens/oversimple/blob/master/oversimple/Oversampling.hpp
 */
struct OversamplingParameters
{
  RangedAudioParameter* order;
  WrappedBoolParameter linearPhase;
};

template<typename Scalar = double>
class OversamplingAttachments
{
  std::unique_ptr<FloatAttachment> orderAttachment;
  std::unique_ptr<BoolAttachment> linearPhaseAttachment;

public:
  OversamplingAttachments(
    OversamplingParameters& parameters,
    AudioProcessorValueTreeState& apvts,
    AudioProcessor* processor,
    std::unique_ptr<oversimple::Oversampling<Scalar>>* oversampling,
    oversimple::OversamplingSettings* oversamplingSettings,
    std::mutex* oversamplingMutex)
  {
    linearPhaseAttachment = std::make_unique<BoolAttachment>(
      apvts,
      parameters.linearPhase.getID(),
      [this,
       processor,
       oversampling,
       oversamplingSettings,
       oversamplingMutex]() {
        if (!linearPhaseAttachment) {
          return;
        }

        const std::lock_guard<std::mutex> lock(*oversamplingMutex);
        processor->suspendProcessing(true);

        oversamplingSettings->linearPhase = linearPhaseAttachment->getValue();

        *oversampling = std::make_unique<oversimple::Oversampling<Scalar>>(
          *oversamplingSettings);

        processor->suspendProcessing(false);
      });

    orderAttachment = std::make_unique<FloatAttachment>(
      apvts,
      parameters.order->paramID,
      [this,
       processor,
       oversampling,
       oversamplingSettings,
       oversamplingMutex]() {
        if (!orderAttachment) {
          return;
        }

        const std::lock_guard<std::mutex> lock(*oversamplingMutex);
        processor->suspendProcessing(true);

        oversamplingSettings->order = orderAttachment->getValue();

        *oversampling = std::make_unique<oversimple::Oversampling<Scalar>>(
          *oversamplingSettings);

        processor->suspendProcessing(false);
      },
      NormalisableRange<float>(0.f, 5.f, 1.f));
  }
};