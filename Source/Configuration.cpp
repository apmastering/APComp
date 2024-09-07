#include "APCommon.h"
#include "PluginProcessor.h"
#include "PluginEditor.h"


juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new APComp(); }
const juce::String APComp::getName() const { return JucePlugin_Name; }
bool APComp::acceptsMidi() const { return false; }
bool APComp::producesMidi() const { return false; }
bool APComp::isMidiEffect() const { return false; }
double APComp::getTailLengthSeconds() const { return 0.0; }
int APComp::getNumPrograms() { return 1; }
int APComp::getCurrentProgram() { return 0; }
void APComp::setCurrentProgram (int index) { }
const juce::String APComp::getProgramName (int index) { return {}; }
void APComp::changeProgramName (int index, const juce::String& newName) {}
bool APComp::hasEditor() const { return true; }
void APComp::releaseResources() {}
bool APComp::isBusesLayoutSupported (const BusesLayout& layouts) const { return true; }

juce::AudioProcessorEditor* APComp::createEditor() { return new GUI (*this); }

void APComp::getStateInformation (juce::MemoryBlock& destData) {
    
    std::unique_ptr<juce::XmlElement> xml (apvts.state.createXml());
    copyXmlToBinary (*xml, destData);
}

void APComp::setStateInformation (const void* data, int sizeInBytes) {
    
        std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
        if (xml != nullptr)
        {
            if (xml->hasTagName (apvts.state.getType()))
            {
                apvts.state = juce::ValueTree::fromXml (*xml);
            }
        }
}
