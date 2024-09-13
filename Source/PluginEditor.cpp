#include <algorithm>

#include "APCommon.h"
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Constants.h"


GUI::GUI (APComp& p)
: AudioProcessorEditor (&p),
audioProcessor (p),
knobLook1(),
backgroundImage (juce::ImageFileFormat::loadFrom(BinaryData::bg_png, BinaryData::bg_pngSize)),
customTypeface (APFont::getFont()),
inGainSlider(),
outGainSlider(),
convexitySlider(),
attackSlider(),
releaseSlider(),
thresholdSlider(),
ratioSlider(),
channelLinkSlider(),
sidechainSlider(),
variMuSlider(),
slowSlider(),
feedbackSlider(),
inertiaSlider(),
inertiaDecaySlider(),
overdriveSlider(),
inGainAttachment (std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "inGain", inGainSlider)),
outGainAttachment (std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "outGain", outGainSlider)),
convexityAttachment (std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "convexity", convexitySlider)),
attackAttachment (std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "attack", attackSlider)),
releaseAttachment (std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "release", releaseSlider)),
thresholdAttachment (std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "threshold", thresholdSlider)),
ratioAttachment (std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "ratio", ratioSlider)),
channelLinkAttachment (std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "channelLink", channelLinkSlider)),
sidechainAttachment (std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "sidechain", sidechainSlider)),
variMuAttachment (std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "variMu", variMuSlider)),
slowAttachment (std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "slow", slowSlider)),
feedbackAttachment (std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "feedback", feedbackSlider)),
inertiaAttachment (std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "inertia", inertiaSlider)),
inertiaDecayAttachment (std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "inertiaDecay", inertiaDecaySlider)),
overdriveAttachment (std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "overdrive", overdriveSlider)),
oversamplingAttachment (std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "oversampling", oversamplingSlider)),
metersActive(true) {
          
    for (size_t i = 0; i < sliders.size(); ++i) {
        juce::Slider& slider = sliders[i].get();
        slider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        addAndMakeVisible(slider);
    }
    
    oversamplingSlider.setVisible(false);
    sidechainSlider.setVisible(false);
    variMuSlider.setVisible(false);
    slowSlider.setVisible(false);
            
    inGainSlider.setLookAndFeel(&knobLook1);
    outGainSlider.setLookAndFeel(&knobLook1);
    convexitySlider.setLookAndFeel(&knobLook1);
    attackSlider.setLookAndFeel(&knobLook1);
    releaseSlider.setLookAndFeel(&knobLook1);
    thresholdSlider.setLookAndFeel(&knobLook1);
    ratioSlider.setLookAndFeel(&knobLook1);
    channelLinkSlider.setLookAndFeel(&knobLook1);
    feedbackSlider.setLookAndFeel(&knobLook1);
    inertiaSlider.setLookAndFeel(&knobLook1);
    inertiaDecaySlider.setLookAndFeel(&knobLook1);
    overdriveSlider.setLookAndFeel(&knobLook1);

    setSize (680, 450);
    
    const int refreshRate = 33;
    
    startTimer(refreshRate);
}


GUI::~GUI() {
    
    stopTimer();
        
    inGainSlider.setLookAndFeel(nullptr);
    outGainSlider.setLookAndFeel(nullptr);
    convexitySlider.setLookAndFeel(nullptr);
    attackSlider.setLookAndFeel(nullptr);
    releaseSlider.setLookAndFeel(nullptr);
    thresholdSlider.setLookAndFeel(nullptr);
    ratioSlider.setLookAndFeel(nullptr);
    channelLinkSlider.setLookAndFeel(nullptr);
    feedbackSlider.setLookAndFeel(nullptr);
    inertiaSlider.setLookAndFeel(nullptr);
    inertiaDecaySlider.setLookAndFeel(nullptr);
    overdriveSlider.setLookAndFeel(nullptr);
}


