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

#include "SplineEditor.h"

SplineEditor::SplineEditor(SplineParameters& parameters,
                           AudioProcessorValueTreeState& apvts,
                           WaveShaperParameters* waveShaperParameters)
  : parameters(parameters)
  , spline(parameters, apvts, [this]() { OnSplineChange(); })
  , rangeX(parameters.rangeX)
  , rangeY(parameters.rangeY)
  , rangeTan(parameters.rangeTan)
  , waveShaperParameters(waveShaperParameters)
{
  if (waveShaperParameters) {
    waveShaperHolder.Initialize<JUICY_MAX_WAVESHAPER_EDITOR_NUM_NODES>();
  }
  else {
    splineHolder.Initialize<JUICY_MAX_SPLINE_EDITOR_NUM_NODES>();
  }

  setSize(400, 400);

  areaInWhichToDrawNodes = getBounds();

  startTimer(50);
}

void
SplineEditor::paint(Graphics& g)
{
  ScopedNoDenormals noDenormals;

  auto const bounds = getLocalBounds().toFloat();

  g.fillAll(backgroundColour);
  g.setFont(font);

  // grid vertical lines
  {
    float const cellWidth =
      (pixelToX(getWidth()) - pixelToX(0.f)) / (float)numGridLines.x;

    float x = cellWidth * std::ceil(pixelToX(0) / cellWidth);

    float const cellWidthPixels = getWidth() / (float)numGridLines.x;

    for (int i = 0; i < numGridLines.x; ++i) {
      float const xCoord = xToPixel(x);

      if (xCoord >= getWidth()) {
        break;
      }

      if (xCoord <= 0.f) {
        x += cellWidth;
        continue;
      }

      g.setColour(gridColour);
      g.drawLine(Line(xCoord, 0.f, xCoord, (float)getHeight()));

      auto const textRectangle =
        Rectangle{ xCoord + 4.f, 4.f, cellWidthPixels - 6.f, 20.f };

      if (bounds.contains(textRectangle)) {
        g.setColour(gridLabelColour);
        g.drawText(String(x, 2), textRectangle, Justification::left);
      }

      x += cellWidth;
    }
  }

  // grid horizontal lines
  {
    float const cellHeight =
      (pixelToY(0.f) - pixelToY(getHeight())) / (float)numGridLines.y;
    float y = cellHeight * std::ceil(pixelToY(getHeight()) / cellHeight);

    for (int i = 0; i < numGridLines.y; ++i) {

      float yCoord = yToPixel(y);

      if (yCoord <= 0.f) {
        break;
      }

      if (yCoord >= getHeight()) {
        y += cellHeight;
        continue;
      }

      g.setColour(gridColour);
      g.drawLine(Line(0.f, yCoord, (float)getWidth(), yCoord));

      auto textRectangle = Rectangle{ 4.f, yCoord - 4.f, 50.f, 20.f };

      if (bounds.contains(textRectangle)) {
        g.setColour(gridLabelColour);
        g.drawText(String(y, 2), textRectangle, Justification::left);
      }

      y += cellHeight;
    }
  }

  // vumeter

  if (vuMeter[0] && vuMeter[1] && splineDsp) {
    vuMeterBuffer[0][0] = vuMeter[0]->load();
    vuMeterBuffer[0][1] = vuMeter[1]->load();
    int const x0 = std::round(xToPixel(vuMeterBuffer[0][0]));
    int const x1 = std::round(xToPixel(vuMeterBuffer[0][1]));
    splineDsp->ProcessBlock(vuMeterBuffer, vuMeterBuffer);
    int const y0 = std::round(yToPixel(vuMeterBuffer[0][0]));
    int const y1 = std::round(yToPixel(vuMeterBuffer[0][1]));
    g.setColour(vuMeterColours[1]);
    g.drawLine(x1, y1, x1, (float)getHeight());
    g.drawLine(0.f, y1, x1, y1);
    g.setColour(vuMeterColours[0]);
    g.drawLine(x0, y0, x0, (float)getHeight());
    g.drawLine(0.f, y0, x0, y0);
  }

  // curves

  if (redrawCurvesFlag) {
    if (waveShaperParameters) {

      waveShaperDsp = parameters.updateSpline(waveShaperHolder);

      for (int c = 0; c < 2; ++c) {
        waveShaperDsp->SetIsSymmetric(
          waveShaperParameters->symmetry.get(c)->getValue() >= 0.5f);
        waveShaperDsp->SetDc(waveShaperParameters->dc.get(c)->get(), c);
        waveShaperDsp->SetWet(waveShaperParameters->dryWet.get(c)->get(), c);
        waveShaperDsp->SetHighPassFrequency(
          waveShaperParameters->dcCutoff.get(c)->get(), c);
      }

      waveShaperDsp->Reset();
      waveShaperDsp->ProcessBlock(inputBuffer, outputbuffer);
    }
    else {
      splineDsp = parameters.updateSpline(splineHolder);
      splineDsp->Reset();
      splineDsp->ProcessBlock(inputBuffer, outputbuffer);
    }
    redrawCurvesFlag = false;
  }

  constexpr float lineThickness = 1;

  for (int c = 1; c >= 0; --c) {
    Path path;

    float prevY = yToPixel(outputbuffer[0][c]);

    for (int i = 1; i < getWidth(); ++i) {

      float const y = jlimit(
        -10.f, getHeight() + 10.f, yToPixelUnclamped(outputbuffer[i][c]));

      path.addLineSegment(Line<float>(i - 1, prevY, i, y), lineThickness);

      prevY = y;
    }
    g.setColour(curveColours[c]);
    g.strokePath(path, PathStrokeType(lineThickness));
  }

  // get mouse position

  bool const isMouseInside = isMouseOver();
  auto const mousePosition = getMouseXYRelative();

  // nodes

  bool const forceNodeDrawing = areaInWhichToDrawNodes.contains(mousePosition);

  if (isMouseInside || forceNodeDrawing) {

    auto const fillNode = [&](Point<float> centre, float diameter) {
      g.fillEllipse(centre.x - diameter * 0.5f,
                    centre.y - diameter * 0.5f,
                    diameter,
                    diameter);
    };

    for (auto& node : spline.nodes) {

      for (int c = 1; c >= 0; --c) {
        auto& params = node.parameters[c];

        Point<float> const coord = { xToPixel(params.x->getValue()),
                                     yToPixel(params.y->getValue()) };

        if (bounds.contains(coord)) {

          bool const isEnabled =
            c == 0 ? node.enabled->getValue()
                   : node.enabled->getValue() && !node.linked->getValue();

          g.setColour(isEnabled ? nodeColours[c]
                                : nodeColours[c].darker(0.5f).withAlpha(0.5f));

          fillNode(coord, bigControlPointSize);

          float const t = params.t->getValue();
          float const dx = widgetOffset / sqrt(1.f + t * t);
          float const dy = -dx * t;
          auto const dt = Point<float>(dx, dy);
          auto const ds = Point<float>(dy, -dx);

          auto const leftTan = coord - dt;
          auto const rightTan = coord + dt;
          auto const smooth = coord - ds;

          fillNode(leftTan, smallControlPointSize);
          fillNode(rightTan, smallControlPointSize);
          fillNode(smooth, smallControlPointSize);

          g.drawLine(Line<float>(leftTan, rightTan), lineThickness);
          g.drawLine(Line<float>(coord, smooth), lineThickness);
        }
      }
    }
  }

  // mouse coordinates

  if (isMouseInside) {

    float const x = pixelToX(mousePosition.x);
    float const y = pixelToY(mousePosition.y);

    String const text = "x=" + String(x, 2) + ", y=" + String(y, 2);

    g.setColour(mousePositionColour);
    g.drawText(text,
               Rectangle{ 0, getHeight() - 25, getWidth() - 10, 20 },
               Justification::right);
  }
}

