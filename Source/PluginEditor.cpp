#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <algorithm>
#include "APCommon.h"

APCompAudioProcessorEditor::APCompAudioProcessorEditor (APCompAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    for (size_t i = 0; i < sliders.size(); ++i) {
            juce::Slider& slider = sliders[i].get();
            slider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
            slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
            slider.setName(sliderNames[i]);
            addAndMakeVisible(slider);
            slider.addListener(this);
        }
    
    //explicit naming mirrored in plugin processor... change this!
    oversamplingBox.addItem("None", 1);
    oversamplingBox.addItem("1x FIR", 2);
    oversamplingBox.addItem("1x IIR", 3);
    oversamplingBox.addItem("2x FIR", 4);
    oversamplingBox.addItem("2x IIR", 5);
    oversamplingBox.addListener(this);
    addAndMakeVisible(oversamplingBox);
        
    addAndMakeVisible(metersOnButton);
    metersOnButton.addListener(this);

    inGainSlider.setLookAndFeel(&knobLook1);
    outGainSlider.setLookAndFeel(&knobLook1);
    convexitySlider.setLookAndFeel(&knobLook4);
    attackSlider.setLookAndFeel(&knobLook3);
    releaseSlider.setLookAndFeel(&knobLook3);
    thresholdSlider.setLookAndFeel(&knobLook2);
    ratioSlider.setLookAndFeel(&knobLook2);
    channelLinkSlider.setLookAndFeel(&knobLook5);
    sidechainSlider.setLookAndFeel(&knobLook5);
    feedbackSlider.setLookAndFeel(&knobLook5);
    inertiaSlider.setLookAndFeel(&knobLook5);
    inertiaDecaySlider.setLookAndFeel(&knobLook6);
    
    metersOnButton.setColour(juce::ToggleButton::ColourIds::tickColourId, juce::Colours::black);
    metersOnButton.setColour(juce::ToggleButton::ColourIds::tickDisabledColourId, juce::Colours::black);

    inGainAttachment.reset (new juce::AudioProcessorValueTreeState::SliderAttachment (audioProcessor.parameters, "inGain", inGainSlider));
    outGainAttachment.reset (new juce::AudioProcessorValueTreeState::SliderAttachment (audioProcessor.parameters, "outGain", outGainSlider));
    convexityAttachment.reset (new juce::AudioProcessorValueTreeState::SliderAttachment (audioProcessor.parameters, "convexity", convexitySlider));
    attackAttachment.reset (new juce::AudioProcessorValueTreeState::SliderAttachment (audioProcessor.parameters, "attack", attackSlider));
    releaseAttachment.reset (new juce::AudioProcessorValueTreeState::SliderAttachment (audioProcessor.parameters, "release", releaseSlider));
    thresholdAttachment.reset (new juce::AudioProcessorValueTreeState::SliderAttachment (audioProcessor.parameters, "threshold", thresholdSlider));
    ratioAttachment.reset (new juce::AudioProcessorValueTreeState::SliderAttachment (audioProcessor.parameters, "ratio", ratioSlider));
    channelLinkAttachment.reset (new juce::AudioProcessorValueTreeState::SliderAttachment (audioProcessor.parameters, "channelLink", channelLinkSlider));
    sidechainAttachment.reset (new juce::AudioProcessorValueTreeState::SliderAttachment (audioProcessor.parameters, "sidechain", sidechainSlider));
    feedbackAttachment.reset (new juce::AudioProcessorValueTreeState::SliderAttachment (audioProcessor.parameters, "feedback", feedbackSlider));
    inertiaAttachment.reset (new juce::AudioProcessorValueTreeState::SliderAttachment (audioProcessor.parameters, "inertia", inertiaSlider));
    inertiaDecayAttachment.reset (new juce::AudioProcessorValueTreeState::SliderAttachment (audioProcessor.parameters, "inertiaDecay", inertiaDecaySlider));
    
    metersOnAttachment.reset (new juce::AudioProcessorValueTreeState::ButtonAttachment (audioProcessor.parameters, "metersOn", metersOnButton));
    
    oversamplingAttachment.reset (new juce::AudioProcessorValueTreeState::ComboBoxAttachment (audioProcessor.parameters, "oversampling", oversamplingBox));
    
    backgroundImage = juce::ImageFileFormat::loadFrom(BinaryData::APMasteringBG_png, BinaryData::APMasteringBG_pngSize);

    auto typeface = juce::Typeface::createSystemTypefaceFor(BinaryData::KnockoutFlyweight_otf, BinaryData::KnockoutFlyweight_otfSize);
    customTypeface = juce::FontOptions (typeface);

    setSize (600, 600);
    startTimer(refreshRate);
    screenTimeoutCountdown = 0;
}

