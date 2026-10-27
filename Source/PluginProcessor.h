#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include "Presets.h"

class UrbanKeysProcessor : public juce::AudioProcessor
{
public:
    UrbanKeysProcessor();
    ~UrbanKeysProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Urban Keys FL26"; }
    bool acceptsMidi() const override  { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 5.0; }

    int getNumPrograms() override { return 12; }
    int getCurrentProgram() override { return currentPreset.load(); }
    void setCurrentProgram (int i) override { loadPreset (i); }
    const juce::String getProgramName (int i) override { return kPresets[juce::jlimit (0, 11, i)].name; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    void loadPreset (int index);

    juce::AudioProcessorValueTreeState apvts;
    juce::MidiKeyboardState keyState;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    juce::Synthesiser synth;
    std::atomic<int> currentPreset { 0 };

    std::atomic<float> *pVolume, *pPan, *pCutoff, *pReso,
                       *pRvbOn, *pRvbMix, *pRvbSize, *pRvbDamp,
                       *pDlyOn, *pDlyMix, *pDlyTime, *pDlyFb,
                       *pChOn, *pChMix, *pPhOn, *pPhMix;

    juce::dsp::StateVariableTPTFilter<float> lp1, lp2;
    juce::dsp::Chorus<float> chorus;
    juce::dsp::Phaser<float> phaser;
    juce::Reverb reverb;
    juce::AudioBuffer<float> delayBuf;
    int delayPos = 0;
    double sr = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (UrbanKeysProcessor)
};
