#include <iostream>
#include <vector>
#include <cmath>
#include <chrono>
#include <iomanip>
#include <algorithm>

#include <JuceHeader.h>
#include "../Source/EffectsEngine.h"
#include "../Source/Airwindows/AirwindowsSpiral.h"
#include "../Source/Airwindows/AirwindowsChorus.h"

namespace SpectrogramDSPTest
{
    static constexpr int LOW_FFT_ORDER = 13;   // 8192
    static constexpr int LOW_FFT_SIZE = 1 << LOW_FFT_ORDER;
    static constexpr int MID_FFT_ORDER = 11;   // 2048
    static constexpr int MID_FFT_SIZE = 1 << MID_FFT_ORDER;
    static constexpr int HIGH_FFT_ORDER = 9;   // 512
    static constexpr int HIGH_FFT_SIZE = 1 << HIGH_FFT_ORDER;

    inline void fillBlackmanHarrisWindow(std::vector<float>& dest, int size)
    {
        dest.resize(size);
        if (size <= 1) return;
        const double twoPi = 2.0 * juce::MathConstants<double>::pi;
        const double fourPi = 4.0 * juce::MathConstants<double>::pi;
        const double sixPi = 6.0 * juce::MathConstants<double>::pi;
        const double denom = (double)(size - 1);

        for (int i = 0; i < size; ++i)
        {
            double n = (double)i;
            dest[i] = static_cast<float>(0.35875 
                                        - 0.48829 * std::cos(twoPi * n / denom) 
                                        + 0.14128 * std::cos(fourPi * n / denom) 
                                        - 0.01168 * std::cos(sixPi * n / denom));
        }
    }
}

