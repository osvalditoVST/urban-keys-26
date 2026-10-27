#include "PluginEditor.h"

namespace
{
    const juce::Colour kPurple (0xff9b4dff);
    const juce::Colour kPurpleDark (0xff3a2a66);
    const juce::Colour kText (0xffe9e1ff);
}

UrbanKeysEditor::UrbanKeysEditor (UrbanKeysProcessor& p)
    : AudioProcessorEditor (&p), proc (p),
      keyboard (p.keyState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    laf.setColour (juce::Slider::rotarySliderFillColourId, kPurple);
    laf.setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colour (0xff2a2145));
    laf.setColour (juce::Slider::thumbColourId, juce::Colours::white);
    laf.setColour (juce::Slider::textBoxTextColourId, kText);
    laf.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    laf.setColour (juce::Label::textColourId, kText);
    laf.setColour (juce::ToggleButton::textColourId, kText);
    laf.setColour (juce::ToggleButton::tickColourId, kPurple);
    laf.setColour (juce::ToggleButton::tickDisabledColourId, juce::Colour (0xff6b5f8f));
    laf.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff17112a));
    laf.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xff5b2fc4));
    laf.setColour (juce::TextButton::textColourOffId, kText);
    laf.setColour (juce::TextButton::textColourOnId, juce::Colours::white);
    laf.setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff17112a));
    laf.setColour (juce::ComboBox::textColourId, kText);
    laf.setColour (juce::ComboBox::outlineColourId, kPurpleDark);
    laf.setColour (juce::ComboBox::arrowColourId, kPurple);
    laf.setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff17112a));
    laf.setColour (juce::PopupMenu::textColourId, kText);
    laf.setColour (juce::PopupMenu::highlightedBackgroundColourId, juce::Colour (0xff5b2fc4));
    setLookAndFeel (&laf);

    for (int i = 0; i < 12; ++i)
    {
        auto& b = presetBtns[(size_t) i];
        b.setButtonText (juce::String::formatted ("%02d   ", i + 1) + kPresets[i].name);
        b.setClickingTogglesState (true);
        b.setRadioGroupId (1001);
        b.setConnectedEdges (juce::Button::ConnectedOnTop | juce::Button::ConnectedOnBottom);
        b.onClick = [this, i] { selectPreset (i); };
        addAndMakeVisible (b);
    }
    presetBtns[(size_t) juce::jlimit (0, 11, proc.getCurrentProgram())].setToggleState (true, juce::dontSendNotification);

    addKnob (volume,  "volume",  "VOLUME",  " dB", 1);
    addKnob (pan,     "pan",     "PAN",     " %");
    addKnob (cutoff,  "cutoff",  "CUTOFF",  " Hz");
    addKnob (reso,    "reso",    "RESO",    "", 2);
    addKnob (attack,  "attack",  "A",       " ms");
    addKnob (decay,   "decay",   "D",       " ms");
    addKnob (sustain, "sustain", "S",       " %");
    addKnob (release, "release", "R",       " ms");
    addKnob (rvbMix,  "rvbMix",  "Mix",     " %");
    addKnob (rvbSize, "rvbSize", "Size",    " %");
    addKnob (rvbDamp, "rvbDamp", "Damp",    " %");
    addKnob (dlyMix,  "dlyMix",  "Mix",     " %");
    addKnob (dlyFb,   "dlyFb",   "Feedback"," %");
    addKnob (chMix,   "chMix",   "Mix",     " %");
    addKnob (phMix,   "phMix",   "Mix",     " %");

    rvbOn.setButtonText ("REVERB");
    dlyOn.setButtonText ("DELAY");
    chOn.setButtonText ("Chorus");
    phOn.setButtonText ("Phaser");
    for (auto* t : { &rvbOn, &dlyOn, &chOn, &phOn })
        addAndMakeVisible (*t);

    dlyTime.addItemList ({ "1/16", "1/8", "1/4", "1/2" }, 1);
    addAndMakeVisible (dlyTime);

    rvbOnAtt   = std::make_unique<ButtonAtt> (proc.apvts, "rvbOn", rvbOn);
    dlyOnAtt   = std::make_unique<ButtonAtt> (proc.apvts, "dlyOn", dlyOn);
    chOnAtt    = std::make_unique<ButtonAtt> (proc.apvts, "chOn", chOn);
    phOnAtt    = std::make_unique<ButtonAtt> (proc.apvts, "phOn", phOn);
    dlyTimeAtt = std::make_unique<ComboAtt>  (proc.apvts, "dlyTime", dlyTime);

    keyboard.setAvailableRange (36, 96);
    keyboard.setKeyWidth (24.0f);
    keyboard.setColour (juce::MidiKeyboardComponent::whiteNoteColourId, juce::Colour (0xffe6d9ff));
    keyboard.setColour (juce::MidiKeyboardComponent::blackNoteColourId, juce::Colour (0xff120c22));
    keyboard.setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId, kPurple);
    keyboard.setColour (juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, juce::Colour (0x558a4bff));
    keyboard.setColour (juce::MidiKeyboardComponent::shadowColourId, juce::Colours::transparentBlack);
    addAndMakeVisible (keyboard);

    setSize (1000, 640);
}

