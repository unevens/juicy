/*
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

/**
 * The class AttachmentBase is almost the same as juce::AttachedControlBase,
 * defined in the file juce_AudioProcessorValueTreeState.cpp. The only
 * difference is that it only uses normalised values for the parameters, so that
 * the attachments can use their own ranges, independent of those of the
 * parameters.
 */
struct AttachmentBase
  : public AudioProcessorValueTreeState::Listener
  , public AsyncUpdater
{
  AttachmentBase(AudioProcessorValueTreeState& s, const String& p)
    : state(s)
    , paramID(p)
    , lastValue(0)
  {
    state.addParameterListener(paramID, this);
  }

  void removeListener() { state.removeParameterListener(paramID, this); }

  void setNewNormalisedValue(float newNormalisedValue)
  {
    if (auto* p = state.getParameter(paramID)) {
      if (p->getValue() != newNormalisedValue)
        p->setValueNotifyingHost(newNormalisedValue);
    }
  }

  void sendInitialUpdate()
  {
    if (auto* v = state.getRawParameterValue(paramID))
      parameterChanged(paramID, *v);
  }

  void parameterChanged(const String&, float newValue) override
  {
    lastValue = newValue;

    if (MessageManager::getInstance()->isThisTheMessageThread()) {
      cancelPendingUpdate();
      setValue(newValue);
    }
    else {
      triggerAsyncUpdate();
    }
  }

  void beginParameterChange()
  {
    if (auto* p = state.getParameter(paramID)) {
      if (state.undoManager != nullptr)
        state.undoManager->beginNewTransaction();

      p->beginChangeGesture();
    }
  }

  void endParameterChange()
  {
    if (AudioProcessorParameter* p = state.getParameter(paramID))
      p->endChangeGesture();
  }

  void handleAsyncUpdate() override { setValue(lastValue); }

  virtual void setValue(float) = 0;

  AudioProcessorValueTreeState& state;
  String paramID;
  float lastValue;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AttachmentBase)
};

/**
 * The class FloatAttachment is almost the same as
 * juce::AudioProcessorValueTreeState::SliderAttachment, but lets you use your
 * own control instead of a Slider. Actually it does not care what control you
 * use. It just takes a functor to call when the parameter changes.
 */
class FloatAttachment : private AttachmentBase
{
public:
  FloatAttachment(AudioProcessorValueTreeState& s,
                  const String& paramID,
                  std::function<void(void)> onValueChanged,
                  NormalisableRange<float> editorRange)
    : AttachmentBase(s, paramID)
    , onValueChanged(onValueChanged)
    , editorRange(editorRange)
    , ignoreCallbacks(false)
    , value(0.f)
  {
    sendInitialUpdate();
  }

  template<typename... T>
  static std::unique_ptr<FloatAttachment> make(T&&... all)
  {
    return std::unique_ptr<FloatAttachment>(
      new FloatAttachment(std::forward<T>(all)...));
  }

  ~FloatAttachment() override { removeListener(); }

  void setValue(float newValue) override
  {
    const ScopedLock selfCallbackLock(selfCallbackMutex);

    {
      ScopedValueSetter<bool> svs(ignoreCallbacks, true);
      value = newValue;
      onValueChanged();
    }
  }

  void setValueFromGui(float newValue)
  {
    const ScopedLock selfCallbackLock(selfCallbackMutex);

    if (!ignoreCallbacks) {
      setNewNormalisedValue(
        editorRange.convertTo0to1(editorRange.snapToLegalValue(newValue)));
    }
  }

  void dragStarted() { beginParameterChange(); }
  void dragEnded() { endParameterChange(); }

  float getValue() const { return value; }

private:
  NormalisableRange<float> editorRange;
  float value;
  std::function<void(void)> onValueChanged;
  bool ignoreCallbacks;
  CriticalSection selfCallbackMutex;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FloatAttachment)
};

/**
 * The class BoolAttachment is almost the same as
 * juce::AudioProcessorValueTreeState::ButtonAttachment, but lets you use your
 * own control instead of a Button. Actually it does not care what control you
 * use. It just takes a functor to call when the parameter changes.
 */
class BoolAttachment : private AttachmentBase
{
public:
  BoolAttachment(AudioProcessorValueTreeState& s,
                 const String& p,
                 std::function<void(void)> v)
    : AttachmentBase(s, p)
    , onValueChanged(v)
    , ignoreCallbacks(false)
    , value(false)
  {
    sendInitialUpdate();
  }

  template<typename... T>
  static std::unique_ptr<BoolAttachment> make(T&&... all)
  {
    return std::unique_ptr<BoolAttachment>(
      new BoolAttachment(std::forward<T>(all)...));
  }

  ~BoolAttachment() override { removeListener(); }

  void setValue(float newValue) override
  {
    const ScopedLock selfCallbackLock(selfCallbackMutex);

    {
      ScopedValueSetter<bool> svs(ignoreCallbacks, true);
      value = newValue >= 0.5f;
      onValueChanged();
    }
  }

  void setValueFromGui(bool newValue)
  {
    const ScopedLock selfCallbackLock(selfCallbackMutex);

    if (!ignoreCallbacks) {
      beginParameterChange();
      value = newValue;
      setNewNormalisedValue(value ? 1.0f : 0.0f);
      endParameterChange();
    }
  }

  void invertValueFromGui() { setValueFromGui(!value); }

  bool getValue() const { return value; }

private:
  bool value;
  std::function<void(void)> onValueChanged;
  bool ignoreCallbacks;
  CriticalSection selfCallbackMutex;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BoolAttachment)
};
