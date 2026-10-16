#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <atomic>

// Writes a line to Documents/DelayCalculator-log.txt.
// Used only on the UI thread, to see the last action before a crash.
inline void dlog (const juce::String& message)
{
    auto file = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                    .getChildFile ("DelayCalculator-log.txt");
    if (file.getSize() > 200000)
        file.deleteFile();
    file.appendText (juce::Time::getCurrentTime().toString (true, true, true, true) + "  " + message + "\n");
}

class DelayCalcProcessor : public juce::AudioProcessor
{
public:
    DelayCalcProcessor();
    ~DelayCalcProcessor() override = default;

    void prepareToPlay (double, int) override {}
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Delay Calculator"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override {}
    void setStateInformation (const void*, int) override {}

    // Tempo reported by the DAW (0 if unknown)
    std::atomic<double> hostBpm { 0.0 };

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DelayCalcProcessor)
};
