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
meterValues { 0 },
outputSample { 0, 0 },
previousGainReduction { -200.0, -200.0 },
gainReduction { 0, 0 },
inertiaVelocity { 0, 0 },
meterDecayCoefficient(0.99f),
totalNumInputChannels(0),
totalNumOutputChannels(0),
parameterCache(static_cast<int>(ParameterNames::END) + 1),
slewedSignal { -200.0, -200.0 },
baseSampleRate(0),
flushDSP(false) {
    
    addParameterListeners();
}


APComp::~APComp() {

    removeParameterListeners();
    
    threadPool.removeAllJobs(true, 1000);
}


void APComp::addParameterListeners() {
    
    for (int i = 0; i < static_cast<int>(ParameterNames::END); ++i) {
        
        if (i == static_cast<int>(ParameterNames::END)) continue;
        
        ParameterNames param = static_cast<ParameterNames>(i);
        std::string paramName = queryParameter(param).id;
        
        apvts.addParameterListener(paramName, this);
    }
}


void APComp::removeParameterListeners() {
    
    for (int i = 0; i < static_cast<int>(ParameterNames::END); ++i) {
        
        if (i == static_cast<int>(ParameterNames::END)) continue;
        
        ParameterNames param = static_cast<ParameterNames>(i);
        std::string paramName = queryParameter(param).id;
        
        apvts.removeParameterListener(paramName, this);
    }
}


void APComp::parameterChanged(const juce::String& parameterID, float newValue) {

    ParameterNames paramEnum = queryParameter(ParameterNames::END, parameterID.toStdString()).parameterEnum;
    
    int index = static_cast<int>(paramEnum);
    
    parameterCache[index].store(newValue, std::memory_order_relaxed);
}


bool APComp::getButtonKnobValue (ParameterNames parameter) const {
    
    int index = static_cast<int>(parameter);

    return parameterCache[index].load(std::memory_order_relaxed) >= 0.5f;
}


float APComp::getKnobValueFromCache(int index) const {
    
    return parameterCache[index].load(std::memory_order_relaxed);
}


bool APComp::getBoolValueFromCache(int index) const {
    
    return parameterCache[index].load(std::memory_order_relaxed) >= 0.5f;
}


void APComp::prepareToPlay(double sampleRate, int samplesPerBlock) {
               
    baseSampleRate.store(static_cast<int>(sampleRate), std::memory_order_relaxed);

    oversamplerReady.store(false);
    
    auto oversamplingJob = new OversamplingJob(*this, sampleRate, samplesPerBlock);
    threadPool.addJob(oversamplingJob, true);

    auto knobRecacheJob = new KnobRecacheJob(*this);
    threadPool.addJob(knobRecacheJob, true);

    flushDSP.store(true, std::memory_order_relaxed);
}


void APComp::recacheAllKnobs() {
    
    for (int param = 0; param < static_cast<int>(ParameterNames::END); ++param) {
        
        std::string parameterName = queryParameter(static_cast<ParameterNames>(param)).id;
        
        auto* parameterPointer = apvts.getRawParameterValue(parameterName);
        
        float parameterValue;
        
        if (parameterPointer) {
            parameterValue = parameterPointer->load();
        } else {
            parameterValue = 0.0f;
        }
        
        parameterChanged(parameterName, parameterValue);
    }
}


void APComp::startOversampler(double sampleRate, int samplesPerBlock) {
    
    oversampler.reset();
    
    oversampler = std::make_unique<juce::dsp::Oversampling<float>>(2, 1, juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple);
    
    oversampler->initProcessing(static_cast<size_t>(samplesPerBlock));
    oversampler->reset();
    
    setLatencySamples(oversampler->getLatencyInSamples());
        
    oversampledSampleRate = static_cast<int>(sampleRate) * static_cast<int>(oversamplingFactor);
 
    std::this_thread::sleep_for(std::chrono::seconds(1));

    oversamplerReady.store(true);
}


void APComp::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
    
    juce::ScopedNoDenormals noDenormals;
    
    int sr = baseSampleRate.load(std::memory_order_relaxed);
    if (sr < 100) return;
    
    totalNumInputChannels = getTotalNumInputChannels();
    totalNumOutputChannels = getTotalNumOutputChannels();
    
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i) buffer.clear(i, 0, buffer.getNumSamples());

    int overSamplingSelection = static_cast<int>(getKnobValueFromCache(static_cast<int>(ParameterNames::oversampling)));

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
        return;
    }
    
    if (!oversamplerReady.load()) return;
        
    juce::dsp::AudioBlock<float> oversampledBlock = oversampler->processSamplesUp (mainBlock);
    
    doCompressionDSP(oversampledBlock, sidechainBlock, oversamplingFactor, oversampledSampleRate);
    
    oversampler->processSamplesDown (originalBlock);
}