void
SplineEditor::resized()
{
  setupZoom({ 0.5f * getWidth(), 0.5f * getHeight() }, { 1.f, 1.f });
}

void
SplineEditor::mouseDown(MouseEvent const& event)
{
  interactingChannel = getChannel(event);

  float minDistance = getWidth() + getHeight();
  int nearestNodeIndex = -1;
  Point<float> nearestNodeCoord;

  for (int n = 0; n < spline.nodes.size(); ++n) {

    auto nodeCoord = getNodeCoord(n, interactingChannel);
    float distance = nodeCoord.getDistanceFrom(event.position);

    if (distance < minDistance) {
      minDistance = distance;
      nearestNodeIndex = n;
      nearestNodeCoord = nodeCoord;
    }
  }

  if (nearestNodeIndex == -1) {
    interaction = InteractionType::Movement;
    prevOffset = offset;
    return;
  }

  auto& params = spline.nodes[nearestNodeIndex].parameters[interactingChannel];

  float const radius = 0.5f * widgetOffset;

  bool hit = false;

  if (minDistance <= radius) {
    interaction = InteractionType::Value;
    params.x->dragStarted();
    params.y->dragStarted();
    hit = true;
  }
  else {
    float const t = params.t->getValue();
    float const dx = widgetOffset / sqrt(1.f + t * t);
    float const dy = -dx * t;
    auto const dt = Point<float>(dx, dy);

    if (event.position.getDistanceFrom(nearestNodeCoord + dt) <= radius) {
      interaction = InteractionType::RightTangent;
      interactionBuffer = params.t->getValue();
      params.t->dragStarted();
      hit = true;
    }
    else if (event.position.getDistanceFrom(nearestNodeCoord - dt) <= radius) {
      interaction = InteractionType::LeftTangent;
      interactionBuffer = params.t->getValue();
      params.t->dragStarted();
      hit = true;
    }
    else if (event.position.getDistanceFrom(nearestNodeCoord -
                                            Point<float>(dy, -dx)) <= radius) {
      interaction = InteractionType::Smoothing;
      interactionBuffer = params.s->getValue();
      params.s->dragStarted();
      hit = true;
    }
    else {
      interaction = InteractionType::Movement;
      prevOffset = offset;
    }
  }

  if (hit) {
    selectedNode = nearestNodeIndex;
    if (nodeEditor) {
      nodeEditor->setSelectedNode(selectedNode);
    }
  }
}

