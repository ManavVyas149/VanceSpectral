/* ==============================================================================
 *  AirwindowsChorus.h - Extracted from Airwindows ChorusEnsemble (MIT License)
 *  Original Author: Chris Johnson (airwindows.com)
 * ==============================================================================
 */

#pragma once

#include <cmath>
#include <vector>
#include <cstdint>
#include <cstdlib>

namespace Airwindows
{

class ChorusEnsemble
{
public:
    ChorusEnsemble()
    {
        reset();
    }
    ~ChorusEnsemble() = default;

    void prepare(double sampleRate)
    {
        currentSampleRate = sampleRate > 0.0 ? sampleRate : 44100.0;
        totalsamples = (int)(currentSampleRate * 0.2); // ~200ms delay buffer
        if (totalsamples < 256) totalsamples = 256;
        dL.assign(totalsamples * 2 + 10, 0.0);
        dR.assign(totalsamples * 2 + 10, 0.0);
        reset();
    }

    void reset()
    {
        gcount = 0;
        sweep = 0.0;
        fpFlip = false;
        airPrevL = airEvenL = airOddL = airFactorL = 0.0;
        airPrevR = airEvenR = airOddR = airFactorR = 0.0;
        fpdL = 1;
        while (fpdL < 16386) fpdL = (uint32_t)rand() * UINT32_MAX;
        fpdR = 1;
        while (fpdR < 16386) fpdR = (uint32_t)rand() * UINT32_MAX;
    }

    void processStereo(float* leftChannel, float* rightChannel, int numSamples, float rateHz, float depth01, float mix01)
    {
        if (mix01 <= 0.0001f || dL.empty())
            return;

        const double speed = ((double)rateHz / currentSampleRate) * 6.283185307179586476;
        const int loopLimit = (int)(totalsamples * 0.49);
        const double range = (0.002 + (double)depth01 * 0.015) * currentSampleRate; // 2ms to 17ms modulation
        const double wet = (double)mix01;
        const double dry = 1.0 - (wet * 0.5); // Constant power blend

        const double start[4] = { range, range * 1.5, range * 2.0, range * 2.5 };

        for (int i = 0; i < numSamples; ++i)
        {
            double inputSampleL = leftChannel[i];
            double inputSampleR = rightChannel[i];
            double drySampleL = inputSampleL;
            double drySampleR = inputSampleR;

            if (gcount < 1 || gcount > loopLimit) { gcount = loopLimit; }
            int count = gcount;
            dL[count + loopLimit] = dL[count] = inputSampleL;
            dR[count + loopLimit] = dR[count] = inputSampleR;
            gcount--;

            sweep += speed;
            if (sweep > 6.283185307179586476) sweep -= 6.283185307179586476;

            // 4-voice multi-tap chorus modulation
            double wetL = 0.0;
            double wetR = 0.0;

            for (int v = 0; v < 4; ++v)
            {
                double phaseOffset = (double)v * 1.5707963267948966; // 90 degree voice offsets
                double modL = start[v] + (range * 0.5 * std::sin(sweep + phaseOffset));
                double modR = start[v] + (range * 0.5 * std::cos(sweep + phaseOffset));

                int offsetIntL = (int)std::floor(modL);
                double fracL = modL - (double)offsetIntL;
                int cL = gcount + offsetIntL;
                wetL += dL[cL] * (1.0 - fracL) + dL[cL + 1] * fracL;

                int offsetIntR = (int)std::floor(modR);
                double fracR = modR - (double)offsetIntR;
                int cR = gcount + offsetIntR;
                wetR += dR[cR] * (1.0 - fracR) + dR[cR + 1] * fracR;
            }

            wetL *= 0.25;
            wetR *= 0.25;

            leftChannel[i] = (float)(drySampleL * dry + wetL * wet);
            rightChannel[i] = (float)(drySampleR * dry + wetR * wet);
        }
    }

private:
    double currentSampleRate{ 44100.0 };
    int totalsamples{ 8820 };
    std::vector<double> dL;
    std::vector<double> dR;
    int gcount{ 0 };
    double sweep{ 0.0 };
    bool fpFlip{ false };
    double airPrevL{ 0.0 }, airEvenL{ 0.0 }, airOddL{ 0.0 }, airFactorL{ 0.0 };
    double airPrevR{ 0.0 }, airEvenR{ 0.0 }, airOddR{ 0.0 }, airFactorR{ 0.0 };
    uint32_t fpdL{ 1 };
    uint32_t fpdR{ 1 };
};

} // namespace Airwindows
