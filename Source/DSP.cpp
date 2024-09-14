//#include <algorithm>
//#include <utility>
//#include <cstring>

#include "APCommon.h"
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Constants.h"


void APComp::updateMeters(float *maxValuesForMeters) {
    
    for (int i = 0; i < meterCount; i++) {
        
        if (maxValuesForMeters[i] > 100) maxValuesForMeters[i] = 100;
        
        if (maxValuesForMeters[i] > meterValues[i]) {
            meterValues[i] = maxValuesForMeters[i];
        } else {
            meterValues[i] = meterValues[i] * meterDecayCoefficient;
        }
        
        meterValuesAtomic[i].store(meterValues[i], std::memory_order_relaxed);
    }
}

void APComp::flushLoopVariables() {
    
    if (flushDSP.load(std::memory_order_relaxed)) {
        for (int channel = 0; channel < 2; channel++) {
            slewedSignal[channel] = -200.0;
            gainReduction[channel] = 0.0f;
            outputSample[channel] = 0.0f;
            previousGainReduction[channel] = -200.0;
            inertiaVelocity[channel] = 0.0f;
        }
        
        flushDSP.store(false);
    }
}

void APComp::doCompressionDSP(juce::dsp::AudioBlock<float>& mainBlock,
                              juce::dsp::AudioBlock<float>& sidechainBlock,
                              size_t oversamplingFactor,
                              int sampleRate) {
    
    flushLoopVariables();
    
    const float attackValue  = linearToExponential(getKnobValueFromCache(static_cast<int>(ParameterNames::attack)),
                                                  Constants::attackMin,
                                                  Constants::attackMax) / 1000;
    const float releaseValue = linearToExponential(getKnobValueFromCache(static_cast<int>(ParameterNames::release)),
                                                  Constants::releaseMin,
                                                  Constants::releaseMax) / 1000;
    const float ratioValue   = linearToExponential(getKnobValueFromCache(static_cast<int>(ParameterNames::ratio)),
                                                   Constants::ratioMin,
                                                   Constants::ratioMax);
    
    const float inputGainValue          = getKnobValueFromCache(static_cast<int>(ParameterNames::inGain));
    const float outGainValue            = getKnobValueFromCache(static_cast<int>(ParameterNames::outGain));
    const float convexityValue          = getKnobValueFromCache(static_cast<int>(ParameterNames::convexity));
    const float thresholdValue          = getKnobValueFromCache(static_cast<int>(ParameterNames::threshold));
    const float channelLinkValue        = getKnobValueFromCache(static_cast<int>(ParameterNames::channelLink)) / 100;
    const float feedbackValue           = getKnobValueFromCache(static_cast<int>(ParameterNames::feedback));
    const float inertiaCoefficientValue = getKnobValueFromCache(static_cast<int>(ParameterNames::inertia));
    const float ceiling                 = getKnobValueFromCache(static_cast<int>(ParameterNames::ceiling));
#if PRO_VERSION
    const float foldback                = getKnobValueFromCache(static_cast<int>(ParameterNames::fold));
#endif
          float inertiaDecayCoefficient = getKnobValueFromCache(static_cast<int>(ParameterNames::inertiaDecay));
          bool  sidechainSelected       = getBoolValueFromCache(static_cast<int>(ParameterNames::sidechain));
    
    if (ratioValue == 0) return;

    const double attackCoefficient  = std::exp(-1.0 / (sampleRate * attackValue));
    const double releaseCoefficient = std::exp(-1.0 / (sampleRate * releaseValue));

    float maxValuesForMeters[meterCount] = {0};

    float* channelData[2];
    float* sidechainChannelData[2];
    
    for (int i = 0; i < totalNumInputChannels     && i < 2; i++) channelData[i]          = mainBlock.getChannelPointer(i);
    for (int i = 0; i < totalNumInputChannels - 2 && i < 2; i++) sidechainChannelData[i] = sidechainBlock.getChannelPointer(i);
        
    inertiaDecayCoefficient = 0.99 + (inertiaDecayCoefficient * 0.01);
    
    for (int sample = 0; sample < mainBlock.getNumSamples(); ++sample) {
        
        float inputSample[4];
        double inputSampledb[2];
        inputSampledb[0] = -200.0f;
        inputSampledb[1] = -200.0f;
        
        for (int channel = 0; channel < 2 && channel < totalNumInputChannels; channel++) {
            inputSample[channel] = channelData[channel][sample];
            inputSample[channel] *= decibelsToGain(inputGainValue);
            inputSampledb[channel] = gainToDecibels(std::abs(inputSample[channel]) + std::abs(outputSample[channel] * feedbackValue));
            outputSample[channel] = 0;
            
            if (std::isnan(inputSampledb[channel])) inputSampledb[channel] = -200.0f;
            
            if (sidechainSelected) {
                
                if (totalNumInputChannels <= channel + 2) inputSampledb[channel] = -200.0f;
                else inputSampledb[channel] = gainToDecibels(std::abs(sidechainChannelData
                                                                      [channel]
                                                                      [sample / static_cast<int>(std::pow(2, oversamplingFactor))]
                                                                      ));
            }
            
            if (inputSampledb[channel] < -200) inputSampledb[channel] = -200;

            if (inputSampledb[channel] > 4) {
                inputSampledb[channel] = 4.0;  // hardcoded 4db feedback path clip. Improve later
                feedbackClip.store(true, std::memory_order_relaxed);
            } else {
                feedbackClip.store(false, std::memory_order_relaxed);
            }
            
            if (inputSampledb[channel] > slewedSignal[channel])
                slewedSignal[channel] = attackCoefficient * (slewedSignal[channel] - inputSampledb[channel]) + inputSampledb[channel];
            else
                slewedSignal[channel] = releaseCoefficient * (slewedSignal[channel] - inputSampledb[channel]) + inputSampledb[channel];
            
            if (slewedSignal[channel] > thresholdValue) {
                double targetLevel = thresholdValue + (slewedSignal[channel] - thresholdValue) / ratioValue;
                gainReduction[channel] = slewedSignal[channel] - targetLevel;
                gainReduction[channel] = std::pow(gainReduction[channel], convexityValue);
            } else {
                slewedSignal[channel] = thresholdValue;
                gainReduction[channel] = 0;
            }
            
            slewedSignal[channel] = std::clamp(slewedSignal[channel], -200.0, 1000.0);
            
            double gainReductionDecimal = decibelsToGain(gainReduction[channel]);
            
            if (inertiaCoefficientValue > 0) {
                if (gainReduction[channel] > previousGainReduction[channel]) inertiaVelocity[channel] += inertiaCoefficientValue * gainReductionDecimal * -0.001;
            } else {
                inertiaVelocity[channel] += inertiaCoefficientValue * gainReductionDecimal * -0.001;
            }
            
            inertiaVelocity[channel] *= inertiaDecayCoefficient;
            if (inertiaVelocity[channel] > 100) inertiaVelocity[channel] = 100;
            if (inertiaVelocity[channel] < -100) inertiaVelocity[channel] = -100;
            
            gainReductionDecimal += inertiaVelocity[channel];
            if (gainReductionDecimal > 1000) gainReductionDecimal = 1000;
            if (gainReductionDecimal < -1000) gainReductionDecimal = -1000;
            
            gainReduction[channel] = gainToDecibels(gainReductionDecimal);
            previousGainReduction[channel] = gainReduction[channel];
        }
        
        double maxGainReduction = gainReduction[0];
        if (gainReduction[0] < gainReduction[1]) {
            maxGainReduction = gainReduction[1];
        }
        
        for (int channel = 0; channel < 2 && channel < totalNumInputChannels; channel++) {
            
            gainReduction[channel] = (maxGainReduction * channelLinkValue) + (gainReduction[channel] * (channelLinkValue - 1) * -1);
            
            if (sidechainSelected) {
                inputSampledb[channel] = gainToDecibels(std::abs(inputSample[channel]));
            }
            
            outputSample[channel] = decibelsToGain(inputSampledb[channel] - gainReduction[channel]) * (inputSample[channel] < 0 ? -1.0f : 1.0f);
            
#if PRO_VERSION
            doProOverdrive(outputSample[channel], ceiling, foldback);
#else
            outputSample[channel] /= ceiling;
            outputSample[channel] = std::tanh(outputSample[channel]);
            outputSample[channel] *= ceiling;
#endif
                  
            outputSample[channel] = outputSample[channel] * decibelsToGain(outGainValue);

            if (std::isnan(outputSample[channel])) {
                outputSample[channel] = 0.0f;
            }
            
            channelData[channel][sample] = outputSample[channel];
            
            if (std::abs(inputSample[channel]) > maxValuesForMeters[channel]) maxValuesForMeters[channel] = std::abs(inputSample[channel]);
            if (std::abs(outputSample[channel]) > maxValuesForMeters[channel+2]) maxValuesForMeters[channel+2] = std::abs(outputSample[channel]);
            if (gainReduction[channel] > maxValuesForMeters[channel+4]) maxValuesForMeters[channel+4] = gainReduction[channel];
        }
    }
    
    updateMeters(maxValuesForMeters);
}
