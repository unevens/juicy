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
#include "Attachments.h"
#include "Linkables.h"
#include "SplineParameters.h"
#include "adsp/Spline.hpp"
#include <JuceHeader.h>

/**
 * A gui for the splines in
 * https://github.com/unevens/audio-dsp/blob/master/adsp/Spline.hpp
 */

#ifndef JUICY_MAX_SPLINE_EDITOR_NUM_KNOTS
#define JUICY_MAX_SPLINE_EDITOR_NUM_KNOTS 17
#endif

struct SplineAttachments
{
  struct KnotAttachments
  {
    std::unique_ptr<FloatAttachment> x;
    std::unique_ptr<FloatAttachment> y;
    std::unique_ptr<FloatAttachment> t;
    std::unique_ptr<FloatAttachment> s;
  };

  struct LinkableKnotAttachments
  {
    std::array<KnotAttachments, 2> parameters;
    std::unique_ptr<BoolAttachment> enabled;
    std::unique_ptr<BoolAttachment> linked;
  };

  std::vector<LinkableKnotAttachments> knots;

  std::array<std::unique_ptr<BoolAttachment>, 2> symmetry;

  SplineAttachments(
    SplineParameters& parameters,
    AudioProcessorValueTreeState& apvts,
    std::function<void(void)> onChange,
    LinkableParameter<WrappedBoolParameter>* symmetryParameter = nullptr);

  int getNumActiveKnots();
};

class SplineEditor;
class SplineKnotEditor;

void
attachAndInitializeSplineEditors(SplineEditor& splineEditor,
                                 SplineKnotEditor& knotEditor,
                                 int selectedKnot = 0);

class SplineEditor
  : public Component
  , public Timer
{
  friend void attachAndInitializeSplineEditors(SplineEditor& splineEditor,
                                               SplineKnotEditor& knotEditor,
                                               int selectedKnot);

public:
  SplineEditor(
    SplineParameters& parameters,
    AudioProcessorValueTreeState& apvts,
    LinkableParameter<WrappedBoolParameter>* symmetryParameter = nullptr);

  void paint(Graphics&) override;
  void resized() override;

  void mouseDown(MouseEvent const& event) override;
  void mouseDrag(MouseEvent const& event) override;
  void mouseUp(MouseEvent const& event) override;
  void mouseDoubleClick(MouseEvent const& event) override;
  void mouseWheelMove(MouseEvent const& event,
                      MouseWheelDetails const& wheel) override;
  void mouseMagnify(MouseEvent const& event, float scaleFactor) override;

  void setSelectedKnot(int knot);

  juce::Rectangle<int> areaInWhichToDrawKnots;
  // When the mouse is inside the editor, the spline knots will be
  // drawn on top of the curve. To have them drawn also when the mouse is
  // over the a SplineKnotEditor instance, you can use the
  // areaInWhichToDrawKnots member. It is a Rectangle in the coordinates of the
  // parent component. When the mouse is in it, the knots will be
  // drawn.

  // These members can be used to customize the appearance of the editor

  float widgetOffset = 20.f;

  float bigKnotSize = 10.f;

  float smallKnotSize = 6.f;

  Point<int> numGridLines = { 8, 8 };

  std::array<std::atomic<float>*, 2> vuMeter = { { nullptr, nullptr } };

  Colour backgroundColour = Colours::black;

  Colour gridColour = Colours::darkgrey.darker(1.f);

  Colour gridLabelColour = Colours::darkgrey;

  Colour mousePositionColour = Colours::white;

  std::array<Colour, 2> haloColours = {
    { Colours::lightseagreen.withAlpha(0.6f),
      Colours::lightcoral.withAlpha(0.6f) }
  };

  std::array<Colour, 2> curveColours = { { Colours::blue, Colours::red } };

  std::array<Colour, 2> knotColours = { { Colours::steelblue,
                                          Colours::orangered } };

  std::array<Colour, 2> vuMeterColours = { { Colours::cadetblue,
                                             Colours::coral } };

  Font font = Font(12);

  float wheelToZoomScaleFactor = 0.25f;

  String xSuffix = "";

  String ySuffix = "";

private:
  SplineParameters& parameters;
  SplineKnotEditor* knotEditor = nullptr;

  Point<float> getKnotCoord(int knotIndex, int channel);

  struct KnotSelectionResult
  {
    int knotIndex;
    float distanceBwtweenKnotAndMouse;
  };

  KnotSelectionResult selectKnot(MouseEvent const& event);

  void setupZoom(Point<float> fixedPoint, Point<float> newZoom);

  void timerCallback() override { repaint(); }

  SplineAttachments spline;

  NormalisableRange<float> rangeX;
  NormalisableRange<float> rangeY;
  NormalisableRange<float> rangeTan;

  std::atomic<bool> redrawCurvesFlag{ true };

  VecBuffer<Vec2d> vuMeterBuffer{ 1 };

  void onSplineChange();

  int selectedKnot = 0;

  enum class InteractionType
  {
    movement,
    value,
    leftTangent,
    rightTangent,
    smoothing
  };

  InteractionType interaction = InteractionType::movement;
  int interactingChannel = 0;

  float interactionBuffer = 0.f;

  Point<float> zoom = { 1.f, 1.f };
  Point<float> offset = { 0.f, 0.f };
  Point<float> prevOffset = { 0.f, 0.f };

  float pixelToX(float pixel);
  float xToPixel(float x);
  float pixelToY(float pixel);
  float yToPixel(float y);
  float yToPixelUnclamped(float y);

  using Spline = adsp::Spline<Vec2d, JUICY_MAX_SPLINE_EDITOR_NUM_KNOTS>;

  aligned_ptr<Spline> splineDsp;

  adsp::SplineDispatcher<Vec2d, JUICY_MAX_SPLINE_EDITOR_NUM_KNOTS>
    splineDispatcher;

  LinkableParameter<WrappedBoolParameter>* symmetryParameter;

  avec::VecBuffer<Vec2d> inputBuffer;
  avec::VecBuffer<Vec2d> outputBuffer;

  void setupSplineInputBuffer();

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SplineEditor)
};

class SplineKnotEditor
  : public Component
  , public Timer
{
  friend void attachAndInitializeSplineEditors(SplineEditor& splineEditor,
                                               SplineKnotEditor& knotEditor,
                                               int selectedKnot);

public:
  SplineKnotEditor(SplineParameters& parameters,
                   AudioProcessorValueTreeState& apvts,
                   String const& midSideParamID = "Mid-Side");

  void resized() override;

  void paint(Graphics& g) override;

  void setSelectedKnot(int newKnotIndex, bool forceUpdate = false);

  void setTableSettings(LinkableControlTable tableSettings);

  String xLabel = "X";
  String yLabel = "Y";

private:
  void setKnot(int newKnotIndex, bool forceUpdate = false);

  void timerCallback() override { repaint(); }

  SplineEditor* splineEditor = nullptr;

  int knotIndex = -1;

  SplineParameters& parameters;
  AudioProcessorValueTreeState& apvts;

  Label label{ {}, "Selected Knot" };

  ComboBox selectedKnot;

  AttachedToggle enabled;
  AttachedToggle linked;

  ChannelLabels channelLabels;

  std::unique_ptr<LinkableControl<AttachedSlider>> x;
  std::unique_ptr<LinkableControl<AttachedSlider>> y;
  std::unique_ptr<LinkableControl<AttachedSlider>> t;
  std::unique_ptr<LinkableControl<AttachedSlider>> s;

  LinkableControlTable tableSettings;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SplineKnotEditor)
};