APCompAudioProcessorEditor::~APCompAudioProcessorEditor()
{    
    for (auto& sliderRef : sliders) {
        juce::Slider& slider = sliderRef.get();
        slider.removeListener(this);
    }
    
    metersOnButton.removeListener(this);
    oversamplingBox.removeListener(this);
    
    inGainSlider.setLookAndFeel(nullptr);
    outGainSlider.setLookAndFeel(nullptr);
    convexitySlider.setLookAndFeel(nullptr);
    attackSlider.setLookAndFeel(nullptr);
    releaseSlider.setLookAndFeel(nullptr);
    thresholdSlider.setLookAndFeel(nullptr);
    ratioSlider.setLookAndFeel(nullptr);
    channelLinkSlider.setLookAndFeel(nullptr);
    sidechainSlider.setLookAndFeel(nullptr);
    feedbackSlider.setLookAndFeel(nullptr);
    inertiaSlider.setLookAndFeel(nullptr);
    inertiaDecaySlider.setLookAndFeel(nullptr);
        
    stopTimer();
}

void APCompAudioProcessorEditor::sliderValueChanged(juce::Slider* slider)
   {
    activeSliderName = slider->getName().toStdString();
    
    // bodge
    if (activeSliderName == "Sidechain Input") {
        activeSliderValue = 0;
        if (slider->getValue()) activeSliderValue = 1;
    } else {
        activeSliderValue = std::round(slider->getValue() * 100.0f) / 100.0f;
    }
    
    screenTimeoutCountdown = (1000 / refreshRate) * screenTimeoutCountdownTimeSeconds;
}

void APCompAudioProcessorEditor::buttonClicked(juce::Button* button)
{
    if (button == &metersOnButton) {
        if (button->getToggleState()) {
            metersActive = true;
        } else {
            metersActive = false;
        }
    }
    
    // redundant? variable refresh rate was abolished
    if (getTimerInterval() != refreshRate) {
        startTimer(refreshRate);
    }
}

void APCompAudioProcessorEditor::comboBoxChanged(juce::ComboBox* comboBox)
{
    if (comboBox == &oversamplingBox) {
        int selectedIndex = oversamplingBox.getSelectedItemIndex();
        // this is kind of weird as there is no real convention here between processor and editor
        audioProcessor.setOversampling(selectedIndex);
    }
}


