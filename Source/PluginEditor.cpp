#include "PluginEditor.h"

namespace
{
    const juce::Colour kBg    (0xff171a2e);
    const juce::Colour kPanel (0xff20243d);
    const juce::Colour kInk   (0xffeceaf7);
    const juce::Colour kMute  (0xff9a9dbd);
    const juce::Colour kLine  (0xff363b5e);
    const juce::Colour kAcc   (0xffff5c93);

    const std::vector<double>& notesFor (int mode)
    {
        static const std::vector<double> delayNotes { 1, 2, 4, 8, 16, 32, 64 };
        static const std::vector<double> preNotes   { 4, 8, 16, 32, 64, 128 };
        static const std::vector<double> decayNotes { 0.25, 0.5, 1, 2, 4, 8 };
        return mode == 0 ? delayNotes : (mode == 1 ? preNotes : decayNotes);
    }

    juce::String noteLabel (double n)
    {
        if (n >= 1.0)
            return "1/" + juce::String ((int) n);
        return juce::String ((int) (1.0 / n)) + " bars";
    }

    void styleButton (juce::TextButton& b)
    {
        b.setColour (juce::TextButton::buttonColourId, kPanel);
        b.setColour (juce::TextButton::buttonOnColourId, kAcc);
        b.setColour (juce::TextButton::textColourOffId, kInk);
        b.setColour (juce::TextButton::textColourOnId, kBg);
    }
}

DelayCalcEditor::DelayCalcEditor (DelayCalcProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    setSize (560, 520);

    bpmBox.setText ("128", false);
    bpmBox.setInputRestrictions (6, "0123456789.");
    bpmBox.setFont (juce::Font (juce::FontOptions (44.0f, juce::Font::bold)));
    bpmBox.setJustification (juce::Justification::centredLeft);
    bpmBox.setColour (juce::TextEditor::backgroundColourId, kPanel);
    bpmBox.setColour (juce::TextEditor::textColourId, kInk);
    bpmBox.setColour (juce::TextEditor::outlineColourId, kLine);
    bpmBox.setColour (juce::TextEditor::focusedOutlineColourId, kAcc);
    bpmBox.onTextChange = [this]
    {
        if (bpmBox.hasKeyboardFocus (true))
            followHost.setToggleState (false, juce::dontSendNotification);
        repaint();
    };
    addAndMakeVisible (bpmBox);

    followHost.setToggleState (true, juce::dontSendNotification);
    followHost.setColour (juce::ToggleButton::textColourId, kInk);
    followHost.setColour (juce::ToggleButton::tickColourId, kAcc);
    addAndMakeVisible (followHost);

    halfBtn.onClick = [this] { followHost.setToggleState (false, juce::dontSendNotification); setBpm (currentBpm() * 0.5); };
    dblBtn.onClick  = [this] { followHost.setToggleState (false, juce::dontSendNotification); setBpm (currentBpm() * 2.0); };
    unitBtn.onClick = [this]
    {
        useHz = ! useHz;
        unitBtn.setButtonText (useHz ? "Hz" : "ms");
        repaint();
    };
    for (auto* b : { &halfBtn, &dblBtn, &unitBtn })
    {
        styleButton (*b);
        addAndMakeVisible (*b);
    }

    const char* names[3] = { "Delay", "Pre-delay", "Decay" };
    for (int i = 0; i < 3; ++i)
    {
        auto& b = modeBtns[i];
        b.setButtonText (names[i]);
        b.setClickingTogglesState (true);
        b.setRadioGroupId (1);
        b.onClick = [this, i] { mode = i; repaint(); };
        styleButton (b);
        addAndMakeVisible (b);
    }
    modeBtns[0].setToggleState (true, juce::dontSendNotification);

    startTimerHz (10);
}

double DelayCalcEditor::currentBpm() const
{
    const auto v = bpmBox.getText().getDoubleValue();
    return v > 0.0 ? v : 0.0;
}

void DelayCalcEditor::setBpm (double bpm)
{
    if (bpm > 0.0)
        bpmBox.setText (juce::String (bpm, 2), false);
    repaint();
}

