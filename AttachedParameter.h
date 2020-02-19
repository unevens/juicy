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
#include <JuceHeader.h>

template<class ControlClass, class AttachmentClass>
class Attached
{
public:
    using Control = ControlClass;

public:
    Attached(Component& owner,
        AudioProcessorValueTreeState& apvts,
        String const& paramID = "",
        std::function<void(Control&)> setup = nullptr)
        : apvts(&apvts)
        , owner(&owner)
        , control(std::make_unique<Control>())
    {
        if (setup) {
            setup(*control);
        }
        SetParameter(paramID);
        owner.addAndMakeVisible(*control);
    }

    virtual ~Attached() { owner->removeChildComponent(control.get()); }

    void SetParameter(String const& paramID)
    {
        if (paramID == "") {
            return;
        }

        attachment = nullptr;
        // it is important to destroy the old attachment before the new
        // one is instantiated!
        attachment = std::make_unique<AttachmentClass>(*apvts, paramID, *control);
    }

    Control& getControl() { return *control; }

    Attached(Attached&&) = default;
    Attached& operator=(Attached&&) = default;

private:
    std::unique_ptr<Control> control;
    std::unique_ptr<AttachmentClass> attachment;
    AudioProcessorValueTreeState* apvts;
    Component* owner;
};

using AttachedToggle =
Attached<ToggleButton, AudioProcessorValueTreeState::ButtonAttachment>;

class AttachedSlider
    : public Attached<Slider, AudioProcessorValueTreeState::SliderAttachment>
{
public:
    AttachedSlider(Component& owner,
        AudioProcessorValueTreeState& apvts,
        String const& paramID = "",
        Slider::SliderStyle style =
        Slider::SliderStyle::RotaryHorizontalVerticalDrag)
        : Attached<Slider, AudioProcessorValueTreeState::SliderAttachment>(
            owner,
            apvts,
            paramID,
            [style](auto& c) { c.setSliderStyle(style); })
    {}
};

class AttachedComboBox
    : public Attached<ComboBox, AudioProcessorValueTreeState::ComboBoxAttachment>
{
public:
    AttachedComboBox(Component& owner,
        AudioProcessorValueTreeState& apvts,
        String const& paramID = "",
        StringArray const& choices = {})
        : Attached<ComboBox, AudioProcessorValueTreeState::ComboBoxAttachment>(
            owner,
            apvts,
            paramID,
            [&](auto& c) {
                int i = 1;
                for (auto& choice : choices) {
                    c.addItem(choice, i++);
                }
                if (choices.size() > 0) {
                    c.setSelectedId(1, dontSendNotification);
                }
            })
    {}
};