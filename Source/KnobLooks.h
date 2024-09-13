#pragma once

#include "APCommon.h"


class KnobLook1 : public juce::LookAndFeel_V4 {
public:
    KnobLook1();
    void drawRotarySlider (juce::Graphics& g, 
                           int x,
                           int y,
                           int width,
                           int height,
                           float sliderPosProportional,
                           float rotaryStartAngle,
                           float rotaryEndAngle, 
                           juce::Slider& slider) override;
private:
    juce::Image knobImage;
};


class KnobLook2 : public juce::LookAndFeel_V4 {
public:
    KnobLook2();
    void drawRotarySlider (juce::Graphics& g,
                           int x,
                           int y,
                           int width,
                           int height,
                           float sliderPosProportional,
                           float rotaryStartAngle,
                           float rotaryEndAngle,
                           juce::Slider& slider) override;
private:
    juce::Image knobImage;
};


void drawKnob(juce::Graphics &g,
              const juce::Image &knobImage,
              float rotaryEndAngle,
              float rotaryStartAngle,
              float sliderPosProportional,
              int width,
              int &x,
              int &y);
