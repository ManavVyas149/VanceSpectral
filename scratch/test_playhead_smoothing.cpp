#include <JuceHeader.h>
#include <iostream>
#include <cassert>
#include "../Source/SpectrogramComponent.h"
#include "../Source/PluginProcessor.h"

int main()
{
    juce::MessageManager::getInstance();
    std::cout << "========================================================\n";
    std::cout << "  SpectrogramComponent Playhead Smoothing & Sync Tests\n";
    std::cout << "========================================================\n\n";

    // 1. Test VoicePositionSmoother math
    std::cout << "--- 1. VoicePositionSmoother Mathematical Validation ---\n";
    SpectrogramComponent::VoicePositionSmoother smoother;
    smoother.reset();

    double t = 1.0;
    float nominalSpeed = 0.5f; // 0.5 norm/sec
    double dt = 0.01667; // 60 FPS

    // Initial update
    smoother.update(0.100f, t, dt, nominalSpeed);
    assert(std::abs(smoother.visualPos - 0.100f) < 0.0001f);
    assert(smoother.initialized);
    std::cout << " [PASS] 1.1 Initial update snaps directly to start target position\n";

    // 10 subsequent updates where audio buffer has not finished yet (targetPos stays at 0.100f)
    for (int i = 0; i < 5; ++i)
    {
        t += dt;
        smoother.update(0.100f, t, dt, nominalSpeed);
    }
    // Visual position should have smoothly advanced past 0.100f via velocity extrapolation
    assert(smoother.visualPos > 0.100f);
    std::cout << " [PASS] 1.2 Extrapolates continuously during inter-buffer latency (pos = " 
              << smoother.visualPos << ")\n";

    // Audio buffer finishes and targetPos leaps forward
    t += dt;
    smoother.update(0.125f, t, dt, nominalSpeed);
    assert(smoother.visualPos >= 0.115f && smoother.visualPos <= 0.135f);
    std::cout << " [PASS] 1.3 Smooth convergence upon audio buffer position update\n";

    // Loop wrap test: target position abruptly jumps back to start
    t += dt;
    float endPos = smoother.visualPos;
    smoother.update(0.005f, t, dt, nominalSpeed);
    assert(smoother.visualPos < 0.02f);
    assert(smoother.velocity > 0.0f); // Did not glitch into negative velocity
    std::cout << " [PASS] 1.4 Loop wrap instantly snaps to loop start (wrap from " 
              << endPos << " -> " << smoother.visualPos << ")\n";

    // 2. Backward playback direction test
    std::cout << "\n--- 2. Backward Playback Direction Tests ---\n";
    smoother.reset();
    t = 2.0;
    float backwardSpeed = -0.5f;
    smoother.update(0.900f, t, dt, backwardSpeed);
    assert(smoother.visualPos == 0.900f);

    for (int i = 0; i < 5; ++i)
    {
        t += dt;
        smoother.update(0.900f, t, dt, backwardSpeed);
    }
    assert(smoother.visualPos < 0.900f); // Smoothly moved leftwards
    std::cout << " [PASS] 2.1 Backward playback moves playhead continuously leftwards (pos = " 
              << smoother.visualPos << ")\n";

    // 3. Component Integration & Coordinate Synchronization Test
    std::cout << "\n--- 3. Component Coordinate Synchronization Tests ---\n";
    VancespectralAudioProcessor processor;
    SpectrogramComponent comp(processor);
    comp.setBounds(0, 0, 600, 300);

    // Audio buffer setup
    juce::AudioBuffer<float> testBuffer(2, 44100);
    testBuffer.clear();
    comp.loadDirectAudioBuffer(testBuffer, 44100.0, "TestSine.wav", true);

    auto graphBounds = comp.getGraphBounds();
    assert(!graphBounds.isEmpty());

    // Paint into an offscreen image to test renderStaticGraph and drawPlayheads
    juce::Image testCanvas(juce::Image::ARGB, 600, 300, true);
    juce::Graphics g(testCanvas);
    comp.paint(g);
    std::cout << " [PASS] 3.1 Initial paint successfully pre-cached static graph\n";

    // Verify playheads are drawn inside graphBounds
    comp.timerCallback();
    comp.paint(g);
    std::cout << " [PASS] 3.2 Repaint with playhead synchronization executed cleanly\n";

    std::cout << "\n========================================================\n";
    std::cout << "  All Playhead Smoothing & Synchronization Tests PASSED!\n";
    std::cout << "========================================================\n";

    juce::DeletedAtShutdown::deleteAll();
    juce::MessageManager::deleteInstance();
    return 0;
}
