#include <algorithm>
#include <cmath>
#include <limits>
#include <unordered_map>

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


std::string floatToStringWithTwoDecimalPlaces(float value) {
    std::stringstream stream;
    stream << std::fixed << std::setprecision(2) << value;
    return stream.str();
}


ParameterQuery queryParameter(ParameterNames paramName, const std::string& parameterStringName) {

    static const std::unordered_map<ParameterNames, ParameterQuery> paramNameMap = {
        {ParameterNames::inGain,        { "inGain",       "Input Gain",   ParameterNames::inGain }},
        {ParameterNames::outGain,       { "outGain",      "Output Gain",  ParameterNames::outGain }},
        {ParameterNames::convexity,     { "convexity",    "Convexity",    ParameterNames::convexity }},
        {ParameterNames::attack,        { "attack",       "Attack",       ParameterNames::attack }},
        {ParameterNames::release,       { "release",      "Release",      ParameterNames::release }},
        {ParameterNames::threshold,     { "threshold",    "Threshold",    ParameterNames::threshold }},
        {ParameterNames::ratio,         { "ratio",        "Ratio",        ParameterNames::ratio }},
        {ParameterNames::channelLink,   { "channelLink",  "Channel Link", ParameterNames::channelLink }},
        {ParameterNames::feedback,      { "feedback",     "Feedback",     ParameterNames::feedback }},
        {ParameterNames::inertia,       { "inertia",      "Inertia",      ParameterNames::inertia }},
        {ParameterNames::inertiaDecay,  { "inertiaDecay", "Inertia Decay",ParameterNames::inertiaDecay }},
        {ParameterNames::sidechain,     { "sidechain",    "Sidechain",    ParameterNames::sidechain }},
        {ParameterNames::oversampling,  { "oversampling", "Oversampling", ParameterNames::oversampling }},
        {ParameterNames::ceiling,       { "ceiling",      "Ceiling",      ParameterNames::ceiling }},
        {ParameterNames::variMu,        { "variMu",       "Vari Mu",      ParameterNames::variMu }},
        {ParameterNames::fold,          { "fold",         "Fold",         ParameterNames::fold }}
    };
    
    if (paramName != ParameterNames::END) {
        auto it = paramNameMap.find(paramName);
        if (it != paramNameMap.end()) return it->second;
    }

    static const std::unordered_map<std::string, ParameterNames> nameToEnumMap = {
        {"inGain",        ParameterNames::inGain},
        {"outGain",       ParameterNames::outGain},
        {"convexity",     ParameterNames::convexity},
        {"attack",        ParameterNames::attack},
        {"release",       ParameterNames::release},
        {"threshold",     ParameterNames::threshold},
        {"ratio",         ParameterNames::ratio},
        {"channelLink",   ParameterNames::channelLink},
        {"feedback",      ParameterNames::feedback},
        {"inertia",       ParameterNames::inertia},
        {"inertiaDecay",  ParameterNames::inertiaDecay},
        {"ceiling",       ParameterNames::ceiling},
        {"sidechain",     ParameterNames::sidechain},
        {"oversampling",  ParameterNames::oversampling},
        {"variMu",        ParameterNames::variMu},
        {"fold",          ParameterNames::fold}
    };
    
    auto strIt = nameToEnumMap.find(parameterStringName);
    if (strIt != nameToEnumMap.end()) return queryParameter(strIt->second);

    throw std::invalid_argument("Both enum and string queries failed for parameter for queryParameter");
}
