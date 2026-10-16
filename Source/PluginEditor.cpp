#include "PluginEditor.h"

namespace
{
    const juce::Colour kBg    (0xff171a2e);
    const juce::Colour kPanel (0xff20243d);
    const juce::Colour kInk   (0xffeceaf7);
    const juce::Colour kMute  (0xff9a9dbd);
    const juce::Colour kLine  (0xff363b5e);
    const juce::Colour kAcc   (0xffff5c93);
    const juce::Colour kDec   (0xff8d9bff);

    const int kTotalDivs[] = { 1, 2, 4, 8, 16, 32 };      // total length units, id 7 = ms
    const int kPreDivs[]   = { 4, 8, 16, 32, 64, 128 };
    const double kVariants[] = { 1.0, 1.5, 2.0 / 3.0 };   // straight, dotted, triplet

    juce::Font font (float height, bool bold = false)
    {
        return juce::Font (juce::FontOptions (height, bold ? juce::Font::bold : juce::Font::plain));
    }

    juce::String trimNumber (double v, int decimals)
    {
        auto s = juce::String (v, decimals);
        if (s.containsChar ('.'))
            s = s.trimCharactersAtEnd ("0").trimCharactersAtEnd (".");
        return s;
    }

    juce::String fmtMs (double ms)
    {
        return juce::String (ms, ms >= 100.0 ? 1 : 2);
    }

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
}

//==============================================================================
NumberBox::NumberBox (double minValue, double maxValue, double stepSize, int numDecimals)
    : minV (minValue), maxV (maxValue), step (stepSize), decimals (numDecimals), value (minValue)
{
    text.setInputRestrictions (9, "0123456789.");
    text.setJustification (juce::Justification::centredLeft);
    text.setSelectAllWhenFocused (true);
    text.onTextChange = [this]
    {
        const auto t = text.getText();
        if (t.isEmpty())
            return;
        const double v = t.getDoubleValue();
        if (v < minV || v > maxV)
            return;
        value = v;
        if (onUserChange)
            onUserChange();
    };
    text.onReturnKey = [this] { showValue(); text.giveAwayKeyboardFocus(); };
    text.onFocusLost = [this] { showValue(); };
    addAndMakeVisible (text);

    for (auto* b : { &minus, &plus })
    {
        b->setWantsKeyboardFocus (false);
        b->setMouseClickGrabsKeyboardFocus (false);
        b->setRepeatSpeed (400, 70);
        addAndMakeVisible (*b);
    }
    minus.onClick = [this] { nudge (-1); };
    plus.onClick  = [this] { nudge (+1); };

    showValue();
}

void NumberBox::setValue (double newValue)
{
    value = juce::jlimit (minV, maxV, newValue);
    showValue();
}

void NumberBox::setStep (double newStep, int newDecimals)
{
    step = newStep;
    decimals = newDecimals;
    showValue();
}

void NumberBox::setFontHeight (float height)
{
    text.setFont (font (height, true));
    text.applyFontToAllText (font (height, true));
}

void NumberBox::nudge (int direction)
{
    value = juce::jlimit (minV, maxV, value + direction * step);
    showValue();
    if (onUserChange)
        onUserChange();
}

void NumberBox::showValue()
{
    text.setText (trimNumber (value, decimals), false);
}

void NumberBox::resized()
{
    auto r = getLocalBounds();
    auto buttons = r.removeFromRight (juce::jmin (30, r.getWidth() / 3));
    plus.setBounds (buttons.removeFromTop (buttons.getHeight() / 2));
    minus.setBounds (buttons);
    text.setBounds (r.withTrimmedRight (2));
}

//==============================================================================
void DelayCalcEditor::prepareButton (juce::Button& b)
{
    b.setWantsKeyboardFocus (false);
    b.setMouseClickGrabsKeyboardFocus (false);
    addAndMakeVisible (b);
}

void DelayCalcEditor::prepareCombo (juce::ComboBox& c)
{
    c.setWantsKeyboardFocus (false);
    c.setMouseClickGrabsKeyboardFocus (false);
    addChildComponent (c);
}

