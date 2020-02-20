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
#include "avec/dsp/Spline.hpp"
#include <JuceHeader.h>

// use these macro to define the maximum number of nodes supported

#ifndef JUICY_MAX_SPLINE_EDITOR_NUM_NODES
#define JUICY_MAX_SPLINE_EDITOR_NUM_NODES 8
#endif // !MAX_SPLINE_EDITOR_NUM_NODES

#ifndef JUICY_MAX_WAVESHAPER_EDITOR_NUM_NODES
#define JUICY_MAX_WAVESHAPER_EDITOR_NUM_NODES 17
#endif // !MAX_SPLINE_EDITOR_NUM_NODES

struct SplineAttachments
{
  struct NodeAttachments
  {
    std::unique_ptr<FloatAttachment> x;
    std::unique_ptr<FloatAttachment> y;
    std::unique_ptr<FloatAttachment> t;
    std::unique_ptr<FloatAttachment> s;
  };

  struct LinkableNodeAttachments
  {
    std::array<NodeAttachments, 2> parameters;
    std::unique_ptr<BoolAttachment> enabled;
    std::unique_ptr<BoolAttachment> linked;
  };

  std::vector<LinkableNodeAttachments> nodes;

  SplineAttachments(SplineParameters& parameters,
                    AudioProcessorValueTreeState& apvts,
                    std::function<void(void)> onChange);

  int getNumActiveNodes();
};

class SplineEditor;
class SplineNodeEditor;

void
AttachSplineEditorsAndInitialize(SplineEditor& splineEditor,
                                 SplineNodeEditor& nodeEditor,
                                 int selectedNode = 0);

class SplineEditor
  : public Component
  , public Timer
{
  friend void AttachSplineEditorsAndInitialize(SplineEditor& splineEditor,
                                               SplineNodeEditor& nodeEditor,
                                               int selectedNode);

public:
  SplineEditor(SplineParameters& parameters,
               AudioProcessorValueTreeState& apvts,
               WaveShaperParameters* waveShaperParameters = nullptr);

  void paint(Graphics&) override;
  void resized() override;

  void mouseDown(MouseEvent const& event) override;
  void mouseDrag(MouseEvent const& event) override;
  void mouseUp(MouseEvent const& event) override;
  void mouseDoubleClick(MouseEvent const& event) override;
  void mouseWheelMove(MouseEvent const& event,
                      MouseWheelDetails const& wheel) override;
  void mouseMagnify(MouseEvent const& event, float scaleFactor) override;

  void setSelectedNode(int node);

  juce::Rectangle<int> areaInWhichToDrawNodes;
  // When the mouse is inside the editor, the spline control points will be
  // drawn on top of the curve. To have them drawn also when the mouse is
  // over the a SplineNodeEditor instance, you can use the
  // areaInWhichToDrawNodes member. It is a Rectangle in the coordinates of the
  // parent component. When the mouse is in it, the control points will be
  // drawn.

  // These members can be used to customize the appearance of the editor

  float widgetOffset = 20.f;

  float bigControlPointSize = 10.f;

  float smallControlPointSize = 6.f;

  Point<int> numGridLines = { 8, 8 };

  std::array<std::atomic<float>*, 2> vuMeter = { { nullptr, nullptr } };

  Colour backgroundColour = Colours::black;

  Colour gridColour = Colours::darkgrey.darker(1.f);

  Colour gridLabelColour = Colours::darkgrey;

  Colour mousePositionColour = Colours::white;

  std::array<Colour, 2> curveColours = { { Colours::blue, Colours::red } };

  std::array<Colour, 2> nodeColours = { { Colours::steelblue,
                                          Colours::orangered } };

  std::array<Colour, 2> vuMeterColours = { { Colours::cadetblue,
                                             Colours::coral } };

  Font font = Font(12);

  float wheelToZoomScaleFactor = 0.25f;

private:
  SplineParameters& parameters;
  SplineNodeEditor* nodeEditor = nullptr;

  Point<float> getNodeCoord(int nodeIndex, int channel);

  int getChannel(MouseEvent const& event);

  void setupZoom(Point<float> fixedPoint, Point<float> newZoom);

  void timerCallback() override { repaint(); }

  SplineAttachments spline;

  NormalisableRange<float> rangeX;
  NormalisableRange<float> rangeY;
  NormalisableRange<float> rangeTan;

  std::atomic<bool> redrawCurvesFlag{ true };

  VecBuffer<Vec2d> vuMeterBuffer{ 1 };

  void OnSplineChange();

  int selectedNode = 0;

  enum class InteractionType
  {
    Movement,
    Value,
    LeftTangent,
    RightTangent,
    Smoothing
  };

  InteractionType interaction = InteractionType::Movement;
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

  avec::SplineHolder<avec::Spline, Vec2d> splineHolder;
  avec::SplineInterface<Vec2d>* splineDsp = nullptr;

  WaveShaperParameters* waveShaperParameters;
  avec::SplineHolder<avec::WaveShaper, Vec2d> waveShaperHolder;
  avec::WaveShaperInterface<Vec2d>* waveShaperDsp = nullptr;

  avec::VecBuffer<Vec2d> inputBuffer;
  avec::VecBuffer<Vec2d> outputBuffer;

  void setupSplineInputBuffer();

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SplineEditor)
};

class SplineNodeEditor
  : public Component
  , public Timer
{
  friend void AttachSplineEditorsAndInitialize(SplineEditor& splineEditor,
                                               SplineNodeEditor& nodeEditor,
                                               int selectedNode);

public:
  SplineNodeEditor(SplineParameters& parameters,
                   AudioProcessorValueTreeState& apvts,
                   String const& midSideParamID = "Mid-Side");

  void resized() override;

  void paint(Graphics& g) override;

  void setSelectedNode(int newNodeIndex);

  void setTableSettings(LinkableControlTable tableSettings);

private:
  void setNode(int newNodeIndex);

  void timerCallback() override { repaint(); }

  SplineEditor* splineEditor = nullptr;

  int nodeIndex = -1;

  SplineParameters& parameters;
  AudioProcessorValueTreeState& apvts;

  Label label{ {}, "Selected Node" };

  ComboBox selectedNode;

  AttachedToggle enabled;
  AttachedToggle linked;

  ChannelLabels channelLabels;

  std::unique_ptr<LinkableControl<AttachedSlider>> x;
  std::unique_ptr<LinkableControl<AttachedSlider>> y;
  std::unique_ptr<LinkableControl<AttachedSlider>> t;
  std::unique_ptr<LinkableControl<AttachedSlider>> s;

  LinkableControlTable tableSettings;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SplineNodeEditor)
};
