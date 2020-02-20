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
SplineParameters::LinkableNodeParameters::NeedsReset()
{
  bool const isEnabled = IsEnabled();
  bool const isLinked = IsLinked();
  bool const resetFlag = (wasEnabled != isEnabled) || (wasLinked != isLinked);
  wasEnabled = isEnabled;
  wasLinked = isLinked;
  return resetFlag;
}

SplineParameters::NodeParameters&
SplineParameters::LinkableNodeParameters::GetActiveParameters(int channel)
{
  if (IsLinked()) {
    return parameters[0];
  }
  return parameters[channel];
}

int
SplineParameters::getNumActiveNodes()
{
  int numNodes = 0;
  for (auto& node : nodes) {
    if (node.IsEnabled()) {
      ++numNodes;
    }
  }
  return numNodes;
}

bool
SplineParameters::needsReset()
{
  for (auto& node : nodes) {
    if (node.NeedsReset()) {
      return true;
    }
  }
  return false;
}

SplineParameters::SplineParameters(
  String splinePrefix,
  std::vector<std::unique_ptr<RangedAudioParameter>>& parametersForApvts,
  int numNodes,
  NormalisableRange<float> rangeX,
  NormalisableRange<float> rangeY,
  NormalisableRange<float> rangeTan)
  : rangeX(rangeX)
  , rangeY(rangeY)
  , rangeTan(rangeTan)
{
  auto const createFloatParameter =
    [&](String name, float value, NormalisableRange<float> range) {
      parametersForApvts.push_back(std::unique_ptr<RangedAudioParameter>(
        new AudioParameterFloat(name, name, range, value)));

      return static_cast<AudioParameterFloat*>(parametersForApvts.back().get());
    };

  auto const createBoolParameter = [&](String name, float value) {
    WrappedBoolParameter wrapper;
    parametersForApvts.push_back(wrapper.createParameter(name, value));
    return wrapper;
  };

  auto const lerp = [&](float a, float b, float alpha) {
    return a + (b - a) * alpha;
  };

  auto const createNodeParameters = [&](String prefix, int i) {
    float alpha = (i + 1) / (float)(numNodes + 1);

    return NodeParameters{

      createFloatParameter(prefix + "x", rangeX.convertFrom0to1(alpha), rangeX),

      createFloatParameter(prefix + "y", rangeY.convertFrom0to1(alpha), rangeY),

      createFloatParameter(prefix + "tangent",
                           (rangeY.end - rangeY.start) /
                             (rangeX.end - rangeX.start),
                           rangeTan),

      createFloatParameter(prefix + "smoothness", 1.0, { 0.0, 1.0, 0.01 })
    };
  };

  auto const createLinkableNodeParameters = [&](int i) {
    String prefix = splinePrefix + "_node_" + std::to_string(i) + "_";
    return LinkableNodeParameters{
      createNodeParameters(prefix + "_ch0", i),
      createNodeParameters(prefix + "_ch1", i),
      createBoolParameter(prefix + "_enabled", true),
      createBoolParameter(prefix + "_linked", true)
    };
  };

  nodes.reserve(numNodes);

  for (int i = 0; i < numNodes; ++i) {
    nodes.push_back(createLinkableNodeParameters(i));
  }
}

SplineParameters::SplineParameters(std::vector<AudioParameterFloat*> parameters,
                                   int numNodes,
                                   NormalisableRange<float> rangeX,
                                   NormalisableRange<float> rangeY,
                                   NormalisableRange<float> rangeTan)
  : rangeX(rangeX)
  , rangeY(rangeY)
  , rangeTan(rangeTan)
{
  assert(parameters.size() == 10 * numNodes);
  int p = 0;
  for (int i = 0; i < numNodes; ++i) {
    nodes.push_back(
      LinkableNodeParameters({ parameters[p],
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
