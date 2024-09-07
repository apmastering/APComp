#include <algorithm>
#include <utility>
#include <cstring>

#include "APCommon.h"
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Constants.h"


void APComp::doCompressionDSP(juce::dsp::AudioBlock<float>& block) {
    
    const float inputGainValue = parameters.getRawParameterValue("inGain")->load();
    const float outGainValue = parameters.getRawParameterValue("outGain")->load();
    const float convexityValue = parameters.getRawParameterValue("convexity")->load();
    const float attackValue = linearToExponential(parameters.getRawParameterValue("attack")->load(), Constants::attackMin, Constants::attackMax) / 1000;
    const float releaseValue = linearToExponential(parameters.getRawParameterValue("release")->load(), Constants::releaseMin, Constants::releaseMax) / 1000;
    const float thresholdValue = parameters.getRawParameterValue("threshold")->load();
    const float ratioValue = parameters.getRawParameterValue("ratio")->load();
    const float channelLinkValue = parameters.getRawParameterValue("channelLink")->load() / 100;
    const float feedbackValue = parameters.getRawParameterValue("feedback")->load();
    bool sidechainSelected = parameters.getRawParameterValue("sidechain")->load();
    const float inertiaCoefficientValue = parameters.getRawParameterValue("inertia")->load();
    float inertiaDecayCoefficient = parameters.getRawParameterValue("inertiaDecay")->load();
    
    const double attackCoefficient = std::exp(-1.0 / (oversampledSampleRate * attackValue));
    const double releaseCoefficient = std::exp(-1.0 / (oversampledSampleRate * releaseValue));
    
    float* channelData[4];
    
    for (int i = 0; i < totalNumInputChannels && i < 4; i++) {
        channelData[i] = block.getChannelPointer(i);
    }
    
    if (totalNumInputChannels < 3) {
        sidechainSelected = false;
    }
    
    inertiaDecayCoefficient = 0.99 + (inertiaDecayCoefficient * 0.01);

    float maxValuesForMeters[6] = {0};
    
    for (int sample = 0; sample < block.getNumSamples(); ++sample)
    {
        float inputSample[4];
        double inputSampledb[2];
        inputSampledb[0] = -200.0f;
        inputSampledb[1] = -200.0f;
        
        for (int i = 0; i < 2 && i < totalNumInputChannels; i++) {
            inputSample[i] = channelData[i][sample];
            inputSample[i] *= decibelsToGain(inputGainValue);
            inputSampledb[i] = gainToDecibels(std::abs(inputSample[i]) + std::abs(outputSample[i] * feedbackValue));
            outputSample[i] = 0;
            
            if (std::isnan(inputSampledb[i])) {
                inputSampledb[i] = -200.0f;
            }
            
            if (sidechainSelected) {
                inputSampledb[i] = gainToDecibels(std::abs(channelData[i + 2][sample]));
            }
            
            if (inputSampledb[i] < -200) inputSampledb[i] = -200;
            feedbackClip = false;
            if (inputSampledb[i] > 4) {
                inputSampledb[i] = 4;  // hardcoded 4db feedback path clip. Improve later
                feedbackClip = true;
            }
            
            if (inputSampledb[i] > slewedSignal[i])
                slewedSignal[i] = attackCoefficient * (slewedSignal[i] - inputSampledb[i]) + inputSampledb[i];
            else
                slewedSignal[i] = releaseCoefficient * (slewedSignal[i] - inputSampledb[i]) + inputSampledb[i];
            
            if (slewedSignal[i] > thresholdValue) {
                double targetLevel = thresholdValue + (slewedSignal[i] - thresholdValue) / ratioValue;
                gainReduction[i] = slewedSignal[i] - targetLevel;
                gainReduction[i] = std::pow(gainReduction[i], convexityValue);
            } else {
                slewedSignal[i] = thresholdValue;
                gainReduction[i] = 0;
            }
            
            slewedSignal[i] = std::clamp(slewedSignal[i], -200.0, 1000.0);
            
            double gainReductionDecimal = decibelsToGain(gainReduction[i]);
            
            if (inertiaCoefficientValue > 0) {
                if (gainReduction[i] > previousGainReduction[i]) inertiaVelocity[i] += inertiaCoefficientValue * gainReductionDecimal * -0.001;
            } else {
                inertiaVelocity[i] += inertiaCoefficientValue * gainReductionDecimal * -0.001;
            }
            
            inertiaVelocity[i] *= inertiaDecayCoefficient;
            if (inertiaVelocity[i] > 100) inertiaVelocity[i] = 100;
            if (inertiaVelocity[i] < -100) inertiaVelocity[i] = -100;
            
            gainReductionDecimal += inertiaVelocity[i];
            if (gainReductionDecimal > 1000) gainReductionDecimal = 1000;
            if (gainReductionDecimal < -1000) gainReductionDecimal = -1000;
            
            gainReduction[i] = gainToDecibels(gainReductionDecimal);
            previousGainReduction[i] = gainReduction[i];
        }
        
        double maxGainReduction = gainReduction[0];
        if (gainReduction[0] < gainReduction[1]) {
            maxGainReduction = gainReduction[1];
        }
        
        for (int i = 0; i < 2 && i < totalNumInputChannels; i++) {
            gainReduction[i] = (maxGainReduction * channelLinkValue) + (gainReduction[i] * (channelLinkValue - 1) * -1);
            
            if (sidechainSelected) {
                inputSampledb[i] = gainToDecibels(std::abs(inputSample[i]));
            }
            
            outputSample[i] = decibelsToGain(inputSampledb[i] - gainReduction[i]) * (inputSample[i] < 0 ? -1.0f : 1.0f);
            outputSample[i] = outputSample[i] * decibelsToGain(outGainValue);
            
            if (std::isnan(outputSample[i])) {
                outputSample[i] = 0.0f;
            }
            
            if (i < totalNumOutputChannels) channelData[i][sample] = outputSample[i];
            
            if (std::abs(inputSample[i]) > maxValuesForMeters[i]) maxValuesForMeters[i] = std::abs(inputSample[i]);
            if (std::abs(outputSample[i]) > maxValuesForMeters[i+2]) maxValuesForMeters[i+2] = std::abs(outputSample[i]);
            if (gainReduction[i] > maxValuesForMeters[i+4]) maxValuesForMeters[i+4] = gainReduction[i];
        }
    }
    
                
    for (int i = 0; i < 6; i++) {
        if (maxValuesForMeters[i] > 100) maxValuesForMeters[i] = 100;
        
        if (maxValuesForMeters[i] > meterValuesBackPointer[i]) {
            meterValuesBackPointer[i] = maxValuesForMeters[i];
        } else {
            meterValuesBackPointer[i] = meterValuesBackPointer[i] * meterDecayCoefficient;
        }
    }
    
    meterValuesBackPointer = std::exchange(meterValuesFrontPointer, meterValuesBackPointer);

    std::memcpy(meterValuesBackPointer, meterValuesFrontPointer, sizeof(meterValues));
}