void DelayCalcEditor::timerCallback()
{
    if (followHost.getToggleState())
    {
        const auto host = proc.hostBpm.load();
        if (host > 0.0 && std::abs (host - currentBpm()) > 0.005)
            setBpm (host);
    }

    if (flashTicks > 0 && --flashTicks == 0)
        repaint();
}

void DelayCalcEditor::resized()
{
    bpmBox.setBounds (16, 44, 170, 60);
    followHost.setBounds (200, 46, 220, 24);
    halfBtn.setBounds (200, 76, 48, 28);
    dblBtn.setBounds (254, 76, 48, 28);
    unitBtn.setBounds (480, 76, 64, 28);

    int x = 16;
    for (auto& b : modeBtns)
    {
        b.setBounds (x, 114, 100, 28);
        x += 108;
    }
}

void DelayCalcEditor::paint (juce::Graphics& g)
{
    g.fillAll (kBg);

    g.setColour (kInk);
    g.setFont (juce::Font (juce::FontOptions (20.0f, juce::Font::bold)));
    g.drawText ("Delay Calculator", 16, 10, 300, 28, juce::Justification::centredLeft);

    g.setColour (kMute);
    g.setFont (14.0f);
    g.drawText ("BPM", 190, 44, 0, 0, juce::Justification::centredLeft);

    const int left = 16, labelW = 110, tableTop = 152, headerH = 24, rowH = 42;
    const int colW = (getWidth() - 32 - labelW) / 3;

    const char* heads[3] = { "Straight", "Dotted", "Triplet" };
    g.setColour (kMute);
    g.setFont (14.0f);
    g.drawText ("Note", left + 8, tableTop, labelW, headerH, juce::Justification::centredLeft);
    for (int c = 0; c < 3; ++c)
        g.drawText (heads[c], left + labelW + c * colW, tableTop, colW - 12, headerH, juce::Justification::centredRight);

    cells.clear();
    const double bpm = currentBpm();
    const auto& notes = notesFor (mode);

    if (bpm <= 0.0)
    {
        g.setColour (kMute);
        g.drawText ("Enter a tempo", left, tableTop + headerH, getWidth() - 32, rowH, juce::Justification::centred);
        return;
    }

    const double factors[3] = { 1.0, 1.5, 2.0 / 3.0 };
    const double beatMs = 60000.0 / bpm;
    int y = tableTop + headerH;

    for (double n : notes)
    {
        g.setColour (kPanel);
        g.fillRect (left, y, getWidth() - 32, rowH - 2);

        const bool quarter = (mode == 0 && n == 4.0);
        g.setColour (quarter ? kAcc : kInk);
        g.setFont (juce::Font (juce::FontOptions (17.0f, juce::Font::bold)));
        g.drawText (noteLabel (n), left + 8, y, labelW, rowH - 2, juce::Justification::centredLeft);

        for (int c = 0; c < 3; ++c)
        {
            const double ms = beatMs * 4.0 / n * factors[c];
            const juce::String num = useHz ? juce::String (1000.0 / ms, 3)
                                           : juce::String (ms, ms >= 100.0 ? 1 : 2);
            juce::Rectangle<int> r (left + labelW + c * colW, y, colW, rowH - 2);

            g.setColour (kInk);
            g.setFont (juce::Font (juce::FontOptions (16.0f)));
            g.drawText (num + (useHz ? " Hz" : " ms"), r.withTrimmedRight (12), juce::Justification::centredRight);

            cells.push_back ({ r, num });
        }
        y += rowH;
    }

    if (flashTicks > 0)
    {
        g.setColour (kAcc);
        g.setFont (14.0f);
        g.drawText (flashText, left, getHeight() - 32, getWidth() - 32, 24, juce::Justification::centredLeft);
    }
    else
    {
        g.setColour (kMute);
        g.setFont (13.0f);
        g.drawText ("Click a value to copy it", left, getHeight() - 32, getWidth() - 32, 24, juce::Justification::centredLeft);
    }
}

void DelayCalcEditor::mouseDown (const juce::MouseEvent& e)
{
    for (auto& c : cells)
    {
        if (c.area.contains (e.getPosition()))
        {
            juce::SystemClipboard::copyTextToClipboard (c.copyText);
            flashText = "Copied: " + c.copyText;
            flashTicks = 15;
            repaint();
            return;
        }
    }
}
