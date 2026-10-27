#pragma once
#include "PluginProcessor.h"

class UrbanKeysEditor : public juce::AudioProcessorEditor
{
public:
    explicit UrbanKeysEditor (UrbanKeysProcessor&);
    ~UrbanKeysEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using SliderAtt = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAtt = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboAtt  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    struct Knob
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<SliderAtt> att;
    };

    void addKnob (Knob& k, const char* id, const juce::String& name, const juce::String& suffix, int decimals = 0);
    void place (Knob& k, juce::Rectangle<int> cell, bool showLabel = true);
    void selectPreset (int index);

    juce::LookAndFeel_V4 laf;
    UrbanKeysProcessor& proc;

    std::array<juce::TextButton, 12> presetBtns;

    Knob volume, pan, cutoff, reso, attack, decay, sustain, release;
    Knob rvbMix, rvbSize, rvbDamp, dlyMix, dlyFb, chMix, phMix;

    juce::ToggleButton rvbOn, dlyOn, chOn, phOn;
    juce::ComboBox dlyTime;
    juce::MidiKeyboardComponent keyboard;

    std::unique_ptr<ButtonAtt> rvbOnAtt, dlyOnAtt, chOnAtt, phOnAtt;
    std::unique_ptr<ComboAtt> dlyTimeAtt;

    juce::Rectangle<int> presetArea, titleArea, mainArea, adsrArea, rvbArea, dlyArea, modArea;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (UrbanKeysEditor)
};
