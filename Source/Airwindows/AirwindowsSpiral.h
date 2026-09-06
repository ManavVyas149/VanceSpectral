/* ==============================================================================
 *  AirwindowsSpiral.h - Extracted from Airwindows (MIT License)
 *  Original Author: Chris Johnson (airwindows.com)
 * ==============================================================================
 */

#pragma once

#include <cmath>
#include <cstdint>
#include <cstdlib>

namespace Airwindows
{

class Spiral
{
public:
    Spiral()
    {
        reset();
    }
    ~Spiral() = default;

    void reset()
    {
        fpdL = 1;
        while (fpdL < 16386) fpdL = (uint32_t)rand() * UINT32_MAX;
        fpdR = 1;
        while (fpdR < 16386) fpdR = (uint32_t)rand() * UINT32_MAX;
    }

    // Process a stereo pair with drive (0.0 = clean, 1.0 = heavy saturation)
    void processStereo(float* leftChannel, float* rightChannel, int numSamples, float driveAmount, float outputGain = 1.0f)
    {
        if (driveAmount <= 0.0001f)
            return;

        // Drive scaling factor: 1.0 at 0% up to 8.0 at 100%
        const double drive = 1.0 + (double)driveAmount * 7.0;
        const double comp = 1.0 / std::sqrt(drive); // Compensate volume boost
        const double outGain = (double)outputGain * comp;

        for (int i = 0; i < numSamples; ++i)
        {
            double inputSampleL = leftChannel[i] * drive;
            double inputSampleR = rightChannel[i] * drive;

            if (std::fabs(inputSampleL) < 1.18e-23) inputSampleL = (double)fpdL * 1.18e-17;
            if (std::fabs(inputSampleR) < 1.18e-23) inputSampleR = (double)fpdR * 1.18e-17;

            // Spiral nonlinear shaping: sin(x * |x|) / |x|
            double magL = std::fabs(inputSampleL);
            double shapedL = (magL == 0.0) ? 0.0 : std::sin(inputSampleL * magL) / magL;

            double magR = std::fabs(inputSampleR);
            double shapedR = (magR == 0.0) ? 0.0 : std::sin(inputSampleR * magR) / magR;

            leftChannel[i] = (float)(shapedL * outGain);
            rightChannel[i] = (float)(shapedR * outGain);
        }
    }

    // Single sample waveshaper function for juce::dsp::WaveShaper / custom stage
    inline float shapeSample(float input, float drive) const
    {
        if (drive <= 0.0001f) return input;
        double d = 1.0 + (double)drive * 6.0;
        double in = (double)input * d;
        double mag = std::fabs(in);
        if (mag == 0.0) return 0.0f;
        double shaped = std::sin(in * mag) / mag;
        return (float)(shaped / std::sqrt(d));
    }

private:
    uint32_t fpdL{ 1 };
    uint32_t fpdR{ 1 };
};

} // namespace Airwindows
