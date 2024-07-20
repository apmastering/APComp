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
    convexitySlider.setLookAndFeel(&knobLook1);
    attackSlider.setLookAndFeel(&knobLook1);
    releaseSlider.setLookAndFeel(&knobLook1);
    thresholdSlider.setLookAndFeel(&knobLook1);
    ratioSlider.setLookAndFeel(&knobLook1);
    channelLinkSlider.setLookAndFeel(&knobLook1);
    sidechainSlider.setLookAndFeel(&knobLook1);
    feedbackSlider.setLookAndFeel(&knobLook1);
    inertiaSlider.setLookAndFeel(&knobLook1);
    inertiaDecaySlider.setLookAndFeel(&knobLook1);
    
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
    
    backgroundImage = juce::ImageFileFormat::loadFrom(BinaryData::bgflat_png, BinaryData::bgflat_pngSize);

    auto typeface = juce::Typeface::createSystemTypefaceFor(BinaryData::KnockoutFlyweight_otf, BinaryData::KnockoutFlyweight_otfSize);
    customTypeface = juce::FontOptions (typeface);

    setSize (760, 400);
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
    
    g.setColour(juce::Colours::black.withAlpha(0.3f));
    g.setFont(customTypeface);
    customTypeface.setHeight(32.0f);

    g.drawFittedText(displayText, 340, 48, 200, 30, juce::Justification::centredTop, 2);
    
    std::string oversamplingString = "";
    oversamplingString = std::to_string(int(audioProcessor.selectedOS));
    oversamplingString += ":";
    oversamplingString += std::to_string(int(audioProcessor.oversampledSampleRate));

    g.setColour(juce::Colours::black.withAlpha(0.3f));
    g.drawFittedText(oversamplingString, 580, 48, 100, 30, juce::Justification::centredTop, 2);
    
    float meterGainReductionL = audioProcessor.meterValues[4];
    float meterGainReductionR = audioProcessor.meterValues[5];
    
    int gainReductionMax = static_cast<int>(meterGainReductionL);
    
    if (meterGainReductionL < meterGainReductionR) gainReductionMax = static_cast<int>(meterGainReductionR);
    
    if (gainReductionMax > previousGainReduction) {
        previousGainReduction = gainReductionMax;
        gainReductionTextHoldCountdown = gainReductionTextHoldConstant;
    }
    
    if (gainReductionTextHoldCountdown > 0) {
        gainReductionTextHoldCountdown--;
    } else {
        previousGainReduction = gainReductionMax;
    }
    
    std::stringstream ss;
    ss << std::setw(2) << std::setfill('0') << previousGainReduction;
    std::string gainReductionString = ss.str() + " db";

    customTypeface.setHeight(38.0f);
    g.setColour(juce::Colours::black.withAlpha(0.3f));
    g.drawFittedText(gainReductionString, 690, 48, 56, 30, juce::Justification::centredTop, 1);
    
    const int meterHeight = 268;
    const int meterLeftStart = 700;
    const int meterTopStart = 104;
    const int spacing = 6;
    
    g.setColour(juce::Colours::black);

    if (metersActive) {
        
        const float meterGrainRedutionMaxDB = 12.0f;
        
        float meterInputL = audioProcessor.meterValues[0];
        float meterInputR = audioProcessor.meterValues[1];
        float meterOutputL = audioProcessor.meterValues[2];
        float meterOutputR = audioProcessor.meterValues[3];
        
        meterInputL = std::clamp(meterInputL, 0.0f, 1.0f);
        meterInputR = std::clamp(meterInputR, 0.0f, 1.0f);
        meterOutputL = std::clamp(meterOutputL, 0.0f, 1.0f);
        meterOutputR = std::clamp(meterOutputR, 0.0f, 1.0f);
        meterGainReductionL = std::clamp(meterGainReductionL, 0.0f, meterGrainRedutionMaxDB);
        meterGainReductionR = std::clamp(meterGainReductionR, 0.0f, meterGrainRedutionMaxDB);

        int meterInputLInt = meterHeight - (meterInputL * meterHeight);
        int meterInputRInt = meterHeight - (meterInputR * meterHeight);
        int meterOutputLInt = meterHeight -  (meterOutputL * meterHeight);
        int meterOutputRInt = meterHeight - (meterOutputR * meterHeight);
        int meterGainReductionLInt = (meterGainReductionL / meterGrainRedutionMaxDB) * meterHeight;
        int meterGainReductionRInt = (meterGainReductionR / meterGrainRedutionMaxDB) * meterHeight;

        g.fillRect(meterLeftStart + (spacing * 0), meterTopStart, spacing, meterInputLInt);
        g.fillRect(meterLeftStart + (spacing * 1), meterTopStart, spacing, meterInputRInt);

        g.fillRect(meterLeftStart + (spacing * 2), meterTopStart + meterGainReductionLInt, spacing, std::clamp(meterHeight, 0, meterHeight - meterGainReductionLInt));
        g.fillRect(meterLeftStart + (spacing * 3), meterTopStart + meterGainReductionRInt, spacing, std::clamp(meterHeight, 0, meterHeight - meterGainReductionRInt));

        g.fillRect(meterLeftStart + (spacing * 4), meterTopStart, spacing, meterOutputLInt);
        g.fillRect(meterLeftStart + (spacing * 5), meterTopStart, spacing, meterOutputRInt);

    } else {
        g.fillRect(meterLeftStart, meterTopStart, spacing * 6, meterHeight);
    }
        
    if (audioProcessor.feedbackClip) g.fillEllipse(424, 145, 6, 6);
}

void APCompAudioProcessorEditor::resized()
{
    attackSlider.setBounds          (347, 288, 100, 100);
    releaseSlider.setBounds         (461, 288, 100, 100);
    convexitySlider.setBounds       (13,  160, 100, 100);
    inGainSlider.setBounds          (13,  288, 100, 100);
    thresholdSlider.setBounds       (125, 288, 100, 100);
    ratioSlider.setBounds           (237, 288, 100, 100);
    outGainSlider.setBounds         (573, 288, 100, 100);
    channelLinkSlider.setBounds     (461, 160, 100,  100);
    sidechainSlider.setBounds       (573, 160, 100,  100);
    feedbackSlider.setBounds        (347, 160, 100,  100);
    inertiaSlider.setBounds         (125, 160, 100,  100);
    inertiaDecaySlider.setBounds    (237, 160, 100,  100);
    metersOnButton.setBounds        (704, 0,   100,  76);
    oversamplingBox.setBounds       (580, 99,  100,  30);
}


void APCompAudioProcessorEditor::timerCallback()
{
    repaint();
    if (screenTimeoutCountdown > 0) screenTimeoutCountdown--;
}


KnobLook1::KnobLook1()
{
    knobImage = juce::ImageFileFormat::loadFrom(BinaryData::knobflat_png, BinaryData::knobflat_pngSize);
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
