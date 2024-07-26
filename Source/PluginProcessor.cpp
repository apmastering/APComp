#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <iostream>
#include <cmath>
#include "APCommon.h"
#include <JuceHeader.h>

APCompAudioProcessor::APCompAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::discreteChannels (4), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
parameters (*this, nullptr, "PARAMETERS", createParameterLayout())

#endif
{
}

APCompAudioProcessor::~APCompAudioProcessor()
{
}

const juce::String APCompAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool APCompAudioProcessor::acceptsMidi() const
{
    return false;
}

bool APCompAudioProcessor::producesMidi() const
{
    return false;
}

bool APCompAudioProcessor::isMidiEffect() const
{
    return false;
}

double APCompAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int APCompAudioProcessor::getNumPrograms()
{
    return 1;
}

int APCompAudioProcessor::getCurrentProgram()
{
    return 0;
}

void APCompAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String APCompAudioProcessor::getProgramName (int index)
{
    return {};
}

void APCompAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

void APCompAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    for (int i = 0; i < 2; i++) {
        slewedSignal[i] = -200.0;
        gainReduction[i] = 0.0f;
        outputSample[i] = 0.0f;
        previousGainReduction[i] = -200.0;
        inertiaVelocity[i] = 0.0f;
    }
    
    std::fill(std::begin(meterValues), std::end(meterValues), 0.0f);
    
    currentSamplesPerBlock = samplesPerBlock;
    currentSampleRate = sampleRate;
    
    auto rawParam = parameters.getRawParameterValue("oversampling");
    if (rawParam == nullptr)
    {
        jassertfalse;
        return;
    }
    
    int rawValue = static_cast<int>(rawParam->load());
    
    setOversampling(rawValue);
}

void APCompAudioProcessor::setOversampling(int selectedIndex)
{
    if (currentSamplesPerBlock < 4 || currentSampleRate < 100) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(oversamplingMutex);
    
    switch (selectedIndex) {
        case 0:
            oversampling = std::make_unique<juce::dsp::Oversampling<float>>(4, 0, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR);
            break;
        case 1:
            oversampling = std::make_unique<juce::dsp::Oversampling<float>>(4, 1, juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple);
            break;
        case 2:
            oversampling = std::make_unique<juce::dsp::Oversampling<float>>(4, 1, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR);
            break;
        case 3:
            oversampling = std::make_unique<juce::dsp::Oversampling<float>>(4, 2, juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple);
            break;
        case 4:
            oversampling = std::make_unique<juce::dsp::Oversampling<float>>(4, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR);
            break;
    }
    
    selectedOS = selectedIndex;

    oversampling->initProcessing(static_cast<size_t>(currentSamplesPerBlock));
    oversampling->reset();
    setLatencySamples(oversampling->getLatencyInSamples());
    oversampledSampleRate = currentSampleRate * oversampling->getOversamplingFactor();
}

void APCompAudioProcessor::releaseResources()
{
}

bool APCompAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return true;
}

void APCompAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    
    if (currentSamplesPerBlock < 4 || currentSampleRate < 100) {
        return;
    }
    
    totalNumInputChannels  = getTotalNumInputChannels();
    totalNumOutputChannels = getTotalNumOutputChannels();
    
    if (oversampledSampleRate < 1) return;
    
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());
    
    juce::dsp::AudioBlock<float> originalBlock (buffer);
    
    std::lock_guard<std::mutex> lock(oversamplingMutex);
    
    juce::dsp::AudioBlock<float> oversampledBlock = oversampling->processSamplesUp (originalBlock);
    
    doCompressionDSP(oversampledBlock);
    
    oversampling->processSamplesDown (originalBlock);
    
}
    
    
void APCompAudioProcessor::doCompressionDSP(juce::dsp::AudioBlock<float> block) {
    
    const float inputGainValue = parameters.getRawParameterValue("inGain")->load();
    const float outGainValue = parameters.getRawParameterValue("outGain")->load();
    const float convexityValue = parameters.getRawParameterValue("convexity")->load();
    const float attackValue = linearToExponential(parameters.getRawParameterValue("attack")->load(), attackMin, attackMax) / 1000;
    const float releaseValue = linearToExponential(parameters.getRawParameterValue("release")->load(), releaseMin, releaseMax) / 1000;
    const float thresholdValue = parameters.getRawParameterValue("threshold")->load();
    const float ratioValue = parameters.getRawParameterValue("ratio")->load();
    const float channelLinkValue = parameters.getRawParameterValue("channelLink")->load() / 100;
    const float feedbackValue = parameters.getRawParameterValue("feedback")->load();
    bool sidechainSelected = parameters.getRawParameterValue("sidechain")->load();
    const float inertiaCoefficientValue = parameters.getRawParameterValue("inertia")->load();
    float inertiaDecayCoefficient = parameters.getRawParameterValue("inertiaDecay")->load();
    
    const double attackCoefficient = std::exp(-1.0 / (oversampledSampleRate * attackValue));
    const double releaseCoefficient = std::exp(-1.0 / (oversampledSampleRate * releaseValue));
    
    float* channelData[4];
    
    for (int i = 0; i < totalNumInputChannels && i < 4; i++) {
        channelData[i] = block.getChannelPointer(i);
    }
    
    if (totalNumInputChannels < 3) {
        sidechainSelected = false;
    }
    
    inertiaDecayCoefficient = 0.99 + (inertiaDecayCoefficient * 0.01);

    float maxValuesForMeters[6] = {0};
    
    for (int sample = 0; sample < block.getNumSamples(); ++sample)
    {
        float inputSample[4];
        double inputSampledb[2];
        inputSampledb[0] = -200.0f;
        inputSampledb[1] = -200.0f;
        
        for (int i = 0; i < 2 && i < totalNumInputChannels; i++) {
            inputSample[i] = channelData[i][sample];
            inputSample[i] *= decibelsToGain(inputGainValue);
            inputSampledb[i] = gainToDecibels(std::abs(inputSample[i]) + std::abs(outputSample[i] * feedbackValue));
            outputSample[i] = 0;
            
            if (std::isnan(inputSampledb[i])) {
                inputSampledb[i] = -200.0f;
            }
            
            if (sidechainSelected) {
                inputSampledb[i] = gainToDecibels(std::abs(channelData[i + 2][sample]));
            }
            
            if (inputSampledb[i] < -200) inputSampledb[i] = -200;
            feedbackClip = false;
            if (inputSampledb[i] > 4) {
                inputSampledb[i] = 4;  // hardcoded 4db feedback path clip. Improve later
                feedbackClip = true;
            }
            
            if (inputSampledb[i] > slewedSignal[i])
                slewedSignal[i] = attackCoefficient * (slewedSignal[i] - inputSampledb[i]) + inputSampledb[i];
            else
                slewedSignal[i] = releaseCoefficient * (slewedSignal[i] - inputSampledb[i]) + inputSampledb[i];
            
            if (slewedSignal[i] > thresholdValue) {
                double targetLevel = thresholdValue + (slewedSignal[i] - thresholdValue) / ratioValue;
                gainReduction[i] = slewedSignal[i] - targetLevel;
                gainReduction[i] = std::pow(gainReduction[i], convexityValue);
            } else {
                slewedSignal[i] = thresholdValue;
                gainReduction[i] = 0;
            }
            
            slewedSignal[i] = std::clamp(slewedSignal[i], -200.0, 1000.0); //infinite feedback stopper... more elegant solution?
            
            double gainReductionDecimal = decibelsToGain(gainReduction[i]);
            
            if (inertiaCoefficientValue > 0) {
                if (gainReduction[i] > previousGainReduction[i]) inertiaVelocity[i] += inertiaCoefficientValue * gainReductionDecimal * -0.001;
            } else {
                inertiaVelocity[i] += inertiaCoefficientValue * gainReductionDecimal * -0.001;
            }
            
            inertiaVelocity[i] *= inertiaDecayCoefficient;
            if (inertiaVelocity[i] > 100) inertiaVelocity[i] = 100;
            if (inertiaVelocity[i] < -100) inertiaVelocity[i] = -100;
            
            gainReductionDecimal += inertiaVelocity[i];
            if (gainReductionDecimal > 1000) gainReductionDecimal = 1000;
            if (gainReductionDecimal < -1000) gainReductionDecimal = -1000;
            
            gainReduction[i] = gainToDecibels(gainReductionDecimal);
            previousGainReduction[i] = gainReduction[i];
        }
        
        // which channel has more gain reduction? for stereo link
        double maxGainReduction = gainReduction[0];
        if (gainReduction[0] < gainReduction[1]) {
            maxGainReduction = gainReduction[1];
        }
        
        for (int i = 0; i < 2 && i < totalNumInputChannels; i++) {
            gainReduction[i] = (maxGainReduction * channelLinkValue) + (gainReduction[i] * (channelLinkValue - 1) * -1);
            
            if (sidechainSelected) {
                inputSampledb[i] = gainToDecibels(std::abs(inputSample[i]));
            }
            
            outputSample[i] = decibelsToGain(inputSampledb[i] - gainReduction[i]) * (inputSample[i] < 0 ? -1.0f : 1.0f);
            outputSample[i] = outputSample[i] * decibelsToGain(outGainValue);
            
            // extreme processing NaNs the output sample and nukes the DSP
            if (std::isnan(outputSample[i])) {
                outputSample[i] = 0.0f;
            }
            
            if (i < totalNumOutputChannels) channelData[i][sample] = outputSample[i];
            
            if (std::abs(inputSample[i]) > maxValuesForMeters[i]) maxValuesForMeters[i] = std::abs(inputSample[i]);
            if (std::abs(outputSample[i]) > maxValuesForMeters[i+2]) maxValuesForMeters[i+2] = std::abs(outputSample[i]);
            if (gainReduction[i] > maxValuesForMeters[i+4]) maxValuesForMeters[i+4] = gainReduction[i];
        }
    }
                
    for (int i = 0; i < 6; i++) {
        if (maxValuesForMeters[i] > 100) maxValuesForMeters[i] = 100;
        
        if (maxValuesForMeters[i] > meterValues[i]) {
            meterValues[i] = maxValuesForMeters[i];
        } else {
            meterValues[i] = meterValues[i] * meterDecayCoefficient;
        }
    }
}

