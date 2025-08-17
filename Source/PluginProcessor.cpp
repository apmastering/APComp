#include <thread>

#include "APCommon.h"
#include "PluginProcessor.h"


APComp::APComp()
: AudioProcessor(BusesProperties()
                 .withInput("Input", juce::AudioChannelSet::stereo(), true)
                 .withInput("Sidechain", juce::AudioChannelSet::stereo(), false)
                 .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
feedbackClip(false),
meterValuesAtomic(meterCount),
oversamplerReady(false),
oversampledSampleRate(0),
apvts(*this, nullptr, "PARAMETERS", createParameterLayout()),
circularBuffer(100),
meterValues { 0 },
outputSample { 0, 0 },
previousGainReduction { -200.0, -200.0 },
gainReduction { 0, 0 },
inertiaVelocity { 0, 0 },
meterDecayCoefficient(0.99f),
totalNumInputChannels(0),
totalNumOutputChannels(0),
slewedSignal { -200.0, -200.0 },
baseSampleRate(0),
flushDSP(false),
parameterList(static_cast<int>(ParameterNames::END) + 1) {
        
    for (int i = 0; i < static_cast<int>(ParameterNames::END); ++i) {
        parameterList[i] = static_cast<juce::AudioParameterFloat*>(apvts.getParameter(queryParameter(static_cast<ParameterNames>(i)).id));
    }
}


void APComp::prepareToPlay(double sampleRate, int samplesPerBlock) {
    baseSampleRate.store(static_cast<int>(sampleRate), std::memory_order_relaxed);
    oversamplerReady.store(false);
    startOversampler(sampleRate, samplesPerBlock);
    flushDSP.store(true, std::memory_order_relaxed);
}


bool APComp::getBoolKnobValue (ParameterNames parameter) const {
    return parameterList[static_cast<int>(parameter)]->get() > 0.5f ? true : false;
}


float APComp::getFloatKnobValue(ParameterNames parameter) const {
    return parameterList[static_cast<int>(parameter)]->get();
}


void APComp::startOversampler(double sampleRate, int samplesPerBlock) {
    oversampler.reset();
    oversampler = std::make_unique<juce::dsp::Oversampling<float>>(2, oversamplingFactor, juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple);
    oversampler->initProcessing(static_cast<size_t>(samplesPerBlock));
    oversampler->reset();
    setLatencySamples(oversampler->getLatencyInSamples());
    oversampledSampleRate = static_cast<int>(sampleRate) * std::pow(2, static_cast<int>(oversamplingFactor));
    oversamplerReady.store(true);
}


void APComp::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
    juce::ScopedNoDenormals noDenormals;
    int sr = baseSampleRate.load(std::memory_order_relaxed);
    if (sr < 100) return;
    auto mainInputBuffer = getBusBuffer(buffer, true, 0);
    auto sidechainInputBuffer = getBusBuffer(buffer, true, 1);
    auto outputBuffer = getBusBuffer(buffer, false, 0);
    for (auto i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
        buffer.clear(i, 0, buffer.getNumSamples());
    int overSamplingSelection = static_cast<int>(getFloatKnobValue(ParameterNames::oversampling));
    juce::dsp::AudioBlock<float> mainBlock(mainInputBuffer);
    juce::dsp::AudioBlock<float> sidechainBlock;

    if (sidechainInputBuffer.getNumChannels() > 0 && sidechainInputBuffer.getNumSamples() > 0) {
        sidechainBlock = juce::dsp::AudioBlock<float>(sidechainInputBuffer);
    } else {
        sidechainBlock = mainBlock;
    }

    if (overSamplingSelection == 0) {
        doCompressionDSP(mainBlock, sidechainBlock, 0, sr);
        return;
    }

    if (!oversamplerReady.load()) return;
    juce::dsp::AudioBlock<float> oversampledBlock = oversampler->processSamplesUp(mainBlock);
    doCompressionDSP(oversampledBlock, sidechainBlock, oversamplingFactor, oversampledSampleRate);
    oversampler->processSamplesDown(mainBlock);
}