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

#include "GainVuMeter.h"

GainVuMeter::GainVuMeter(std::array<std::atomic<float>*, 2> source,
                         float range,
                         std::function<float(float)> scaling,
                         Colour lowColour,
                         Colour highColour,
                         Colour backgroundColour)
  : source(source)
  , range(range)
  , scaling(scaling)
  , lowColour(lowColour)
  , highColour(highColour)
  , backgroundColour(backgroundColour)
{
  setSize(16, 128);
  startTimer(50.f);
}

void
GainVuMeter::paint(Graphics& g)
{
  float const dx = getWidth() / 3.f;

  g.setColour(Colours::black);
  g.fillRect(0.f, 0.f, dx, (float)getHeight());
  g.fillRect(2.f * dx, 0.f, dx, (float)getHeight());

  g.setFont(12);
  float const halfHeight = getHeight() * 0.5f;

  g.setColour(Colours::darkgrey);

  for (int c = 0; c < 2; ++c) {
    float const db = jlimit(-range, range, source[c]->load());

    minValue[c] = jmin(db, minValue[c]);
    maxValue[c] = jmax(db, maxValue[c]);

    float const yNorm = jlimit(-1.f, 1.f, db / range);
    float const y = std::copysign(scaling(abs(yNorm)), yNorm);

    float left = c == 0 ? 0.f : 2.f * dx;
    if (y > 0.f) {
      g.setGradientFill(topGradient);
      g.fillRect(left, halfHeight * (1.f - y), dx, y * halfHeight);
    }
    else {
      g.setGradientFill(bottomGradient);
      g.fillRect(left, halfHeight, dx, -halfHeight * y);
    }

    g.setGradientFill(topGradient);
    float const maxY = scaling(jmin(1.f, maxValue[c] / range));
    float const maxYCoord = halfHeight * (1.f - maxY);
    g.drawLine(left, maxYCoord, left + dx, maxYCoord, 2);

    if (maxYCoord >= 24 && maxYCoord < halfHeight - 20) {
      g.drawText(String(maxValue[c], 1),
                 Rectangle((int)left, (int)maxYCoord - 24, (int)dx, 20),
                 Justification::centred);
    }

    g.setGradientFill(bottomGradient);
    float const minY = scaling(abs(jmax(-1.f, minValue[c] / range)));
    float const minYCoord = halfHeight * (1.f + minY);
    g.drawLine(left, minYCoord, left + dx, minYCoord, 2);

    if (minYCoord + 24 < getHeight() && minYCoord > halfHeight + 20) {
      g.drawText(String(minValue[c], 1),
                 Rectangle((int)left, (int)minYCoord + 4, (int)dx, 20),
                 Justification::centred);
    }

    g.setColour(Colours::black);

    if (db >= 0.1f) {
      g.drawText(String(db, 1),
                 Rectangle((int)left, (int)halfHeight - 18, (int)dx, 20),
                 Justification::centred);
    }
    else if (db <= -0.1f) {
      g.drawText(String(db, 1),
                 Rectangle((int)left, (int)halfHeight + 2, (int)dx, 20),
                 Justification::centred);
    }
  }

  g.setColour(Colours::black);
  g.drawLine(dx, halfHeight, 2.f * dx, halfHeight, 2);

  g.setFont({ 12, Font::bold });

  std::function<void(int)> const drawReferenceLine = [&](int db) {
    if (abs(db) > range) {
      return;
    }

    int const y = jlimit(
      1,
      getHeight() - 1,
      (int)(halfHeight -
            std::copysign(scaling(abs(db / range)), db / range) * halfHeight));

    g.drawLine(dx, y, 2.f * dx, y, 2);

    int const textHeight = y + (db > 0.f ? 0 : -16);

    g.drawText((db > 0 ? "+" : "-") + String(abs(db)),
               Rectangle((int)dx, textHeight, (int)dx, 16),
               Justification::centred);

    if (db > 0) {
      drawReferenceLine(-db);
    }
  };

  drawReferenceLine(1);
  drawReferenceLine(3);
  drawReferenceLine(6);
  drawReferenceLine(12);
  drawReferenceLine(24);
  drawReferenceLine(36);
}

void
GainVuMeter::resized()
{
  updateGradients();
  reset();
}

void
GainVuMeter::mouseDown(MouseEvent const& event)
{
  reset();
}

void
GainVuMeter::setColours(Colour low, Colour high)
{
  highColour = high;
  lowColour = low;
  updateGradients();
}

void
GainVuMeter::updateGradients()
{
  topGradient = ColourGradient(
    lowColour, 0.f, getHeight() * 0.5f, highColour, 0.f, 0.f, false);

  topGradient.addColour(0.5, Colours::yellow);

  bottomGradient = ColourGradient(
    lowColour, 0.f, getHeight() * 0.5f, highColour, 0.f, getHeight(), false);

  bottomGradient.addColour(0.5, Colours::yellow);
}

void
GainVuMeter::reset()
{
  minValue[0] = minValue[1] = maxValue[0] = maxValue[1] = 0.f;
}
