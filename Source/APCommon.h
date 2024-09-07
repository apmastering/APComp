#pragma once

#include <map>
#include <string>
#include <JuceHeader.h>


enum class OversamplingOption {
    None   = 0,
    FIR_1x = 1,
    IIR_1x = 2,
    FIR_2x = 3,
    IIR_2x = 4
};


double linearToExponential(double linearValue, double minValue, double maxValue);
double gainToDecibels(double gain);
double decibelsToGain(double decibels);
std::string getOversamplingOptionString(OversamplingOption option);
OversamplingOption getOversamplingOptionFromIndex(int index);

class APFont {
public:
    static juce::Font getFont() {
        
        static juce::Font customTypeface = createFont();
        return customTypeface;
    }

private:
    static juce::Font createFont() {
        
        auto typeface = juce::Typeface::createSystemTypefaceFor(
            BinaryData::KnockoutFlyweight_otf, BinaryData::KnockoutFlyweight_otfSize);

        if (typeface != nullptr)
        {
            return juce::Font(juce::FontOptions().withTypeface(typeface));
        }
        
        return juce::Font(juce::FontOptions(14.0f));
    }
};