void APCompAudioProcessorEditor::paint (juce::Graphics& g)
{
    
    if (backgroundImage.isValid()) {
        g.drawImage(backgroundImage, getLocalBounds().toFloat());
    } else {
        g.fillAll(juce::Colours::lightgrey);
        g.setColour (juce::Colours::black);
        g.setFont (24.0f);
        g.drawFittedText ("AP Mastering Compressor: GUI error", getLocalBounds(), juce::Justification::centredTop, 1);
    }
    
    std::string displayText = defaultScreenText;
    
    float valueToWrite = activeSliderValue;
        
    if (screenTimeoutCountdown > 0) {
        
        // unacceptable levels of bodge. you have entered bodge city
        // ---------------------------------------------------------
        std::istringstream iss(activeSliderName);
        std::string token1, token2;
        std::getline(iss, token1, '-');
        std::getline(iss, token2);
        
        if (token1 == "Attack") {
            valueToWrite = linearToExponential(valueToWrite, audioProcessor.attackMin, audioProcessor.attackMax);
        }
        if (token1 == "Release") {
            valueToWrite = linearToExponential(valueToWrite, audioProcessor.releaseMin, audioProcessor.releaseMax);
        }
        
        std::stringstream stream;
        stream << std::fixed << std::setprecision(2) << valueToWrite;
        std::string displayValue = stream.str();
        size_t decimalPos = displayValue.find('.');
        if (decimalPos != std::string::npos) {
            displayValue.erase(displayValue.find_last_not_of('0') + 1, std::string::npos);
            if (displayValue.back() == '.') {
                displayValue.pop_back();
            }
        }
        
        displayText = token1 + "\n" + displayValue + " " + token2;
        
        if (token1 == "Convexity") {
            if (valueToWrite < 0.5) {
                displayText = token1 + "\n" + displayValue + " (extreme value)";
            }
            if (valueToWrite > 1.5) {
                displayText = token1 + "\n" + displayValue + " (extreme value)";
            }
        }
        
        if (token1 == "Sidechain Input") {
            displayText = token1 + "\nOFF";
            if (valueToWrite == 1) {
                displayText = token1 + "\nON";
            }
        }
        
        // end of bodge city
        // ---------------------------------------------------------
    }
    
    // is putting spaces in between the letters necessary? Is there a font style tweak instead?
    std::string transformedDisplayText = "";
    for (int i = 0; i < displayText.length(); ++i)
        {
            transformedDisplayText += displayText[i];
            if (i != displayText.length() - 1)
                transformedDisplayText += ' ';
        }
    
    g.setColour(juce::Colours::black.withAlpha(0.6f));
    g.setFont(customTypeface);
    customTypeface.setHeight(32.0f);

    g.drawFittedText(transformedDisplayText, 42, 324, 400, 60, juce::Justification::topLeft, 2);
    
    std::string oversamplingString = "";
    oversamplingString = std::to_string(int(audioProcessor.selectedOS));
    oversamplingString += ":";
    oversamplingString += std::to_string(int(audioProcessor.oversampledSampleRate));

    g.setColour(juce::Colours::white.withAlpha(0.3f));
    g.drawFittedText(oversamplingString, 494, 12, 400, 400, juce::Justification::topLeft, 2);
    
    int gainReductionMax = audioProcessor.metersGainReduction[0];
    int gainReductionR = audioProcessor.metersGainReduction[1];
    
    // text rendering of GR is currently a bit lame. Delete entirely?
    if (gainReductionMax < gainReductionR) gainReductionMax = gainReductionR;
    
    std::stringstream ss;
    ss << std::setw(2) << std::setfill('0') << gainReductionMax;
    std::string gainReductionString = ss.str() + " db";

    customTypeface.setHeight(38.0f);
    g.setColour(juce::Colours::black.withAlpha(0.6f));
    g.drawFittedText(gainReductionString, 240, 350, 200, 100, juce::Justification::topLeft, 1);
    
    if (metersActive) {
        int meterL1 = 0;
        int meterR1 = 0;
        int meterL2 = 0;
        int meterR2 = 0;
        int meterL3 = 0;
        int meterR3 = 0;
        bool meterL1Clip = false;
        bool meterR1Clip = false;
        bool meterL2Clip = false;
        bool meterR2Clip = false;
        bool meterL3Clip = false;
        bool meterR3Clip = false;
        
        meterL1 = std::abs(audioProcessor.meterSignalInput[0]) * -197.0f;
        meterR1 = std::abs(audioProcessor.meterSignalInput[1]) * -197.0f;
        meterL2 = audioProcessor.metersGainReduction[0] * 16.416f;
        meterR2 = audioProcessor.metersGainReduction[1] * 16.416f;
        meterL3 = std::abs(audioProcessor.meterSignalOutput[0]) * -197.0f;
        meterR3 = std::abs(audioProcessor.meterSignalOutput[1]) * -197.0f;
        
        if (meterL1 < -197) {
            meterL1 = -197;
            meterL1Clip = true;
        }
        if (meterR1 < -197) {
            meterR1 = -197;
            meterR1Clip = true;
            
        }
        if (meterL2 > 197) {
            meterL2 = 197;
            meterL2Clip = true;
        }
        if (meterR2 > 197) {
            meterR2 = 197;
            meterR2Clip = true;
        }
        if (meterL2 < 0) {
            meterL2 = 0;
            meterL2Clip = true;
        }
        if (meterR2 < 0) {
            meterR2 = 0;
            meterR2Clip = true;
        }
        if (meterL3 < -197) {
            meterL3 = -197;
            meterL3Clip = true;
            
        }
        if (meterR3 < -197) {
            meterR3 = -197;
            meterR3Clip = true;
        }
        
        g.setColour(juce::Colours::black.withAlpha(0.25f));
        
        g.fillRect(495, 255, 7, meterL1);
        g.fillRect(502, 255, 7, meterR1);
        g.fillRect(509, 58, 7, meterL2);
        g.fillRect(516, 58, 7, meterR2);
        g.fillRect(523, 255, 7, meterL3);
        g.fillRect(530, 255, 7, meterR3);
        
        g.setColour(juce::Colours::red);
        
        if (meterL1Clip) g.fillRect(495, 58, 7, 4);
        if (meterR1Clip) g.fillRect(502, 58, 7, 4);
        if (meterL2Clip) g.fillRect(509, 255, 7, 4);
        if (meterR2Clip) g.fillRect(516, 255, 7, 4);
        if (meterL3Clip) g.fillRect(523, 58, 7, 4);
        if (meterR3Clip) g.fillRect(530, 58, 7, 4);
    }
        
    if (audioProcessor.feedbackClip) g.fillEllipse(558, 342, 6, 6);
}