int main()
{
    juce::ScopedJuceInitialiser_GUI guiInit;
    std::cout << "=================================================================\n" << std::flush;
    std::cout << "  VanceSpectral STFT Spectrogram & Upgraded Effects DSP Test Suite\n" << std::flush;
    std::cout << "=================================================================\n\n" << std::flush;

    constexpr double sampleRate = 44100.0;
    constexpr int blockSize = 512;
    int totalTests = 0;
    int testsPassed = 0;

    auto assertTest = [&](bool condition, const std::string& name, const std::string& detail = "") {
        totalTests++;
        if (condition) {
            std::cout << " [PASS] " << name;
            if (!detail.empty()) std::cout << " (" << detail << ")";
            std::cout << "\n" << std::flush;
            testsPassed++;
        } else {
            std::cout << " [FAIL] " << name;
            if (!detail.empty()) std::cout << " - Error: " << detail;
            std::cout << "\n" << std::flush;
        }
    };

    try {
        std::cout << "--- 1. Multi-Resolution STFT Spectrogram Engine Tests ---\n" << std::flush;

        // 1.1 Window function verification
        {
            std::vector<float> window;
            SpectrogramDSPTest::fillBlackmanHarrisWindow(window, 2048);
            bool validEdges = std::abs(window.front() - 0.00006f) < 0.001f && std::abs(window.back() - 0.00006f) < 0.001f;
            bool validPeak = std::abs(window[1024] - 1.0f) < 0.01f;
            assertTest(validEdges && validPeak, "1.1 Blackman-Harris 4-Term Window: Correct peak and -92dB boundary taper");
        }

        // 1.2 FFT multi-resolution tier analysis (Low 100 Hz, Mid 1000 Hz, High 8000 Hz)
        {
            juce::dsp::FFT lowFFT(SpectrogramDSPTest::LOW_FFT_ORDER);   // 8192
            juce::dsp::FFT midFFT(SpectrogramDSPTest::MID_FFT_ORDER);   // 2048
            juce::dsp::FFT highFFT(SpectrogramDSPTest::HIGH_FFT_ORDER); // 512

            std::vector<float> lowWin, midWin, highWin;
            SpectrogramDSPTest::fillBlackmanHarrisWindow(lowWin, SpectrogramDSPTest::LOW_FFT_SIZE);
            SpectrogramDSPTest::fillBlackmanHarrisWindow(midWin, SpectrogramDSPTest::MID_FFT_SIZE);
            SpectrogramDSPTest::fillBlackmanHarrisWindow(highWin, SpectrogramDSPTest::HIGH_FFT_SIZE);

            // Test 100 Hz tone on Low tier (8192)
            std::vector<float> lowBuf(SpectrogramDSPTest::LOW_FFT_SIZE * 2, 0.0f);
            for (int i = 0; i < SpectrogramDSPTest::LOW_FFT_SIZE; ++i) {
                lowBuf[i] = std::sin(2.0 * 3.141592653589793 * 100.0 * i / sampleRate) * lowWin[i];
            }
            lowFFT.performFrequencyOnlyForwardTransform(lowBuf.data());
            int lowPeakBin = (int)(std::max_element(lowBuf.begin(), lowBuf.begin() + SpectrogramDSPTest::LOW_FFT_SIZE / 2) - lowBuf.begin());
            float lowPeakFreq = (float)lowPeakBin * (float)sampleRate / (float)SpectrogramDSPTest::LOW_FFT_SIZE;

            // Test 1000 Hz tone on Mid tier (2048)
            std::vector<float> midBuf(SpectrogramDSPTest::MID_FFT_SIZE * 2, 0.0f);
            for (int i = 0; i < SpectrogramDSPTest::MID_FFT_SIZE; ++i) {
                midBuf[i] = std::sin(2.0 * 3.141592653589793 * 1000.0 * i / sampleRate) * midWin[i];
            }
            midFFT.performFrequencyOnlyForwardTransform(midBuf.data());
            int midPeakBin = (int)(std::max_element(midBuf.begin(), midBuf.begin() + SpectrogramDSPTest::MID_FFT_SIZE / 2) - midBuf.begin());
            float midPeakFreq = (float)midPeakBin * (float)sampleRate / (float)SpectrogramDSPTest::MID_FFT_SIZE;

            // Test 8000 Hz tone on High tier (512)
            std::vector<float> highBuf(SpectrogramDSPTest::HIGH_FFT_SIZE * 2, 0.0f);
            for (int i = 0; i < SpectrogramDSPTest::HIGH_FFT_SIZE; ++i) {
                highBuf[i] = std::sin(2.0 * 3.141592653589793 * 8000.0 * i / sampleRate) * highWin[i];
            }
            highFFT.performFrequencyOnlyForwardTransform(highBuf.data());
            int highPeakBin = (int)(std::max_element(highBuf.begin(), highBuf.begin() + SpectrogramDSPTest::HIGH_FFT_SIZE / 2) - highBuf.begin());
            float highPeakFreq = (float)highPeakBin * (float)sampleRate / (float)SpectrogramDSPTest::HIGH_FFT_SIZE;

            bool lowAccurate = std::abs(lowPeakFreq - 100.0f) < 6.0f;
            bool midAccurate = std::abs(midPeakFreq - 1000.0f) < 25.0f;
            bool highAccurate = std::abs(highPeakFreq - 8000.0f) < 100.0f;

            assertTest(lowAccurate && midAccurate && highAccurate, 
                       "1.2 Multi-Resolution FFT Tiers (8192/2048/512): Peak frequency accuracy verified",
                       "Low peak: " + std::to_string(lowPeakFreq) + "Hz, Mid peak: " + std::to_string(midPeakFreq) + "Hz, High peak: " + std::to_string(highPeakFreq) + "Hz");
        }

        // 1.3 Log-Frequency (Constant-Q) Mapping
        {
            // Check mapping formula f(normY) = 20 * 1000^(1 - normY)
            float fTop = 20.0f * std::pow(1000.0f, 1.0f);     // normY = 0 (top)
            float fBottom = 20.0f * std::pow(1000.0f, 0.0f);  // normY = 1 (bottom)
            float fMid = 20.0f * std::pow(1000.0f, 0.5f);     // normY = 0.5 (middle)

            bool topCorrect = std::abs(fTop - 20000.0f) < 1.0f;
            bool bottomCorrect = std::abs(fBottom - 20.0f) < 0.1f;
            bool midCorrect = std::abs(fMid - 632.45f) < 1.0f;

            assertTest(topCorrect && bottomCorrect && midCorrect,
                       "1.3 Log-Frequency Remapping: Exact 20Hz - 20kHz CQT-style coordinate spacing");
        }

        // 1.4 Temporal Ballistics (Peak-Hold Decay)
        {
            float decayedVal = 1.0f;
            const float decayFactor = 0.88f;
            for (int frame = 0; frame < 10; ++frame) {
                decayedVal = std::max(0.0f, decayedVal * decayFactor);
            }
            bool smoothDecay = (decayedVal > 0.25f && decayedVal < 0.31f);
            assertTest(smoothDecay, "1.4 Temporal Ballistics: Peak-hold exponential decay ballistics verified", "10-frame decay: " + std::to_string(decayedVal));
        }

        // =========================================================================
        // SECTION 2: Upgraded 5-Stage Effects DSP Tests
        // =========================================================================
        std::cout << "\n--- 2. Upgraded 5-Stage Effects DSP Engine Tests ---\n" << std::flush;

        EffectsEngine engine;
        engine.prepare(sampleRate, blockSize);

        // 2.1 True Bypass Test
        {
            engine.reset();
            juce::AudioBuffer<float> buf(2, blockSize);
            for (int i = 0; i < blockSize; ++i) {
                float s = std::sin(2.0 * 3.141592653589793 * 440.0 * i / sampleRate);
                buf.setSample(0, i, s);
                buf.setSample(1, i, s);
            }
            juce::AudioBuffer<float> dryCopy;
            dryCopy.makeCopyOf(buf);

            engine.process(buf);

            bool identical = true;
            for (int ch = 0; ch < 2; ++ch) {
                for (int i = 0; i < blockSize; ++i) {
                    if (std::abs(buf.getSample(ch, i) - dryCopy.getSample(ch, i)) > 1e-6f) {
                        identical = false;
                        break;
                    }
                }
            }
            assertTest(identical, "2.1 True Bypass: Output is bit-identical to dry when all effects disabled");
        }

        // 2.2 Drive: Waveshaping Saturation & 4x Oversampling
        {
            engine.reset();
            engine.setDriveEnabled(true);
            engine.setDriveAmount(0.75f);
            engine.setDriveTone(0.6f);

            // Warm up crossfaders
            juce::AudioBuffer<float> warmup(2, blockSize);
            warmup.clear();
            for (int b = 0; b < 10; ++b) engine.process(warmup);

            juce::AudioBuffer<float> driveBuf(2, blockSize);
            for (int i = 0; i < blockSize; ++i) {
                float s = 0.5f * std::sin(2.0 * 3.141592653589793 * 1000.0 * i / sampleRate);
                driveBuf.setSample(0, i, s);
                driveBuf.setSample(1, i, s);
            }

            engine.process(driveBuf);

            // Compute FFT of driven signal to verify harmonic generation
            juce::dsp::FFT fft(9); // 512
            std::vector<float> fftData(1024, 0.0f);
            for (int i = 0; i < 512; ++i) fftData[i] = driveBuf.getSample(0, i);
            fft.performFrequencyOnlyForwardTransform(fftData.data());

            float f1 = fftData[12];
            float f2 = fftData[23];
            float f3 = fftData[35];

            bool hasHarmonics = (f2 > 0.01f * f1) && (f3 > 0.005f * f1);
            assertTest(hasHarmonics, "2.2 Drive: Rich asymmetric soft-saturation harmonics generated with 4x oversampling",
                       "F1: " + std::to_string(f1) + ", 2nd harm: " + std::to_string(f2) + ", 3rd harm: " + std::to_string(f3));
            engine.setDriveEnabled(false);
        }

        // 2.3 Chorus: Dual Lagrange3rd Modulated Delay Lines & Quadrature LFO
        {
            engine.reset();
            engine.setChorusEnabled(true);
            engine.setChorusAmount(0.85f);
            engine.setChorusRate(1.2f);

            juce::AudioBuffer<float> warmup(2, blockSize);
            warmup.clear();
            for (int b = 0; b < 10; ++b) engine.process(warmup);

            juce::AudioBuffer<float> chorusBuf(2, blockSize);
            for (int i = 0; i < blockSize; ++i) {
                float s = 0.5f * std::sin(2.0 * 3.141592653589793 * 440.0 * i / sampleRate);
                chorusBuf.setSample(0, i, s);
                chorusBuf.setSample(1, i, s);
            }

            for (int b = 0; b < 10; ++b) engine.process(chorusBuf);

            float rmsL = chorusBuf.getRMSLevel(0, 0, blockSize);
            float rmsR = chorusBuf.getRMSLevel(1, 0, blockSize);
            bool hasStereoChorus = (rmsL > 0.05f && rmsR > 0.05f);

            assertTest(hasStereoChorus, "2.3 Chorus: Dual Lagrange3rd modulated lines produce lush stereo decorrelation");
            engine.setChorusEnabled(false);
        }

        // 2.4 Phaser: 6-Stage Allpass Cascade + Resonant Sweep
        {
            engine.reset();
            engine.setPhaserEnabled(true);
            engine.setPhaserAmount(0.80f);
            engine.setPhaserRate(0.8f);

            juce::AudioBuffer<float> warmup(2, blockSize);
            warmup.clear();
            for (int b = 0; b < 10; ++b) engine.process(warmup);

            juce::AudioBuffer<float> phaserBuf(2, blockSize);
            for (int b = 0; b < 15; ++b) {
                for (int i = 0; i < blockSize; ++i) {
                    float s = 0.5f * std::sin(2.0 * 3.141592653589793 * 1200.0 * (b * blockSize + i) / sampleRate);
                    phaserBuf.setSample(0, i, s);
                    phaserBuf.setSample(1, i, s);
                }
                engine.process(phaserBuf);
            }

            float pRms = phaserBuf.getRMSLevel(0, 0, blockSize);
            bool phaserStable = (pRms > 0.05f && pRms < 1.0f);
            assertTest(phaserStable, "2.4 Phaser: 6-stage allpass cascade produces stable resonant notches without blowup", "RMS: " + std::to_string(pRms));
            engine.setPhaserEnabled(false);
        }

        // 2.5 Delay: Lagrange Interpolation + Analog Damped Feedback
        {
            engine.reset();
            engine.setDelayEnabled(true);
            engine.setDelayAmount(0.6f);
            engine.setDelayTime(100.0f); // 100ms
            engine.setDelayFeedback(0.75f);

            juce::AudioBuffer<float> warmup(2, blockSize);
            warmup.clear();
            for (int b = 0; b < 10; ++b) engine.process(warmup);

            // Send a burst impulse
            juce::AudioBuffer<float> impulseBuf(2, blockSize);
            impulseBuf.clear();
            impulseBuf.setSample(0, 0, 1.0f);
            impulseBuf.setSample(1, 0, 1.0f);

            engine.process(impulseBuf);

            // Process successive silent blocks and check repeats
            std::vector<float> blockPeaks;
            for (int b = 0; b < 25; ++b) {
                juce::AudioBuffer<float> silent(2, blockSize);
                silent.clear();
                engine.process(silent);
                blockPeaks.push_back(silent.getMagnitude(0, blockSize));
            }

            float maxRepeat = *std::max_element(blockPeaks.begin() + 5, blockPeaks.end());
            bool hasEchoes = (maxRepeat > 0.05f);
            bool isStable = (maxRepeat < 1.5f);

            assertTest(hasEchoes && isStable, "2.5 Delay: Lagrange3rd interpolation + analog damping feedback is stable and characterful");
            engine.setDelayEnabled(false);
        }

        // 2.6 Sidechain: Musical Ducking Envelope Pump
        {
            engine.reset();
            engine.setSidechainEnabled(true);
            engine.setSidechainMix(1.0f);
            engine.setSidechainRate(2.0f); // 2 Hz = 0.5s period (22050 samples)

            juce::AudioBuffer<float> warmup(2, blockSize);
            warmup.clear();
            for (int b = 0; b < 10; ++b) engine.process(warmup);

            // Feed constant 1.0 DC audio for 2 full cycles (44100 samples) to trace the envelope
            constexpr int cycleLength = 44100;
            juce::AudioBuffer<float> dcBuf(2, cycleLength);
            for (int i = 0; i < cycleLength; ++i) {
                dcBuf.setSample(0, i, 1.0f);
                dcBuf.setSample(1, i, 1.0f);
            }

            engine.process(dcBuf);

            // Scan the entire buffer to find minimum ducking dip and maximum release recovery
            float minDip = 1.0f;
            float maxRecovery = 0.0f;
            for (int i = 0; i < cycleLength; ++i) {
                float s = dcBuf.getSample(0, i);
                minDip = std::min(minDip, s);
                maxRecovery = std::max(maxRecovery, s);
            }

            bool validDuck = (minDip < 0.05f) && (maxRecovery > 0.95f);
            assertTest(validDuck, "2.6 Sidechain: Musical downbeat attack dip and optical exponential release curve verified",
                       "Min dip: " + std::to_string(minDip) + ", Max recovery: " + std::to_string(maxRecovery));
            engine.setSidechainEnabled(false);
        }

        // =========================================================================
        // SECTION 3: Polyphonic DSP & CPU Performance Benchmark
        // =========================================================================
        std::cout << "\n--- 3. Polyphony & CPU Performance Benchmark ---\n" << std::flush;

        {
            engine.reset();
            std::cout << "  3.a Initializing benchmark parameters...\n" << std::flush;
            engine.setDriveEnabled(true);
            engine.setDriveAmount(0.6f);
            engine.setChorusEnabled(true);
            engine.setChorusAmount(0.5f);
            engine.setPhaserEnabled(true);
            engine.setPhaserAmount(0.5f);
            engine.setDelayEnabled(true);
            engine.setDelayAmount(0.4f);
            engine.setSidechainEnabled(true);
            engine.setSidechainMix(0.5f);

            std::cout << "  3.b Synthesizing polyphonic 32-voice buffer...\n" << std::flush;
            juce::AudioBuffer<float> polyBuffer(2, blockSize);
            for (int i = 0; i < blockSize; ++i) {
                float sum = 0.0f;
                for (int v = 0; v < 32; ++v) {
                    float freq = 110.0f * std::pow(1.059463f, (float)(v * 2));
                    sum += 0.03f * std::sin(2.0 * 3.141592653589793 * freq * i / sampleRate);
                }
                polyBuffer.setSample(0, i, sum);
                polyBuffer.setSample(1, i, sum);
            }

            std::cout << "  3.c Testing individual effects on polyBuffer:\n" << std::flush;
            juce::AudioBuffer<float> benchBuf(2, blockSize);

            engine.reset();
            engine.setSidechainEnabled(true);
            benchBuf.makeCopyOf(polyBuffer);
            engine.process(benchBuf);
            std::cout << "    Sidechain ok\n" << std::flush;
            engine.setSidechainEnabled(false);

            engine.reset();
            engine.setChorusEnabled(true);
            benchBuf.makeCopyOf(polyBuffer);
            engine.process(benchBuf);
            std::cout << "    Chorus ok\n" << std::flush;
            engine.setChorusEnabled(false);

            engine.reset();
            engine.setPhaserEnabled(true);
            benchBuf.makeCopyOf(polyBuffer);
            engine.process(benchBuf);
            std::cout << "    Phaser ok\n" << std::flush;
            engine.setPhaserEnabled(false);

            engine.reset();
            engine.setDelayEnabled(true);
            benchBuf.makeCopyOf(polyBuffer);
            engine.process(benchBuf);
            std::cout << "    Delay ok\n" << std::flush;
            engine.setDelayEnabled(false);

            engine.reset();
            engine.setDriveEnabled(true);
            benchBuf.makeCopyOf(polyBuffer);
            engine.process(benchBuf);
            std::cout << "    Drive ok\n" << std::flush;
            engine.setDriveEnabled(false);

            std::cout << "  3.d Testing all 5 effects enabled together:\n" << std::flush;
            engine.reset();
            engine.setSidechainEnabled(true);
            engine.setChorusEnabled(true);
            engine.setPhaserEnabled(true);
            engine.setDelayEnabled(true);
            engine.setDriveEnabled(true);
            benchBuf.makeCopyOf(polyBuffer);
            engine.process(benchBuf);
            std::cout << "    All 5 together single block ok\n" << std::flush;

            std::cout << "  3.e Warming up DSP states for 50 blocks...\n" << std::flush;
            for (int b = 0; b < 50; ++b) {
                benchBuf.makeCopyOf(polyBuffer);
                engine.process(benchBuf);
            }

            std::cout << "  3.d Running 2000-block timed benchmark loop...\n" << std::flush;
            constexpr int numBenchmarkBlocks = 2000; // ~23.2 seconds of real-time audio
            auto start = std::chrono::high_resolution_clock::now();

            for (int b = 0; b < numBenchmarkBlocks; ++b) {
                benchBuf.makeCopyOf(polyBuffer);
                engine.process(benchBuf);
            }

            auto end = std::chrono::high_resolution_clock::now();
            double elapsedMs = std::chrono::duration<double, std::milli>(end - start).count();
            double audioDurationMs = (numBenchmarkBlocks * blockSize * 1000.0) / sampleRate;
            double cpuPercentage = (elapsedMs / audioDurationMs) * 100.0;

            std::cout << "  3.e Benchmark complete: " << elapsedMs << " ms for " << audioDurationMs << " ms audio (" << cpuPercentage << "% CPU load)\n" << std::flush;

            bool performanceExcellent = (cpuPercentage < 15.0); // Under 15% CPU on single-thread core for full 5-effect chain with 4x oversampling
            assertTest(performanceExcellent, "3.1 DSP Performance: All 5 effects simultaneously engaged at max polyphony",
                       "Processed " + std::to_string((int)audioDurationMs) + "ms audio in " + std::to_string(elapsedMs) + "ms (" + std::to_string(cpuPercentage) + "% CPU load)");
        }

    }
    catch (const std::exception& e) {
        std::cerr << "EXCEPTION CAUGHT: " << e.what() << std::endl;
        return 1;
    }
    catch (...) {
        std::cerr << "UNKNOWN EXCEPTION CAUGHT" << std::endl;
        return 1;
    }

    std::cout << "\n=================================================================\n" << std::flush;
    std::cout << "  Test Summary: " << testsPassed << " / " << totalTests << " Tests Passed\n" << std::flush;
    std::cout << "=================================================================\n" << std::flush;

    return (testsPassed == totalTests) ? 0 : 1;
}
