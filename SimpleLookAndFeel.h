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

#pragma once
#include < JuceHeader.h>

class SimpleLookAndFeel : public LookAndFeel_V4
{
public:
  SimpleLookAndFeel();

  int simpleFontSize = 16;
  int simpleSliderLabelFontSize = 15;
  Font::FontStyleFlags simpleFontStyle = Font::bold;
  
  Colour frontColour = Colours::white;
  Colour sliderFillColour = Colours::blue;
  Colour sliderOutlineColour = Colours::blueviolet;
  Colour sliderThumbColour = Colours::fuchsia;

  void apply();

  Font getTextButtonFont(TextButton&, int buttonHeight) override
  {
    return Font(simpleFontSize, simpleFontStyle);
  }

  Font getLabelFont(Label& label) override;


  void drawToggleButton(Graphics& g,
                        ToggleButton& button,
                        bool shouldDrawButtonAsHighlighted,
                        bool shouldDrawButtonAsDown) override;
};
