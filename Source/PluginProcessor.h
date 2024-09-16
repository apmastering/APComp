#pragma once

#include <vector>
#include <atomic>

#include "CircularBuffer.h"


class APComp  : public juce::AudioProcessor {
    
public:
    
    APComp();
        
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;
    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
        
    bool getBoolKnobValue (ParameterNames parameter) const;
    float getFloatKnobValue(ParameterNames parameter) const;

    void startOversampler(double sampleRate, int samplesPerBlock);

    static constexpr int meterCount = 6;
    
    std::atomic<bool> feedbackClip;
    std::vector<std::atomic<float>> meterValuesAtomic;
    std::atomic<bool> oversamplerReady;
    
    const size_t oversamplingFactor = 1;
    int oversampledSampleRate;

    juce::AudioProcessorValueTreeState apvts;
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    
    CircularBuffer circularBuffer;

private:
    
    void updateMeters(float *maxValuesForMeters);
    void flushLoopVariables();
    void doCompressionDSP(juce::dsp::AudioBlock<float>& block,
                          juce::dsp::AudioBlock<float>& sidechainBlock,
                          size_t oversamplingFactor,
                          int sampleRate);
    void startClock();
    void stopClock();
    
    float meterValues[meterCount];
    double outputSample[2];
    double previousGainReduction[2];
    double gainReduction[2];
    float inertiaVelocity[2];
    float meterDecayCoefficient;
    int totalNumInputChannels;
    int totalNumOutputChannels;
    std::vector<std::atomic<float>> parameterCache;
    double slewedSignal[2];
    std::atomic<int> baseSampleRate;
    std::atomic<bool> flushDSP;
    
    std::chrono::time_point<std::chrono::high_resolution_clock> startTime;
    
    std::vector<juce::AudioParameterFloat*> parameterList;
        
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampler;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (APComp)
};