void GUI::paintMeters(juce::Graphics &g) {
    
    const float meterHeight = 237.0f;
    const float spacing = 7.6;
    
    g.setColour(juce::Colours::black);
    
    if (metersActive) {
        
        const float meterGrainRedutionMaxDB = 12.0f;
        
        float meterInputL         = audioProcessor.meterValuesAtomic[0].load(std::memory_order_relaxed);
        float meterInputR         = audioProcessor.meterValuesAtomic[1].load(std::memory_order_relaxed);
        float meterOutputL        = audioProcessor.meterValuesAtomic[2].load(std::memory_order_relaxed);
        float meterOutputR        = audioProcessor.meterValuesAtomic[3].load(std::memory_order_relaxed);
        float meterGainReductionL = audioProcessor.meterValuesAtomic[4].load(std::memory_order_relaxed);
        float meterGainReductionR = audioProcessor.meterValuesAtomic[5].load(std::memory_order_relaxed);
        
        meterInputL = std::clamp(meterInputL, 0.0f, 1.0f);
        meterInputR = std::clamp(meterInputR, 0.0f, 1.0f);
        meterOutputL = std::clamp(meterOutputL, 0.0f, 1.0f);
        meterOutputR = std::clamp(meterOutputR, 0.0f, 1.0f);
        meterGainReductionL = std::clamp(meterGainReductionL, 0.0f, meterGrainRedutionMaxDB);
        meterGainReductionR = std::clamp(meterGainReductionR, 0.0f, meterGrainRedutionMaxDB);
        
        float meterInputLAdjusted          = meterHeight - (meterInputL * meterHeight);
        float meterInputRAdjusted          = meterHeight - (meterInputR * meterHeight);
        float meterOutputLAdjusted         = meterHeight - (meterOutputL * meterHeight);
        float meterOutputRAdjusted         = meterHeight - (meterOutputR * meterHeight);
        float meterGainReductionLAdjusted  = (meterGainReductionL / meterGrainRedutionMaxDB) * meterHeight;
        float meterGainReductionRAdjusted  = (meterGainReductionR / meterGrainRedutionMaxDB) * meterHeight;
                
        g.fillRect(metersLeft + spacing * 0, metersTop, spacing, meterInputLAdjusted);
        g.fillRect(metersLeft + spacing * 1, metersTop, spacing, meterInputRAdjusted);
        
        g.fillRect(metersLeft + spacing * 2,
                   metersTop + meterGainReductionLAdjusted,
                   spacing,
                   std::clamp(meterHeight, 0.0f, meterHeight - meterGainReductionLAdjusted));
        g.fillRect(metersLeft + spacing * 3,
                   metersTop + meterGainReductionRAdjusted,
                   spacing,
                   std::clamp(meterHeight, 0.0f, meterHeight - meterGainReductionRAdjusted));
        
        g.fillRect(metersLeft + spacing * 4, metersTop, spacing, meterOutputLAdjusted);
        g.fillRect(metersLeft + spacing * 5, metersTop, spacing, meterOutputRAdjusted);
        
    } else {
        g.fillRect(metersLeft, metersTop, spacing * 6, meterHeight);
    }
    
    g.setColour(juce::Colours::white.withAlpha(0.7f));

    const int clipLightRadius = 4;
    if (audioProcessor.feedbackClip.load(std::memory_order_relaxed)) g.fillEllipse(550 - clipLightRadius,
                                                                                   291 - clipLightRadius,
                                                                                   clipLightRadius * 2,
                                                                                   clipLightRadius * 2);
}


void GUI::paintBackground(juce::Graphics &g) {
    if (backgroundImage.isValid()) {
        g.drawImage(backgroundImage, getLocalBounds().toFloat());
    } else {
        g.fillAll(juce::Colours::lightgrey);
        g.setColour (juce::Colours::black);
        g.setFont (24.0f);
        g.drawFittedText ("AP Mastering - Advanced Compressor: GUI error", getLocalBounds(), juce::Justification::centredTop, 1);
    }
}


void GUI::paintButtons(juce::Graphics &g) {

    const int oversamplingX = 411;
    const int sidechainX    = 589;
    const int row1          = 28;
    const int row2          = 49;
    const int radius        = 4;
    
    if (audioProcessor.getButtonKnobValue(ParameterNames::oversampling))
        g.fillEllipse(oversamplingX - radius, row1 - radius, radius * 2, radius * 2);
    else
        g.fillEllipse(oversamplingX - radius, row2 - radius, radius * 2, radius * 2);
    
    if (audioProcessor.getButtonKnobValue(ParameterNames::sidechain))
        g.fillEllipse(sidechainX - radius, row2 - radius, radius * 2, radius * 2);
    else
        g.fillEllipse(sidechainX - radius, row1 - radius, radius * 2, radius * 2);
}


void GUI::paint (juce::Graphics& g) {
    
    paintBackground(g);
    
    paintMeters(g);
    
    paintButtons(g);
}


void GUI::resized() {
    
    const int knobWidth  = 54;
    const int knobHeight = knobWidth;
    const int radius     = knobWidth / 2;
    
    const int IOKnobsX = 642;
    const int inputY   = 313;
    const int outputY  = 388;
    
    convexitySlider.setBounds       (knobColumn1 - radius, knobRow1 - radius, knobWidth, knobHeight);
    inertiaSlider.setBounds         (knobColumn1 - radius, knobRow2 - radius, knobWidth, knobHeight);
    inertiaDecaySlider.setBounds    (knobColumn1 - radius, knobRow3 - radius, knobWidth, knobHeight);
    
    ratioSlider.setBounds           (knobColumn2 - radius, knobRow1 - radius, knobWidth, knobHeight);
    thresholdSlider.setBounds       (knobColumn2 - radius, knobRow2 - radius, knobWidth, knobHeight);

    attackSlider.setBounds          (knobColumn3 - radius, knobRow1 - radius, knobWidth, knobHeight);
    releaseSlider.setBounds         (knobColumn3 - radius, knobRow2 - radius, knobWidth, knobHeight);
    
    channelLinkSlider.setBounds     (knobColumn4 - radius, knobRow1 - radius, knobWidth, knobHeight);
    feedbackSlider.setBounds        (knobColumn4 - radius, knobRow2 - radius, knobWidth, knobHeight);
    overdriveSlider.setBounds       (knobColumn4 - radius, knobRow3 - radius, knobWidth, knobHeight);
 
    inGainSlider.setBounds          (IOKnobsX    - radius, inputY   - radius, knobWidth, knobHeight);
    outGainSlider.setBounds         (IOKnobsX    - radius, outputY  - radius, knobWidth, knobHeight);
}


