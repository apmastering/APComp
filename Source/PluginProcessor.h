#pragma once

#include <vector>
#include <atomic>


class BackgroundTimerThread;


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
        
    bool getButtonKnobValue (ParameterNames parameter) const;
    void parameterChanged(const juce::String& parameterID, float newValue) override;
    void startOversampler(double sampleRate, int samplesPerBlock);
    void recacheAllKnobs();

    static constexpr int meterCount = 6;
    
    std::atomic<bool> feedbackClip;
    std::vector<std::atomic<float>> meterValuesAtomic;
    std::atomic<bool> oversamplerReady;
    
    const size_t oversamplingFactor = 1;
    int oversampledSampleRate;

    juce::AudioProcessorValueTreeState apvts;
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    std::unique_ptr<juce::dsp::Oversampling<float>> oversampler;

private:
    
    void updateMeters(float *maxValuesForMeters);
    void flushLoopVariables();
    void doCompressionDSP(juce::dsp::AudioBlock<float>& block, juce::dsp::AudioBlock<float>& sidechainBlock, size_t oversamplingFactor);
    void initializeParameterList();
    void addParameterListeners();
    void removeParameterListeners();
    float getKnobValueFromCache(int index) const;
    bool getBoolValueFromCache(int index) const;
    
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
    
    std::atomic<bool> flushDSP;
    
    juce::ThreadPool threadPool;
    
    //std::unique_ptr<BackgroundTimerThread> backgroundTimerThread;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (APComp)
};


class OversamplingJob : public juce::ThreadPoolJob {
public:
    OversamplingJob(APComp& processor, double sampleRate, int samplesPerBlock)
    : juce::ThreadPoolJob("OversamplingJob"),
    processor(processor),
    sampleRate(sampleRate),
    samplesPerBlock(samplesPerBlock) {}

    juce::ThreadPoolJob::JobStatus runJob() override {
        
        processor.startOversampler(sampleRate, samplesPerBlock);
        
        return juce::ThreadPoolJob::JobStatus::jobHasFinished;
    }

private:
    APComp& processor;
    double sampleRate;
    int samplesPerBlock;
};


class KnobRecacheJob : public juce::ThreadPoolJob {
public:
    KnobRecacheJob(APComp& processor)
    : juce::ThreadPoolJob("KnobRecacheJob"),
    processor(processor) {}

    juce::ThreadPoolJob::JobStatus runJob() override {
        
        processor.recacheAllKnobs();
        
        return juce::ThreadPoolJob::JobStatus::jobHasFinished;
    }

private:
    APComp& processor;
};
