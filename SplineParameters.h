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
#include "adsp/Spline.hpp"
#include <JuceHeader.h>
#include <array>
#include <functional>

/**
 * Linkable parameters for the splines in
 * https://github.com/unevens/audio-dsp/blob/master/adsp/Spline.hpp
 */

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
    AudioProcessorValueTreeState::ParameterLayout& layout,
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

  template<class Vec, int maxNumKnots>
  int updateSpline(adsp::AutoSpline<Vec, maxNumKnots>& spline)
  {
    auto splineKnots = spline.spline.knots;
    auto automationKnots = spline.automationKnots;
    int n = 0;
    for (auto& knot : fixedKnots) {
      for (int c = 0; c < 2; ++c) {
        automationKnots[n].x[c] = splineKnots[n].x[c] = knot.x;
        automationKnots[n].y[c] = splineKnots[n].y[c] = knot.y;
        automationKnots[n].t[c] = splineKnots[n].t[c] = knot.t;
        automationKnots[n].s[c] = splineKnots[n].s[c] = knot.s;
      }
      ++n;
    }

    for (auto& knot : knots) {
      if (knot.IsEnabled()) {
        for (int c = 0; c < 2; ++c) {
          auto& params = knot.getActiveParameters(c);
          automationKnots[n].x[c] = params.x->get();
          automationKnots[n].y[c] = params.y->get();
          automationKnots[n].t[c] = params.t->get();
          automationKnots[n].s[c] = params.s->get();
        }
        ++n;
      }
    }

    if (needsReset()) {
      spline.reset();
    }

    return n;
  }

  template<class Vec, int maxNumKnots>
  int updateSpline(adsp::Spline<Vec, maxNumKnots>& spline)
  {
    auto splineKnots = spline.knots;
    int n = 0;
    for (auto& knot : fixedKnots) {
      for (int c = 0; c < 2; ++c) {
        splineKnots[n].x[c] = knot.x;
        splineKnots[n].y[c] = knot.y;
        splineKnots[n].t[c] = knot.t;
        splineKnots[n].s[c] = knot.s;
      }
      ++n;
    }

    for (auto& knot : knots) {
      if (knot.IsEnabled()) {
        for (int c = 0; c < 2; ++c) {
          auto& params = knot.getActiveParameters(c);
          splineKnots[n].x[c] = params.x->get();
          splineKnots[n].y[c] = params.y->get();
          splineKnots[n].t[c] = params.t->get();
          splineKnots[n].s[c] = params.s->get();
        }
        ++n;
      }
    }

    return n;
  }
};
