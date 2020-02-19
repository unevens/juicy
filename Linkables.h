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

template<class AttachedControlClass>
class LinkableControl : public AudioProcessorValueTreeState::Listener
{
protected:
  std::unique_ptr<AttachedToggle> linked;
  std::array<AttachedControlClass, 2> controls;
  std::unique_ptr<Label> label;
  std::array<String, 2> paramIDs;
  String linkParamID;
  AudioProcessorValueTreeState* apvts;

public:
  using Control = typename AttachedControlClass::Control;

  LinkableControl(Component& owner,
                  AudioProcessorValueTreeState& apvts,
                  String const& name,
                  String const& linkParamID,
                  String const& channel0ParamID,
                  String const& channel1ParamID,
                  bool makeLinkedControl = true)
    : paramIDs{ { channel0ParamID, channel1ParamID } }
    , linkParamID(linkParamID)
    , apvts(&apvts)
    , label(std::make_unique<Label>("", name))
    , linked(makeLinkedControl
               ? std::make_unique<AttachedToggle>(owner, apvts, linkParamID)
               : nullptr)
    , controls{ { { owner, apvts, channel0ParamID },
                  { owner, apvts, channel0ParamID } } }
  {
    owner.addAndMakeVisible(*label);
    parameterChanged("", apvts.getParameter(linkParamID)->getValue());
    apvts.addParameterListener(linkParamID, this);
  }

  template<class ParameterClass>
  LinkableControl(Component& owner,
                  AudioProcessorValueTreeState& apvts,
                  String const& name,
                  LinkableParameter<ParameterClass>& linkableParameters)
    : LinkableControl(owner,
                      apvts,
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

  Label& getLabel() { return *label; }

  Control& getControl(int channel) { return controls[channel].getControl(); }

  void resize(Point<float> topLeft, float width, float rowHeight, float rowGap)
  {
    label->setTopLeftPosition(topLeft.x, topLeft.y);
    label->setSize(width, rowHeight);
    topLeft.y += rowHeight + rowGap;
    for (int i = 0; i < 2; ++i) {
      controls[i].getControl().setTopLeftPosition(topLeft.x, topLeft.y);
      controls[i].getControl().setSize(width, rowHeight);
      topLeft.y += rowHeight + rowGap;
    }
    if (linked) {
      linked->getControl().setTopLeftPosition(topLeft.x, topLeft.y);
      linked->getControl().setSize(width, rowHeight);
    }
  }

  LinkableControl(LinkableControl&&) = default;
  LinkableControl& operator=(LinkableControl&&) = default;
};

class LinkableComboBox : public LinkableControl<AttachedComboBox>
{
public:
  LinkableComboBox(Component& owner,
                   AudioProcessorValueTreeState& apvts,
                   String const& name,
                   StringArray const& choices,
                   String const& linkParamID,
                   String const& channel0ParamID,
                   String const& channel1ParamID,
                   bool makeLinkedControl = true)
    : LinkableControl<AttachedComboBox>(owner,
                                        apvts,
                                        name,
                                        linkParamID,
                                        "",
                                        "",
                                        makeLinkedControl)
  {
    this->controls[0] =
      AttachedComboBox(owner, apvts, channel0ParamID, choices);
    this->controls[1] =
      AttachedComboBox(owner, apvts, channel1ParamID, choices);
  }

  template<class ParameterClass>
  LinkableComboBox(Component& owner,
                   AudioProcessorValueTreeState& apvts,
                   String const& name,
                   StringArray const& choices,
                   LinkableParameter<ParameterClass>& linkableParameters)
    : LinkableComboBox(owner,
                       apvts,
                       name,
                       choices,
                       linkableParameters.linked.getID(),
                       linkableParameters.parameters[0]->paramID,
                       linkableParameters.parameters[1]->paramID,
                       true)
  {}
};

class ChannelLabels final : public AudioProcessorValueTreeState::Listener
{
  std::array<std::unique_ptr<Label>, 2> labels;
  std::unique_ptr<Label> linkLabel;
  String midSideParamID;
  AudioProcessorValueTreeState* apvts;
  Component* owner;

public:
  ChannelLabels(Component& owner,
                AudioProcessorValueTreeState& apvts,
                String const& midSideParamID,
                bool makeLinkLabel = true)
    : midSideParamID(midSideParamID)
    , apvts(&apvts)
    , owner(&owner)
    , labels{ { std::make_unique<Label>("", "Left"),
                std::make_unique<Label>("", "Right") } }
    , linkLabel(makeLinkLabel ? std::make_unique<Label>("", "Link") : nullptr)
  {
    parameterChanged("", apvts.getParameter(midSideParamID)->getValue());
    apvts.addParameterListener(midSideParamID, this);
    owner.addAndMakeVisible(*labels[0]);
    owner.addAndMakeVisible(*labels[1]);
    if (linkLabel) {
      owner.addAndMakeVisible(*linkLabel);
    }
  }

  ~ChannelLabels()
  {
    apvts->removeParameterListener(midSideParamID, this);
    owner->removeChildComponent(labels[0].get());
    owner->removeChildComponent(labels[1].get());
    if (linkLabel) {
      owner->removeChildComponent(linkLabel.get());
    }
  }

  void parameterChanged(const String&, float newValue) override
  {
    bool const isMidSide = newValue >= 0.5f;
    labels[0]->setText(isMidSide ? "Mid" : "Left", dontSendNotification);
    labels[1]->setText(isMidSide ? "Side" : "Right", dontSendNotification);
  }

  Label& getLabel(int channel) { return *labels[channel]; }

  void resize(Point<float> topLeft,
              float width,
              float rowHeight,
              float rowGap = 0.f,
              bool leaveFirstRowBlack = true)
  {
    if (leaveFirstRowBlack) {
      topLeft.y += rowHeight + rowGap;
    }
    for (int i = 0; i < 2; ++i) {
      labels[i]->setTopLeftPosition(topLeft.x, topLeft.y);
      labels[i]->setSize(width, rowHeight);
      topLeft.y += rowHeight + rowGap;
    }
    if (linkLabel) {
      linkLabel->setTopLeftPosition(topLeft.x, topLeft.y);
      linkLabel->setSize(width, rowHeight);
    }
  }

  ChannelLabels(ChannelLabels&&) = default;
  ChannelLabels& operator=(ChannelLabels&&) = default;
};
