#pragma once
#include "PluginProcessor.h"

class DelayCalcEditor : public juce::AudioProcessorEditor,
                        private juce::Timer
{
public:
    explicit DelayCalcEditor (DelayCalcProcessor&);
    ~DelayCalcEditor() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;

private:
    struct Cell
    {
        juce::Rectangle<int> area;
        juce::String copyText;
    };

    void timerCallback() override;
    double currentBpm() const;
    void setBpm (double bpm);

    DelayCalcProcessor& proc;

    juce::TextEditor bpmBox;
    juce::ToggleButton followHost { "Follow host tempo" };
    juce::TextButton halfBtn { "/2" }, dblBtn { "x2" }, unitBtn { "ms" };
    juce::TextButton modeBtns[3];

    int mode = 0;          // 0 delay, 1 pre-delay, 2 decay
    bool useHz = false;
    std::vector<Cell> cells;
    juce::String flashText;
    int flashTicks = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DelayCalcEditor)
};
