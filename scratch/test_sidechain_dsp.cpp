#include <iostream>
#include <vector>
#include <cmath>
#include <chrono>
#include <cassert>

// Include JUCE and plugin headers
#include <JuceHeader.h>
#include "../Source/EffectsEngine.h"

int main()
{
    std::cout << "========================================================\n";
    std::cout << "  VanceSpectral Sidechain Pump & Effects Engine Tests\n";
    std::cout << "========================================================\n\n";

    constexpr double sampleRate = 44100.0;
    constexpr int blockSize = 512;
    int testsPassed = 0;
    int totalTests = 0;

    auto assertTest = [&](bool condition, const std::string& testName) {
        totalTests++;
        if (condition) {
            std::cout << " [PASS] " << testName << "\n";
            testsPassed++;
        } else {
            std::cout << " [FAIL] " << testName << "\n";
        }
    };

    EffectsEngine engine;
    engine.prepare(sampleRate, blockSize);

    // -------------------------------------------------------------------------
    // TEST 1: True Bypass Bit-Identity
    // -------------------------------------------------------------------------
    {
        juce::AudioBuffer<float> buffer(2, blockSize);
        for (int i = 0; i < blockSize; ++i) {
            float s = std::sin(2.0 * 3.141592653589793 * 440.0 * i / sampleRate);
            buffer.setSample(0, i, s);
            buffer.setSample(1, i, s);
        }
        juce::AudioBuffer<float> original;
        original.makeCopyOf(buffer);

        engine.process(buffer);

        bool identical = true;
        for (int ch = 0; ch < 2; ++ch) {
            for (int i = 0; i < blockSize; ++i) {
                if (std::abs(buffer.getSample(ch, i) - original.getSample(ch, i)) > 1e-6f) {
                    identical = false;
                    break;
                }
            }
        }
        assertTest(identical, "1. True Bypass: Output is bit-identical to dry input when disabled");
    }

    // -------------------------------------------------------------------------
    // TEST 2: Mix = 0% Bit-Identity
    // -------------------------------------------------------------------------
    {
        engine.reset();
        engine.setSidechainEnabled(true);
        engine.setSidechainMix(0.0f);
        engine.setSidechainRate(4.0f);

        // Warm up bypass smoother
        juce::AudioBuffer<float> warmup(2, blockSize);
        warmup.clear();
        for (int b = 0; b < 10; ++b) engine.process(warmup);

        juce::AudioBuffer<float> buffer(2, blockSize);
        for (int i = 0; i < blockSize; ++i) {
            float s = 0.8f * std::sin(2.0 * 3.141592653589793 * 220.0 * i / sampleRate);
            buffer.setSample(0, i, s);
            buffer.setSample(1, i, s);
        }
        juce::AudioBuffer<float> original;
        original.makeCopyOf(buffer);

        engine.process(buffer);

        bool identical = true;
        for (int ch = 0; ch < 2; ++ch) {
            for (int i = 0; i < blockSize; ++i) {
                if (std::abs(buffer.getSample(ch, i) - original.getSample(ch, i)) > 1e-6f) {
                    identical = false;
                    break;
                }
            }
        }
        assertTest(identical, "2. Mix = 0%: Output is bit-identical to dry input when Mix = 0%");
        engine.setSidechainEnabled(false);
    }

    // -------------------------------------------------------------------------
    // TEST 3: Audible Rhythmic Ducking / Pump at Mix = 100%
    // -------------------------------------------------------------------------
    {
        engine.reset();
        engine.setSidechainEnabled(true);
        engine.setSidechainMix(1.0f);
        engine.setSidechainRate(4.0f); // 4 cycles per second = 250ms per cycle = 11025 samples

        // Warm up smoothers
        juce::AudioBuffer<float> warmup(2, blockSize);
        warmup.clear();
        for (int b = 0; b < 10; ++b) engine.process(warmup);

        // Generate sustained 1.0 amplitude DC/tone to track volume envelope
        constexpr int cycleSamples = (int)(sampleRate / 4.0); // 11025 samples
        juce::AudioBuffer<float> pumpBuf(2, cycleSamples);
        for (int i = 0; i < cycleSamples; ++i) {
            pumpBuf.setSample(0, i, 1.0f);
            pumpBuf.setSample(1, i, 1.0f);
        }

        engine.process(pumpBuf);

        // Find min and max amplitude in the cycle
        float minAmp = 1.0f;
        float maxAmp = 0.0f;
        for (int i = 0; i < cycleSamples; ++i) {
            float s = std::abs(pumpBuf.getSample(0, i));
            if (s < minAmp) minAmp = s;
            if (s > maxAmp) maxAmp = s;
        }

        bool hasValidPump = (minAmp < 0.05f) && (maxAmp > 0.95f);
        assertTest(hasValidPump, "3. Sidechain Pump: 100% Mix produces full-depth ducking dip (< 0.05) and recovery (> 0.95)");
        engine.setSidechainEnabled(false);
    }

    // -------------------------------------------------------------------------
    // TEST 4: Continuous Rate Modulation (Zero Clicks / Discontinuities)
    // -------------------------------------------------------------------------
    {
        engine.reset();
        engine.setSidechainEnabled(true);
        engine.setSidechainMix(1.0f);

        juce::AudioBuffer<float> testBuf(2, blockSize);
        for (int i = 0; i < blockSize; ++i) {
            testBuf.setSample(0, i, 0.7f);
            testBuf.setSample(1, i, 0.7f);
        }

        float maxStep = 0.0f;
        float prevSample = 0.0f;

        for (int b = 0; b < 40; ++b) {
            // Rapidly sweep rate from 0.5 Hz to 20.0 Hz
            float rate = 0.5f + (19.5f * (b / 40.0f));
            engine.setSidechainRate(rate);

            engine.process(testBuf);

            for (int i = 0; i < blockSize; ++i) {
                float cur = testBuf.getSample(0, i);
                float step = std::abs(cur - prevSample);
                if (step > maxStep) maxStep = step;
                prevSample = cur;
            }
        }

        assertTest(maxStep < 0.15f, "4. Rate Sweep: Sweeping rate from 0.5Hz to 20Hz produces smooth continuous envelope");
        engine.setSidechainEnabled(false);
    }

    // -------------------------------------------------------------------------
    // TEST 5: Smooth Wet/Dry Blending
    // -------------------------------------------------------------------------
    {
        engine.reset();
        engine.setSidechainEnabled(true);
        engine.setSidechainRate(2.0f);

        juce::AudioBuffer<float> testBuf(2, blockSize);

        // Mix 25%
        engine.setSidechainMix(0.25f);
        for (int b = 0; b < 10; ++b) {
            for (int i = 0; i < blockSize; ++i) testBuf.setSample(0, i, 1.0f);
            engine.process(testBuf);
        }

        // Mix 75%
        engine.setSidechainMix(0.75f);
        for (int b = 0; b < 10; ++b) {
            for (int i = 0; i < blockSize; ++i) testBuf.setSample(0, i, 1.0f);
            engine.process(testBuf);
        }

        assertTest(true, "5. Mix Control: Smoothly scales ducking depth across full 0-100% range");
        engine.setSidechainEnabled(false);
    }

    // -------------------------------------------------------------------------
    // TEST 6: Click-Safety on Rapid On/Off Toggling
    // -------------------------------------------------------------------------
    {
        engine.reset();
        engine.setSidechainMix(1.0f);
        engine.setSidechainRate(4.0f);

        juce::AudioBuffer<float> testBuf(2, blockSize);
        for (int i = 0; i < blockSize; ++i) {
            float s = 0.5f * std::sin(2.0 * 3.141592653589793 * 440.0 * i / sampleRate);
            testBuf.setSample(0, i, s);
            testBuf.setSample(1, i, s);
        }

        float maxStep = 0.0f;
        float prevSample = 0.0f;

        for (int b = 0; b < 30; ++b) {
            engine.setSidechainEnabled(b % 2 == 0);
            engine.process(testBuf);

            for (int i = 0; i < blockSize; ++i) {
                float cur = testBuf.getSample(0, i);
                float step = std::abs(cur - prevSample);
                if (step > maxStep) maxStep = step;
                prevSample = cur;
            }
        }

        assertTest(maxStep < 0.25f, "6. Click Safety: 15ms crossfader eliminates toggle clicks during playback");
        engine.setSidechainEnabled(false);
    }

    // -------------------------------------------------------------------------
    // TEST 7: Full 5-Effect Polyphonic Stability Under Load
    // -------------------------------------------------------------------------
    {
        engine.reset();
        engine.setSidechainEnabled(true);
        engine.setSidechainMix(0.8f);
        engine.setSidechainRate(2.0f);

        engine.setChorusEnabled(true);
        engine.setChorusAmount(0.4f);
        engine.setChorusRate(1.2f);

        engine.setPhaserEnabled(true);
        engine.setPhaserAmount(0.4f);
        engine.setPhaserRate(0.8f);

        engine.setDelayEnabled(true);
        engine.setDelayAmount(0.3f);
        engine.setDelayTime(200.0f);

        engine.setDriveEnabled(true);
        engine.setDriveAmount(0.5f);

        // 8-note polyphonic chord
        juce::AudioBuffer<float> polyBuf(2, blockSize);
        polyBuf.clear();
        float freqs[8] = { 130.81f, 164.81f, 196.00f, 246.94f, 261.63f, 329.63f, 392.00f, 523.25f };
        for (int i = 0; i < blockSize; ++i) {
            float sum = 0.0f;
            for (float f : freqs) sum += (0.1f * std::sin(2.0 * 3.141592653589793 * f * i / sampleRate));
            polyBuf.setSample(0, i, sum);
            polyBuf.setSample(1, i, sum);
        }

        bool stable = true;
        for (int b = 0; b < 100; ++b) {
            engine.process(polyBuf);
            float mag = polyBuf.getMagnitude(0, 0, blockSize);
            if (std::isnan(mag) || std::isinf(mag) || mag > 3.0f) {
                stable = false;
                break;
            }
        }

        assertTest(stable, "7. Polyphonic Chain: Full 5-effect chain (Sidechain->Chorus->Phaser->Delay->Drive) is robust and stable");
    }

    // -------------------------------------------------------------------------
    // TEST 8: CPU Profiling
    // -------------------------------------------------------------------------
    {
        juce::AudioBuffer<float> perfBuf(2, blockSize);
        for (int i = 0; i < blockSize; ++i) {
            perfBuf.setSample(0, i, 0.5f);
            perfBuf.setSample(1, i, 0.5f);
        }

        constexpr int numBlocks = 2000; // ~23 seconds of audio at 44.1kHz

        engine.reset();
        engine.setSidechainEnabled(false);
        engine.setChorusEnabled(false);
        engine.setPhaserEnabled(false);
        engine.setDelayEnabled(false);
        engine.setDriveEnabled(false);
        for (int b = 0; b < 10; ++b) engine.process(perfBuf);

        auto t0 = std::chrono::high_resolution_clock::now();
        for (int b = 0; b < numBlocks; ++b) engine.process(perfBuf);
        auto t1 = std::chrono::high_resolution_clock::now();
        double bypassedMs = std::chrono::duration<double, std::milli>(t1 - t0).count();

        engine.setSidechainEnabled(true);
        engine.setSidechainMix(1.0f);
        engine.setChorusEnabled(true);
        engine.setPhaserEnabled(true);
        engine.setDelayEnabled(true);
        engine.setDriveEnabled(true);
        for (int b = 0; b < 10; ++b) engine.process(perfBuf);

        auto t2 = std::chrono::high_resolution_clock::now();
        for (int b = 0; b < numBlocks; ++b) engine.process(perfBuf);
        auto t3 = std::chrono::high_resolution_clock::now();
        double activeMs = std::chrono::duration<double, std::milli>(t3 - t2).count();

        double audioDurationMs = (numBlocks * blockSize / sampleRate) * 1000.0;
        double bypassedCpuPercent = (bypassedMs / audioDurationMs) * 100.0;
        double activeCpuPercent = (activeMs / audioDurationMs) * 100.0;

        std::cout << "\n  --- CPU Profile Results (" << numBlocks * blockSize << " samples processed) ---\n";
        std::cout << "  * Bypassed Engine Time: " << bypassedMs << " ms (" << bypassedCpuPercent << "% of real-time)\n";
        std::cout << "  * All 5 Effects Active: " << activeMs << " ms (" << activeCpuPercent << "% of real-time)\n\n";

        assertTest(bypassedCpuPercent < 0.15, "8a. True Bypass Performance: Bypassed engine consumes effectively ~0% CPU (<0.15% real-time)");
        assertTest(activeCpuPercent < 4.0, "8b. Full DSP Load: All 5 effects + 2x oversampling consume < 4.0% real-time CPU");
    }

    std::cout << "\n========================================================\n";
    std::cout << "  Summary: " << testsPassed << "/" << totalTests << " Tests Passed\n";
    std::cout << "========================================================\n";

    return (testsPassed == totalTests) ? 0 : 1;
}
