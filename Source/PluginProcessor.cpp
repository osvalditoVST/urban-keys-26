#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "PianoVoice.h"

juce::AudioProcessorValueTreeState::ParameterLayout UrbanKeysProcessor::createLayout()
{
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> p;

    auto addF = [&p] (const char* id, const char* name, float lo, float hi, float step, float def, float skew = 1.0f)
    {
        p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name,
                                                            NormalisableRange<float> (lo, hi, step, skew), def));
    };
    auto addB = [&p] (const char* id, const char* name, bool def)
    {
        p.push_back (std::make_unique<AudioParameterBool> (ParameterID { id, 1 }, name, def));
    };

    addF ("volume",  "Volume",   -30.0f, 6.0f, 0.1f, -3.0f);
    addF ("pan",     "Pan",      -100.0f, 100.0f, 1.0f, 0.0f);
    addF ("attack",  "Attack",   1.0f, 2000.0f, 1.0f, 10.0f, 0.4f);
    addF ("decay",   "Decay",    10.0f, 3000.0f, 1.0f, 350.0f, 0.4f);
    addF ("sustain", "Sustain",  0.0f, 100.0f, 1.0f, 70.0f);
    addF ("release", "Release",  10.0f, 5000.0f, 1.0f, 1200.0f, 0.4f);
    addF ("cutoff",  "Cutoff",   80.0f, 20000.0f, 1.0f, 12000.0f, 0.3f);
    addF ("reso",    "Reso",     0.0f, 1.0f, 0.01f, 0.4f);

    addB ("rvbOn", "Reverb On", true);
    addF ("rvbMix",  "Reverb Mix",  0.0f, 100.0f, 1.0f, 35.0f);
    addF ("rvbSize", "Reverb Size", 0.0f, 100.0f, 1.0f, 60.0f);
    addF ("rvbDamp", "Reverb Damp", 0.0f, 100.0f, 1.0f, 45.0f);

    addB ("dlyOn", "Delay On", true);
    addF ("dlyMix", "Delay Mix",      0.0f, 100.0f, 1.0f, 28.0f);
    addF ("dlyFb",  "Delay Feedback", 0.0f, 90.0f, 1.0f, 32.0f);
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "dlyTime", 1 }, "Delay Time",
                                                         StringArray { "1/16", "1/8", "1/4", "1/2" }, 2));

    addB ("chOn", "Chorus On", true);
    addF ("chMix", "Chorus Mix", 0.0f, 100.0f, 1.0f, 15.0f);
    addB ("phOn", "Phaser On", false);
    addF ("phMix", "Phaser Mix", 0.0f, 100.0f, 1.0f, 0.0f);

    return { p.begin(), p.end() };
}

UrbanKeysProcessor::UrbanKeysProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
    auto P = [this] (const char* id) { return apvts.getRawParameterValue (id); };
    pVolume = P ("volume");  pPan = P ("pan");  pCutoff = P ("cutoff");  pReso = P ("reso");
    pRvbOn = P ("rvbOn");  pRvbMix = P ("rvbMix");  pRvbSize = P ("rvbSize");  pRvbDamp = P ("rvbDamp");
    pDlyOn = P ("dlyOn");  pDlyMix = P ("dlyMix");  pDlyTime = P ("dlyTime");  pDlyFb = P ("dlyFb");
    pChOn = P ("chOn");  pChMix = P ("chMix");  pPhOn = P ("phOn");  pPhMix = P ("phMix");

    synth.addSound (new PianoSound());
    for (int i = 0; i < 16; ++i)
        synth.addVoice (new PianoVoice (P ("attack"), P ("decay"), P ("sustain"), P ("release"), &currentPreset));
}

bool UrbanKeysProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void UrbanKeysProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    synth.setCurrentPlaybackSampleRate (sampleRate);

    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) samplesPerBlock, 2 };
    lp1.prepare (spec);  lp2.prepare (spec);
    lp1.setType (juce::dsp::StateVariableTPTFilterType::lowpass);
    lp2.setType (juce::dsp::StateVariableTPTFilterType::lowpass);
    lp2.setResonance (0.707f);

    chorus.prepare (spec);
    chorus.setRate (0.9f);  chorus.setDepth (0.3f);  chorus.setCentreDelay (7.0f);  chorus.setFeedback (0.0f);

    phaser.prepare (spec);
    phaser.setRate (0.5f);  phaser.setDepth (0.7f);  phaser.setCentreFrequency (900.0f);  phaser.setFeedback (0.4f);

    reverb.setSampleRate (sampleRate);
    reverb.reset();

    delayBuf.setSize (2, (int) (sampleRate * 4.0) + 8);
    delayBuf.clear();
    delayPos = 0;
}

void UrbanKeysProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();

    keyState.processNextMidiBuffer (midi, 0, n, true);

    buffer.clear();
    synth.renderNextBlock (buffer, midi, 0, n);

    juce::dsp::AudioBlock<float> block (buffer);
    juce::dsp::ProcessContextReplacing<float> ctx (block);

    // Filtro pasa-bajos 24 dB/oct (dos etapas)
    const float cutoff = juce::jmin (pCutoff->load(), (float) sr * 0.45f);
    lp1.setCutoffFrequency (cutoff);
    lp2.setCutoffFrequency (cutoff);
    lp1.setResonance (0.707f + pReso->load() * 4.0f);
    lp1.process (ctx);
    lp2.process (ctx);

    // Chorus / Phaser
    const float chMix = pChMix->load() / 100.0f;
    if (pChOn->load() > 0.5f && chMix > 0.001f)
    {
        chorus.setMix (chMix);
        chorus.process (ctx);
    }
    const float phMix = pPhMix->load() / 100.0f;
    if (pPhOn->load() > 0.5f && phMix > 0.001f)
    {
        phaser.setMix (phMix);
        phaser.process (ctx);
    }

    // Delay estereo sincronizado al tempo del DAW
    float* l = buffer.getWritePointer (0);
    float* r = buffer.getWritePointer (1);
    const float dMix = pDlyMix->load() / 100.0f;

    if (pDlyOn->load() > 0.5f && dMix > 0.001f)
    {
        double bpm = 120.0;
        if (auto* ph = getPlayHead())
            if (auto pos = ph->getPosition())
                if (auto b = pos->getBpm())
                    bpm = *b;

        static const double beats[4] = { 0.25, 0.5, 1.0, 2.0 };
        const int idx = juce::jlimit (0, 3, (int) pDlyTime->load());
        const int size = delayBuf.getNumSamples();
        const int dsamp = juce::jlimit (1, size - 1, (int) (60.0 / juce::jmax (30.0, bpm) * beats[idx] * sr));
        const float fb = pDlyFb->load() / 100.0f;
        float* bl = delayBuf.getWritePointer (0);
        float* br = delayBuf.getWritePointer (1);

        for (int i = 0; i < n; ++i)
        {
            int rp = delayPos - dsamp;
            if (rp < 0) rp += size;
            const float dl = bl[rp], dr = br[rp];
            const float inL = l[i], inR = r[i];
            bl[delayPos] = inL + fb * dr;
            br[delayPos] = inR + fb * dl;
            l[i] = inL + dMix * dl;
            r[i] = inR + dMix * dr;
            if (++delayPos >= size) delayPos = 0;
        }
    }
    else
    {
        delayBuf.clear();
    }

    // Reverb
    if (pRvbOn->load() > 0.5f)
    {
        juce::Reverb::Parameters rp;
        rp.roomSize = pRvbSize->load() / 100.0f;
        rp.damping  = pRvbDamp->load() / 100.0f;
        rp.wetLevel = (pRvbMix->load() / 100.0f) * 0.4f;
        rp.dryLevel = 1.0f;
        rp.width    = 1.0f;
        rp.freezeMode = 0.0f;
        reverb.setParameters (rp);
        reverb.processStereo (l, r, n);
    }

    // Volumen y paneo
    const float vol = juce::Decibels::decibelsToGain (pVolume->load());
    const float angle = (pPan->load() / 100.0f + 1.0f) * juce::MathConstants<float>::pi * 0.25f;
    buffer.applyGain (0, 0, n, vol * std::cos (angle) * 1.41421356f);
    buffer.applyGain (1, 0, n, vol * std::sin (angle) * 1.41421356f);
}

void UrbanKeysProcessor::loadPreset (int index)
{
    index = juce::jlimit (0, 11, index);
    currentPreset.store (index);
    const auto& p = kPresets[index];

    auto set = [this] (const char* id, float v)
    {
        if (auto* prm = apvts.getParameter (id))
            prm->setValueNotifyingHost (prm->convertTo0to1 (v));
    };
    set ("cutoff", p.cutoff);   set ("reso", p.reso);
    set ("attack", p.attackMs); set ("release", p.releaseMs);
    set ("rvbMix", p.rvbMix);   set ("dlyMix", p.dlyMix);   set ("chMix", p.chorusMix);
}

void UrbanKeysProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    auto state = apvts.copyState();
    state.setProperty ("preset", currentPreset.load(), nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, dest);
}

void UrbanKeysProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto tree = juce::ValueTree::fromXml (*xml);
            currentPreset.store (juce::jlimit (0, 11, (int) tree.getProperty ("preset", 0)));
            apvts.replaceState (tree);
        }
}

juce::AudioProcessorEditor* UrbanKeysProcessor::createEditor()
{
    return new UrbanKeysEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new UrbanKeysProcessor();
}