bool APCompAudioProcessor::hasEditor() const
{
    return true;
}
juce::AudioProcessorEditor* APCompAudioProcessor::createEditor()
{
    return new APCompAudioProcessorEditor (*this);
}

void APCompAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    std::unique_ptr<juce::XmlElement> xml (parameters.state.createXml());
    copyXmlToBinary (*xml, destData);
}

void APCompAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
        std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
        if (xml != nullptr)
        {
            if (xml->hasTagName (parameters.state.getType()))
            {
                parameters.state = juce::ValueTree::fromXml (*xml);
            }
        }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new APCompAudioProcessor();
}

juce::AudioProcessorValueTreeState::ParameterLayout APCompAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterFloat> ("inGain", "Input Gain", -12.0f, 24.0f, 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("outGain", "Output Gain", -12.0f, 24.0f, 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("convexity", "Convexity", -2.0f, 2.0f, 1.0f));
    params.push_back (std::make_unique<juce::AudioParameterInt> ("attack", "Attack", attackMin, attackMax, 90));
    params.push_back (std::make_unique<juce::AudioParameterInt> ("release", "Release", releaseMin, releaseMax, 400));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("threshold", "Threshold", -70.0f, 0.0f, 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("ratio", "Ratio", 1.0f, 6.0f, 6.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("channelLink", "Channel Link", 0.0f, 100.0f, 100.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("feedback", "Feedback", 0.0f, 1.0f, 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("inertia", "Inertia", -1.0f, 0.3f, 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("inertiaDecay", "Inertia Decay", 0.8f, 0.96f, 0.94f));
    params.push_back (std::make_unique<juce::AudioParameterBool> ("sidechain", "Sidechain Input", false));
    params.push_back (std::make_unique<juce::AudioParameterBool> ("metersOn", "Metering On", true));

    //explicit list naming also used in plugin editor, draw from a single source!
    juce::StringArray choices = {"None", "1x FIR", "1x IIR", "2x FIR", "2x IIR"};
    params.push_back(std::make_unique<juce::AudioParameterChoice>("oversampling", "Oversampling", choices, 0));

    return { params.begin(), params.end() };
}
