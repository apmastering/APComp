#include <thread>
#include <chrono>

#include "APCommon.h"
#include "PluginProcessor.h"

#if PRO_VERSION
    #include "ProDSP.h"
#endif


APComp::APComp()
: AudioProcessor(BusesProperties()
                 .withInput("Input", juce::AudioChannelSet::quadraphonic(), true)
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
    
    #if DEBUG_MODE
    auto start = std::chrono::high_resolution_clock::now();
    #endif
    
    baseSampleRate.store(static_cast<int>(sampleRate), std::memory_order_relaxed);

    oversamplerReady.store(false);
    
    startOversampler(sampleRate, samplesPerBlock);

    flushDSP.store(true, std::memory_order_relaxed);
    
    #if DEBUG_MODE
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duration = end - start;
    std::cout << "prepareToPlay completed in : " << duration.count() << " milliseconds" << std::endl;
    #endif
}


bool APComp::getBoolKnobValue (ParameterNames parameter) const {
    
    return parameterList[static_cast<int>(parameter)]->get() > 0.5f ? true : false;
}


float APComp::getFloatKnobValue(ParameterNames parameter) const {
    
    return parameterList[static_cast<int>(parameter)]->get();
}


void APComp::startOversampler(double sampleRate, int samplesPerBlock) {
    
    oversampler.reset();
    
    oversampler = std::make_unique<juce::dsp::Oversampling<float>>(2, 1, juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple);
    
    oversampler->initProcessing(static_cast<size_t>(samplesPerBlock));
    oversampler->reset();
    
    setLatencySamples(oversampler->getLatencyInSamples());
        
    oversampledSampleRate = static_cast<int>(sampleRate) * static_cast<int>(oversamplingFactor);
 
    oversamplerReady.store(true);
}


void APComp::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
    
    juce::ScopedNoDenormals noDenormals;
    
    #if DEBUG_MODE
    startClock();
    #endif
    
    int sr = baseSampleRate.load(std::memory_order_relaxed);
    if (sr < 100) return;
    
    totalNumInputChannels = getTotalNumInputChannels();
    totalNumOutputChannels = getTotalNumOutputChannels();
    
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i) buffer.clear(i, 0, buffer.getNumSamples());

    int overSamplingSelection = static_cast<int>(getFloatKnobValue(ParameterNames::oversampling));

    juce::dsp::AudioBlock<float> originalBlock(buffer);
    juce::dsp::AudioBlock<float> mainBlock;
    juce::dsp::AudioBlock<float> sidechainBlock;
    
    switch (totalNumInputChannels) {

        case 1:
            mainBlock       = originalBlock.getSingleChannelBlock(0);
            sidechainBlock  = originalBlock.getSingleChannelBlock(0);
            break;
        case 2: 
            mainBlock       = originalBlock.getSubsetChannelBlock(0, 2);
            sidechainBlock  = originalBlock.getSubsetChannelBlock(0, 2);
            break;
        case 3:
            mainBlock       = originalBlock.getSubsetChannelBlock(0, 2);
            sidechainBlock  = originalBlock.getSingleChannelBlock(2);
            break;
        case 4:
            mainBlock       = originalBlock.getSubsetChannelBlock(0, 2);
            sidechainBlock  = originalBlock.getSubsetChannelBlock(2, 2);
            break;
        default:
            return;
    }
    
    if (overSamplingSelection == 0) {
        
        doCompressionDSP(mainBlock, sidechainBlock, 0, sr);
        
        #if DEBUG_MODE
        stopClock();
        #endif
        return;
    }

    if (!oversamplerReady.load()) return;
        
    juce::dsp::AudioBlock<float> oversampledBlock = oversampler->processSamplesUp (mainBlock);
    
    doCompressionDSP(oversampledBlock, sidechainBlock, oversamplingFactor, oversampledSampleRate);
    
    oversampler->processSamplesDown (mainBlock);
    
    #if DEBUG_MODE
    stopClock();
    #endif
}


void APComp::startClock() {
    
    startTime = std::chrono::high_resolution_clock::now();
}

void APComp::stopClock() {
    
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duration = end - startTime;
    circularBuffer.add(duration.count());
}
