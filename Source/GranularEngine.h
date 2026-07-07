#pragma once
#include <JuceHeader.h>
#include <vector>
#include <random>
#include <cmath>

/**
    A simple granular engine for pitch and time manipulation.

    How it works, step by step:
      1. Every incoming sample is written into a circular "history" buffer,
         so we always have the last ~2 seconds of audio available to read from.
      2. New "grains" (short snippets, e.g. 10-300ms) are spawned at a rate
         controlled by `density`.
      3. Each grain starts reading from the history buffer near the current
         write position, offset backwards by a random "scatter" amount -
         this randomised offset is what gives granular textures their
         characteristic smeared, cloud-like quality.
      4. Each grain plays back at a rate controlled by `pitchRatio`. This is
         what shifts the pitch: rate > 1 = higher pitch, rate < 1 = lower
         pitch, independent of how fast grains are being spawned.
      5. Each grain is shaped with a Hann window (fades in, fades out) so it
         has no clicks at its edges, and overlapping grains are simply added
         together (overlap-add) to produce continuous output.

    This is intentionally simple/readable rather than maximally optimised -
    a great base to build pitch-shift ratio smoothing, stereo spread,
    reverse grains, envelopes, etc. on top of.
*/
class GranularEngine
{
public:
    void prepare (double newSampleRate, int /*maxBlockSize*/, int newNumChannels)
    {
        sampleRate  = newSampleRate;
        numChannels = juce::jmax (1, newNumChannels);

        // ~2 seconds of history is plenty for grain sizes up to a few hundred ms
        bufferLength = (int) (sampleRate * 2.0);
        circularBuffer.assign ((size_t) numChannels, std::vector<float> ((size_t) bufferLength, 0.0f));
        writePos = 0;

        grains.clear();
        grains.resize (maxGrains);

        wetAccum.assign ((size_t) numChannels, 0.0f);

        samplesUntilNextGrain = 0;
        rng.seed (std::random_device {} ());
    }

    void setParameters (float grainSizeMsIn, float densityIn, float pitchSemitonesIn,
                         float scatterMsIn, float mixIn)
    {
        grainSizeMs = grainSizeMsIn;
        density     = densityIn;
        pitchRatio  = std::pow (2.0f, pitchSemitonesIn / 12.0f);
        scatterMs   = scatterMsIn;
        mix         = mixIn;
    }

    void process (juce::AudioBuffer<float>& buffer)
    {
        const int numSamples = buffer.getNumSamples();
        const int chans = juce::jmin (numChannels, buffer.getNumChannels());

        for (int i = 0; i < numSamples; ++i)
        {
            // 1. write dry input into circular history buffer
            for (int ch = 0; ch < chans; ++ch)
                circularBuffer[(size_t) ch][(size_t) writePos] = buffer.getReadPointer (ch)[i];

            // 2. maybe spawn a new grain
            if (samplesUntilNextGrain <= 0)
            {
                spawnGrain();
                float grainsPerSecond = juce::jmax (0.5f, density);
                samplesUntilNextGrain = (int) (sampleRate / grainsPerSecond);
            }
            --samplesUntilNextGrain;

            // 3. clear wet accumulator for this sample
            for (int ch = 0; ch < chans; ++ch)
                wetAccum[(size_t) ch] = 0.0f;

            // 4. render all currently active grains and sum them (overlap-add)
            for (auto& g : grains)
            {
                if (! g.active)
                    continue;

                float phase = (float) g.age / (float) g.duration;
                float win = 0.5f - 0.5f * std::cos (2.0f * juce::MathConstants<float>::pi * phase);

                for (int ch = 0; ch < chans; ++ch)
                {
                    float sample = readInterpolated (ch, g.readPos);
                    wetAccum[(size_t) ch] += sample * win;
                }

                g.readPos += g.playbackRate;
                if (g.readPos >= (double) bufferLength) g.readPos -= (double) bufferLength;
                if (g.readPos < 0.0) g.readPos += (double) bufferLength;

                ++g.age;
                if (g.age >= g.duration)
                    g.active = false;
            }

            // 5. mix dry/wet and write output back into the buffer
            for (int ch = 0; ch < chans; ++ch)
            {
                float dry = buffer.getReadPointer (ch)[i];
                buffer.getWritePointer (ch)[i] = dry * (1.0f - mix) + wetAccum[(size_t) ch] * mix;
            }

            writePos = (writePos + 1) % bufferLength;
        }
    }

private:
    struct Grain
    {
        bool active = false;
        double readPos = 0.0;
        double playbackRate = 1.0;
        int age = 0;
        int duration = 1;
    };

    void spawnGrain()
    {
        // find a free grain "voice" in the fixed-size pool
        for (auto& g : grains)
        {
            if (g.active)
                continue;

            g.active = true;
            g.age = 0;
            g.duration = juce::jmax (64, (int) (grainSizeMs * 0.001 * sampleRate));
            g.playbackRate = pitchRatio;

            std::uniform_real_distribution<float> dist (-scatterMs, scatterMs);
            float offsetMs = dist (rng);
            double offsetSamples = offsetMs * 0.001 * sampleRate;

            double idealStart = (double) writePos - (double) g.duration - offsetSamples;
            idealStart = std::fmod (idealStart, (double) bufferLength);
            if (idealStart < 0.0) idealStart += (double) bufferLength;

            g.readPos = idealStart;
            return;
        }
        // all grain voices are busy - simplest policy is to just skip spawning
        // this cycle. Increase maxGrains below if you need denser textures.
    }

    float readInterpolated (int channel, double position) const
    {
        int idx0 = (int) position;
        float frac = (float) (position - (double) idx0);

        idx0 %= bufferLength;
        if (idx0 < 0) idx0 += bufferLength;
        int idx1 = (idx0 + 1) % bufferLength;

        float s0 = circularBuffer[(size_t) channel][(size_t) idx0];
        float s1 = circularBuffer[(size_t) channel][(size_t) idx1];
        return s0 + frac * (s1 - s0); // linear interpolation between samples
    }

    double sampleRate = 44100.0;
    int numChannels = 2;
    int bufferLength = 0;
    int writePos = 0;

    std::vector<std::vector<float>> circularBuffer;
    std::vector<float> wetAccum;

    static constexpr int maxGrains = 40; // max simultaneous overlapping grains
    std::vector<Grain> grains;

    int samplesUntilNextGrain = 0;

    // current parameter values, updated every block from the processor
    float grainSizeMs = 80.0f;
    float density     = 15.0f;
    float pitchRatio  = 1.0f;
    float scatterMs   = 20.0f;
    float mix         = 0.5f;

    std::mt19937 rng;
};
