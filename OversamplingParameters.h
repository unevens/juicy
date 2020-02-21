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
#include "oversimple/AsyncOversampling.hpp"

struct OversamplingParameters
{
  RangedAudioParameter* order;
  WrappedBoolParameter linearPhase;
};

class OversamplingAttachments
{
  oversimple::AsyncOversampling* asyncOversampling;

  std::unique_ptr<FloatAttachment> orderAttachment;
  std::unique_ptr<BoolAttachment> linearPhaseAttachment;

public:
  OversamplingAttachments(AudioProcessorValueTreeState& apvts,
                          oversimple::AsyncOversampling& asyncOversampling_,
                          OversamplingParameters& parameters)
    : asyncOversampling(&asyncOversampling_)
  {
    linearPhaseAttachment = std::make_unique<BoolAttachment>(
      apvts, parameters.linearPhase.getID(), [this]() {
        if (!linearPhaseAttachment) {
          return;
        }
        bool value = linearPhaseAttachment->getValue();
        asyncOversampling->submitMessage(
          [=](oversimple::OversamplingSettings& oversampling) {
            oversampling.linearPhase = value;
          });
      });

    orderAttachment = std::make_unique<FloatAttachment>(
      apvts,
      parameters.order->paramID,
      [this]() {
        if (!orderAttachment) {
          return;
        }
        int order = (int)std::round(orderAttachment->getValue());
        asyncOversampling->submitMessage(
          [=](oversimple::OversamplingSettings& oversampling) {
            oversampling.order = order;
          });
      },
      NormalisableRange<float>(0.f, 5.f, 1.f));
  }
};