void
SplineEditor::mouseDrag(MouseEvent const& event)
{
  if (interaction == InteractionType::Movement) {
    offset.x = prevOffset.x - event.getDistanceFromDragStartX();
    offset.x = jlimit(0.f, getWidth() * (zoom.x - 1.f), offset.x);
    offset.y = prevOffset.y + event.getDistanceFromDragStartY();
    offset.y = jlimit(0.f, getHeight() * (zoom.y - 1.f), offset.y);
    setupSplineInputBuffer();
  }

  auto& params = spline.nodes[selectedNode].parameters[interactingChannel];

  float constexpr tangentDragSpeed = 0.030625;
  float constexpr smoothnessDragSpeed = 0.005;

  switch (interaction) {

    case InteractionType::Value: {
      float const x = pixelToX(event.position.x);
      float const y = pixelToY(event.position.y);
      params.x->setValueFromGui(x);
      params.y->setValueFromGui(y);
    } break;

    case InteractionType::LeftTangent: {
      float const d = tangentDragSpeed * event.getDistanceFromDragStartY();
      params.t->setValueFromGui(interactionBuffer + d);
    } break;

    case InteractionType::RightTangent: {
      float const d = tangentDragSpeed * event.getDistanceFromDragStartY();
      params.t->setValueFromGui(interactionBuffer - d);
    } break;

    case InteractionType::Smoothing: {
      float const d = smoothnessDragSpeed * event.getDistanceFromDragStartX();
      params.s->setValueFromGui(interactionBuffer + d);
    } break;

    default:
    case InteractionType::Movement:
      break;
  }
}

