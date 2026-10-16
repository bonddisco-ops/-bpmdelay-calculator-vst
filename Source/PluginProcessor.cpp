#include "PluginProcessor.h"
#include "PluginEditor.h"

DelayCalcProcessor::DelayCalcProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
}

bool DelayCalcProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return out == layouts.getMainInputChannelSet();
}

void DelayCalcProcessor::processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&)
{
    // Audio passes through untouched. We only read the tempo.
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            if (auto bpm = pos->getBpm())
                hostBpm.store (*bpm);
}

juce::AudioProcessorEditor* DelayCalcProcessor::createEditor()
{
    return new DelayCalcEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DelayCalcProcessor();
}
