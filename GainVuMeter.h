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
#include <JuceHeader.h>
#include <array>

class GainVuMeter
  : public Component
  , public Timer
{
public:
  GainVuMeter(
    std::array<std::atomic<float>*, 2> source,
    float range = 36.f,
    std::function<float(float)> scaling = [](float x) { return x; },
    Colour lowColour = Colours::green,
    Colour highColour = Colours::red,
    Colour backgroundColour = Colours::black);

  void paint(Graphics& g) override;
  void resized() override;
  void mouseDown(MouseEvent const& event) override;

  Colour backgroundColour = Colours::black;
  Colour internalColour = Colours::transparentBlack;
  Colour labelColour = Colours::lightgrey;
  Colour lineColour = Colours::grey;
  
  float range;

  void setColours(Colour lowColour, Colour highColour);

  std::function<float(float)> scaling;

  std::array<std::atomic<float>*, 2> source;

private:
  void timerCallback() override { repaint(); }

  void updateGradients();

  void reset();

  Colour lowColour;
  Colour highColour;
  ColourGradient topGradient;
  ColourGradient bottomGradient;

  std::array<float, 2> minValue = { { 0.f, 0.f } };
  std::array<float, 2> maxValue = { { 0.f, 0.f } };
};