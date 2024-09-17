#include "APCommon.h"
#include "PluginProcessor.h"
#include "Constants.h"


std::unique_ptr<juce::RangedAudioParameter> newFloatParam(ParameterNames paramName,
                                                          float minValue,
                                                          float maxValue,
                                                          float defaultValue) {
    
    ParameterQuery parameterQuery = queryParameter(paramName);
    
    return std::make_unique<juce::AudioParameterFloat>(juce::ParameterID(parameterQuery.id, static_cast<int>(paramName) + 1),
                                                       parameterQuery.label,
                                                       juce::NormalisableRange<float>(minValue, maxValue),
                                                       defaultValue);
}


std::unique_ptr<juce::RangedAudioParameter> newIntParam(ParameterNames paramName,
                                                        int minValue,
                                                        int maxValue,
                                                        int defaultValue) {
    
    ParameterQuery parameterQuery = queryParameter(paramName);

    
    return std::make_unique<juce::AudioParameterInt>(juce::ParameterID(parameterQuery.id, static_cast<int>(paramName) + 1),
                                                     parameterQuery.id,
                                                     minValue,
                                                     maxValue,
                                                     defaultValue,
                                                     parameterQuery.label);
}


juce::AudioProcessorValueTreeState::ParameterLayout APComp::createParameterLayout() {
    
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(newFloatParam(ParameterNames::attack,  Constants::attackMin,   Constants::attackMax,   90.0f   ));
    params.push_back(newFloatParam(ParameterNames::release, Constants::releaseMin,  Constants::releaseMax,  400.0f  ));
    params.push_back(newFloatParam(ParameterNames::ratio,   Constants::ratioMin,    Constants::ratioMax,    20.0f   ));

    params.push_back(newFloatParam(ParameterNames::inGain,       -12.0f,    24.0f,      0.0f    ));
    params.push_back(newFloatParam(ParameterNames::outGain,      -12.0f,    24.0f,      0.0f    ));
    params.push_back(newFloatParam(ParameterNames::convexity,    -2.0f,     2.0f,       1.0f    ));
    params.push_back(newFloatParam(ParameterNames::threshold,    -60.0f,    6.0f,       0.0f    ));
    params.push_back(newFloatParam(ParameterNames::channelLink,  0.0f,      100.0f,     100.0f  ));
    params.push_back(newFloatParam(ParameterNames::feedback,     0.0f,      1.0f,       0.0f    ));
    params.push_back(newFloatParam(ParameterNames::inertia,      -1.0f,     0.3f,       0.0f    ));
    params.push_back(newFloatParam(ParameterNames::inertiaDecay, 0.8f,      0.96f,      0.94f   ));
    params.push_back(newFloatParam(ParameterNames::ceiling,      0.3f,      3.0f,       1.0f    ));
    params.push_back(newFloatParam(ParameterNames::fold,         0.0f,      1.0f,       0.0f    ));

    params.push_back(newIntParam(ParameterNames::variMu,         0,         1,          0       ));
    params.push_back(newIntParam(ParameterNames::sidechain,      0,         1,          0       ));
    params.push_back(newIntParam(ParameterNames::oversampling,   0,         1,          1       ));

    return { params.begin(), params.end() };
}
