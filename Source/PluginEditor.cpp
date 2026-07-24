#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
VancespectralAudioProcessorEditor::VancespectralAudioProcessorEditor(VancespectralAudioProcessor& p)
    : AudioProcessorEditor(&p),
      audioProcessor(p)
{
    spectrogram = std::make_unique<SpectrogramComponent>(audioProcessor);

    addAndMakeVisible(*spectrogram);

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

    auto topArea = area.removeFromTop(area.proportionOfHeight(0.60f));

    auto spectrogramArea = topArea.removeFromLeft(topArea.proportionOfWidth(0.72f));

    topArea.removeFromLeft(gap);

    auto playbackArea = topArea;

//==============================
// BOTTOM SECTION
//==============================

area.removeFromTop(gap);

auto bottomArea = area;

// FX panel on the right
auto fxArea = bottomArea.removeFromRight(330);

// Gap
bottomArea.removeFromRight(gap);

// Bottom graph
auto graphArea = bottomArea.removeFromBottom(130);

// Gap
bottomArea.removeFromBottom(gap);

// Filter panel
auto filterArea = bottomArea.removeFromRight(bottomArea.getWidth() / 2);

// Gap
bottomArea.removeFromRight(gap);

// AMP panel
auto ampArea = bottomArea;

    //==============================
    // DRAW PANELS
    //==============================

    g.setColour(juce::Colour(8, 8, 8));
    g.fillRoundedRectangle(spectrogramArea.toFloat(), 12.0f);

    g.setColour(juce::Colour(35,35,35));
    g.fillRoundedRectangle(playbackArea.toFloat(),12.0f);
    g.fillRoundedRectangle(ampArea.toFloat(),12.0f);
    g.fillRoundedRectangle(filterArea.toFloat(),12.0f);
    g.fillRoundedRectangle(fxArea.toFloat(),12.0f);

    g.setColour(juce::Colour(8,8,8));
    g.fillRoundedRectangle(graphArea.toFloat(),12.0f);

    //==============================
    // LABELS
    //==============================

    g.setColour(juce::Colours::white);
    g.setFont(24.0f);

    auto drawTitle = [&](juce::String text, juce::Rectangle<int> bounds)
    {
        g.drawText(text,
                   bounds.removeFromTop(35).reduced(18,0),
                   juce::Justification::left);
    };

    drawTitle("Spectrogram", spectrogramArea);
    drawTitle("Playback", playbackArea);
    drawTitle("AMP", ampArea);
    drawTitle("FILTER", filterArea);
    drawTitle("Effects", fxArea);
}

void VancespectralAudioProcessorEditor::resized()
{
    constexpr int margin = 25;
    constexpr int gap = 20;

    auto area = getLocalBounds().reduced(margin);

    auto topArea = area.removeFromTop(
        static_cast<int>(area.getHeight() * 0.60f));

    auto spectrogramPanel =
        topArea.removeFromLeft(
            static_cast<int>(topArea.getWidth() * 0.72f));

    topArea.removeFromLeft(gap);

    auto playbackPanel = topArea;

    if (spectrogram)
        spectrogram->setBounds(spectrogramPanel);
}