void APCompAudioProcessorEditor::resized()
{
    attackSlider.setBounds          (60,  160, 100, 100);
    releaseSlider.setBounds         (200, 160, 100, 100);
    convexitySlider.setBounds       (350, 160, 100, 100);
    inGainSlider.setBounds          (32,  474, 100, 100);
    thresholdSlider.setBounds       (171, 474, 100, 100);
    ratioSlider.setBounds           (319, 474, 100, 100);
    outGainSlider.setBounds         (466, 474, 100, 100);
    channelLinkSlider.setBounds     (448, 346, 60, 60);
    sidechainSlider.setBounds       (378, 346, 60, 60);
    feedbackSlider.setBounds        (517, 346, 60, 60);
    inertiaSlider.setBounds         (310, 346, 60, 60);
    inertiaDecaySlider.setBounds    (295, 400, 60, 60);
    metersOnButton.setBounds        (540, 241, 24, 24);
    oversamplingBox.setBounds       (484, 270, 82, 26);
}


void APCompAudioProcessorEditor::timerCallback()
{
    repaint();
    if (screenTimeoutCountdown > 0) screenTimeoutCountdown--;
}


KnobLook1::KnobLook1()
{
    knobImage = juce::ImageFileFormat::loadFrom(BinaryData::knob1_png, BinaryData::knob1_pngSize);
}
KnobLook2::KnobLook2()
{
    knobImage = juce::ImageFileFormat::loadFrom(BinaryData::knob2_png, BinaryData::knob2_pngSize);
}
KnobLook3::KnobLook3()
{
    knobImage = juce::ImageFileFormat::loadFrom(BinaryData::knob3_png, BinaryData::knob3_pngSize);
}
KnobLook4::KnobLook4()
{
    knobImage = juce::ImageFileFormat::loadFrom(BinaryData::knob4_png, BinaryData::knob4_pngSize);
}
KnobLook5::KnobLook5()
{
    knobImage = juce::ImageFileFormat::loadFrom(BinaryData::knob5_png, BinaryData::knob5_pngSize);
}
KnobLook6::KnobLook6()
{
    knobImage = juce::ImageFileFormat::loadFrom(BinaryData::screw_png, BinaryData::screw_pngSize);
}

void KnobLook1::drawRotarySlider(juce::Graphics& g,
                                         int x, int y, int width, int height,
                                         float sliderPosProportional,
                                         float rotaryStartAngle,
                                         float rotaryEndAngle,
                                         juce::Slider& slider)
{
    const float radius = juce::jmin(width / 2, height / 2);
    const float centreX = x + width * 0.5f;
    const float centreY = y + height * 0.5f;
    const float rx = centreX - radius;
    const float ry = centreY - radius;
    const float angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);
        
    if (knobImage.isValid())
    {
        g.saveState();
        g.addTransform(juce::AffineTransform::rotation(angle, centreX, centreY));
        g.drawImageTransformed(knobImage, juce::AffineTransform::translation(rx, ry), false);
        g.restoreState();
    }
}

