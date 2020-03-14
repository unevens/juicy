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
  setColour(Slider::ColourIds::textBoxOutlineColourId, Colours::darkgrey);
  setColour(ComboBox::ColourIds::arrowColourId, Colours::white);
  setColour(ComboBox::ColourIds::backgroundColourId, Colours::black);
  setColour(ComboBox::ColourIds::textColourId, Colours::white);
  setColour(ComboBox::ColourIds::buttonColourId, Colours::white);
  setColour(ComboBox::ColourIds::outlineColourId, Colours::grey);
  setColour(ComboBox::ColourIds::focusedOutlineColourId, Colours::white);
  setColour(PopupMenu::ColourIds::backgroundColourId, Colours::black);
  setColour(PopupMenu::ColourIds::highlightedBackgroundColourId,
            Colours::darkgrey);
  setColour(PopupMenu::ColourIds::textColourId, Colours::white);
  setColour(PopupMenu::ColourIds::highlightedTextColourId, Colours::white);
  setColour(ToggleButton::ColourIds::textColourId, frontColour);
  setColour(ToggleButton::ColourIds::tickColourId, frontColour);
  setColour(ToggleButton::ColourIds::tickDisabledColourId, frontColour);
}

Font
SimpleLookAndFeel::getLabelFont(Label& label)
{
  bool const isInsideSlider =
    dynamic_cast<Slider*>(label.getParentComponent()) != nullptr;

  if (isInsideSlider) {
    return Font(simpleSliderLabelFontSize);
  }

  bool const isInsideComboBox =
    dynamic_cast<ComboBox*>(label.getParentComponent()) != nullptr;

  if (isInsideComboBox) {
    return Font(simpleFontSize);
  }

  return Font(simpleFontSize, simpleFontStyle);
}

void
SimpleLookAndFeel::drawToggleButton(Graphics& g,
                                    ToggleButton& button,
                                    bool shouldDrawButtonAsHighlighted,
                                    bool shouldDrawButtonAsDown)
{
  Rectangle<float> tickBounds(
    4.0f,
    ((float)button.getHeight() - simpleToggleTickWidth) * 0.5f,
    simpleToggleTickWidth,
    simpleToggleTickWidth);

  g.setColour(button.findColour(ToggleButton::tickDisabledColourId));
  if (!button.isEnabled()) {
    g.setOpacity(0.5f);
  }
  g.drawRoundedRectangle(tickBounds, 4.0f, 1.0f);

  if (button.getToggleState()) {
    g.setColour(button.findColour(ToggleButton::tickColourId));
    if (!button.isEnabled()) {
      g.setOpacity(0.5f);
    }
    auto tick = getTickShape(0.75f);
    g.fillPath(
      tick,
      tick.getTransformToScaleToFit(tickBounds.reduced(4, 5).toFloat(), false));
  }

  g.setColour(button.findColour(ToggleButton::textColourId));
  g.setFont(Font(simpleFontSize, simpleFontStyle));

  if (!button.isEnabled()) {
    g.setOpacity(0.5f);
  }

  g.drawFittedText(button.getButtonText(),
                   button.getLocalBounds()
                     .withTrimmedLeft(roundToInt(simpleToggleTickWidth) + 10)
                     .withTrimmedRight(2),
                   Justification::centredLeft,
                   10);
}

void
SimpleLookAndFeel::drawRotarySlider(Graphics& g,
                                    int x,
                                    int y,
                                    int width,
                                    int height,
                                    float sliderPos,
                                    const float rotaryStartAngle,
                                    const float rotaryEndAngle,
                                    Slider& slider)
{
  auto outline = slider.findColour(Slider::rotarySliderOutlineColourId);
  auto fill = slider.findColour(Slider::rotarySliderFillColourId);

  auto bounds = Rectangle<int>(x, y, width, height)
                  .toFloat()
                  .reduced(simpleRotarySliderOffset);

  auto radius = jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
  auto toAngle =
    rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
  auto lineW = jmin(8.0f, radius * 0.5f);
  auto arcRadius = radius - lineW * 0.5f;

  Path backgroundArc;
  backgroundArc.addCentredArc(bounds.getCentreX(),
                              bounds.getCentreY(),
                              arcRadius,
                              arcRadius,
                              0.0f,
                              rotaryStartAngle,
                              rotaryEndAngle,
                              true);

  g.setColour(outline);
  if (!slider.isEnabled()) {
    g.setOpacity(0.5f);
  }
  g.strokePath(
    backgroundArc,
    PathStrokeType(lineW, PathStrokeType::curved, PathStrokeType::rounded));

  if (slider.isEnabled()) {
    Path valueArc;
    valueArc.addCentredArc(bounds.getCentreX(),
                           bounds.getCentreY(),
                           arcRadius,
                           arcRadius,
                           0.0f,
                           rotaryStartAngle,
                           toAngle,
                           true);

    g.setColour(fill);
    if (!slider.isEnabled()) {
      g.setOpacity(0.5f);
    }
    g.strokePath(
      valueArc,
      PathStrokeType(lineW, PathStrokeType::curved, PathStrokeType::rounded));
  }

  auto thumbWidth = lineW * 2.0f;
  Point<float> thumbPoint(
    bounds.getCentreX() +
      arcRadius * std::cos(toAngle - MathConstants<float>::halfPi),
    bounds.getCentreY() +
      arcRadius * std::sin(toAngle - MathConstants<float>::halfPi));

  g.setColour(slider.findColour(Slider::thumbColourId));
  if (!slider.isEnabled()) {
    g.setOpacity(0.5f);
  }
  g.fillEllipse(
    Rectangle<float>(thumbWidth, thumbWidth).withCentre(thumbPoint));
}
