#include <algorithm>
#include <cmath>
#include <limits>

#include "APCommon.h"


double linearToExponential(double linearValue, double minValue, double maxValue)
{
    linearValue = std::clamp(linearValue, minValue, maxValue);
    double normalized = (linearValue - minValue) / (maxValue - minValue);
    double exponentialValue = std::pow(normalized, 2.0);
    double result = minValue + exponentialValue * (maxValue - minValue);
    return result;
}

double gainToDecibels(double gain) {
    if (gain <= 0.0)
        return -1000;
    
    if (gain > 1000) gain = 1000;

    return 20.0 * std::log10(gain);
}

double decibelsToGain(double decibels) {
    if (decibels <= -1000)
        return 0.0;
    
    if (decibels > 1000) decibels = 1000;
    
    return std::pow(10.0, decibels / 20.0);
}


const std::map<OversamplingOption, std::string> oversamplingOptionToString = {
    { OversamplingOption::None,   "None" },
    { OversamplingOption::FIR_1x, "1x FIR" },
    { OversamplingOption::IIR_1x, "1x IIR" },
    { OversamplingOption::FIR_2x, "2x FIR" },
    { OversamplingOption::IIR_2x, "2x IIR" }
};


OversamplingOption getOversamplingOptionFromIndex(int index) {
    switch (index) {
        case 0: return OversamplingOption::None;
        case 1: return OversamplingOption::FIR_1x;
        case 2: return OversamplingOption::IIR_1x;
        case 3: return OversamplingOption::FIR_2x;
        case 4: return OversamplingOption::IIR_2x;
        default: throw std::out_of_range("Invalid index for OversamplingOption");
    }
}


std::string getOversamplingOptionString(OversamplingOption option) {
    
    auto it = oversamplingOptionToString.find(option);
    
    if (it != oversamplingOptionToString.end()) {
        return it->second;
    } 
    
    return "";
}