ButtonName GUI::determineButton(const juce::MouseEvent &event) {

    const int row1 = 20;
    const int row2 = 40;
    const int row3 = 60;
    const int oversamplingLeft = 383;
    const int oversamplingRight = 423;
    const int sidechainLeft = 535;
    const int sidechainRight = 605;
    const int logoRight = 230;
    const int buttonRadius = 20;
        
    if (event.x > oversamplingLeft &&
        event.x < oversamplingRight &&
        event.y > row1 &&
        event.y < row2) {
        
        return ButtonName::oversamplingON;
    }
    
    if (event.x > oversamplingLeft &&
        event.x < oversamplingRight &&
        event.y > row2 &&
        event.y < row3) {
        
        return ButtonName::oversamplingOFF;
    }
    
    if (event.x > sidechainLeft &&
        event.x < sidechainRight &&
        event.y > row1 &&
        event.y < row2) {
        
        return ButtonName::sidechainInternal;
    }
    
    if (event.x > sidechainLeft &&
        event.x < sidechainRight &&
        event.y > row2 &&
        event.y < row3) {
        
        return ButtonName::sidechainExternal;
    }
    
    if (event.x > knobColumn2 - buttonRadius &&
        event.x < knobColumn2 + buttonRadius &&
        event.y > knobRow3 - buttonRadius &&
        event.y < knobRow3 + buttonRadius) {
        
        return ButtonName::variMu;
    }
    
    if (event.x > knobColumn3 - buttonRadius &&
        event.x < knobColumn3 + buttonRadius &&
        event.y > knobRow3 - buttonRadius &&
        event.y < knobRow3 + buttonRadius) {
        
        return ButtonName::slow;
    }
    
    if (event.x > metersLeft &&
        event.x < metersRight &&
        event.y > metersTop &&
        event.y < metersBottom) {
        
        return ButtonName::meters;
    }
    
    if (event.x < logoRight &&
        event.y < row3) {
        
        return ButtonName::logo;
    }
    
    return ButtonName::none;
}


void GUI::mouseDown (const juce::MouseEvent& event) {
    
    ButtonName button = determineButton(event);

    switch (button) {
            
        case ButtonName::none:                                            return;
        case ButtonName::logo:                                            return;
        case ButtonName::meters:            { toggleMeters();             return; }
        case ButtonName::oversamplingON:    { switchOversampling(true);   return; }
        case ButtonName::oversamplingOFF:   { switchOversampling(false);  return; }
        case ButtonName::sidechainInternal: { switchSidechain(false);     return; }
        case ButtonName::sidechainExternal: { switchSidechain(true);      return; }
        case ButtonName::variMu:            { toggleVariMu();             return; }
        case ButtonName::slow:              { toggleSlow();               return; }
        default: return;
    }
}


void GUI::switchSidechain(bool active) {
    
    sidechainSlider.setValue(active ? 1 : 0);
}


void GUI::toggleVariMu() {
    
    bool currentValue = audioProcessor.getButtonKnobValue(ParameterNames::variMu);
    
    if (currentValue) variMuSlider.setValue(false);
    else variMuSlider.setValue(true);
}


void GUI::toggleSlow() {
    
    bool currentValue = audioProcessor.getButtonKnobValue(ParameterNames::slow);
    
    if (currentValue) slowSlider.setValue(false);
    else slowSlider.setValue(true);
}


void GUI::switchOversampling(bool active) {
    
    oversamplingSlider.setValue(active ? 1 : 0);
}


void GUI::toggleMeters() {
 
    metersActive = !metersActive;
}


void GUI::timerCallback() {
    
    repaint();
}


KnobLook1::KnobLook1() {
    
    knobImage = juce::ImageFileFormat::loadFrom(BinaryData::knob_png, BinaryData::knob_pngSize);
}

void KnobLook1::drawRotarySlider(juce::Graphics& g,
                                         int x, int y, int width, int height,
                                         float sliderPosProportional,
                                         float rotaryStartAngle,
                                         float rotaryEndAngle,
                                         juce::Slider& slider) {

    const float radius = width / 2;
    
    x += radius;
    y += radius;
    
    const float centreX = x + radius;
    const float centreY = y + radius;
    const float rx = x - radius;
    const float ry = y - radius;
    const float angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);
        
    if (knobImage.isValid()) {
        
        g.saveState();
        g.addTransform(juce::AffineTransform::rotation(angle, centreX, centreY).scaled(0.5));
        g.drawImageTransformed(knobImage, juce::AffineTransform::translation(rx, ry), false);
        g.restoreState();
    }
}
