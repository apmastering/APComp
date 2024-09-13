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


enum class ButtonName {
    oversamplingON,
    oversamplingOFF,
    sidechainInternal,
    sidechainExternal,
    logo,
    meters,
    variMu,
    slow,
    none
};


enum class ParameterNames {
    inGain          = 0,
    outGain         = 1,
    convexity       = 2,
    attack          = 3,
    release         = 4,
    threshold       = 5,
    ratio           = 6,
    channelLink     = 7,
    feedback        = 8,
    inertia         = 9,
    inertiaDecay    = 10,
    sidechain       = 11,
    metersOn        = 12,
    oversampling    = 13,
    overdrive       = 14,
    variMu          = 15,
    slow            = 16,
    END             = 17
};


double linearToExponential(double linearValue, double minValue, double maxValue);
double gainToDecibels(double gain);
double decibelsToGain(double decibels);
std::string getParameterNameFromEnum(ParameterNames index);
ParameterNames getParameterEnumFromParameterName(const std::string& name);


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

        if (typeface != nullptr) {
            
            return juce::Font(juce::FontOptions(typeface));
        }
        
        return juce::Font(juce::FontOptions(14.0f));
    }
};