void
SplineEditor::mouseUp(MouseEvent const& event)
{
  if (interaction == InteractionType::Movement) {
    return;
  }

  auto& params = spline.nodes[selectedNode].parameters[interactingChannel];

  switch (interaction) {

    case InteractionType::Value: {
      params.x->dragEnded();
      params.y->dragEnded();
    } break;

    case InteractionType::LeftTangent: {
      params.t->dragEnded();
    } break;

    case InteractionType::RightTangent: {
      params.t->dragEnded();
    } break;

    case InteractionType::Smoothing: {
      params.s->dragEnded();
    } break;

    default:
    case InteractionType::Movement:
      break;
  }
}

void
SplineEditor::mouseDoubleClick(MouseEvent const& event)
{
  float minDistance = getWidth() + getHeight();
  int node = -1;
  int channel = -1;
  Point<float> nodeCoord;

  for (int c = 0; c < 2; ++c) {
    for (int n = 0; n < spline.nodes.size(); ++n) {

      nodeCoord = getNodeCoord(n, c);
      float distance = nodeCoord.getDistanceFrom(event.position);

      if (distance < minDistance) {
        minDistance = distance;
        node = n;
        channel = c;
      }
    }
  }

  if (node == -1) {
    return;
  }

  if (minDistance > widgetOffset) {
    return;
  }

  if (channel == 0) {
    spline.nodes[node].enabled->invertValueFromGui();
  }
  else {
    spline.nodes[node].linked->invertValueFromGui();
  }

  selectedNode = node;

  if (nodeEditor) {
    nodeEditor->setSelectedNode(selectedNode);
  }
}

void
SplineEditor::mouseWheelMove(MouseEvent const& event,
                             MouseWheelDetails const& wheel)
{
  mouseMagnify(event, 1.f + wheel.deltaY * wheelToZoomScaleFactor);
}

void
SplineEditor::mouseMagnify(MouseEvent const& event, float scaleFactor)
{
  auto const newZoom = Point<float>(jmax(1.f, scaleFactor * zoom.x),
                                    jmax(1.f, scaleFactor * zoom.y));

  setupZoom(event.position, newZoom);
}

void
SplineEditor::setSelectedNode(int node)
{
  selectedNode = node;
  repaint();
}

void
SplineEditor::OnSplineChange()
{
  redrawCurvesFlag = true;
  repaint();
}

float
SplineEditor::pixelToX(float pixel)
{
  return rangeX.convertFrom0to1(
    (jlimit(0.f, 1.f, (pixel + offset.x) / (getWidth() * zoom.x))));
}

float
SplineEditor::xToPixel(float x)
{
  return rangeX.convertTo0to1(rangeX.snapToLegalValue(x)) *
           (getWidth() * zoom.x) -
         offset.x;
}

float
SplineEditor::pixelToY(float pixel)
{
  return rangeY.convertFrom0to1(jlimit(
    0.f, 1.f, (getHeight() - pixel + offset.y) / (getHeight() * zoom.y)));
}

float
SplineEditor::yToPixel(float y)
{
  return getHeight() - (rangeY.convertTo0to1(rangeY.snapToLegalValue(y)) *
                          (getHeight() * zoom.y) -
                        offset.y);
}

float
SplineEditor::yToPixelUnclamped(float y)
{
  return getHeight() - (((y - rangeY.start) / (rangeY.end - rangeY.start)) *
                          (getHeight() * zoom.y) -
                        offset.y);
}

void
SplineEditor::setupSplineInputBuffer()
{
  inputBuffer.SetNumSamples(getWidth());
  outputbuffer.SetNumSamples(getWidth());

  for (int i = 0; i < getWidth(); ++i) {
    inputBuffer[i] = pixelToX(i);
  }

  redrawCurvesFlag = true;
}

int
SplineEditor::getChannel(MouseEvent const& event)
{
  return (event.mods.isAltDown() || event.mods.isRightButtonDown()) ? 1 : 0;
}

