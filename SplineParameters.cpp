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

#include "SplineParameters.h"
#include <cassert>

bool
SplineParameters::LinkableKnotParameters::needsReset()
{
  bool const isEnabled = IsEnabled();
  bool const isLinked = IsLinked();
  bool const resetFlag = (wasEnabled != isEnabled) || (wasLinked != isLinked);
  wasEnabled = isEnabled;
  wasLinked = isLinked;
  return resetFlag;
}

SplineParameters::KnotParameters&
SplineParameters::LinkableKnotParameters::getActiveParameters(int channel)
{
  if (IsLinked()) {
    return parameters[0];
  }
  return parameters[channel];
}

int
SplineParameters::getNumActiveKnots()
{
  int numKnots = fixedKnots.size();
  for (auto& knot : knots) {
    if (knot.IsEnabled()) {
      ++numKnots;
    }
  }
  return numKnots;
}

bool
SplineParameters::needsReset()
{
  for (auto& knot : knots) {
    if (knot.needsReset()) {
      return true;
    }
  }
  return false;
}

SplineParameters::SplineParameters(
  String splinePrefix,
  AudioProcessorValueTreeState::ParameterLayout& layout,
  int numKnots,
  NormalisableRange<float> rangeX,
  NormalisableRange<float> rangeY,
  NormalisableRange<float> rangeTan,
  std::function<bool(int)> isKnotActive,
  std::vector<KnotData> fixedKnots)
  : rangeX(rangeX)
  , rangeY(rangeY)
  , rangeTan(rangeTan)
  , fixedKnots(fixedKnots)
{
  auto const createFloatParameter =
    [&](String name, float value, NormalisableRange<float> range) {
      auto p = new AudioParameterFloat(name, name, range, value);
      layout.add(std::unique_ptr<RangedAudioParameter>(p));
      return static_cast<AudioParameterFloat*>(p);
    };

  auto const createBoolParameter = [&](String name, float value) {
    WrappedBoolParameter wrapper;
    layout.add(wrapper.createParameter(name, value));
    return wrapper;
  };

  auto const lerp = [&](float a, float b, float alpha) {
    return a + (b - a) * alpha;
  };

  auto const createKnotParameters = [&](String prefix, String postfix, int i) {
    float alpha = (i + 1) / (float)(numKnots + 1);

    return KnotParameters{

      createFloatParameter(
        prefix + "X" + postfix, rangeX.convertFrom0to1(alpha), rangeX),

      createFloatParameter(
        prefix + "Y" + postfix, rangeY.convertFrom0to1(alpha), rangeY),

      createFloatParameter(prefix + "Tangent" + postfix,
                           (rangeY.end - rangeY.start) /
                             (rangeX.end - rangeX.start),
                           rangeTan),

      createFloatParameter(
        prefix + "Smoothness" + postfix, 1.0, { 0.0, 1.0, 0.01 })
    };
  };

  auto const createLinkableKnotParameters = [&](int i) {
    String postfix = "_k" + std::to_string(i + 1);

    // parameters are constructed in the order in which they will appear to the
    // host

    auto enabled =
      createBoolParameter(splinePrefix + "enabled" + postfix, isKnotActive(i));
    auto linked = createBoolParameter(splinePrefix + "linked" + postfix, true);
    auto ch0 = createKnotParameters(splinePrefix, postfix + "_ch0", i);
    auto ch1 = createKnotParameters(splinePrefix, postfix + "_ch1", i);

    // and stored in their struct

    return LinkableKnotParameters{
      std::move(ch0), std::move(ch1), std::move(enabled), std::move(linked)
    };
  };

  knots.reserve(numKnots);

  for (int i = 0; i < numKnots; ++i) {
    knots.push_back(createLinkableKnotParameters(i));
  }
}

SplineParameters::SplineParameters(std::vector<AudioParameterFloat*> parameters,
                                   int numKnots,
                                   NormalisableRange<float> rangeX,
                                   NormalisableRange<float> rangeY,
                                   NormalisableRange<float> rangeTan,
                                   std::vector<KnotData> fixedKnots)
  : rangeX(rangeX)
  , rangeY(rangeY)
  , rangeTan(rangeTan)
  , fixedKnots(fixedKnots)
{
  assert(parameters.size() == 10 * numKnots);
  int p = 0;
  for (int i = 0; i < numKnots; ++i) {
    knots.push_back(
      LinkableKnotParameters({ parameters[p],
                               parameters[p + 1],
                               parameters[p + 2],
                               parameters[p + 3] },
                             { parameters[p + 4],
                               parameters[p + 5],
                               parameters[p + 6],
                               parameters[p + 7] },
                             WrappedBoolParameter(parameters[p + 8]),
                             WrappedBoolParameter(parameters[p + 9])));
    p += 10;
  }
}
