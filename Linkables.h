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
#include "AttachedParameter.h"
#include "WrappedBoolParameter.h"
#include <array>

template<class ParameterClass>
struct LinkableParameter
{
  WrappedBoolParameter linked;
  std::array<ParameterClass*, 2> parameters;

  ParameterClass* get(int channel)
  {
    if (linked.getValue()) {
      return parameters[0];
    }
    return parameters[channel];
  }
};

struct LinkableControlTable
{
  Colour backgroundColour = Colours::transparentBlack;
  Colour lineColour = Colours::white;
  bool drawHorizontalLines = true;
  bool drawLeftVericalLine = true;
  bool drawRightVericalLine = true;
  float lineThickness = 1.f;
  float gap = 8.f;

  void paintTable(Graphics& g, int width, int height, bool hasLinked)
  {
    int rowHeight = hasLinked ? height / 4.f : height / 3.f;

    g.fillAll(backgroundColour);

    g.setColour(lineColour);

    if (drawHorizontalLines) {
      g.drawLine(0.f, 1.f, width, 1.f, lineThickness);
      g.drawLine(0.f, rowHeight, width, rowHeight, lineThickness);
      g.drawLine(0.f, 2 * rowHeight, width, 2 * rowHeight, lineThickness);
      if (hasLinked) {
        g.drawLine(0.f, 3 * rowHeight, width, 3 * rowHeight, lineThickness);
      }
      g.drawLine(0.f, height - 1, width, height - 1, lineThickness);
    }

    if (drawLeftVericalLine) {
      g.drawLine(1, 0, 1, height, lineThickness);
    }

    if (drawRightVericalLine) {
      g.drawLine(width - 1, 0, width - 1, height, lineThickness);
    }
  }
};

template<class AttachedControlClass>
class LinkableControl
  : public Component
  , public AudioProcessorValueTreeState::Listener
{
protected:
  std::unique_ptr<AttachedToggle> linked;
  std::array<AttachedControlClass, 2> controls;
  Label label;
  std::array<String, 2> paramIDs;
  String linkParamID;
  AudioProcessorValueTreeState* apvts;

public:
  LinkableControlTable tableSettings;

  using Control = typename AttachedControlClass::Control;

  LinkableControl(AudioProcessorValueTreeState& apvts,
                  String const& name,
                  String const& linkParamID,
                  String const& channel0ParamID,
                  String const& channel1ParamID,
                  bool makeLinkedControl = true)
    : paramIDs{ { channel0ParamID, channel1ParamID } }
    , linkParamID(linkParamID)
    , apvts(&apvts)
    , label("", name)
    , linked(makeLinkedControl
               ? std::make_unique<AttachedToggle>(*this, apvts, linkParamID)
               : nullptr)
    , controls{ { { *this, apvts, channel0ParamID },
                  { *this, apvts, channel0ParamID } } }
  {
    addAndMakeVisible(label);
    parameterChanged("", apvts.getParameter(linkParamID)->getValue());
    apvts.addParameterListener(linkParamID, this);
    label.setJustificationType(Justification::centred);
    setOpaque(false);
    setSize(90, linked ? 120 : 90);
  }

  template<class ParameterClass>
  LinkableControl(AudioProcessorValueTreeState& apvts,
                  String const& name,
                  LinkableParameter<ParameterClass>& linkableParameters)
    : LinkableControl(apvts,
                      name,
                      linkableParameters.linked.getID(),
                      linkableParameters.parameters[0]->paramID,
                      linkableParameters.parameters[1]->paramID,
                      true)
  {}

  ~LinkableControl() { apvts->removeParameterListener(linkParamID, this); }

  void parameterChanged(const String&, float newValue) override
  {
    bool const isLinked = newValue >= 0.5f;
    controls[1].SetParameter(paramIDs[isLinked ? 0 : 1]);
  }

  ToggleButton* getLinked() { return linked ? &linked->getControl() : nullptr; }

  Label& getLabel() { return label; }

  Control& getControl(int channel) { return controls[channel].getControl(); }

  void resized() override
  {
    Grid grid;
    using Track = Grid::TrackInfo;
    grid.templateColumns = { Track(1_fr) };

    for (int i = 0; i < (linked ? 4 : 3); ++i) {
      grid.templateRows.add(Track(1_fr));
    }

    int const rowHeight = linked ? (getHeight()) / 4.f : (getHeight()) / 3.f;

    constexpr int controlGap = std::is_same_v<Control, Slider> ? 0 : 4;

    grid.items = { GridItem(label)
                     .withWidth(getWidth() - 2 * tableSettings.gap)
                     .withAlignSelf(GridItem::AlignSelf::center)
                     .withJustifySelf(GridItem::JustifySelf::center),
                   GridItem(controls[0].getControl())
                     .withWidth(getWidth() - 2 * tableSettings.gap)
                     .withHeight(rowHeight - 2 * controlGap)
                     .withAlignSelf(GridItem::AlignSelf::center)
                     .withJustifySelf(GridItem::JustifySelf::center),
                   GridItem(controls[1].getControl())
                     .withWidth(getWidth() - 2 * tableSettings.gap)
                     .withHeight(rowHeight - 2 * controlGap)
                     .withAlignSelf(GridItem::AlignSelf::center)
                     .withJustifySelf(GridItem::JustifySelf::center) };

    if (linked) {
      grid.items.add(GridItem(linked->getControl())
                       .withWidth(26)
                       .withAlignSelf(GridItem::AlignSelf::center)
                       .withJustifySelf(GridItem::JustifySelf::center));
    }

    grid.performLayout(getLocalBounds());

    /*int rowHeight = linked ? (getHeight()) / 4.f : (getHeight()) / 3.f;

    label.setTopLeftPosition(0, 0);
    label.setSize(getWidth(), rowHeight);

    int y = rowHeight;

    for (int i = 0; i < 2; ++i) {
      constexpr int controlGap = std::is_same_v<Control, Slider> ? 0 : 4;
      int const controlHeight = rowHeight - 2 * controlGap;

      controls[i].getControl().setTopLeftPosition(tableSettings.gap,
                                                  y + controlGap);
      controls[i].getControl().setSize(getWidth() - 2.f * tableSettings.gap,
                                       controlHeight);
      y += rowHeight;
    }

    if (linked) {
      linked->getControl().setTopLeftPosition(
        jmax(0.f, 8 + 0.5f * (getWidth() - rowHeight)), y);
      linked->getControl().setSize(rowHeight, rowHeight);
    }*/
  }

  void paint(Graphics& g) override
  {
    tableSettings.paintTable(g, getWidth(), getHeight(), linked ? true : false);
  }
};

