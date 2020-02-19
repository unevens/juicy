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

class WrappedBoolParameter
{
  AudioParameterBool* boolParameter;
  AudioParameterFloat* floatParameter;

public:
  bool getValue();

  String const& getID();

  std::unique_ptr<RangedAudioParameter> createParameter(String const& name,
                                                        bool value,
                                                        bool useFloat = false);

  WrappedBoolParameter(AudioParameterFloat* floatParameter = nullptr,
                       AudioParameterBool* boolParameter = nullptr)
    : floatParameter(floatParameter)
    , boolParameter(boolParameter)

  {}
};

inline bool
WrappedBoolParameter::getValue()
{
  if (boolParameter) {
    return boolParameter->get();
  }
  return floatParameter->get();
}

inline String const&
WrappedBoolParameter::getID()
{
  if (boolParameter) {
    return boolParameter->paramID;
  }
  return floatParameter->paramID;
}

inline std::unique_ptr<RangedAudioParameter>
WrappedBoolParameter::createParameter(String const& name,
                                      bool value,
                                      bool useFloat)
{
  if (useFloat) {
    boolParameter = new AudioParameterBool(name, name, value);
    floatParameter = nullptr;
    return std::unique_ptr<RangedAudioParameter>(boolParameter);
  }
  else {
    boolParameter = nullptr;
    floatParameter = new AudioParameterFloat(
      name, name, { 0.0f, 1.f, 1.0f }, value ? 1.f : 0.f);
    return std::unique_ptr<RangedAudioParameter>(floatParameter);
  }
}
