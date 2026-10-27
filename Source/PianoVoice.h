#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <atomic>
#include <cmath>
#include "Presets.h"

struct PianoSound : public juce::SynthesiserSound
{
    bool appliesToNote (int) override    { return true; }
    bool appliesToChannel (int) override { return true; }
};

// Voz de piano por sintesis aditiva: armonicos con decaimiento individual,
// dos "cuerdas" ligeramente desafinadas por armonico y golpe de martillo.
class PianoVoice : public juce::SynthesiserVoice
{
public:
    PianoVoice (std::atomic<float>* a, std::atomic<float>* d, std::atomic<float>* s,
                std::atomic<float>* r, std::atomic<int>* presetIdx)
        : pA (a), pD (d), pS (s), pR (r), pPreset (presetIdx) {}

    bool canPlaySound (juce::SynthesiserSound* s) override
    {
        return dynamic_cast<PianoSound*> (s) != nullptr;
    }

    void startNote (int midiNote, float velocity, juce::SynthesiserSound*, int pitchWheelPos) override
    {
        const auto& p = kPresets[juce::jlimit (0, 11, pPreset->load())];
        sr = getSampleRate();
        f0 = (float) juce::MidiMessage::getMidiNoteInHertz (midiNote);
        bend = std::pow (2.0f, ((float) (pitchWheelPos - 8192) / 8192.0f) * 2.0f / 12.0f);
        detune = std::pow (2.0f, p.detuneCents / 1200.0f);
        noiseAmt = p.noise * velocity;
        noiseEnv = 1.0f;
        noiseLp = 0.0f;

        const float rolloff = p.rolloff + (1.0f - velocity) * 0.8f;
        const float pitchDecay = juce::jlimit (0.3f, 2.5f, std::pow (261.63f / f0, 0.4f));
        const float velGain = 0.12f * (0.35f + 0.65f * velocity);

        numP = 0;
        auto addPartial = [&] (float ratio, float amp, float tau)
        {
            if (numP >= maxP) return;
            if (f0 * ratio * 1.2f > 0.45f * (float) sr) return;
            ratios[numP] = ratio;
            env[numP]    = amp * velGain;
            coef[numP]   = std::exp (-1.0f / ((float) sr * juce::jmax (0.02f, tau)));
            ph1[numP] = ph2[numP] = 0.0f;
            ++numP;
        };

        for (int k = 1; k <= 12; ++k)
        {
            const float kf = (float) k;
            float ratio = kf * std::sqrt (1.0f + p.inharm * kf * kf);
            float amp = std::pow (kf, -rolloff);
            if (k % 2 == 0) amp *= p.evenGain;
            const float tau = p.decay * pitchDecay / (1.0f + 0.6f * (kf - 1.0f));
            addPartial (ratio, amp, tau);
        }

        if (p.bell > 0.0f)
        {
            addPartial (2.76f, p.bell,         p.decay * 0.30f * pitchDecay);
            addPartial (5.40f, p.bell * 0.60f, p.decay * 0.20f * pitchDecay);
            addPartial (8.93f, p.bell * 0.35f, p.decay * 0.12f * pitchDecay);
        }

        adsr.setSampleRate (sr);
        adsr.setParameters ({ pA->load() / 1000.0f, pD->load() / 1000.0f,
                              pS->load() / 100.0f,  pR->load() / 1000.0f });
        adsr.noteOn();
    }

    void stopNote (float, bool allowTailOff) override
    {
        if (allowTailOff)
            adsr.noteOff();
        else
        {
            adsr.reset();
            clearCurrentNote();
        }
    }

    void pitchWheelMoved (int v) override
    {
        bend = std::pow (2.0f, ((float) (v - 8192) / 8192.0f) * 2.0f / 12.0f);
    }

    void controllerMoved (int, int) override {}

    void renderNextBlock (juce::AudioBuffer<float>& out, int startSample, int numSamples) override
    {
        if (! adsr.isActive())
        {
            clearCurrentNote();
            return;
        }

        constexpr float twoPi = juce::MathConstants<float>::twoPi;
        float inc[maxP];
        for (int k = 0; k < numP; ++k)
            inc[k] = twoPi * f0 * ratios[k] * bend / (float) sr;

        const int numCh = out.getNumChannels();

        for (int i = 0; i < numSamples; ++i)
        {
            float s = 0.0f;
            for (int k = 0; k < numP; ++k)
            {
                s += env[k] * (std::sin (ph1[k]) + std::sin (ph2[k])) * 0.5f;
                ph1[k] += inc[k];
                ph2[k] += inc[k] * detune;
                if (ph1[k] > twoPi) ph1[k] -= twoPi;
                if (ph2[k] > twoPi) ph2[k] -= twoPi;
                env[k] *= coef[k];
            }

            if (noiseEnv > 0.0005f)
            {
                noiseLp += (rng.nextFloat() * 2.0f - 1.0f - noiseLp) * 0.35f;
                s += noiseLp * noiseAmt * noiseEnv * 0.25f;
                noiseEnv *= std::exp (-1.0f / ((float) sr * 0.012f));
            }

            s *= adsr.getNextSample();

            for (int ch = 0; ch < numCh; ++ch)
                out.addSample (ch, startSample + i, s);

            if (! adsr.isActive())
            {
                clearCurrentNote();
                return;
            }
        }

        // libera la voz cuando la cuerda ya se apago por completo
        float peak = 0.0f;
        for (int k = 0; k < numP; ++k) peak = juce::jmax (peak, env[k]);
        if (peak < 1.0e-5f && noiseEnv <= 0.0005f)
        {
            adsr.reset();
            clearCurrentNote();
        }
    }

private:
    static constexpr int maxP = 16;

    std::atomic<float>* pA; std::atomic<float>* pD; std::atomic<float>* pS; std::atomic<float>* pR;
    std::atomic<int>* pPreset;

    juce::ADSR adsr;
    juce::Random rng;

    double sr = 44100.0;
    float f0 = 440.0f, bend = 1.0f, detune = 1.0f;
    float noiseAmt = 0.0f, noiseEnv = 0.0f, noiseLp = 0.0f;
    int numP = 0;
    float ratios[maxP] {}, env[maxP] {}, coef[maxP] {}, ph1[maxP] {}, ph2[maxP] {};
};
