#pragma once
#include <JuceHeader.h>
#include <vector>
#include <memory>
#include <mutex>
#include <thread>
#include <atomic>

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
    
    std::atomic<float> meterValues[6] = {0};
    std::atomic<bool> feedbackClip { false };
    std::atomic<int> selectedOS { -1 };
    std::atomic<double> oversampledSampleRate { 0.0 };
    
    const int attackMin = 0;
    const int attackMax = 200;
    const int releaseMin = 10;
    const int releaseMax = 3000;

    juce::AudioProcessorValueTreeState parameters;
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    
    juce::ValueTree state;
        
private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (APCompAudioProcessor)
    
    std::mutex oversamplingMutex;
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampling;
    
    double outputSample[2] = {0};
    double slewedSignal[2] = {-200.0};
    double previousGainReduction[2] = {-200.0};
    double gainReduction[2] = {0};
    float meterDecayCoefficient = 0.9999f;
    int totalNumInputChannels = 0;
    int totalNumOutputChannels = 0;
    float inertiaVelocity[2] = {0};
    size_t currentSamplesPerBlock = 0;
    int currentSampleRate = 0;
};
