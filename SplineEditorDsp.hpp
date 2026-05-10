/*
Copyright 2026 Dario Mambro

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.
*/

#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <vector>

namespace juicy {

// Scalar 2-channel cubic Hermite spline used by the GUI to draw the curve
// preview and the VU meter readout. Same control-point shape as
// adsp::Spline<Vec2d, N>, but no SIMD intrinsics — keeps the editor TU
// out of the clang frontend's NEON-builtin codegen path that otherwise
// destabilises the build.
class GuiSpline final
{
public:
  struct Knot
  {
    double x[2]{};
    double y[2]{};
    double t[2]{};
    double s[2]{};
  };

  explicit GuiSpline(int maxNumKnots)
    : knots_(static_cast<std::size_t>(maxNumKnots))
  {}

  void setIsSymmetric(int channel, bool value)
  {
    isSymmetric_[channel] = value;
  }

  Knot& knot(int n) { return knots_[static_cast<std::size_t>(n)]; }
  Knot const& knot(int n) const { return knots_[static_cast<std::size_t>(n)]; }
  int maxNumKnots() const { return static_cast<int>(knots_.size()); }

  // Evaluate the spline at a single input value for one channel.
  // Mirrors adsp::Spline::VecSpline::process() but in scalar form.
  double process(double in_raw, int channel, int numActiveKnots) const
  {
    bool const sym = isSymmetric_[channel];
    double const in = sym ? std::abs(in_raw) : in_raw;

    double x0 = std::numeric_limits<float>::lowest();
    double y0 = 0.0, t0 = 0.0, s0 = 0.0;
    double x1 = std::numeric_limits<float>::max();
    double y1 = 0.0, t1 = 0.0, s1 = 0.0;

    auto const& first = knots_[0];
    double x_low = first.x[channel], y_low = first.y[channel], t_low = first.t[channel];
    double x_high = first.x[channel], y_high = first.y[channel], t_high = first.t[channel];

    for (int n = 0; n < numActiveKnots; ++n) {
      auto const& k = knots_[static_cast<std::size_t>(n)];
      double const kx = k.x[channel];
      double const ky = k.y[channel];
      double const kt = k.t[channel];
      double const ks = k.s[channel];

      if (in > kx && kx > x0) { x0 = kx; y0 = ky; t0 = kt; s0 = ks; }
      if (in <= kx && kx < x1) { x1 = kx; y1 = ky; t1 = kt; s1 = ks; }
      if (kx < x_low)  { x_low = kx;  y_low = ky;  t_low = kt; }
      if (kx > x_high) { x_high = kx; y_high = ky; t_high = kt; }
    }

    bool const is_high = x1 == std::numeric_limits<float>::max();
    bool const is_low  = x0 == std::numeric_limits<float>::lowest();

    double const dx = std::max(x1 - x0,
                               static_cast<double>(std::numeric_limits<float>::min()));
    double const dy = y1 - y0;
    double const a = t0 * dx - dy;
    double const b = -t1 * dx + dy;
    double const ix = 1.0 / dx;
    double const m = dy * ix;
    double const o = y0 - m * x0;

    double const j = (in - x0) * ix;
    double const k = 1.0 - j;
    double const hermite = k * y0 + j * y1 + j * k * (a * k + b * j);

    double const segment = m * in + o;
    double const smoothness = s1 + k * (s0 - s1);
    double const curve = segment + smoothness * (hermite - segment);

    double const low = y_low + (in - x_low) * t_low;
    double const high = y_high + (in - x_high) * t_high;

    double out;
    if (is_high)     out = high;
    else if (is_low) out = low;
    else             out = curve;

    return sym ? std::copysign(out, in_raw) : out;
  }

private:
  std::array<bool, 2> isSymmetric_{ { false, false } };
  std::vector<Knot> knots_;
};

} // namespace juicy
