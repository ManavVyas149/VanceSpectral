#pragma once

#include "SpectralUILookAndFeel.h"
#include <JuceHeader.h>

class PresetBarComponent : public juce::Component {
public:
  PresetBarComponent();
  ~PresetBarComponent() override = default;

  void paint(juce::Graphics &g) override;
  void resized() override;
  void mouseDown(const juce::MouseEvent &e) override;

  juce::String getCurrentPresetName() const { return currentPresetName; }
  void setPresetName(const juce::String &name);
  void setBankName(const juce::String &bank) {
    bankName = bank;
    repaint();
  }

  std::function<void()> onPrevClicked;
  std::function<void()> onNextClicked;
  std::function<void()> onBrowseClicked;
  std::function<void()> onSaveStateClicked;

private:
  class ChevronButton : public juce::Button {
  public:
    ChevronButton(bool pointsRight)
        : juce::Button(pointsRight ? "Next" : "Prev"), isRight(pointsRight) {}

    void paintButton(juce::Graphics &g, bool isHighlighted,
                     bool isDown) override {
      auto bounds = getLocalBounds().toFloat();
      juce::Colour col =
          isDown ? SpectralUILookAndFeel::accentColour
                 : (isHighlighted ? SpectralUILookAndFeel::textMainColour
                                  : SpectralUILookAndFeel::textMutedColour);

      juce::Path p;
      float cx = bounds.getCentreX();
      float cy = bounds.getCentreY();
      float sz = 4.0f;

      if (isRight) {
        p.startNewSubPath(cx - sz, cy - sz);
        p.lineTo(cx + sz * 0.5f, cy);
        p.lineTo(cx - sz, cy + sz);
      } else {
        p.startNewSubPath(cx + sz, cy - sz);
        p.lineTo(cx - sz * 0.5f, cy);
        p.lineTo(cx + sz, cy + sz);
      }

      g.setColour(col);
      g.strokePath(p, juce::PathStrokeType(1.4f, juce::PathStrokeType::mitered,
                                           juce::PathStrokeType::square));
    }

  private:
    bool isRight;
  };

  juce::String bankName{"FACTORY"};
  juce::String currentPresetName{"Cold Synth"};

  ChevronButton prevButton{false};
  ChevronButton nextButton{true};
  juce::TextButton saveStateButton{ "SAVE STATE" };

  juce::Rectangle<float> clickTargetBounds;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetBarComponent)
};
