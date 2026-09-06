#pragma once

#include <JuceHeader.h>
#include <samplerate.h>
#include <cmath>
#include <vector>

#if defined(_MSC_VER)
  #pragma comment(lib, "samplerate.lib")
#endif

class AudioResampler
{
public:
    /**
     * Resamples an AudioBuffer from sourceRate to targetRate using libsamplerate (Secret Rabbit Code).
     * If sourceRate and targetRate match (or are invalid), returns the input buffer untouched with zero overhead.
     *
     * @param inBuffer Input audio buffer (mono or stereo).
     * @param sourceRate Native sample rate of the input buffer (e.g. 48000.0).
     * @param targetRate Target sample rate (e.g. 44100.0).
     * @param quality libsamplerate quality mode (default: SRC_SINC_BEST_QUALITY).
     * @return Converted AudioBuffer at targetRate, or original buffer if rates match.
     */
    static juce::AudioBuffer<float> resampleIfNeeded(
        const juce::AudioBuffer<float>& inBuffer,
        double sourceRate,
        double targetRate,
        int quality = SRC_SINC_BEST_QUALITY)
    {
        const int numChannels = inBuffer.getNumChannels();
        const int numFrames = inBuffer.getNumSamples();

        if (numFrames == 0 || numChannels == 0)
            return inBuffer;

        if (sourceRate <= 0.0 || targetRate <= 0.0 || std::abs(sourceRate - targetRate) < 0.001)
        {
            // Exact or matching sample rate: skip conversion completely
            return inBuffer;
        }

        const double ratio = targetRate / sourceRate;
        const long outFramesCapacity = (long)std::ceil((double)numFrames * ratio) + 256;

        // Interleave input audio channels into contiguous buffer
        std::vector<float> inInterleaved((size_t)(numChannels * numFrames));
        for (int ch = 0; ch < numChannels; ++ch)
        {
            const float* r = inBuffer.getReadPointer(ch);
            for (int i = 0; i < numFrames; ++i)
            {
                inInterleaved[(size_t)(i * numChannels + ch)] = r[i];
            }
        }

        std::vector<float> outInterleaved((size_t)(numChannels * outFramesCapacity), 0.0f);

        SRC_DATA srcData;
        std::memset(&srcData, 0, sizeof(srcData));
        srcData.data_in = inInterleaved.data();
        srcData.data_out = outInterleaved.data();
        srcData.input_frames = (long)numFrames;
        srcData.output_frames = outFramesCapacity;
        srcData.src_ratio = ratio;
        srcData.end_of_input = 1;

        int error = src_simple(&srcData, quality, numChannels);
        if (error != 0 || srcData.output_frames_gen <= 0)
        {
            DBG("AudioResampler::resampleIfNeeded - libsamplerate error: " << src_strerror(error));
            return inBuffer; // Fallback to original buffer on conversion failure
        }

        const long genFrames = srcData.output_frames_gen;
        juce::AudioBuffer<float> outBuffer(numChannels, (int)genFrames);

        // De-interleave output audio channels into planar JUCE AudioBuffer
        for (int ch = 0; ch < numChannels; ++ch)
        {
            float* w = outBuffer.getWritePointer(ch);
            for (long i = 0; i < genFrames; ++i)
            {
                w[i] = outInterleaved[(size_t)(i * numChannels + ch)];
            }
        }

        // Smooth boundary micro-ramps (first and last 16 frames) to eliminate sinc window truncation edge artifacts
        const int rampFrames = juce::jmin(16, (int)genFrames / 4);
        if (rampFrames > 0)
        {
            for (int ch = 0; ch < numChannels; ++ch)
            {
                float* w = outBuffer.getWritePointer(ch);
                for (int i = 0; i < rampFrames; ++i)
                {
                    float factor = 0.5f * (1.0f - std::cos(juce::MathConstants<float>::pi * (float)i / (float)rampFrames));
                    w[i] *= factor;
                    w[genFrames - 1 - i] *= factor;
                }
            }
        }

        return outBuffer;
    }
};
