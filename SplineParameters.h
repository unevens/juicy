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
  struct KnotData
  {
    float x;
    float y;
    float t;
    float s;
  };

  struct KnotParameters
  {
    AudioParameterFloat* x;
    AudioParameterFloat* y;
    AudioParameterFloat* t;
    AudioParameterFloat* s;
  };

  class LinkableKnotParameters
  {
    bool wasLinked = false;
    bool wasEnabled = false;

  public:
    std::array<KnotParameters, 2> parameters;
    WrappedBoolParameter enabled;
    WrappedBoolParameter linked;

    LinkableKnotParameters(KnotParameters parameters0,
                           KnotParameters parameters1,
                           WrappedBoolParameter enabled,
                           WrappedBoolParameter linked)
      : parameters{ parameters0, parameters1 }
      , enabled{ enabled }
      , linked{ linked }
    {}

    bool IsEnabled() { return enabled.getValue(); };
    bool IsLinked() { return linked.getValue(); };

    bool needsReset();

    KnotParameters& getActiveParameters(int channel);
  };

  std::vector<LinkableKnotParameters> knots;
  std::vector<KnotData> fixedKnots;

  NormalisableRange<float> rangeX;
  NormalisableRange<float> rangeY;
  NormalisableRange<float> rangeTan;

  int getNumActiveKnots();

  bool needsReset();

  SplineParameters(
    String splinePrefix,
    std::vector<std::unique_ptr<RangedAudioParameter>>& parametersForApvts,
    int numKnots,
    NormalisableRange<float> rangeX,
    NormalisableRange<float> rangeY,
    NormalisableRange<float> rangeTan,
    std::function<bool(int)> isKnotActive = [](int) { return true; },
    std::vector<KnotData> fixedKnots = {});

  SplineParameters(std::vector<AudioParameterFloat*> parameters,
                   int numKnots,
                   NormalisableRange<float> rangeX,
                   NormalisableRange<float> rangeY,
                   NormalisableRange<float> rangeTan,
                   std::vector<KnotData> fixedKnots = {});

  template<class SplineHolderClass>
  typename SplineHolderClass::SplineInterface* updateSpline(
    SplineHolderClass& splines)
  {
    int const numKnots = getNumActiveKnots();
    auto spline = splines.getSpline(numKnots);
    if (!spline) {
      return nullptr;
    }
    auto splineKnots = spline->getKnots();
    int n = 0;
    for (auto& knot : fixedKnots) {
      for (int c = 0; c < 2; ++c) {
        splineKnots[n].target.x[c] = knot.x;
        splineKnots[n].target.y[c] = knot.y;
        splineKnots[n].target.t[c] = knot.t;
        splineKnots[n].target.s[c] = knot.s;
      }
      ++n;
    }
    for (auto& knot : knots) {
      if (knot.IsEnabled()) {
        for (int c = 0; c < 2; ++c) {
          auto& params = knot.getActiveParameters(c);
          splineKnots[n].target.x[c] = params.x->get();
          splineKnots[n].target.y[c] = params.y->get();
          splineKnots[n].target.t[c] = params.t->get();
          splineKnots[n].target.s[c] = params.s->get();
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
