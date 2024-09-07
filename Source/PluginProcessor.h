#pragma once

#include <vector>


class APComp  : public juce::AudioProcessor , public juce::AudioProcessorValueTreeState::Listener {
    
public:
    
    APComp();
    ~APComp() override;
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
    
    void doCompressionDSP(juce::dsp::AudioBlock<float>& block);
    void setOversampling(int selectedIndex);
    
    static constexpr int meterCount = 6;

    float meterValues[meterCount];
    float meterValuesBack[meterCount];
    
    float* meterValuesFrontPointer;
    float* meterValuesBackPointer;
    
    bool feedbackClip;
    int selectedOS;
    int oversampledSampleRate;
    
    juce::AudioProcessorValueTreeState apvts;
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
            
private:
    
    std::shared_ptr<juce::dsp::Oversampling<float>> managedOversampler;
    std::shared_ptr<juce::dsp::Oversampling<float>> getCurrentOversampler() const { return managedOversampler; }

    double outputSample[2];
    double slewedSignal[2];
    double previousGainReduction[2];
    double gainReduction[2];
    float inertiaVelocity[2];
    float meterDecayCoefficient;
    int totalNumInputChannels;
    int totalNumOutputChannels;
    int currentSampleRate;
    int cachedOversamplingIndex;
    size_t currentSamplesPerBlock;
    std::vector<float> parameterCache;
    
    void initializeParameterList();
    void parameterChanged(const juce::String& parameterID, float newValue) override;
    void addParameterListeners();
    void removeParameterListeners();
    float getKnobValueFromCache(int index) const;
    bool getBoolValueFromCache(int index) const;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (APComp)
};