DelayCalcEditor::DelayCalcEditor (DelayCalcProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    dlog ("v1.1 editor opened");

    laf.setColour (juce::ResizableWindow::backgroundColourId, kBg);
    laf.setColour (juce::TextEditor::backgroundColourId, kPanel);
    laf.setColour (juce::TextEditor::textColourId, kInk);
    laf.setColour (juce::TextEditor::outlineColourId, kLine);
    laf.setColour (juce::TextEditor::focusedOutlineColourId, kAcc);
    laf.setColour (juce::TextEditor::highlightColourId, kAcc.withAlpha (0.4f));
    laf.setColour (juce::CaretComponent::caretColourId, kAcc);
    laf.setColour (juce::TextButton::buttonColourId, kPanel);
    laf.setColour (juce::TextButton::buttonOnColourId, kAcc);
    laf.setColour (juce::TextButton::textColourOffId, kInk);
    laf.setColour (juce::TextButton::textColourOnId, kBg);
    laf.setColour (juce::ComboBox::outlineColourId, kLine);
    laf.setColour (juce::ToggleButton::textColourId, kInk);
    laf.setColour (juce::ToggleButton::tickColourId, kAcc);
    laf.setColour (juce::ToggleButton::tickDisabledColourId, kMute);
    laf.setColour (juce::ComboBox::backgroundColourId, kPanel);
    laf.setColour (juce::ComboBox::textColourId, kInk);
    laf.setColour (juce::ComboBox::arrowColourId, kAcc);
    laf.setColour (juce::PopupMenu::backgroundColourId, kPanel);
    laf.setColour (juce::PopupMenu::textColourId, kInk);
    laf.setColour (juce::PopupMenu::highlightedBackgroundColourId, kAcc);
    laf.setColour (juce::PopupMenu::highlightedTextColourId, kBg);
    setLookAndFeel (&laf);

    // Tempo
    bpmBox.setValue (128.0);
    bpmBox.setFontHeight (34.0f);
    bpmBox.onUserChange = [this]
    {
        dlog ("bpm edited");
        stopFollowingHost();
        updatePreDelay();
        repaint();
    };
    addAndMakeVisible (bpmBox);

    followHost.setToggleState (true, juce::dontSendNotification);
    followHost.onClick = [this] { dlog ("follow host clicked"); };
    prepareButton (followHost);

    halfBtn.onClick = [this]
    {
        dlog ("/2 clicked");
        stopFollowingHost();
        bpmBox.setValue (bpm() * 0.5);
        updatePreDelay();
        repaint();
    };
    dblBtn.onClick = [this]
    {
        dlog ("x2 clicked");
        stopFollowingHost();
        bpmBox.setValue (bpm() * 2.0);
        updatePreDelay();
        repaint();
    };
    unitBtn.onClick = [this]
    {
        dlog ("unit clicked");
        useHz = ! useHz;
        unitBtn.setButtonText (useHz ? "Hz" : "ms");
        repaint();
    };
    prepareButton (halfBtn);
    prepareButton (dblBtn);
    prepareButton (unitBtn);

    const char* names[4] = { "Delay", "Pre-delay", "Decay", "Sum" };
    for (int i = 0; i < 4; ++i)
    {
        auto& b = modeBtns[i];
        b.setButtonText (names[i]);
        b.setClickingTogglesState (true);
        b.setRadioGroupId (1);
        b.onClick = [this, i] { dlog ("mode " + juce::String (i)); setMode (i); };
        prepareButton (b);
    }
    modeBtns[0].setToggleState (true, juce::dontSendNotification);

    // Sum mode
    totalBox.setValue (4.0);
    totalBox.setFontHeight (18.0f);
    totalBox.onUserChange = [this] { repaint(); };
    addChildComponent (totalBox);

    totalUnit.addItemList ({ "x 1/1 (bar)", "x 1/2", "x 1/4", "x 1/8", "x 1/16", "x 1/32", "ms" }, 1);
    totalUnit.setSelectedId (1, juce::dontSendNotification);
    totalUnit.onChange = [this]
    {
        dlog ("total unit changed");
        if (totalUnit.getSelectedId() == 7)
            totalBox.setStep (10.0, 1);
        else
            totalBox.setStep (0.25, 2);
        repaint();
    };
    prepareCombo (totalUnit);

    preNote.addItemList ({ "1/4", "1/8", "1/16", "1/32", "1/64", "1/128" }, 1);
    preNote.setSelectedId (4, juce::dontSendNotification);
    preVariant.addItemList ({ "Straight", "Dotted", "Triplet" }, 1);
    preVariant.setSelectedId (1, juce::dontSendNotification);
    for (auto* c : { &preNote, &preVariant })
    {
        c->onChange = [this]
        {
            dlog ("pre-delay note changed");
            preCustom = false;
            updatePreDelay();
            repaint();
        };
        prepareCombo (*c);
    }

    preMsBox.setFontHeight (18.0f);
    preMsBox.onUserChange = [this] { preCustom = true; repaint(); };
    addChildComponent (preMsBox);

    setSize (600, 540);
    updatePreDelay();
    startTimerHz (10);
}

DelayCalcEditor::~DelayCalcEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void DelayCalcEditor::stopFollowingHost()
{
    followHost.setToggleState (false, juce::dontSendNotification);
}

