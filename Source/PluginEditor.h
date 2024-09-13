#pragma once

#include "PluginProcessor.h"


class KnobLook1 : public juce::LookAndFeel_V4 {
    
public:
    
    KnobLook1();
    ~KnobLook1() override = default;
    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPosProportional, float rotaryStartAngle, float rotaryEndAngle, juce::Slider& slider) override;

    
private:
    
    juce::Image knobImage;
};


class GUI  : public juce::AudioProcessorEditor, private juce::Timer {
    
public:
    
    GUI (APComp&);
    ~GUI() override;
        
    void paint (juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;
    ButtonName determineButton(const juce::MouseEvent &event);
    
    void mouseDown (const juce::MouseEvent& event) override;

    
private:
    
    APComp& audioProcessor;
        
    KnobLook1 knobLook1;
    
    juce::Image backgroundImage;
    
    juce::Font customTypeface;
        
    juce::Slider inGainSlider;
    juce::Slider outGainSlider;
    juce::Slider convexitySlider;
    juce::Slider attackSlider;
    juce::Slider releaseSlider;
    juce::Slider thresholdSlider;
    juce::Slider ratioSlider;
    juce::Slider channelLinkSlider;
    juce::Slider sidechainSlider;
    juce::Slider variMuSlider;
    juce::Slider slowSlider;
    juce::Slider feedbackSlider;
    juce::Slider inertiaSlider;
    juce::Slider inertiaDecaySlider;
    juce::Slider overdriveSlider;
    juce::Slider oversamplingSlider;
            
    std::vector<std::reference_wrapper<juce::Slider>> sliders {
        std::ref(inGainSlider),
        std::ref(outGainSlider),
        std::ref(convexitySlider),
        std::ref(attackSlider),
        std::ref(releaseSlider),
        std::ref(thresholdSlider),
        std::ref(ratioSlider),
        std::ref(channelLinkSlider),
        std::ref(sidechainSlider),
        std::ref(variMuSlider),
        std::ref(slowSlider),
        std::ref(feedbackSlider),
        std::ref(inertiaSlider),
        std::ref(inertiaDecaySlider),
        std::ref(overdriveSlider),
        std::ref(oversamplingSlider)
    };

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> inGainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> outGainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> convexityAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attackAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> releaseAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> thresholdAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> ratioAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> channelLinkAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sidechainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> variMuAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> slowAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> feedbackAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> inertiaAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> inertiaDecayAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> overdriveAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> oversamplingAttachment;
        
    bool metersActive;

    const float metersLeft    = 619.3f;
    const float metersRight   = 662.0f;
    const float metersTop     = 23.0f;
    const float metersBottom  = 261.0f;
    
    const int knobRow1 = 244;
    const int knobRow2 = 327;
    const int knobRow3 = 411;
    
    const int knobColumn1 = 74;
    const int knobColumn2 = 221;
    const int knobColumn3 = 370;
    const int knobColumn4 = 519;
    
    void toggleMeters();
    void toggleSlow();
    void toggleVariMu();
    void switchOversampling(bool active);
    void switchSidechain(bool active);
    void paintMeters(juce::Graphics &g);
    void paintButtons(juce::Graphics &g);
    void paintBackground(juce::Graphics &g);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GUI)
};
