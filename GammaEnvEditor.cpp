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

#include "GammaEnvEditor.h"

GammaEnvEditor::GammaEnvEditor(AudioProcessorValueTreeState& apvts,
                               GammaEnvParameters& parameters,
                               String const& midSideParamID)
  : attack(apvts, "Attack", parameters.attack)
  , release(apvts, "Release", parameters.release)
  , releaseDelay(apvts, "Release Delay", parameters.releaseDelay)
  , attackDelay(apvts, "Attack Delay", parameters.attackDelay)
  , rmsTime(apvts, "RMS Time", parameters.rmsTime)
  , channelLabels(apvts, midSideParamID)
{
  addAndMakeVisible(attack);
  addAndMakeVisible(release);
  addAndMakeVisible(releaseDelay);
  addAndMakeVisible(attackDelay);
  addAndMakeVisible(rmsTime);
  addAndMakeVisible(channelLabels);
  
  for (int c = 0; c < 2; ++c) {
    rmsTime.getControl(c).setTextValueSuffix("ms");
    attack.getControl(c).setTextValueSuffix("ms");
    attackDelay.getControl(c).setTextValueSuffix("%");
    release.getControl(c).setTextValueSuffix("ms");
    releaseDelay.getControl(c).setTextValueSuffix("%");
  }

  setSize(fullSizeWidth, 160);
  setOpaque(false);
}

void
GammaEnvEditor::resized()
{
  float widthFactor = (getWidth()) / (float)(fullSizeWidth);

  int left = 0;

  auto const resize = [&](auto& component, float width) {
    component.setTopLeftPosition(left, 0);
    component.setSize((int)width, getHeight());
    left += (int)width - 1;
  };

  resize(channelLabels, 55 * widthFactor);
  int elementWidth = 136 * widthFactor;
  resize(rmsTime, elementWidth);
  resize(attack, elementWidth);
  resize(release, elementWidth );
  resize(attackDelay, elementWidth);
  resize(releaseDelay, elementWidth);
}

void
GammaEnvEditor::setTableSettings(LinkableControlTable tableSettings)
{
  channelLabels.tableSettings = tableSettings;
  rmsTime.tableSettings = tableSettings;
  attack.tableSettings = tableSettings;
  release.tableSettings = tableSettings;
  attackDelay.tableSettings = tableSettings;
  releaseDelay.tableSettings = tableSettings;
}
