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

    const float attackValue  = linearToExponential(getFloatKnobValue(ParameterNames::attack),
                                                  Constants::attackMin,
                                                  Constants::attackMax) / 1000;
    const float releaseValue = linearToExponential(getFloatKnobValue(ParameterNames::release),
                                                  Constants::releaseMin,
                                                  Constants::releaseMax) / 1000;
    const float ratioValue   = linearToExponential(getFloatKnobValue(ParameterNames::ratio),
                                                   Constants::ratioMin,
                                                   Constants::ratioMax);
    const float inputGainValue          = getFloatKnobValue(ParameterNames::inGain);
    const float outGainValue            = getFloatKnobValue(ParameterNames::outGain);
    const float convexityValue          = getFloatKnobValue(ParameterNames::convexity);
    const float thresholdValue          = getFloatKnobValue(ParameterNames::threshold);
    const float channelLinkValue        = getFloatKnobValue(ParameterNames::channelLink) / 100;
    const float feedbackValue           = getFloatKnobValue(ParameterNames::feedback);
    const float inertiaCoefficientValue = getFloatKnobValue(ParameterNames::inertia);
    const float ceiling                 = getFloatKnobValue(ParameterNames::ceiling);
    float inertiaDecayCoefficient       = getFloatKnobValue(ParameterNames::inertiaDecay);
    bool  sidechainSelected             = getBoolKnobValue(ParameterNames::sidechain);
    const double attackCoefficient  = std::exp(-1.0 / (sampleRate * attackValue));
    const double releaseCoefficient = std::exp(-1.0 / (sampleRate * releaseValue));
    float maxValuesForMeters[meterCount] = {0};
    const int mainChannels = static_cast<int>(mainBlock.getNumChannels());
    const int sidechainChannels = static_cast<int>(sidechainBlock.getNumChannels());
    const int numSamples = static_cast<int>(mainBlock.getNumSamples());
    const int sidechainSamples = static_cast<int>(sidechainBlock.getNumSamples());

    if (mainChannels == 0 || numSamples == 0) return;
    if (ratioValue == 0) return;

    float* channelData[2] = {nullptr, nullptr};
    float* sidechainChannelData[2] = {nullptr, nullptr};

    for (int i = 0; i < std::min(mainChannels, 2); i++) {
        channelData[i] = mainBlock.getChannelPointer(i);
    }

    if (sidechainSelected && sidechainChannels > 0 && sidechainSamples > 0) {
        for (int i = 0; i < std::min(sidechainChannels, 2); i++) {
            sidechainChannelData[i] = sidechainBlock.getChannelPointer(i);
        }
    }

    inertiaDecayCoefficient = 0.99 + (inertiaDecayCoefficient * 0.01);

    for (int sample = 0; sample < numSamples; ++sample) {

        float inputSample[4];
        double inputSampledb[2];
        inputSampledb[0] = -200.0f;
        inputSampledb[1] = -200.0f;

        const int channelsToProcess = std::min(mainChannels, 2);

        for (int channel = 0; channel < channelsToProcess; channel++) {
            inputSample[channel] = channelData[channel][sample];
            inputSample[channel] *= decibelsToGain(inputGainValue);
            inputSampledb[channel] = gainToDecibels(std::abs(inputSample[channel]) + std::abs(outputSample[channel] * feedbackValue));
            outputSample[channel] = 0;

            if (std::isnan(inputSampledb[channel])) inputSampledb[channel] = -200.0f;

            if (sidechainSelected && sidechainChannelData[0] != nullptr) {
                int sidechainSampleIndex = sample;
                if (oversamplingFactor > 0) {
                    sidechainSampleIndex = sample / static_cast<int>(std::pow(2, oversamplingFactor));
                }

                if (sidechainSampleIndex < sidechainSamples) {
                    float sidechainSample = 0.0f;
                    if (sidechainChannels == 1) {
                        sidechainSample = sidechainChannelData[0][sidechainSampleIndex];
                    }
                    else if (channel < sidechainChannels && sidechainChannelData[channel] != nullptr) {
                        sidechainSample = sidechainChannelData[channel][sidechainSampleIndex];
                    }
                    else {
                        sidechainSample = sidechainChannelData[0][sidechainSampleIndex];
                    }
                    inputSampledb[channel] = gainToDecibels(std::abs(sidechainSample));
                } else {
                    inputSampledb[channel] = -200.0f;
                }
            }

            if (inputSampledb[channel] < -200) inputSampledb[channel] = -200;

            if (inputSampledb[channel] > 4) {
                inputSampledb[channel] = 4.0;
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

            if (inertiaCoefficientValue > 0)
                if (gainReduction[channel] > previousGainReduction[channel]) inertiaVelocity[channel] += inertiaCoefficientValue * gainReductionDecimal * -0.001;
            else inertiaVelocity[channel] += inertiaCoefficientValue * gainReductionDecimal * -0.001;

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
        if (channelsToProcess > 1 && gainReduction[0] < gainReduction[1]) {
            maxGainReduction = gainReduction[1];
        }

        for (int channel = 0; channel < channelsToProcess; channel++) {

            if (channelsToProcess > 1) {
                gainReduction[channel] = (maxGainReduction * channelLinkValue) + (gainReduction[channel] * (channelLinkValue - 1) * -1);
            }

            if (sidechainSelected) inputSampledb[channel] = gainToDecibels(std::abs(inputSample[channel]));

            outputSample[channel] = decibelsToGain(inputSampledb[channel] - gainReduction[channel]) * (inputSample[channel] < 0 ? -1.0f : 1.0f);
            outputSample[channel] /= ceiling;
            outputSample[channel] = std::tanh(outputSample[channel]);
            outputSample[channel] *= ceiling;
            outputSample[channel] = outputSample[channel] * decibelsToGain(outGainValue);

            if (std::isnan(outputSample[channel])) outputSample[channel] = 0.0f;

            channelData[channel][sample] = outputSample[channel];

            if (std::abs(inputSample[channel]) > maxValuesForMeters[channel]) maxValuesForMeters[channel] = std::abs(inputSample[channel]);
            if (std::abs(outputSample[channel]) > maxValuesForMeters[channel+2]) maxValuesForMeters[channel+2] = std::abs(outputSample[channel]);
            if (gainReduction[channel] > maxValuesForMeters[channel+4]) maxValuesForMeters[channel+4] = gainReduction[channel];
        }
    }

    updateMeters(maxValuesForMeters);
}