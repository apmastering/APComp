#include "KnobLooks.h"


KnobLook1::KnobLook1() {
    knobImage = juce::ImageFileFormat::loadFrom(BinaryData::knob_png, BinaryData::knob_pngSize);
}


KnobLook2::KnobLook2() {
    knobImage = juce::ImageFileFormat::loadFrom(BinaryData::knob2_png, BinaryData::knob2_pngSize);
}


void KnobLook1::drawRotarySlider(juce::Graphics& g,
                                 int x, int y, int width, int height,
                                 float sliderPosProportional,
                                 float rotaryStartAngle,
                                 float rotaryEndAngle,
                                 juce::Slider& slider) {
    
    drawKnob(g, knobImage, rotaryEndAngle, rotaryStartAngle, sliderPosProportional, width, x, y);
}


void KnobLook2::drawRotarySlider(juce::Graphics& g,
                                 int x, int y, int width, int height,
                                 float sliderPosProportional,
                                 float rotaryStartAngle,
                                 float rotaryEndAngle,
                                 juce::Slider& slider) {
    
    drawKnob(g, knobImage, rotaryEndAngle, rotaryStartAngle, sliderPosProportional, width, x, y);
}


void drawKnob(juce::Graphics &g,
              const juce::Image &knobImage,
              float rotaryEndAngle,
              float rotaryStartAngle,
              float sliderPosProportional,
              int width,
              int &x,
              int &y) {
    
    const float radius = width / 2;
    
    x += radius;
    y += radius;
    
    const float centreX = x + radius;
    const float centreY = y + radius;
    const float rx = x - radius;
    const float ry = y - radius;
    const float angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);
    const float scalingFactor = 0.5;
    
    if (knobImage.isValid()) {
        
        g.saveState();
        g.addTransform(juce::AffineTransform::rotation(angle, centreX, centreY).scaled(scalingFactor));
        g.drawImageTransformed(knobImage, juce::AffineTransform::translation(rx, ry), false);
        g.restoreState();
    }
}
