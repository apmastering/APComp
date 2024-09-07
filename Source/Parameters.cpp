#include "APCommon.h"
#include "PluginProcessor.h"
#include "Constants.h"


juce::AudioProcessorValueTreeState::ParameterLayout APComp::createParameterLayout() {
    
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterFloat> ("inGain",        "Input Gain",   -12.0f, 24.0f, 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("outGain",       "Output Gain",  -12.0f, 24.0f, 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("convexity",     "Convexity",    -2.0f, 2.0f, 1.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("attack",        "Attack",   Constants::attackMin, Constants::attackMax, 90.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("release",       "Release",  Constants::releaseMin, Constants::releaseMax, 400.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("threshold",     "Threshold",    -70.0f, 0.0f, 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("ratio",         "Ratio",        1.0f, 6.0f, 6.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("channelLink",   "Channel Link", 0.0f, 100.0f, 100.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("feedback",      "Feedback",     0.0f, 1.0f, 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("inertia",       "Inertia",      -1.0f, 0.3f, 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("inertiaDecay",  "Inertia Decay", 0.8f, 0.96f, 0.94f));
    params.push_back (std::make_unique<juce::AudioParameterBool>  ("sidechain",     "Sidechain Input",  false));
    params.push_back (std::make_unique<juce::AudioParameterBool>  ("metersOn",      "Metering On",      true));
    
    juce::StringArray choices = {
        getOversamplingOptionString(OversamplingOption::None),
        getOversamplingOptionString(OversamplingOption::FIR_1x),
        getOversamplingOptionString(OversamplingOption::IIR_1x),
        getOversamplingOptionString(OversamplingOption::FIR_2x),
        getOversamplingOptionString(OversamplingOption::IIR_2x) 
    };
    
    params.push_back(std::make_unique<juce::AudioParameterChoice>("oversampling", "Oversampling", choices, 0));

    return { params.begin(), params.end() };
}
