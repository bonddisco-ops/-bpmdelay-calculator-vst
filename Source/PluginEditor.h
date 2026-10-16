#pragma once
#include "PluginProcessor.h"
#include <functional>
#include <vector>

// Number field with - / + buttons (hold a button to repeat).
class NumberBox : public juce::Component
{
public:
    NumberBox (double minValue, double maxValue, double stepSize, int numDecimals);

    std::function<void()> onUserChange;   // called when the user types or presses - / +

    double getValue() const noexcept { return value; }
    void setValue (double newValue);      // does not call onUserChange
    void setStep (double newStep, int newDecimals);
    void setFontHeight (float height);
    void resized() override;

private:
    void nudge (int direction);
    void showValue();

    double minV, maxV, step;
    int decimals;
    double value;
    juce::TextEditor text;
    juce::TextButton minus { "-" }, plus { "+" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NumberBox)
};

class DelayCalcEditor : public juce::AudioProcessorEditor,
                        private juce::Timer
{
public:
    explicit DelayCalcEditor (DelayCalcProcessor&);
    ~DelayCalcEditor() override;

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
    double bpm() const { return bpmBox.getValue(); }
    double beatMs() const { return bpm() > 0.0 ? 60000.0 / bpm() : 0.0; }
    double totalMs() const;
    void setMode (int newMode);
    void updatePreDelay();
    void stopFollowingHost();
    void paintTable (juce::Graphics&);
    void paintSum (juce::Graphics&);
    void flash (const juce::String& message);
    void prepareButton (juce::Button&);
    void prepareCombo (juce::ComboBox&);

    DelayCalcProcessor& proc;
    juce::LookAndFeel_V4 laf;   // declared first so it outlives every child component

    NumberBox bpmBox { 20.0, 400.0, 1.0, 2 };
    juce::ToggleButton followHost { "Follow host tempo" };
    juce::TextButton halfBtn { "/2" }, dblBtn { "x2" }, unitBtn { "ms" };
    juce::TextButton modeBtns[4];

    // "Sum" mode controls
    NumberBox totalBox { 0.0, 100000.0, 0.25, 2 };
    juce::ComboBox totalUnit, preNote, preVariant;
    NumberBox preMsBox { 0.0, 100000.0, 1.0, 2 };
    bool preCustom = false;

    int mode = 0;          // 0 delay, 1 pre-delay, 2 decay, 3 sum
    bool useHz = false;
    std::vector<Cell> cells;
    juce::String flashText;
    int flashTicks = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DelayCalcEditor)
};
