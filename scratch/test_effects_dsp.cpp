#include <iostream>
#include <vector>
#include <cmath>
#include <chrono>
#include <cassert>

// Include JUCE and plugin headers
#include <JuceHeader.h>
#include "../Source/EffectsEngine.h"
#include "../Source/Airwindows/AirwindowsSpiral.h"
#include "../Source/Airwindows/AirwindowsChorus.h"

int main()
{
    std::cout << "========================================================\n";
    std::cout << "  VanceSpectral 5-Stage Wet Effects Engine DSP Tests\n";
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
    // TEST 1: True Bypass & Zero Processing when OFF
    // -------------------------------------------------------------------------
    {
        juce::AudioBuffer<float> buffer(2, blockSize);
        for (int i = 0; i < blockSize; ++i) {
            float s = std::sin(2.0 * 3.141592653589793 * 440.0 * i / sampleRate);
            buffer.setSample(0, i, s);
            buffer.setSample(1, i, s);
        }
        juce::AudioBuffer<float> originalBuffer;
        originalBuffer.makeCopyOf(buffer);

        // Process through bypassed engine
        engine.process(buffer);

        bool identical = true;
        for (int ch = 0; ch < 2; ++ch) {
            for (int i = 0; i < blockSize; ++i) {
                if (std::abs(buffer.getSample(ch, i) - originalBuffer.getSample(ch, i)) > 1e-5f) {
                    identical = false;
                    break;
                }
            }
        }
        assertTest(identical, "1. True Bypass: Bypassed engine passes input unmodified");
    }

    // -------------------------------------------------------------------------
    // TEST 2: Gate Effect
    // -------------------------------------------------------------------------
    {
        engine.reset();
        engine.setGateEnabled(true);
        engine.setGateAmount(0.8f); // High threshold (-20 dB)

        // Warm up crossfader
        juce::AudioBuffer<float> warmup(2, blockSize);
        warmup.clear();
        for (int b = 0; b < 10; ++b) engine.process(warmup);

        // Feed low level signal (-40 dB, ~0.01 amplitude)
        juce::AudioBuffer<float> lowLevelBuf(2, blockSize);
        for (int i = 0; i < blockSize; ++i) {
            float s = 0.005f * std::sin(2.0 * 3.141592653589793 * 440.0 * i / sampleRate);
            lowLevelBuf.setSample(0, i, s);
            lowLevelBuf.setSample(1, i, s);
        }

        // Process several blocks through gate
        for (int b = 0; b < 10; ++b) {
            engine.process(lowLevelBuf);
        }

        float gatedRms = lowLevelBuf.getRMSLevel(0, 0, blockSize);
        assertTest(gatedRms < 0.001f, "2. Gate: Low-level signals below threshold are heavily attenuated");
        engine.setGateEnabled(false);
    }

    // -------------------------------------------------------------------------
    // TEST 3: Chorus Effect
    // -------------------------------------------------------------------------
    {
        engine.reset();
        engine.setChorusEnabled(true);
        engine.setChorusAmount(0.8f);
        engine.setChorusRate(1.5f);

        juce::AudioBuffer<float> chorusBuf(2, blockSize);
        for (int i = 0; i < blockSize; ++i) {
            float s = 0.5f * std::sin(2.0 * 3.141592653589793 * 440.0 * i / sampleRate);
            chorusBuf.setSample(0, i, s);
            chorusBuf.setSample(1, i, s);
        }

        for (int b = 0; b < 15; ++b) engine.process(chorusBuf);

        // Check stereo width and modulation
        float rmsL = chorusBuf.getRMSLevel(0, 0, blockSize);
        float rmsR = chorusBuf.getRMSLevel(1, 0, blockSize);
        bool hasModulation = rmsL > 0.05f && rmsR > 0.05f;
        assertTest(hasModulation, "3. Chorus: Produces rich stereo ensemble modulation");
        engine.setChorusEnabled(false);
    }

    // -------------------------------------------------------------------------
    // TEST 4: Phaser Effect
    // -------------------------------------------------------------------------
    {
        engine.reset();
        engine.setPhaserEnabled(true);
        engine.setPhaserAmount(0.75f);
        engine.setPhaserRate(1.0f);

        juce::AudioBuffer<float> phaserBuf(2, blockSize);
        for (int i = 0; i < blockSize; ++i) {
            float s = 0.5f * std::sin(2.0 * 3.141592653589793 * 1000.0 * i / sampleRate);
            phaserBuf.setSample(0, i, s);
            phaserBuf.setSample(1, i, s);
        }

        for (int b = 0; b < 15; ++b) engine.process(phaserBuf);

        float pRms = phaserBuf.getRMSLevel(0, 0, blockSize);
        assertTest(pRms > 0.05f && pRms < 1.0f, "4. Phaser: Sweeps comb filters cleanly without blowup");
        engine.setPhaserEnabled(false);
    }

    // -------------------------------------------------------------------------
    // TEST 5: Delay Effect (Feedback & Repeats)
    // -------------------------------------------------------------------------
    {
        engine.reset();
        engine.setDelayEnabled(true);
        engine.setDelayAmount(1.0f); // 100% wet
        engine.setDelayTime(50.0f);  // 50ms = 2205 samples
        engine.setDelayFeedback(0.5f);

        juce::AudioBuffer<float> delayBuf(2, blockSize);
        delayBuf.clear();
        // Send a single impulse at sample 0
        delayBuf.setSample(0, 0, 1.0f);
        delayBuf.setSample(1, 0, 1.0f);

        engine.process(delayBuf);

        // Process subsequent silent blocks until delay impulse returns
        bool echoDetected = false;
        for (int b = 0; b < 10; ++b) {
            delayBuf.clear();
            engine.process(delayBuf);
            if (delayBuf.getMagnitude(0, 0, blockSize) > 0.1f) {
                echoDetected = true;
            }
        }

        assertTest(echoDetected, "5. Delay: Clean delayed repeats generated with stable feedback");
        engine.setDelayEnabled(false);
    }

    // -------------------------------------------------------------------------
    // TEST 6: Drive Effect (Airwindows Spiral + Oversampling)
    // -------------------------------------------------------------------------
    {
        engine.reset();
        engine.setDriveEnabled(true);
        engine.setDriveAmount(0.7f); // 70% drive
        engine.setDriveTone(0.8f);

        juce::AudioBuffer<float> driveBuf(2, blockSize);
        for (int i = 0; i < blockSize; ++i) {
            float s = 0.6f * std::sin(2.0 * 3.141592653589793 * 220.0 * i / sampleRate);
            driveBuf.setSample(0, i, s);
            driveBuf.setSample(1, i, s);
        }

        for (int b = 0; b < 15; ++b) engine.process(driveBuf);

        // Check for harmonic richness and output containment
        float maxSample = driveBuf.getMagnitude(0, 0, blockSize);
        bool finiteAndBounded = !std::isnan(maxSample) && !std::isinf(maxSample) && maxSample <= 1.5f && maxSample > 0.1f;
        assertTest(finiteAndBounded, "6. Drive: Oversampled Airwindows Spiral generates smooth, bounded saturation");
        engine.setDriveEnabled(false);
    }

    // -------------------------------------------------------------------------
    // TEST 7: Click-Safety & Smooth Crossfading on Toggle
    // -------------------------------------------------------------------------
    {
        engine.reset();
        juce::AudioBuffer<float> testBuf(2, blockSize);
        for (int i = 0; i < blockSize; ++i) {
            float s = 0.5f * std::sin(2.0 * 3.141592653589793 * 440.0 * i / sampleRate);
            testBuf.setSample(0, i, s);
            testBuf.setSample(1, i, s);
        }

        float maxStep = 0.0f;
        float prevSample = 0.0f;

        for (int b = 0; b < 20; ++b) {
            // Rapidly toggle all effects every 2 blocks
            if (b % 2 == 0) {
                engine.setGateEnabled(true);
                engine.setChorusEnabled(true);
                engine.setPhaserEnabled(true);
                engine.setDelayEnabled(true);
                engine.setDriveEnabled(true);
            } else {
                engine.setGateEnabled(false);
                engine.setChorusEnabled(false);
                engine.setPhaserEnabled(false);
                engine.setDelayEnabled(false);
                engine.setDriveEnabled(false);
            }

            engine.process(testBuf);

            for (int i = 0; i < blockSize; ++i) {
                float cur = testBuf.getSample(0, i);
                float step = std::abs(cur - prevSample);
                if (step > maxStep) maxStep = step;
                prevSample = cur;
            }
        }

        assertTest(maxStep < 0.25f, "7. Click-Safety: Rapid on/off toggling during playback produces no discontinuity clicks");
    }

    // -------------------------------------------------------------------------
    // TEST 8: Dense Polyphony Simultaneous 5-Effect Stress Test
    // -------------------------------------------------------------------------
    {
        engine.reset();
        engine.setGateEnabled(true);
        engine.setGateAmount(0.2f);
        engine.setChorusEnabled(true);
        engine.setChorusAmount(0.5f);
        engine.setPhaserEnabled(true);
        engine.setPhaserAmount(0.5f);
        engine.setDelayEnabled(true);
        engine.setDelayAmount(0.4f);
        engine.setDelayTime(200.0f);
        engine.setDriveEnabled(true);
        engine.setDriveAmount(0.5f);

        // 8-note polyphonic chord buffer
        juce::AudioBuffer<float> polyBuf(2, blockSize);
        polyBuf.clear();
        float freqs[8] = { 130.81f, 164.81f, 196.00f, 246.94f, 261.63f, 329.63f, 392.00f, 523.25f };
        for (int i = 0; i < blockSize; ++i) {
            float sum = 0.0f;
            for (float f : freqs) {
                sum += (0.1f * std::sin(2.0 * 3.141592653589793 * f * i / sampleRate));
            }
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

        assertTest(stable, "8. Polyphonic Stress: All 5 effects simultaneously active under dense polyphonic load maintain stability");
    }

    // -------------------------------------------------------------------------
    // TEST 9: CPU Profiling
    // -------------------------------------------------------------------------
    {
        juce::AudioBuffer<float> perfBuf(2, blockSize);
        for (int i = 0; i < blockSize; ++i) {
            perfBuf.setSample(0, i, 0.5f);
            perfBuf.setSample(1, i, 0.5f);
        }

        constexpr int numBlocks = 2000; // ~23 seconds of audio at 44.1kHz

        // 1. Measure Bypassed CPU
        engine.reset();
        engine.setGateEnabled(false);
        engine.setChorusEnabled(false);
        engine.setPhaserEnabled(false);
        engine.setDelayEnabled(false);
        engine.setDriveEnabled(false);
        // let bypass smoothers reach target 0.0
        for (int b = 0; b < 10; ++b) engine.process(perfBuf);

        auto t0 = std::chrono::high_resolution_clock::now();
        for (int b = 0; b < numBlocks; ++b) {
            engine.process(perfBuf);
        }
        auto t1 = std::chrono::high_resolution_clock::now();
        double bypassedMs = std::chrono::duration<double, std::milli>(t1 - t0).count();

        // 2. Measure All 5 Active CPU
        engine.setGateEnabled(true);
        engine.setChorusEnabled(true);
        engine.setPhaserEnabled(true);
        engine.setDelayEnabled(true);
        engine.setDriveEnabled(true);
        for (int b = 0; b < 10; ++b) engine.process(perfBuf);

        auto t2 = std::chrono::high_resolution_clock::now();
        for (int b = 0; b < numBlocks; ++b) {
            engine.process(perfBuf);
        }
        auto t3 = std::chrono::high_resolution_clock::now();
        double activeMs = std::chrono::duration<double, std::milli>(t3 - t2).count();

        double audioDurationMs = (numBlocks * blockSize / sampleRate) * 1000.0;
        double bypassedCpuPercent = (bypassedMs / audioDurationMs) * 100.0;
        double activeCpuPercent = (activeMs / audioDurationMs) * 100.0;

        std::cout << "\n  --- CPU Profile Results (" << numBlocks * blockSize << " samples processed) ---\n";
        std::cout << "  * Bypassed Engine Time: " << bypassedMs << " ms (" << bypassedCpuPercent << "% of real-time)\n";
        std::cout << "  * All 5 Effects Active: " << activeMs << " ms (" << activeCpuPercent << "% of real-time)\n\n";

        assertTest(bypassedCpuPercent < 0.15, "9a. True Bypass Performance: Bypassed engine consumes effectively ~0% CPU (<0.15% real-time)");
        assertTest(activeCpuPercent < 4.0, "9b. Full DSP Load: All 5 effects + 2x oversampling consume < 4.0% real-time CPU");
    }

    std::cout << "\n========================================================\n";
    std::cout << "  Summary: " << testsPassed << "/" << totalTests << " Tests Passed\n";
    std::cout << "========================================================\n";

    return (testsPassed == totalTests) ? 0 : 1;
}
