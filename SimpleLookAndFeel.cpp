/*
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

#include "SimpleLookAndFeel.h"

SimpleLookAndFeel::SimpleLookAndFeel()
  : LookAndFeel_V4()
{
  apply();
}

void
SimpleLookAndFeel::apply()
{
  setColour(Label::ColourIds::textColourId, frontColour);
  setColour(Slider::ColourIds::thumbColourId, sliderThumbColour);
  setColour(Slider::ColourIds::rotarySliderFillColourId, sliderFillColour);
  setColour(Slider::ColourIds::rotarySliderOutlineColourId,
            sliderOutlineColour);
  setColour(Slider::ColourIds::textBoxTextColourId, Colours::white);
  setColour(Slider::ColourIds::textBoxBackgroundColourId, Colours::black);
  setColour(Slider::ColourIds::textBoxHighlightColourId, Colours::darkgrey);
  setColour(Slider::ColourIds::textBoxOutlineColourId, Colours::darkgrey);
  setColour(ComboBox::ColourIds::arrowColourId, Colours::white);
  setColour(ComboBox::ColourIds::backgroundColourId, Colours::black);
  setColour(ComboBox::ColourIds::textColourId, Colours::white);
  setColour(ComboBox::ColourIds::buttonColourId, Colours::white);
  setColour(ComboBox::ColourIds::outlineColourId, Colours::grey);
  setColour(ComboBox::ColourIds::focusedOutlineColourId, Colours::white);
  setColour(ToggleButton::ColourIds::textColourId, frontColour);
  setColour(ToggleButton::ColourIds::tickColourId, frontColour);
  setColour(ToggleButton::ColourIds::tickDisabledColourId, frontColour);
}

void
SimpleLookAndFeel::drawToggleButton(Graphics& g,
                                       ToggleButton& button,
                                       bool shouldDrawButtonAsHighlighted,
                                       bool shouldDrawButtonAsDown)
{
  auto fontSize = simpleFontSize;
  auto tickWidth = fontSize * 1.1f;

  drawTickBox(g,
              button,
              4.0f,
              (button.getHeight() - tickWidth) * 0.5f,
              tickWidth,
              tickWidth,
              button.getToggleState(),
              button.isEnabled(),
              shouldDrawButtonAsHighlighted,
              shouldDrawButtonAsDown);

  g.setColour(button.findColour(ToggleButton::textColourId));
  g.setFont(Font(fontSize, simpleFontStyle));

  if (!button.isEnabled())
    g.setOpacity(0.5f);

  g.drawFittedText(button.getButtonText(),
                   button.getLocalBounds()
                     .withTrimmedLeft(roundToInt(tickWidth) + 10)
                     .withTrimmedRight(2),
                   Justification::centredLeft,
                   10);
}