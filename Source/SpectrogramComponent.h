#pragma once

#include <JuceHeader.h>
#include "SpectralUILookAndFeel.h"
#include "ToolbarComponent.h"

class VancespectralAudioProcessor;

struct SelectionRegion {
    int id = 1;
    ToolType type = ToolType::RectangleSelect;
    juce::Path normalizedPath;               // Normalized coordinates (0..1 time, 0..1 freq)
    juce::Rectangle<float> normalizedBounds; // Normalized bounds (0..1 time, 0..1 freq)
    bool isSelected = false;
};

class SpectrogramComponent : public juce::Component,
                             public juce::FileDragAndDropTarget,
                             public juce::ChangeListener,
                             public juce::Timer
{
public:
    SpectrogramComponent(VancespectralAudioProcessor&);
    ~SpectrogramComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void fileDragEnter(const juce::StringArray& files, int x, int y) override;
    void fileDragMove(const juce::StringArray& files, int x, int y) override;
    void fileDragExit(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;

    void loadAudioFile(const juce::File& file, bool isPartOfPresetLoad = false);
    void loadDirectAudioBuffer(const juce::AudioBuffer<float>& buffer, double sampleRate, const juce::String& fileName, bool isLooping);
    void restoreFromProcessorState();
    void restorePresetSnapshot(float startRegion, float endRegion, const juce::var& selectionsVar);
    void generateRandomSelections();
    juce::var getSelectionsAsVar() const;
    float getStartRegion() const { return startPosition; }
    float getEndRegion() const { return endPosition; }
    bool isLoopEnabled() const { return loopEnabled; }
    void setLoopEnabled(bool loop);
    juce::File getLoadedFile() const { return loadedFile; }
    const juce::AudioBuffer<float>& getAudioBuffer() const { return audioBuffer; }

    std::function<void()> onManualSampleLoaded;

    void changeListenerCallback(juce::ChangeBroadcaster*) override;
    void timerCallback() override;

    void mouseMove(const juce::MouseEvent& e) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    bool keyPressed(const juce::KeyPress& key) override;

    void setActiveTool(ToolType tool) { currentTool = tool; }
    ToolType getActiveTool() const { return currentTool; }

    bool isFileLoaded() const { return fileLoaded; }
    std::function<void(bool)> onFileLoadedStateChanged;

    juce::Rectangle<float> getGraphBounds() const;
    bool isWaveformCached() const { return !cachedWaveformEnvelope.isEmpty(); }

    struct VisualPlayhead {
        float positionNorm = 0.0f;
        float alpha = 1.0f;
    };

    struct VoicePositionSmoother {
        float visualPos = 0.0f;
        float lastTargetPos = 0.0f;
        float velocity = 0.0f;
        double lastTargetTimeSec = 0.0;
        bool initialized = false;

        void reset() {
            visualPos = 0.0f;
            lastTargetPos = 0.0f;
            velocity = 0.0f;
            lastTargetTimeSec = 0.0;
            initialized = false;
        }

        void update(float targetPos, double nowSec, double dt, float nominalSpeed) {
            if (!initialized) {
                visualPos = targetPos;
                lastTargetPos = targetPos;
                velocity = nominalSpeed;
                lastTargetTimeSec = nowSec;
                initialized = true;
                return;
            }

            float diff = targetPos - lastTargetPos;
            bool isDiscontinuous = std::abs(targetPos - visualPos) > 0.12f;

            // Detect loop wrap, restart, or abrupt jump
            if (isDiscontinuous || (diff < -0.05f && velocity > 0.0f) || (diff > 0.05f && velocity < 0.0f)) {
                visualPos = targetPos;
                lastTargetPos = targetPos;
                lastTargetTimeSec = nowSec;
                if (nominalSpeed < 0.0f)
                    velocity = -std::abs(velocity != 0.0f ? velocity : nominalSpeed);
                else
                    velocity = std::abs(velocity != 0.0f ? velocity : nominalSpeed);

                if (std::abs(velocity) < 0.0001f)
                    velocity = nominalSpeed;
                return;
            }

            if (std::abs(diff) > 0.00001f) {
                double timeSinceLast = nowSec - lastTargetTimeSec;
                if (timeSinceLast > 0.004 && timeSinceLast < 0.200) {
                    float measuredVel = (float)(diff / timeSinceLast);
                    velocity = velocity * 0.6f + measuredVel * 0.4f;
                }
                lastTargetPos = targetPos;
                lastTargetTimeSec = nowSec;
            }

            // Continuous forward extrapolation using velocity
            visualPos += velocity * (float)dt;

            // Soft spring pull towards audio thread target
            float error = targetPos - visualPos;
            visualPos += error * 0.30f;

            visualPos = juce::jlimit(0.0f, 1.0f, visualPos);
        }
    };

private:
    class LoopButton : public juce::Button
    {
    public:
        LoopButton() : juce::Button("LOOP") {}

        void paintButton(juce::Graphics& g, bool isHighlighted, bool isDown) override
        {
            juce::ignoreUnused(isDown);
            auto bounds = getLocalBounds().toFloat().reduced(1.0f);
            bool active = getToggleState();

            // Active fill burple, inactive light card
            juce::Colour bg = active ? SpectralUILookAndFeel::accentColour
                                     : (isHighlighted ? SpectralUILookAndFeel::dividerColour.withAlpha(0.4f)
                                                      : SpectralUILookAndFeel::panelBgColour);
            
            g.setColour(bg);
            g.fillRoundedRectangle(bounds, 3.0f);

            juce::Colour borderCol = active ? SpectralUILookAndFeel::accentBright
                                            : SpectralUILookAndFeel::dividerColour;
            g.setColour(borderCol);
            g.drawRoundedRectangle(bounds, 3.0f, 1.0f);

            g.setFont(SpectralUILookAndFeel::getMonospaceFont(9.5f, true));
            g.setColour(active ? juce::Colours::black : SpectralUILookAndFeel::textMainColour);
            g.drawText(active ? "[LOOP ON]" : "[LOOP]", bounds.toNearestInt(), juce::Justification::centred, false);
        }
    };

    static float yToFrequency(float normY);
    static float frequencyToY(float hz);
    static juce::String formatFrequency(float hz);
    void updateFrequencyFilterFromSelections();

    juce::Rectangle<float> getAxisBounds() const;

    void generateSpectrogramImage();
    void generateWaveformPaths(juce::Rectangle<float> bounds);
    void drawWaveform(juce::Graphics& g, juce::Rectangle<float> bounds);
    void drawFrequencyAxis(juce::Graphics& g, juce::Rectangle<float> axisBounds);
    juce::Colour getSpectrogramColor(float magnitudeNormalized);
    void dismissActiveDialog();

    void renderStaticGraph();
    void invalidateStaticGraph();
    void drawPlayheads(juce::Graphics& g, juce::Rectangle<float> graphBounds);
    void drawInteractiveOverlays(juce::Graphics& g, juce::Rectangle<float> graphBounds);

    static constexpr int maxVoiceSmoothers = 16;
    VoicePositionSmoother voiceSmoothers[maxVoiceSmoothers];
    double lastTimerTimeSec = 0.0;

    juce::Array<VisualPlayhead> renderedPlayheads;
    juce::Array<VisualPlayhead> prevVoicePlayheads;
    bool wasPlaying = false;

    juce::Image cachedStaticGraph;
    bool staticGraphDirty = true;

    juce::Path cachedWaveformEnvelope;
    juce::Path cachedWaveformTop;
    juce::Path cachedWaveformBottom;
    juce::Rectangle<float> cachedWaveformBounds;

    juce::Component::SafePointer<juce::AlertWindow> activeAlertWindow;

    VancespectralAudioProcessor& processor;
    ToolType currentTool = ToolType::RectangleSelect;

    bool dragActive = false;
    juce::File loadedFile;
    juce::AudioFormatManager formatManager;
    std::unique_ptr<juce::AudioFormatReader> reader;
    juce::AudioBuffer<float> audioBuffer;
    bool fileLoaded = false;
    bool isLoadingSample = false;

    juce::Image spectrogramImage;

    // Selections
    juce::Array<SelectionRegion> selections;
    int nextSelectionId = 1;
    int activeSelectionIndex = -1;

    bool isDrawing = false;
    juce::Point<float> dragStartPosNormalized;
    juce::Point<float> dragCurrentPosNormalized;
    juce::Path currentDrawingPathNormalized;

    enum class DragState {
        None,
        DrawingNew,
        MovingSelection,
        ResizingTopLeft,
        ResizingTopRight,
        ResizingBottomLeft,
        ResizingBottomRight,
        DraggingStartMarker,
        DraggingEndMarker
    };

    DragState dragState = DragState::None;
    juce::Point<float> dragStartMousePosNormalized;
    juce::Rectangle<float> initialSelectionBoundsNormalized;

    // Start & End Region Endpoints (0..1 normalized)
    float startPosition = 0.0f;
    float endPosition = 1.0f;

    juce::File createTempWavForExport(bool exportSelectionOnly);
    void startExternalDrag(const juce::MouseEvent& e);
    bool isExternalDragging = false;
    juce::Point<float> mouseDownPos;

    LoopButton loopButton;
    bool loopEnabled = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectrogramComponent)
};