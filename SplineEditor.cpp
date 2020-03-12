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

SplineEditor::SplineEditor(
  SplineParameters& parameters,
  AudioProcessorValueTreeState& apvts,
  LinkableParameter<WrappedBoolParameter>* symmetryParameter)
  : parameters(parameters)
  , spline(
      parameters,
      apvts,
      [this]() { onSplineChange(); },
      symmetryParameter)
  , rangeX(parameters.rangeX)
  , rangeY(parameters.rangeY)
  , rangeTan(parameters.rangeTan)
  , symmetryParameter(symmetryParameter)
  , splineDsp(avec::Aligned<Spline>::make())
{
  setSize(400, 400);

  areaInWhichToDrawKnots = getBounds();

  startTimer(50);
}

void
SplineEditor::paint(Graphics& g)
{
  ScopedNoDenormals noDenormals;

  auto const bounds = getLocalBounds().toFloat();
  bool const isMouseInside = isMouseOver();
  auto const mousePosition = getMouseXYRelative();

  constexpr float lineThickness = 1;

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

  int const numKnots = parameters.updateSpline(*splineDsp);

  // vumeter

  if (vuMeter[0] && vuMeter[1]) {
    vuMeterBuffer[0][0] = vuMeter[0]->load();
    vuMeterBuffer[0][1] = vuMeter[1]->load();
    int const x0 = std::round(xToPixel(vuMeterBuffer[0][0]));
    int const x1 = std::round(xToPixel(vuMeterBuffer[0][1]));
    splineDsp->processBlock(vuMeterBuffer, vuMeterBuffer, numKnots);
    int const y0 = std::round(yToPixel(vuMeterBuffer[0][0]));
    int const y1 = std::round(yToPixel(vuMeterBuffer[0][1]));
    g.setColour(vuMeterColours[1]);
    g.drawLine(x1, y1, x1, (float)getHeight());
    g.drawLine(0.f, y1, x1, y1);
    g.setColour(vuMeterColours[0]);
    g.drawLine(x0, y0, x0, (float)getHeight());
    g.drawLine(0.f, y0, x0, y0);
  }

  // knots

  bool const forceKnotDrawing = areaInWhichToDrawKnots.contains(mousePosition);

  if (isMouseInside || forceKnotDrawing) {

    auto const fillKnot = [&](Point<float> centre, float diameter) {
      g.drawEllipse(centre.x - diameter * 0.5f,
                    centre.y - diameter * 0.5f,
                    diameter,
                    diameter,
                    1.f);
    };

    // halo around selcted knots

    auto const fillHalo = [&](int channel) {
      auto const coord = getKnotCoord(selectedKnot, channel);

      auto const& knot = spline.knots[selectedKnot];

      bool const isEnabled =
        channel == 0 ? knot.enabled->getValue()
                     : knot.enabled->getValue() && !knot.linked->getValue();

      auto const diameter = isEnabled ? 2.f * widgetOffset : widgetOffset;

      g.setGradientFill(ColourGradient(haloColours[channel],
                                       coord.x,
                                       coord.y,
                                       Colours::transparentBlack,
                                       coord.x + diameter * 0.5f,
                                       coord.y,
                                       true));

      g.fillEllipse(coord.x - diameter * 0.5f,
                    coord.y - diameter * 0.5f,
                    diameter,
                    diameter);
    };

    fillHalo(0);
    fillHalo(1);

    // knots

    for (auto& knot : spline.knots) {

      for (int c = 1; c >= 0; --c) {
        auto& params = knot.parameters[c];

        Point<float> const coord = { xToPixel(params.x->getValue()),
                                     yToPixel(params.y->getValue()) };

        if (bounds.contains(coord)) {

          bool const isEnabled =
            c == 0 ? knot.enabled->getValue()
                   : knot.enabled->getValue() && !knot.linked->getValue();

          g.setColour(isEnabled ? knotColours[c]
                                : knotColours[c].darker(0.5f).withAlpha(0.5f));

          fillKnot(coord, bigKnotSize);

          float const t = params.t->getValue();
          float const dx = widgetOffset / sqrt(1.f + t * t);
          float const dy = -dx * t;
          auto const dt = Point<float>(dx, dy);
          auto const ds = Point<float>(dy, -dx);

          auto const leftTan = coord - dt;
          auto const rightTan = coord + dt;
          auto const smooth = coord - ds;

          fillKnot(leftTan, smallKnotSize);
          fillKnot(rightTan, smallKnotSize);
          fillKnot(smooth, smallKnotSize);

          g.drawLine(Line<float>(leftTan, rightTan), lineThickness);
          g.drawLine(Line<float>(coord, smooth), lineThickness);
        }
      }
    }
  }

  // curves

  if (redrawCurvesFlag) {
    redrawCurvesFlag = false;

    if (symmetryParameter) {
      for (int c = 0; c < 2; ++c) {
        splineDsp->setIsSymmetric(symmetryParameter->get(c)->getValue() >=
                                  0.5f);
      }
    }
    splineDispatcher.processBlock(
      *splineDsp, inputBuffer, outputBuffer, numKnots);
  }

  for (int c = 1; c >= 0; --c) {
    Path path;

    float prevY = yToPixel(outputBuffer[0][c]);

    for (int i = 1; i < getWidth(); ++i) {

      float const y = jlimit(
        -10.f, getHeight() + 10.f, yToPixelUnclamped(outputBuffer[i][c]));

      path.addLineSegment(Line<float>(i - 1, prevY, i, y), lineThickness);

      prevY = y;
    }
    g.setColour(curveColours[c]);
    g.strokePath(path, PathStrokeType(lineThickness));
  }

  // mouse coordinates

  if (isMouseInside) {

    float const x = pixelToX(mousePosition.x);
    float const y = pixelToY(mousePosition.y);

    String const text =
      "x=" + String(x, 2) + xSuffix + ", y=" + String(y, 2) + ySuffix;

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

SplineEditor::KnotSelectionResult
SplineEditor::selectKnot(MouseEvent const& event)
{
  float maxDistance = getWidth() + getHeight();
  float minDistances[2] = { maxDistance, maxDistance };
  int knots[2] = { -1, -1 };
  Point<float> knotCoords[2];

  for (int c = 0; c < 2; ++c) {
    for (int n = 0; n < spline.knots.size(); ++n) {

      knotCoords[c] = getKnotCoord(n, c);
      float distance = knotCoords[c].getDistanceFrom(event.position);

      if (distance < minDistances[c]) {
        minDistances[c] = distance;
        knots[c] = n;
      }
    }
  }

  interactingChannel =
    (event.mods.isAltDown() || event.mods.isRightButtonDown()) ? 1 : 0;

  interactingChannel =
    interactingChannel == 1 ? 1 : (minDistances[0] <= minDistances[1] ? 0 : 1);

  return { knots[interactingChannel], minDistances[interactingChannel] };
}

void
SplineEditor::mouseDown(MouseEvent const& event)
{
  auto [knot, minDistance] = selectKnot(event);

  if (knot == -1) {
    interaction = InteractionType::movement;
    prevOffset = offset;
    return;
  }

  Point<float> knotCoord = getKnotCoord(knot, interactingChannel);

  auto& params = spline.knots[knot].parameters[interactingChannel];

  float const radius = 0.5f * widgetOffset;

  bool hit = false;

  if (minDistance <= radius) {
    interaction = InteractionType::value;
    params.x->dragStarted();
    params.y->dragStarted();
    hit = true;
  }
  else {
    float const t = params.t->getValue();
    float const dx = widgetOffset / sqrt(1.f + t * t);
    float const dy = -dx * t;
    auto const dt = Point<float>(dx, dy);

    if (event.position.getDistanceFrom(knotCoord + dt) <= radius) {
      interaction = InteractionType::rightTangent;
      interactionBuffer = params.t->getValue();
      params.t->dragStarted();
      hit = true;
    }
    else if (event.position.getDistanceFrom(knotCoord - dt) <= radius) {
      interaction = InteractionType::leftTangent;
      interactionBuffer = params.t->getValue();
      params.t->dragStarted();
      hit = true;
    }
    else if (event.position.getDistanceFrom(knotCoord -
                                            Point<float>(dy, -dx)) <= radius) {
      interaction = InteractionType::smoothing;
      interactionBuffer = params.s->getValue();
      params.s->dragStarted();
      hit = true;
    }
    else {
      interaction = InteractionType::movement;
      prevOffset = offset;
    }
  }

  if (hit) {
    selectedKnot = knot;
    if (knotEditor) {
      knotEditor->setSelectedKnot(selectedKnot);
    }
  }
}

void
SplineEditor::mouseDrag(MouseEvent const& event)
{
  if (interaction == InteractionType::movement) {
    offset.x = prevOffset.x - event.getDistanceFromDragStartX();
    offset.x = jlimit(0.f, getWidth() * (zoom.x - 1.f), offset.x);
    offset.y = prevOffset.y + event.getDistanceFromDragStartY();
    offset.y = jlimit(0.f, getHeight() * (zoom.y - 1.f), offset.y);
    setupSplineInputBuffer();
  }

  auto& params = spline.knots[selectedKnot].parameters[interactingChannel];

  float constexpr tangentDragSpeed = 0.030625;
  float constexpr smoothnessDragSpeed = 0.005;

  switch (interaction) {

    case InteractionType::value: {
      float const x = pixelToX(event.position.x);
      float const y = pixelToY(event.position.y);
      params.x->setValueFromGui(x);
      params.y->setValueFromGui(y);
    } break;

    case InteractionType::leftTangent: {
      float const d = tangentDragSpeed * event.getDistanceFromDragStartY();
      params.t->setValueFromGui(interactionBuffer + d);
    } break;

    case InteractionType::rightTangent: {
      float const d = tangentDragSpeed * event.getDistanceFromDragStartY();
      params.t->setValueFromGui(interactionBuffer - d);
    } break;

    case InteractionType::smoothing: {
      float const d = smoothnessDragSpeed * event.getDistanceFromDragStartX();
      params.s->setValueFromGui(interactionBuffer + d);
    } break;

    default:
    case InteractionType::movement:
      break;
  }
}

void
SplineEditor::mouseUp(MouseEvent const& event)
{
  if (interaction == InteractionType::movement) {
    return;
  }

  auto& params = spline.knots[selectedKnot].parameters[interactingChannel];

  switch (interaction) {

    case InteractionType::value: {
      params.x->dragEnded();
      params.y->dragEnded();
    } break;

    case InteractionType::leftTangent: {
      params.t->dragEnded();
    } break;

    case InteractionType::rightTangent: {
      params.t->dragEnded();
    } break;

    case InteractionType::smoothing: {
      params.s->dragEnded();
    } break;

    default:
    case InteractionType::movement:
      break;
  }
}

void
SplineEditor::mouseDoubleClick(MouseEvent const& event)
{
  auto [knot, minDistance] = selectKnot(event);

  if (knot == -1) {
    return;
  }

  if (minDistance > widgetOffset) {
    return;
  }

  if (interactingChannel == 0) {
    spline.knots[knot].enabled->invertValueFromGui();
  }
  else {
    spline.knots[knot].linked->invertValueFromGui();
  }

  selectedKnot = knot;

  if (knotEditor) {
    knotEditor->setSelectedKnot(selectedKnot);
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
SplineEditor::setSelectedKnot(int knot)
{
  selectedKnot = knot;
  repaint();
}

void
SplineEditor::onSplineChange()
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
  inputBuffer.setNumSamples(getWidth());
  outputBuffer.setNumSamples(getWidth());

  for (int i = 0; i < getWidth(); ++i) {
    inputBuffer[i] = pixelToX(i);
  }

  redrawCurvesFlag = true;
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
SplineEditor::getKnotCoord(int knotIndex, int channel)
{
  auto& knotParams = spline.knots[knotIndex].parameters[channel];
  return Point<float>(xToPixel(knotParams.x->getValue()),
                      yToPixel(knotParams.y->getValue()));
}

SplineAttachments::SplineAttachments(
  SplineParameters& parameters,
  AudioProcessorValueTreeState& apvts,
  std::function<void(void)> onChange,
  LinkableParameter<WrappedBoolParameter>* symmetryParameter)
{
  auto const makeKnotAttachments =
    [&](SplineParameters::LinkableKnotParameters knot, int channel) {
      return SplineAttachments::KnotAttachments{
        FloatAttachment::make(apvts,
                              knot.parameters[channel].x->paramID,
                              onChange,
                              parameters.rangeX),
        FloatAttachment::make(apvts,
                              knot.parameters[channel].y->paramID,
                              onChange,
                              parameters.rangeY),
        FloatAttachment::make(apvts,
                              knot.parameters[channel].t->paramID,
                              onChange,
                              parameters.rangeTan),
        FloatAttachment::make(apvts,
                              knot.parameters[channel].s->paramID,
                              onChange,
                              NormalisableRange<float>{ 0.f, 1.f, 0.01f })
      };
    };

  for (auto& knot : parameters.knots) {
    knots.push_back(SplineAttachments::LinkableKnotAttachments{
      std::array<SplineAttachments::KnotAttachments, 2>{
        { makeKnotAttachments(knot, 0), makeKnotAttachments(knot, 1) } },
      BoolAttachment::make(apvts, knot.enabled.getID(), onChange),
      BoolAttachment::make(apvts, knot.linked.getID(), onChange) });
  }

  if (symmetryParameter) {
    for (int c = 0; c < 2; ++c) {
      symmetry[c] =
        BoolAttachment::make(apvts, symmetryParameter->getID(c), onChange);
    }
  }
}

int
SplineAttachments::getNumActiveKnots()
{
  int numKnots = 0;
  for (auto& knot : knots) {
    if (knot.enabled->getValue()) {
      ++numKnots;
    }
  }
  return numKnots;
}

SplineKnotEditor::SplineKnotEditor(SplineParameters& parameters,
                                   AudioProcessorValueTreeState& apvts,
                                   String const& midSideParamID)
  : parameters(parameters)
  , apvts(apvts)
  , enabled(*this, apvts)
  , linked(*this, apvts)
  , channelLabels(apvts, midSideParamID, false)
{
  enabled.getControl().setButtonText("Knot is Active");
  linked.getControl().setButtonText("Knot is Linked");

  addAndMakeVisible(label);
  addAndMakeVisible(channelLabels);
  addAndMakeVisible(selectedKnot);

  for (int i = 1; i <= parameters.knots.size(); ++i) {
    selectedKnot.addItem(std::to_string(i), i);
  }

  selectedKnot.onChange = [this] {
    int knot = selectedKnot.getSelectedId() - 1;
    setKnot(knot);
    if (splineEditor) {
      splineEditor->setSelectedKnot(knot);
    }
  };

  label.setFont(label.getFont().boldened());

  setOpaque(false);
  setSize(360, 120);

  setKnot(0);

  startTimer(50);
}

void
SplineKnotEditor::resized()
{
  int const rowHeight = getHeight() / 4;

  float const widthFactor = getWidth() / 598.f;
  int const selectedWidth = (2.f / 5.f) * getWidth();

  label.setTopLeftPosition(0, 0);
  label.setSize(130 * widthFactor, rowHeight);
  selectedKnot.setTopLeftPosition(130 * widthFactor, rowHeight * 0.1);
  selectedKnot.setSize(60, rowHeight * 0.8);

  Grid grid;
  using Track = Grid::TrackInfo;

  grid.templateRows = { Track(1_fr) };
  grid.templateColumns = { Track(1_fr), Track(1_fr) };
  grid.items = { GridItem(enabled.getControl()),
                 GridItem(linked.getControl()) };

  int const offset = 130 * widthFactor + 60 + 30 * widthFactor;
  grid.performLayout({ offset, 0, getWidth() - offset, rowHeight });

  int const secondRow = rowHeight;
  int left = 0;

  auto const resize = [&](auto& component, int width) {
    component.setTopLeftPosition(left, secondRow);
    component.setSize(width, rowHeight * 3);
    left += width - 1;
  };

  resize(channelLabels, 50 * widthFactor);

  if (!x || !y || !s || !t) {
    return;
  }

  int width = std::floor(((float)getWidth() - 50 * widthFactor + 4.f) / 4.f);

  resize(*x, width);
  resize(*y, width);
  resize(*t, width);
  resize(*s, width);
}

void
SplineKnotEditor::paint(Graphics& g)
{
  int right = s->getBounds().getRight();
  g.setColour(tableSettings.backgroundColour);
  g.fillRect(0, 0, right, getHeight() / 4);

  g.setColour(tableSettings.lineColour);
  g.drawRect(0, 0, right, 1 + getHeight() / 4);
}

void
SplineKnotEditor::setSelectedKnot(int newKnotIndex, bool forceUpdate)
{
  setKnot(newKnotIndex, forceUpdate);
  selectedKnot.setSelectedId(newKnotIndex + 1, sendNotification);
}

void
SplineKnotEditor::setTableSettings(LinkableControlTable tableSettings)
{
  this->tableSettings = tableSettings;
  channelLabels.tableSettings = tableSettings;
  x->tableSettings = tableSettings;
  y->tableSettings = tableSettings;
  t->tableSettings = tableSettings;
  s->tableSettings = tableSettings;
}

void
SplineKnotEditor::setKnot(int newKnotIndex, bool forceUpdate)
{
  if (!forceUpdate && knotIndex == newKnotIndex) {
    return;
  }

  knotIndex = newKnotIndex;

  auto& knot = parameters.knots[knotIndex];

  auto& linkedParamID = knot.linked.getID();
  auto& enabledParamID = knot.enabled.getID();

  linked.setParameter(linkedParamID);
  enabled.setParameter(enabledParamID);

  bool isLinked = apvts.getParameter(linkedParamID)->getValue() >= 0.5f;

  if (x) {
    removeChildComponent(x.get());
  }

  x = std::make_unique<LinkableControl<AttachedSlider>>(
    apvts,
    xLabel,
    linkedParamID,
    knot.parameters[0].x->paramID,
    knot.parameters[1].x->paramID,
    false);

  addAndMakeVisible(*x);

  if (y) {
    removeChildComponent(y.get());
  }

  y = std::make_unique<LinkableControl<AttachedSlider>>(
    apvts,
    yLabel,
    linkedParamID,
    knot.parameters[0].y->paramID,
    knot.parameters[1].y->paramID,
    false);

  addAndMakeVisible(*y);

  if (t) {
    removeChildComponent(t.get());
  }

  t = std::make_unique<LinkableControl<AttachedSlider>>(
    apvts,
    "Tangent",
    linkedParamID,
    knot.parameters[0].t->paramID,
    knot.parameters[1].t->paramID,
    false);

  addAndMakeVisible(*t);

  if (s) {
    removeChildComponent(s.get());
  }

  s = std::make_unique<LinkableControl<AttachedSlider>>(
    apvts,
    "Smoothness",
    linkedParamID,
    knot.parameters[0].s->paramID,
    knot.parameters[1].s->paramID,
    false);

  addAndMakeVisible(*s);

  if (splineEditor) {
    for (int c = 0; c < 2; ++c) {
      x->getControl(c).setTextValueSuffix(splineEditor->xSuffix);
      y->getControl(c).setTextValueSuffix(splineEditor->ySuffix);
      if (splineEditor->ySuffix != splineEditor->xSuffix) {
        t->getControl(c).setTextValueSuffix(splineEditor->ySuffix + "/" +
                                            splineEditor->xSuffix);
      }
    }
  }

  setTableSettings(tableSettings);

  resized();
}

void
attachAndInitializeSplineEditors(SplineEditor& splineEditor,
                                 SplineKnotEditor& knotEditor,
                                 int selectedKnot)
{
  splineEditor.knotEditor = &knotEditor;
  knotEditor.splineEditor = &splineEditor;
  knotEditor.setSelectedKnot(selectedKnot, true);
}
