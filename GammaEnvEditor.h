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
#include "Linkables.h"
#include <JuceHeader.h>
#include <array>

/**
 * Linkable parameters and a Component to control a GammaEnv envelope follower
 * in either left/right or mid/side stereo. See
 * https://github.com/unevens/audio-dsp/blob/master/adsp/GammaEnv.hpp
 */

struct GammaEnvParameters
{
  LinkableParameter<AudioParameterFloat> attack;
  LinkableParameter<AudioParameterFloat> release;
  LinkableParameter<AudioParameterFloat> attackDelay;
  LinkableParameter<AudioParameterFloat> releaseDelay;
  LinkableParameter<AudioParameterFloat> rmsTime;
};

class GammaEnvEditor : public Component
{
public:
  static constexpr int fullSizeWidth = 745;

  GammaEnvEditor(AudioProcessorValueTreeState& apvts,
                 GammaEnvParameters& parameters,
                 String const& midSideParamID = "Mid-Side");

  void resized() override;

  void setTableSettings(LinkableControlTable tableSettings);

private:
  ChannelLabels channelLabels;

  LinkableControl<AttachedSlider> rmsTime;
  LinkableControl<AttachedSlider> attack;
  LinkableControl<AttachedSlider> release;
  LinkableControl<AttachedSlider> attackDelay;
  LinkableControl<AttachedSlider> releaseDelay;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GammaEnvEditor)
};