class LinkableComboBox : public LinkableControl<AttachedComboBox>
{
public:
  LinkableComboBox(AudioProcessorValueTreeState& apvts,
                   String const& name,
                   StringArray const& choices,
                   String const& linkParamID,
                   String const& channel0ParamID,
                   String const& channel1ParamID,
                   bool makeLinkedControl = true)
    : LinkableControl<AttachedComboBox>(apvts,
                                        name,
                                        linkParamID,
                                        "",
                                        "",
                                        makeLinkedControl)
  {
    this->controls[0] =
      AttachedComboBox(*this, apvts, channel0ParamID, choices);
    this->controls[1] =
      AttachedComboBox(*this, apvts, channel1ParamID, choices);
  }

  template<class ParameterClass>
  LinkableComboBox(AudioProcessorValueTreeState& apvts,
                   String const& name,
                   StringArray const& choices,
                   LinkableParameter<ParameterClass>& linkableParameters)
    : LinkableComboBox(apvts,
                       name,
                       choices,
                       linkableParameters.linked.getID(),
                       linkableParameters.parameters[0]->paramID,
                       linkableParameters.parameters[1]->paramID,
                       true)
  {}
};

class ChannelLabels
  : public Component
  , public AudioProcessorValueTreeState::Listener
{
  std::array<Label, 2> labels;
  std::unique_ptr<Label> linkLabel;
  String midSideParamID;
  AudioProcessorValueTreeState* apvts;

public:
  LinkableControlTable tableSettings;

  ChannelLabels(AudioProcessorValueTreeState& apvts,
                String const& midSideParamID,
                bool makeLinkLabel = true)
    : midSideParamID(midSideParamID)
    , apvts(&apvts)
    , labels{ { Label("", "Left"), Label("", "Right") } }
    , linkLabel(makeLinkLabel ? std::make_unique<Label>("", "Link") : nullptr)
  {
    parameterChanged("", apvts.getParameter(midSideParamID)->getValue());
    apvts.addParameterListener(midSideParamID, this);
    addAndMakeVisible(labels[0]);
    addAndMakeVisible(labels[1]);
    if (linkLabel) {
      addAndMakeVisible(*linkLabel);
    }
  }

  ~ChannelLabels() { apvts->removeParameterListener(midSideParamID, this); }

  void parameterChanged(const String&, float newValue) override
  {
    bool const isMidSide = newValue >= 0.5f;
    labels[0].setText(isMidSide ? "Mid" : "Left", dontSendNotification);
    labels[1].setText(isMidSide ? "Side" : "Right", dontSendNotification);
  }

  Label& getLabel(int channel) { return labels[channel]; }
  Label* getLinkLabel() { return linkLabel ? linkLabel.get() : nullptr; }

  void resized() override
  {
    constexpr float rowGap = 3.f;
    float rowHeight = linkLabel ? (getHeight() - 3.f * rowGap) / 4.f
                                : (getHeight() - 2.f * rowGap) / 3.f;

    float y = rowHeight + rowGap;
    for (int i = 0; i < 2; ++i) {
      labels[i].setTopLeftPosition(0, y);
      labels[i].setSize(getWidth(), rowHeight);
      y += rowHeight + rowGap;
    }
    if (linkLabel) {
      linkLabel->setTopLeftPosition(0, y);
      linkLabel->setSize(getWidth(), rowHeight);
    }
  }

  void paint(Graphics& g)
  {
    tableSettings.paintTable(
      g, getWidth(), getHeight(), linkLabel ? true : false);
  }
};
