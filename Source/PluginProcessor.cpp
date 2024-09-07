#include "APCommon.h"
#include "PluginProcessor.h"


APComp::APComp()
: AudioProcessor(BusesProperties()
                 .withInput("Input", juce::AudioChannelSet::quadraphonic(), true)
                 .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
meterValues {},
meterValuesBack {},
meterValuesFrontPointer(meterValues),
meterValuesBackPointer(meterValuesBack),
feedbackClip(false),
selectedOS(-1),
oversampledSampleRate(0),
parameters(*this, nullptr, "PARAMETERS", createParameterLayout()),
outputSample { 0, 0 },
slewedSignal { -200.0, -200.0 },
previousGainReduction { -200.0, -200.0 },
gainReduction { 0, 0 },
meterDecayCoefficient(0.99f),
totalNumInputChannels(0),
totalNumOutputChannels(0),
inertiaVelocity { 0, 0 },
currentSamplesPerBlock(0),
currentSampleRate(0),
cachedOversamplingIndex(-1) {}


void APComp::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    for (int i = 0; i < 2; i++) {
        slewedSignal[i] = -200.0;
        gainReduction[i] = 0.0f;
        outputSample[i] = 0.0f;
        previousGainReduction[i] = -200.0;
        inertiaVelocity[i] = 0.0f;
    }
        
    currentSamplesPerBlock = samplesPerBlock;
    currentSampleRate = sampleRate;
}


void APComp::setOversampling(int selectedIndex) {
    
    if (currentSamplesPerBlock < 4 || currentSampleRate < 100) { return; }

    std::shared_ptr<juce::dsp::Oversampling<float>> newOversampler;
    
    OversamplingOption selectedOversampling = getOversamplingOptionFromIndex(selectedIndex);

    switch (selectedOversampling) {
        case OversamplingOption::None:
            newOversampler = std::make_shared<juce::dsp::Oversampling<float>>(4, 0, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR);
            break;
        case OversamplingOption::FIR_1x:
            newOversampler = std::make_shared<juce::dsp::Oversampling<float>>(4, 1, juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple);
            break;
        case OversamplingOption::IIR_1x:
            newOversampler = std::make_shared<juce::dsp::Oversampling<float>>(4, 1, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR);
            break;
        case OversamplingOption::FIR_2x:
            newOversampler = std::make_shared<juce::dsp::Oversampling<float>>(4, 2, juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple);
            break;
        case OversamplingOption::IIR_2x:
            newOversampler = std::make_shared<juce::dsp::Oversampling<float>>(4, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR);
            break;
    }
    
    newOversampler->initProcessing(static_cast<size_t>(currentSamplesPerBlock));
    newOversampler->reset();
    
    setLatencySamples(newOversampler->getLatencyInSamples());
    
    managedOversampler = newOversampler;
    
    selectedOS = selectedIndex;

    oversampledSampleRate = static_cast<int>(currentSampleRate) * static_cast<int>(newOversampler->getOversamplingFactor());
}


void APComp::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
    
    juce::ScopedNoDenormals noDenormals;
    
    int overSamplingSelection = parameters.getRawParameterValue("oversampling")->load();

    if (cachedOversamplingIndex != overSamplingSelection) {

        cachedOversamplingIndex = overSamplingSelection;
        
        setOversampling(overSamplingSelection);
        
        return;
    }
    
    totalNumInputChannels = getTotalNumInputChannels();
    totalNumOutputChannels = getTotalNumOutputChannels();
    
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i) buffer.clear(i, 0, buffer.getNumSamples());
    
    if (oversampledSampleRate < 1) return;

    auto oversampler = getCurrentOversampler();

    if (!oversampler) return;
    
    juce::dsp::AudioBlock<float> originalBlock (buffer);
    juce::dsp::AudioBlock<float> oversampledBlock = oversampler->processSamplesUp (originalBlock);
    
    doCompressionDSP(oversampledBlock);

    oversampler->processSamplesDown (originalBlock);
}
    