void KnobLook2::drawRotarySlider(juce::Graphics& g,
                                         int x, int y, int width, int height,
                                         float sliderPosProportional,
                                         float rotaryStartAngle,
                                         float rotaryEndAngle,
                                         juce::Slider& slider)
{
    const float radius = juce::jmin(width / 2, height / 2);
    const float centreX = x + width * 0.5f;
    const float centreY = y + height * 0.5f;
    const float rx = centreX - radius;
    const float ry = centreY - radius;
    const float angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);
        
    if (knobImage.isValid())
    {
        g.saveState();
        g.addTransform(juce::AffineTransform::rotation(angle, centreX, centreY));
        g.drawImageTransformed(knobImage, juce::AffineTransform::translation(rx, ry), false);
        g.restoreState();
    }
}

void KnobLook3::drawRotarySlider(juce::Graphics& g,
                                         int x, int y, int width, int height,
                                         float sliderPosProportional,
                                         float rotaryStartAngle,
                                         float rotaryEndAngle,
                                         juce::Slider& slider)
{
    const float radius = juce::jmin(width / 2, height / 2);
    const float centreX = x + width * 0.5f;
    const float centreY = y + height * 0.5f;
    const float rx = centreX - radius;
    const float ry = centreY - radius;
    const float angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);
        
    if (knobImage.isValid())
    {
        g.saveState();
        g.addTransform(juce::AffineTransform::rotation(angle, centreX, centreY));
        g.drawImageTransformed(knobImage, juce::AffineTransform::translation(rx, ry), false);
        g.restoreState();
    }
}

void KnobLook4::drawRotarySlider(juce::Graphics& g,
                                         int x, int y, int width, int height,
                                         float sliderPosProportional,
                                         float rotaryStartAngle,
                                         float rotaryEndAngle,
                                         juce::Slider& slider)
{
    const float radius = juce::jmin(width / 2, height / 2);
    const float centreX = x + width * 0.5f;
    const float centreY = y + height * 0.5f;
    const float rx = centreX - radius;
    const float ry = centreY - radius;
    const float angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);
        
    if (knobImage.isValid())
    {
        g.saveState();
        g.addTransform(juce::AffineTransform::rotation(angle, centreX, centreY));
        g.drawImageTransformed(knobImage, juce::AffineTransform::translation(rx, ry), false);
        g.restoreState();
    }
}

void KnobLook5::drawRotarySlider(juce::Graphics& g,
                                         int x, int y, int width, int height,
                                         float sliderPosProportional,
                                         float rotaryStartAngle,
                                         float rotaryEndAngle,
                                         juce::Slider& slider)
{
    const float radius = juce::jmin(width / 2, height / 2);
    const float centreX = x + width * 0.5f;
    const float centreY = y + height * 0.5f;
    const float rx = centreX - radius;
    const float ry = centreY - radius;
    const float angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);
        
    if (knobImage.isValid())
    {
        g.saveState();
        g.addTransform(juce::AffineTransform::rotation(angle, centreX, centreY));
        g.drawImageTransformed(knobImage, juce::AffineTransform::translation(rx, ry), false);
        g.restoreState();
    }
}
void KnobLook6::drawRotarySlider(juce::Graphics& g,
                                         int x, int y, int width, int height,
                                         float sliderPosProportional,
                                         float rotaryStartAngle,
                                         float rotaryEndAngle,
                                         juce::Slider& slider)
{
    const float radius = juce::jmin(15 / 2, 15 / 2);
    const float centreX = x + 15 * 0.5f;
    const float centreY = y + 15 * 0.5f;
    const float rx = centreX - radius;
    const float ry = centreY - radius;
    const float angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);
        
    if (knobImage.isValid())
    {
        g.saveState();
        g.addTransform(juce::AffineTransform::rotation(angle, centreX, centreY));
        g.drawImageTransformed(knobImage, juce::AffineTransform::translation(rx, ry), false);
        g.restoreState();
    }
}