void DelayCalcEditor::setMode (int newMode)
{
    mode = newMode;
    const bool sum = (mode == 3);
    for (juce::Component* c : { (juce::Component*) &totalBox, (juce::Component*) &totalUnit,
                                (juce::Component*) &preNote, (juce::Component*) &preVariant,
                                (juce::Component*) &preMsBox })
        c->setVisible (sum);
    unitBtn.setVisible (! sum);
    updatePreDelay();
    repaint();
}

void DelayCalcEditor::updatePreDelay()
{
    if (preCustom || beatMs() <= 0.0)
        return;
    const int noteIdx = juce::jlimit (0, 5, preNote.getSelectedId() - 1);
    const int varIdx  = juce::jlimit (0, 2, preVariant.getSelectedId() - 1);
    preMsBox.setValue (beatMs() * 4.0 / kPreDivs[noteIdx] * kVariants[varIdx]);
}

double DelayCalcEditor::totalMs() const
{
    const double n = totalBox.getValue();
    const int id = totalUnit.getSelectedId();
    if (id == 7)
        return n;
    const int idx = juce::jlimit (0, 5, id - 1);
    return n * beatMs() * 4.0 / kTotalDivs[idx];
}

void DelayCalcEditor::flash (const juce::String& message)
{
    flashText = message;
    flashTicks = 15;
    repaint();
}

void DelayCalcEditor::timerCallback()
{
    if (followHost.getToggleState())
    {
        const auto host = proc.hostBpm.load();
        if (host > 0.0 && std::abs (host - bpm()) > 0.005)
        {
            bpmBox.setValue (host);
            updatePreDelay();
            repaint();
        }
    }

    if (flashTicks > 0 && --flashTicks == 0)
        repaint();
}

void DelayCalcEditor::resized()
{
    bpmBox.setBounds (16, 46, 180, 52);
    followHost.setBounds (256, 46, 220, 24);
    halfBtn.setBounds (256, 74, 48, 26);
    dblBtn.setBounds (310, 74, 48, 26);
    unitBtn.setBounds (getWidth() - 80, 74, 64, 26);

    int x = 16;
    for (auto& b : modeBtns)
    {
        b.setBounds (x, 114, 132, 30);
        x += 140;
    }

    totalBox.setBounds (150, 166, 130, 34);
    totalUnit.setBounds (290, 166, 140, 34);
    preNote.setBounds (150, 214, 80, 34);
    preVariant.setBounds (238, 214, 110, 34);
    preMsBox.setBounds (356, 214, 130, 34);
}

void DelayCalcEditor::paint (juce::Graphics& g)
{
    g.fillAll (kBg);

    g.setColour (kInk);
    g.setFont (font (20.0f, true));
    g.drawText ("Delay Calculator", 16, 10, 300, 28, juce::Justification::centredLeft);

    g.setColour (kMute);
    g.setFont (font (15.0f, true));
    g.drawText ("BPM", 204, 46, 44, 52, juce::Justification::centredLeft);

    cells.clear();

    if (bpm() <= 0.0)
    {
        g.drawText ("Enter a tempo", 16, 170, getWidth() - 32, 40, juce::Justification::centred);
    }
    else if (mode == 3)
    {
        paintSum (g);
    }
    else
    {
        paintTable (g);
    }

    g.setFont (font (13.0f));
    if (flashTicks > 0)
    {
        g.setColour (kAcc);
        g.drawText (flashText, 16, getHeight() - 32, getWidth() - 32, 24, juce::Justification::centredLeft);
    }
    else
    {
        g.setColour (kMute);
        g.drawText ("Click a value to copy it", 16, getHeight() - 32, getWidth() - 32, 24, juce::Justification::centredLeft);
    }
}

void DelayCalcEditor::paintTable (juce::Graphics& g)
{
    const int left = 16, labelW = 110, tableTop = 160, headerH = 24, rowH = 42;
    const int colW = (getWidth() - 32 - labelW) / 3;

    const char* heads[3] = { "Straight", "Dotted", "Triplet" };
    g.setColour (kMute);
    g.setFont (font (14.0f));
    g.drawText ("Note", left + 8, tableTop, labelW, headerH, juce::Justification::centredLeft);
    for (int c = 0; c < 3; ++c)
        g.drawText (heads[c], left + labelW + c * colW, tableTop, colW - 12, headerH, juce::Justification::centredRight);

    int y = tableTop + headerH;
    for (double n : notesFor (mode))
    {
        g.setColour (kPanel);
        g.fillRect (left, y, getWidth() - 32, rowH - 2);

        const bool quarter = (mode == 0 && n == 4.0);
        g.setColour (quarter ? kAcc : kInk);
        g.setFont (font (17.0f, true));
        g.drawText (noteLabel (n), left + 8, y, labelW, rowH - 2, juce::Justification::centredLeft);

        for (int c = 0; c < 3; ++c)
        {
            const double ms = beatMs() * 4.0 / n * kVariants[c];
            const juce::String num = useHz ? juce::String (1000.0 / ms, 3) : fmtMs (ms);
            juce::Rectangle<int> r (left + labelW + c * colW, y, colW, rowH - 2);

            g.setColour (kInk);
            g.setFont (font (16.0f));
            g.drawText (num + (useHz ? " Hz" : " ms"), r.withTrimmedRight (12), juce::Justification::centredRight);

            cells.push_back ({ r, num });
        }
        y += rowH;
    }
}