void
SplineEditor::setupZoom(Point<float> fixedPoint, Point<float> newZoom)
{
  offset.x = (newZoom.x / zoom.x) * (fixedPoint.x + offset.x) - fixedPoint.x;
  offset.y = (newZoom.y / zoom.y) * (getHeight() - fixedPoint.y + offset.y) -
             getHeight() + fixedPoint.y;

  zoom = newZoom;

  offset.x = jlimit(0.f, getWidth() * (zoom.x - 1.f), offset.x);
  offset.y = jlimit(0.f, getHeight() * (zoom.y - 1.f), offset.y);

  setupSplineInputBuffer();
}

Point<float>
SplineEditor::getNodeCoord(int nodeIndex, int channel)
{
  auto& nodeParams = spline.nodes[nodeIndex].parameters[channel];
  return Point<float>(xToPixel(nodeParams.x->getValue()),
                      yToPixel(nodeParams.y->getValue()));
}

SplineAttachments::SplineAttachments(SplineParameters& parameters,
                                     AudioProcessorValueTreeState& apvts,
                                     std::function<void(void)> onChange)
{
  auto const makeNodeAttachments =
    [&](SplineParameters::LinkableNodeParameters node, int channel) {
      return SplineAttachments::NodeAttachments{
        FloatAttachment::New(apvts,
                             node.parameters[channel].x->paramID,
                             onChange,
                             parameters.rangeX),
        FloatAttachment::New(apvts,
                             node.parameters[channel].y->paramID,
                             onChange,
                             parameters.rangeY),
        FloatAttachment::New(apvts,
                             node.parameters[channel].t->paramID,
                             onChange,
                             parameters.rangeTan),
        FloatAttachment::New(apvts,
                             node.parameters[channel].s->paramID,
                             onChange,
                             NormalisableRange<float>{ 0.f, 1.f, 0.01f })
      };
    };

  for (auto& node : parameters.nodes) {
    nodes.push_back(SplineAttachments::LinkableNodeAttachments{
      std::array<SplineAttachments::NodeAttachments, 2>{
        { makeNodeAttachments(node, 0), makeNodeAttachments(node, 1) } },
      BoolAttachment::New(apvts, node.enabled.getID(), onChange),
      BoolAttachment::New(apvts, node.linked.getID(), onChange) });
  }
}

int
SplineAttachments::getNumActiveNodes()
{
  int numNodes = 0;
  for (auto& node : nodes) {
    if (node.enabled->getValue()) {
      ++numNodes;
    }
  }
  return numNodes;
}

SplineNodeEditor::SplineNodeEditor(SplineParameters& parameters,
                                   AudioProcessorValueTreeState& apvts,
                                   String const& midSideParamID)
  : parameters(parameters)
  , apvts(apvts)
  , enabled(*this, apvts)
  , linked(*this, apvts)
  , channelLabels(apvts, midSideParamID, false)
{
  enabled.getControl().setButtonText("Node is Active");
  linked.getControl().setButtonText("Node is Linked");

  addAndMakeVisible(label);
  addAndMakeVisible(channelLabels);
  addAndMakeVisible(selectedNode);

  for (int i = 1; i <= parameters.nodes.size(); ++i) {
    selectedNode.addItem(std::to_string(i), i);
  }

  selectedNode.onChange = [this] {
    int node = selectedNode.getSelectedId() - 1;
    setNode(node);
    if (splineEditor) {
      splineEditor->setSelectedNode(node);
    }
  };

  label.setFont(label.getFont().boldened());

  setOpaque(false);
  setSize(360, 120);

  setNode(0);

  startTimer(50);
}

