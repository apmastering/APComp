#pragma once
#include <JuceHeader.h>
#include <vector>

class APCompAudioProcessor  : public juce::AudioProcessor
{
public:
    APCompAudioProcessor();
    ~APCompAudioProcessor() override;
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
    
    void doCompressionDSP(juce::dsp::AudioBlock<float> block);
    
    void setOversampling(int selectedIndex);
    
    double outputSample[2];
    double slewedSignal[2];
    double previousGainReduction[2];
    float meterSignalInput[2];
    float meterSignalOutput[2];
    float metersGainReduction[2];
    bool feedbackClip = false;
    
    int selectedOS = -1;
    
    int totalNumInputChannels = 0;
    int totalNumOutputChannels = 0;
    
    float inertiaVelocity[2];
    
    const int attackMin = 0;
    const int attackMax = 200;
    const int releaseMin = 10;
    const int releaseMax = 3000;

    juce::AudioProcessorValueTreeState parameters;
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    
    juce::ValueTree state;
    
    double oversampledSampleRate = 0;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (APCompAudioProcessor)

    juce::dsp::Oversampling<float>* oversampling;

    double gainReduction[2];
    float meterDecayCoefficient = 0.9999f;
    
    size_t currentSamplesPerBlock = 0;
    int currentSampleRate = 0;
    
};
