#include "APCommon.h"
#include "PluginProcessor.h"


APComp::APComp()
: AudioProcessor (BusesProperties()
                  .withInput  ("Input",  juce::AudioChannelSet::quadraphonic(), true)
                  .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
feedbackClip(false),
selectedOS(-1),
oversampledSampleRate(0),
parameters (*this, nullptr, "PARAMETERS", createParameterLayout()) {
    
    std::fill(meterValues, meterValues + meterCount, 0.0f);
    std::fill(meterValuesBack,  meterValuesBack  + meterCount, 0.0f);
    
    meterValuesFrontPointer = meterValues;
    meterValuesBackPointer = meterValuesBack;
}


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
    
    auto rawParam = parameters.getRawParameterValue("oversampling");
    if (rawParam == nullptr)
    {
        jassertfalse;
        return;
    }
    
    int rawValue = static_cast<int>(rawParam->load());
    
    setOversampling(rawValue);
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
    