UrbanKeysEditor::~UrbanKeysEditor()
{
    setLookAndFeel (nullptr);
}

void UrbanKeysEditor::addKnob (Knob& k, const char* id, const juce::String& name,
                               const juce::String& suffix, int decimals)
{
    k.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    k.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 74, 16);
    k.slider.setTextValueSuffix (suffix);
    addAndMakeVisible (k.slider);

    k.label.setText (name, juce::dontSendNotification);
    k.label.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (k.label);

    k.att = std::make_unique<SliderAtt> (proc.apvts, id, k.slider);
    k.slider.setNumDecimalPlacesToDisplay (decimals);
}

void UrbanKeysEditor::place (Knob& k, juce::Rectangle<int> cell, bool showLabel)
{
    k.label.setVisible (showLabel);
    if (showLabel)
        k.label.setBounds (cell.removeFromTop (16));
    k.slider.setBounds (cell);
}

void UrbanKeysEditor::selectPreset (int index)
{
    proc.loadPreset (index);
    repaint();
}

void UrbanKeysEditor::resized()
{
    auto r = getLocalBounds().reduced (12);

    keyboard.setBounds (r.removeFromBottom (90));
    r.removeFromBottom (8);

    presetArea = r.removeFromLeft (210);
    r.removeFromLeft (10);
    auto right = r.removeFromRight (290);
    r.removeFromRight (10);

    // presets
    {
        auto a = presetArea.reduced (6);
        const int h = a.getHeight() / 12;
        for (auto& b : presetBtns)
            b.setBounds (a.removeFromTop (h).reduced (0, 2));
    }

    // centro
    titleArea = r.removeFromTop (70);
    r.removeFromTop (60);               // espacio del nombre del preset y etiquetas (se dibuja en paint)
    r.removeFromTop (12);
    mainArea = r.removeFromTop (150);
    r.removeFromTop (10);
    adsrArea = r.removeFromTop (150);

    {
        auto a = mainArea.reduced (6);
        const int w = a.getWidth() / 4;
        place (volume, a.removeFromLeft (w));
        place (pan,    a.removeFromLeft (w));
        place (cutoff, a.removeFromLeft (w));
        place (reso,   a);
    }
    {
        auto a = adsrArea.reduced (6);
        a.removeFromTop (18);           // titulo AMP ENVELOPE
        const int w = a.getWidth() / 4;
        place (attack,  a.removeFromLeft (w));
        place (decay,   a.removeFromLeft (w));
        place (sustain, a.removeFromLeft (w));
        place (release, a);
    }

    // efectos
    rvbArea = right.removeFromTop (160);
    right.removeFromTop (10);
    dlyArea = right.removeFromTop (160);
    right.removeFromTop (10);
    modArea = right;

    {
        auto a = rvbArea.reduced (8);
        rvbOn.setBounds (a.removeFromTop (26).removeFromLeft (120));
        const int w = a.getWidth() / 3;
        place (rvbMix,  a.removeFromLeft (w));
        place (rvbSize, a.removeFromLeft (w));
        place (rvbDamp, a);
    }
    {
        auto a = dlyArea.reduced (8);
        auto top = a.removeFromTop (26);
        dlyOn.setBounds (top.removeFromLeft (120));
        dlyTime.setBounds (top.removeFromRight (90));
        const int w = a.getWidth() / 3;
        place (dlyMix, a.removeFromLeft (w));
        place (dlyFb,  a.removeFromLeft (w));
        // el tercer espacio queda libre; el tiempo se elige en el menu de arriba
    }
    {
        auto a = modArea.reduced (8);
        a.removeFromTop (22);           // titulo MODULACION
        const int h = a.getHeight() / 2;
        auto row1 = a.removeFromTop (h);
        auto row2 = a;
        chOn.setBounds (row1.removeFromLeft (110).withSizeKeepingCentre (110, 26));
        place (chMix, row1, false);
        phOn.setBounds (row2.removeFromLeft (110).withSizeKeepingCentre (110, 26));
        place (phMix, row2, false);
    }
}

