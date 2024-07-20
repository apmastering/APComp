#include "APCommon.h"
#include <algorithm>
#include <cmath>
#include <limits>

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