void
SplineNodeEditor::resized()
{
  int const rowHeight = getHeight() / 4;

  label.setTopLeftPosition(0, 0);
  label.setSize(116, rowHeight);
  selectedNode.setTopLeftPosition(116, rowHeight * 0.1);
  selectedNode.setSize(60, rowHeight * 0.8);

  Grid grid;
  using Track = Grid::TrackInfo;

  grid.templateRows = { Track(1_fr) };
  grid.templateColumns = { Track(1_fr), Track(1_fr) };
  grid.items = { GridItem(enabled.getControl()),
                 GridItem(linked.getControl()) };

  grid.performLayout({ 220, 0, getWidth() - 220, rowHeight });

  int const secondRow = rowHeight;
  int left = 0;

  auto const resize = [&](auto& component, int width) {
    component.setTopLeftPosition(left, secondRow);
    component.setSize(width, rowHeight * 3);
    left += width - 1;
  };

  resize(channelLabels, 50);

  if (!x || !y || !s || !t) {
    return;
  }

  int width = std::floor(((float)getWidth() - 45.f) / 4.f);

  resize(*x, width);
  resize(*y, width);
  resize(*t, width);
  resize(*s, width);
}

void
SplineNodeEditor::paint(Graphics& g)
{
  int right = s->getBounds().getRight();
  g.setColour(tableSettings.backgroundColour);
  g.fillRect(0, 0, right, getHeight() / 4);

  g.setColour(tableSettings.lineColour);
  g.drawRect(0, 0, right, 1 + getHeight() / 4);
}

void
SplineNodeEditor::setSelectedNode(int newNodeIndex)
{
  setNode(newNodeIndex);
  selectedNode.setSelectedId(newNodeIndex + 1, sendNotification);
}

void
SplineNodeEditor::setTableSettings(LinkableControlTable tableSettings)
{
  this->tableSettings = tableSettings;
  channelLabels.tableSettings = tableSettings;
  x->tableSettings = tableSettings;
  y->tableSettings = tableSettings;
  t->tableSettings = tableSettings;
  s->tableSettings = tableSettings;
}

void
SplineNodeEditor::setNode(int newNodeIndex)
{
  if (nodeIndex == newNodeIndex) {
    return;
  }

  nodeIndex = newNodeIndex;

  auto& node = parameters.nodes[nodeIndex];

  auto& linkedParamID = node.linked.getID();
  auto& enabledParamID = node.enabled.getID();

  linked.setParameter(linkedParamID);
  enabled.setParameter(enabledParamID);

  bool isLinked = apvts.getParameter(linkedParamID)->getValue() >= 0.5f;

  if (x) {
    removeChildComponent(x.get());
  }

  x = std::make_unique<LinkableControl<AttachedSlider>>(
    apvts,
    "x",
    linkedParamID,
    node.parameters[0].x->paramID,
    node.parameters[1].x->paramID,
    false);

  addAndMakeVisible(*x);

  if (y) {
    removeChildComponent(y.get());
  }

  y = std::make_unique<LinkableControl<AttachedSlider>>(
    apvts,
    "y",
    linkedParamID,
    node.parameters[0].y->paramID,
    node.parameters[1].y->paramID,
    false);

  addAndMakeVisible(*y);

  if (t) {
    removeChildComponent(t.get());
  }

  t = std::make_unique<LinkableControl<AttachedSlider>>(
    apvts,
    "Tangent",
    linkedParamID,
    node.parameters[0].t->paramID,
    node.parameters[1].t->paramID,
    false);

  addAndMakeVisible(*t);

  if (s) {
    removeChildComponent(s.get());
  }

  s = std::make_unique<LinkableControl<AttachedSlider>>(
    apvts,
    "Smoothness",
    linkedParamID,
    node.parameters[0].s->paramID,
    node.parameters[1].s->paramID,
    false);

  addAndMakeVisible(*s);

  setTableSettings(tableSettings);

  resized();
}

void
AttachSplineEditorsAndInitialize(SplineEditor& splineEditor,
                                 SplineNodeEditor& nodeEditor,
                                 int selectedNode)
{
  splineEditor.nodeEditor = &nodeEditor;
  nodeEditor.splineEditor = &splineEditor;
  nodeEditor.setSelectedNode(selectedNode);
}
