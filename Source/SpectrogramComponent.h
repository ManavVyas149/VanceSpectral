#pragma once

#include <JuceHeader.h>
#include <memory>

class VancespectralAudioProcessor;

class SpectrogramComponent : public juce::Component, public juce::FileDragAndDropTarget, public juce::ChangeListener
{
public:
    SpectrogramComponent(VancespectralAudioProcessor&);
    ~SpectrogramComponent() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;
    
    bool isInterestedInFileDrag (const juce::StringArray& files) override;

    void fileDragEnter (const juce::StringArray& files,
                        int x,
                        int y) override;

    void fileDragMove (const juce::StringArray& files,
                    int x,
                    int y) override;

    void fileDragExit (const juce::StringArray& files) override;

    void filesDropped (const juce::StringArray& files,
                    int x,
                    int y) override;
    void loadAudioFile(const juce::File& file);
    void drawWaveform(juce::Graphics& g);
    void changeListenerCallback(juce::ChangeBroadcaster*) override;
    
    void mouseUp(const juce::MouseEvent&) override;

    void mouseDown(const juce::MouseEvent& e) override;

    void mouseDrag(const juce::MouseEvent& e) override;

private:

    VancespectralAudioProcessor& processor;

    bool dragActive = false;

    juce::File loadedFile;

    juce::AudioFormatManager formatManager;

    std::unique_ptr<juce::AudioFormatReader> reader;

    juce::AudioBuffer<float> audioBuffer;

    bool fileLoaded = false;

    bool isPlaying = false;

    juce::TextButton loopButton{"LOOP"};

    bool loopEnabled = false;

    double startPosition = 0.0;
    double endPosition = 1.0;

    bool draggingStart = false;
    bool draggingEnd = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectrogramComponent)
};