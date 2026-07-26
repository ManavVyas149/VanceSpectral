#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
VancespectralAudioProcessorEditor::VancespectralAudioProcessorEditor(VancespectralAudioProcessor& p)
    : AudioProcessorEditor(&p),
      audioProcessor(p),
      ampADSRPanel(p.getAPVTS(), "AMP", "AMP_"),
      filterADSRPanel(p.getAPVTS(), "FILTER", "FILTER_")
{
    spectrogram = std::make_unique<SpectrogramComponent>(audioProcessor);

    addAndMakeVisible(*spectrogram);
    addAndMakeVisible(ampADSRPanel);
    addAndMakeVisible(filterADSRPanel);

    setSize(1200, 700);
}

VancespectralAudioProcessorEditor::~VancespectralAudioProcessorEditor()
{
}

//==============================================================================
void VancespectralAudioProcessorEditor::paint(juce::Graphics& g)
{
    //==============================
    // Background
    //==============================
    g.fillAll(juce::Colour::fromRGB(28, 28, 28));

    constexpr int margin = 25;
    constexpr int gap = 20;

    auto area = getLocalBounds().reduced(margin);

    //==============================
    // TOP SECTION
    //==============================
    auto topArea = area.removeFromTop(area.proportionOfHeight(0.54f));
    auto spectrogramArea = topArea.removeFromLeft(topArea.proportionOfWidth(0.72f));
    topArea.removeFromLeft(gap);
    auto playbackArea = topArea;

    //==============================
    // BOTTOM SECTION
    //==============================
    area.removeFromTop(gap);
    auto bottomArea = area;

    auto fxArea = bottomArea.removeFromRight(320);
    bottomArea.removeFromRight(gap);

    auto graphArea = bottomArea.removeFromBottom(100);
    bottomArea.removeFromBottom(gap);

    auto filterArea = bottomArea.removeFromRight(bottomArea.getWidth() / 2);
    bottomArea.removeFromRight(gap);

    auto ampArea = bottomArea;

    //==============================
    // DRAW PANELS
    //==============================
    g.setColour(juce::Colour(8, 8, 8));
    g.fillRoundedRectangle(spectrogramArea.toFloat(), 12.0f);

    g.setColour(juce::Colour(35, 35, 35));
    g.fillRoundedRectangle(playbackArea.toFloat(), 12.0f);
    g.fillRoundedRectangle(ampArea.toFloat(), 12.0f);
    g.fillRoundedRectangle(filterArea.toFloat(), 12.0f);
    g.fillRoundedRectangle(fxArea.toFloat(), 12.0f);

    g.setColour(juce::Colour(8, 8, 8));
    g.fillRoundedRectangle(graphArea.toFloat(), 12.0f);

    //==============================
    // LABELS
    //==============================
    g.setColour(juce::Colours::white);
    g.setFont(24.0f);

    auto drawTitle = [&](juce::String text, juce::Rectangle<int> bounds)
    {
        if (bounds.getWidth() > 36 && bounds.getHeight() > 10)
        {
            auto titleBounds = bounds.removeFromTop(juce::jmin(35, bounds.getHeight()));
            g.drawText(text,
                       titleBounds.reduced(juce::jmin(18, titleBounds.getWidth() / 4), 0),
                       juce::Justification::left);
        }
    };

    drawTitle("Spectrogram", spectrogramArea);
    drawTitle("Playback", playbackArea);
    drawTitle("Effects", fxArea);
}

void VancespectralAudioProcessorEditor::resized()
{
    constexpr int margin = 25;
    constexpr int gap = 20;

    auto area = getLocalBounds().reduced(margin);

    auto topArea = area.removeFromTop(
        static_cast<int>(area.getHeight() * 0.54f));

    auto spectrogramPanel =
        topArea.removeFromLeft(
            static_cast<int>(topArea.getWidth() * 0.72f));

    topArea.removeFromLeft(gap);

    if (spectrogram)
        spectrogram->setBounds(spectrogramPanel);

    //==============================
    // BOTTOM SECTION
    //==============================
    area.removeFromTop(gap);
    auto bottomArea = area;

    auto fxArea = bottomArea.removeFromRight(320);
    bottomArea.removeFromRight(gap);

    auto graphArea = bottomArea.removeFromBottom(100);
    bottomArea.removeFromBottom(gap);

    auto filterArea = bottomArea.removeFromRight(bottomArea.getWidth() / 2);
    bottomArea.removeFromRight(gap);

    auto ampArea = bottomArea;

    ampADSRPanel.setBounds(ampArea.reduced(5));
    filterADSRPanel.setBounds(filterArea.reduced(5));
}