void UrbanKeysEditor::paint (juce::Graphics& g)
{
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff07060f), 0.0f, 0.0f,
                                             juce::Colour (0xff1d1236), 0.0f, (float) getHeight(), false));
    g.fillAll();

    auto panel = [&g] (juce::Rectangle<int> a)
    {
        g.setColour (juce::Colour (0xcc120d20));
        g.fillRoundedRectangle (a.toFloat(), 10.0f);
        g.setColour (kPurpleDark);
        g.drawRoundedRectangle (a.toFloat(), 10.0f, 1.0f);
    };
    for (auto a : { presetArea, mainArea, adsrArea, rvbArea, dlyArea, modArea })
        panel (a);

    // titulo
    g.setColour (juce::Colours::white);
    g.setFont (g.getCurrentFont().withHeight (40.0f).boldened());
    auto t = titleArea;
    g.drawText ("URBAN KEYS", t.removeFromTop (46), juce::Justification::centred);
    g.setColour (kPurple);
    g.setFont (g.getCurrentFont().withHeight (13.0f));
    g.drawText (juce::String::fromUTF8 ("FL26   \xc2\xb7   PIANO URBANO"), t, juce::Justification::centred);

    // nombre del preset actual
    const int cur = juce::jlimit (0, 11, proc.getCurrentProgram());
    auto nameArea = juce::Rectangle<int> (mainArea.getX(), titleArea.getBottom() + 2, mainArea.getWidth(), 60);
    g.setColour (juce::Colours::white);
    g.setFont (g.getCurrentFont().withHeight (24.0f).boldened());
    g.drawText (kPresets[cur].name, nameArea.removeFromTop (34), juce::Justification::centred);
    g.setColour (juce::Colour (0xffb9a6e8));
    g.setFont (g.getCurrentFont().withHeight (13.0f));
    g.drawText (juce::String::fromUTF8 (kPresets[cur].tags), nameArea, juce::Justification::centredTop);

    // titulos de seccion
    g.setColour (kText);
    g.setFont (g.getCurrentFont().withHeight (13.0f).boldened());
    g.drawText ("AMP ENVELOPE", adsrArea.reduced (12, 8).removeFromTop (16), juce::Justification::left);
    g.drawText (juce::String::fromUTF8 ("MODULACI\xc3\x93N"), modArea.reduced (12, 8).removeFromTop (16), juce::Justification::left);
}
