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

struct OversamplingParameters
{
  RangedAudioParameter* order;
  WrappedBoolParameter linearPhase;
};

template<typename Scalar>
class OversamplingAttachments
{
  std::unique_ptr<FloatAttachment> orderAttachment;
  std::unique_ptr<BoolAttachment> linearPhaseAttachment;

public:
  OversamplingAttachments(
    OversamplingParameters& parameters,
    AudioProcessorValueTreeState& apvts,
    std::function<void(int order, bool linearPhase)> updateLatency)
    : updateLatency(std::move(updateLatency))
  {
    linearPhaseAttachment = std::make_unique<BoolAttachment>(
      apvts, parameters.linearPhase.getID(), [this] { update(); });

    orderAttachment = std::make_unique<FloatAttachment>(
      apvts,
      parameters.order->paramID,
      [this] { update(); },
      NormalisableRange<float>(0.f, 5.f, 1.f));
  }

private:

  void update() {
    if (!linearPhaseAttachment || !orderAttachment) {
      return;
    }
    auto const linearPhase = linearPhaseAttachment->getValue();
    auto const order = (int)orderAttachment->getValue();
    updateLatency(order, linearPhase);
  }

  std::function<void(int order, bool linearPhase)> updateLatency;
};