void DelayCalcEditor::paintSum (juce::Graphics& g)
{
    g.setColour (kInk);
    g.setFont (font (16.0f, true));
    g.drawText ("Total length", 16, 166, 130, 34, juce::Justification::centredLeft);
    g.drawText ("Pre-delay", 16, 214, 130, 34, juce::Justification::centredLeft);
    g.setColour (kMute);
    g.setFont (font (15.0f));
    g.drawText ("ms", 494, 214, 40, 34, juce::Justification::centredLeft);

    const double total = totalMs();
    const double pre = preMsBox.getValue();
    const double dec = total - pre;

    // Bar: pink = pre-delay, blue = decay
    juce::Rectangle<float> bar (16.0f, 266.0f, (float) getWidth() - 32.0f, 14.0f);
    g.setColour (kLine);
    g.fillRoundedRectangle (bar, 7.0f);
    if (total > 0.0)
    {
        const float preW = bar.getWidth() * (float) juce::jlimit (0.0, 1.0, pre / total);
        g.saveState();
        g.reduceClipRegion (bar.toNearestInt());
        g.setColour (kAcc);
        g.fillRect (bar.withWidth (preW));
        if (dec > 0.0)
        {
            g.setColour (kDec);
            g.fillRect (bar.withTrimmedLeft (preW));
        }
        g.restoreState();
    }

    // Cards
    const int gap = 8, cardY = 296, cardH = 72;
    const int cardW = (getWidth() - 32 - 2 * gap) / 3;
    struct CardInfo { const char* title; double ms; juce::Colour colour; };
    const CardInfo cards[3] = { { "Pre-delay",    pre,   kAcc },
                                { "Decay (rest)", dec,   kDec },
                                { "Total",        total, kMute } };

    for (int i = 0; i < 3; ++i)
    {
        juce::Rectangle<int> r (16 + i * (cardW + gap), cardY, cardW, cardH);
        g.setColour (kPanel);
        g.fillRoundedRectangle (r.toFloat(), 10.0f);
        g.setColour (kLine);
        g.drawRoundedRectangle (r.toFloat().reduced (0.5f), 10.0f, 1.0f);

        g.setColour (cards[i].colour);
        g.setFont (font (13.0f, true));
        g.drawText (cards[i].title, r.reduced (12, 8).removeFromTop (18), juce::Justification::centredLeft);

        const bool valid = cards[i].ms >= 0.0 && total > 0.0;
        const auto num = valid ? fmtMs (cards[i].ms) : juce::String ("-");
        g.setColour (kInk);
        g.setFont (font (22.0f, true));
        g.drawText (valid ? num + " ms" : num, r.reduced (12, 8).withTrimmedTop (20), juce::Justification::centredLeft);

        if (valid)
            cells.push_back ({ r, num });
    }

    g.setFont (font (14.0f, true));
    g.setColour (kAcc);
    if (total <= 0.0)
        g.drawText ("Enter a total length", 16, 380, getWidth() - 32, 24, juce::Justification::centredLeft);
    else if (dec < 0.0)
        g.drawText ("Pre-delay is longer than the total by " + fmtMs (-dec) + " ms",
                    16, 380, getWidth() - 32, 24, juce::Justification::centredLeft);
    else
    {
        g.setColour (kMute);
        g.setFont (font (13.0f));
        g.drawText ("Decay = total - pre-delay. Set the reverb decay to the blue value.",
                    16, 380, getWidth() - 32, 24, juce::Justification::centredLeft);
    }
}

void DelayCalcEditor::mouseDown (const juce::MouseEvent& e)
{
    dlog ("click on editor");
    for (auto& c : cells)
    {
        if (c.area.contains (e.getPosition()))
        {
            juce::SystemClipboard::copyTextToClipboard (c.copyText);
            flash ("Copied: " + c.copyText);
            return;
        }
    }
}
