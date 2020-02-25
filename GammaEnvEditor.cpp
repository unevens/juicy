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
  , metric(apvts, "Metric", { "Peak", "RMS" }, parameters.metric)
  , channelLabels(apvts, midSideParamID)
{
  addAndMakeVisible(attack);
  addAndMakeVisible(release);
  addAndMakeVisible(releaseDelay);
  addAndMakeVisible(attackDelay);
  addAndMakeVisible(metric);
  addAndMakeVisible(channelLabels);

  setSize(fullSizeWidth, 160);
  setOpaque(false);
}

void
GammaEnvEditor::resized()
{
  float widthFactor = (getWidth()) / (float)(fullSizeWidth);

  int left = 0;

  auto const resize = [&](auto& component, int width) {
    component.setTopLeftPosition(left, 0.f);
    component.setSize(width, getHeight());
    left += width - 1;
  };

  resize(channelLabels, 50 * widthFactor);

  resize(metric, 120 * widthFactor);
  resize(attack, 140 * widthFactor);
  resize(release, 140 * widthFactor);
  resize(attackDelay, 140 * widthFactor);
  resize(releaseDelay, 140 * widthFactor);
}

void
GammaEnvEditor::setTableSettings(LinkableControlTable tableSettings)
{
  channelLabels.tableSettings = tableSettings;
  metric.tableSettings = tableSettings;
  attack.tableSettings = tableSettings;
  release.tableSettings = tableSettings;
  attackDelay.tableSettings = tableSettings;
  releaseDelay.tableSettings = tableSettings;
}
