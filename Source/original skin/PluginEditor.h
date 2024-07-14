#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include <juce_gui_basics/juce_gui_basics.h>


class KnobLook1 : public juce::LookAndFeel_V4
{
public:
    KnobLook1();
    ~KnobLook1() override = default;
    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPosProportional, float rotaryStartAngle, float rotaryEndAngle, juce::Slider& slider) override;
private:
    juce::Image knobImage;
};

class KnobLook2 : public juce::LookAndFeel_V4
{
public:
    KnobLook2();
    ~KnobLook2() override = default;
    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPosProportional, float rotaryStartAngle, float rotaryEndAngle, juce::Slider& slider) override;
private:
    juce::Image knobImage;
};

class KnobLook3 : public juce::LookAndFeel_V4
{
public:
    KnobLook3();
    ~KnobLook3() override = default;
    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPosProportional, float rotaryStartAngle, float rotaryEndAngle, juce::Slider& slider) override;
private:
    juce::Image knobImage;
};

class KnobLook4 : public juce::LookAndFeel_V4
{
public:
    KnobLook4();
    ~KnobLook4() override = default;
    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPosProportional, float rotaryStartAngle, float rotaryEndAngle, juce::Slider& slider) override;
private:
    juce::Image knobImage;
};

class KnobLook5 : public juce::LookAndFeel_V4
{
public:
    KnobLook5();
    ~KnobLook5() override = default;
    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPosProportional, float rotaryStartAngle, float rotaryEndAngle, juce::Slider& slider) override;
private:
    juce::Image knobImage;
};

class KnobLook6 : public juce::LookAndFeel_V4
{
public:
    KnobLook6();
    ~KnobLook6() override = default;
    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPosProportional, float rotaryStartAngle, float rotaryEndAngle, juce::Slider& slider) override;
private:
    juce::Image knobImage;
};


class APCompAudioProcessorEditor  : public juce::AudioProcessorEditor, private juce::Timer, public juce::Slider::Listener, public juce::Button::Listener, public juce::ComboBox::Listener
{
public:
    APCompAudioProcessorEditor (APCompAudioProcessor&);
    ~APCompAudioProcessorEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;
    void sliderValueChanged(juce::Slider* slider) override;
    void buttonClicked(juce::Button* button) override;
    void comboBoxChanged(juce::ComboBox* comboBox) override;
    
    bool metersActive = true;

private:
    APCompAudioProcessor& audioProcessor;
        
    KnobLook1 knobLook1;
    KnobLook2 knobLook2;
    KnobLook3 knobLook3;
    KnobLook4 knobLook4;
    KnobLook5 knobLook5;
    KnobLook6 knobLook6;
    
    juce::Image backgroundImage;
    juce::Image meterImage1;
    juce::Image meterImage2;
    juce::Image meterImage3;
    juce::Image meterImage4;
    juce::Image meterImage5;
    juce::Image meterImage6;
    
    juce::Font customTypeface;
    
    std::string activeSliderName = "";
    std::string defaultScreenText = "AP Mastering\nCompressor V1";
    float activeSliderValue = 0.0f;
    
    juce::Slider inGainSlider;
    juce::Slider outGainSlider;
    juce::Slider convexitySlider;
    juce::Slider attackSlider;
    juce::Slider releaseSlider;
    juce::Slider thresholdSlider;
    juce::Slider ratioSlider;
    juce::Slider channelLinkSlider;
    juce::Slider sidechainSlider;
    juce::Slider feedbackSlider;
    juce::Slider inertiaSlider;
    juce::Slider inertiaDecaySlider;
    
    juce::ToggleButton metersOnButton;
    
    juce::ComboBox oversamplingBox;
    
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
        std::ref(feedbackSlider),
        std::ref(inertiaSlider),
        std::ref(inertiaDecaySlider)
    };
    
    std::vector<std::string> sliderNames {
        "Input Gain-db",
        "Output Gain-db",
        "Convexity",
        "Attack-ms",
        "Release-ms",
        "Threshold-db",
        "Ratio-:1",
        "Channel Link-%",
        "Sidechain Input",
        "Feedback",
        "Inertia",
        "Inertia Decay"
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
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> feedbackAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> inertiaAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> inertiaDecayAttachment;
    
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> metersOnAttachment;
    
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> oversamplingAttachment;

        
    void timerCallback() override;
    int refreshRate = 33;
    int screenTimeoutCountdown = 0;
    int screenTimeoutCountdownTimeSeconds = 3;
        
    float slewedMeterSignalInput[2];
    float slewedMeterSignalOutput[2];
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (APCompAudioProcessorEditor)
};
