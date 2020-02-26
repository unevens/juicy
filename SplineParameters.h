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
#include "Linkables.h"
#include <JuceHeader.h>
#include <array>
#include <functional>

struct SplineParameters
{
  struct NodeParameters
  {
    AudioParameterFloat* x;
    AudioParameterFloat* y;
    AudioParameterFloat* t;
    AudioParameterFloat* s;
  };

  class LinkableNodeParameters
  {
    bool wasLinked = false;
    bool wasEnabled = false;

  public:
    std::array<NodeParameters, 2> parameters;
    WrappedBoolParameter enabled;
    WrappedBoolParameter linked;

    LinkableNodeParameters(NodeParameters parameters0,
                           NodeParameters parameters1,
                           WrappedBoolParameter enabled,
                           WrappedBoolParameter linked)
      : parameters{ parameters0, parameters1 }
      , enabled{ enabled }
      , linked{ linked }
    {}

    bool IsEnabled() { return enabled.getValue(); };
    bool IsLinked() { return linked.getValue(); };

    bool needsReset();

    NodeParameters& getActiveParameters(int channel);
  };

  std::vector<LinkableNodeParameters> nodes;

  NormalisableRange<float> rangeX;
  NormalisableRange<float> rangeY;
  NormalisableRange<float> rangeTan;

  int getNumActiveNodes();

  bool needsReset();

  SplineParameters(
    String splinePrefix,
    std::vector<std::unique_ptr<RangedAudioParameter>>& parametersForApvts,
    int numNodes,
    NormalisableRange<float> rangeX,
    NormalisableRange<float> rangeY,
    NormalisableRange<float> rangeTan,
    std::function<bool(int)> isNodeActive = [](int) { return true; });

  SplineParameters(std::vector<AudioParameterFloat*> parameters,
                   int numNodes,
                   NormalisableRange<float> rangeX,
                   NormalisableRange<float> rangeY,
                   NormalisableRange<float> rangeTan);

  template<class SplineHolderClass>
  typename SplineHolderClass::SplineInterface* updateSpline(
    SplineHolderClass& splines)
  {
    int const numNodes = getNumActiveNodes();
    auto spline = splines.getSpline(numNodes);
    if (!spline) {
      return nullptr;
    }
    auto splineNodes = spline->getNodes();
    int n = 0;
    for (auto& node : nodes) {
      if (node.IsEnabled()) {
        for (int c = 0; c < 2; ++c) {
          auto& params = node.getActiveParameters(c);
          splineNodes[n].target.x[c] = params.x->get();
          splineNodes[n].target.y[c] = params.y->get();
          splineNodes[n].target.t[c] = params.t->get();
          splineNodes[n].target.s[c] = params.s->get();
        }
        ++n;
      }
    }
    return spline;
  }
};

struct WaveShaperParameters
{
  LinkableParameter<AudioParameterFloat> dc;
  LinkableParameter<AudioParameterFloat> dryWet;
  LinkableParameter<AudioParameterFloat> dcCutoff;
  LinkableParameter<WrappedBoolParameter> symmetry;
};
