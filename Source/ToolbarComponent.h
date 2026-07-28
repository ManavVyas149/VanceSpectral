#pragma once

#include <JuceHeader.h>
#include "SpectralUILookAndFeel.h"

enum class ToolType { None, Freehand, RectangleSelect, Erase, Copy };

class ToolbarComponent : public juce::Component {
public:
  ToolbarComponent();
  ~ToolbarComponent() override = default;

  void paint(juce::Graphics &g) override;
  void resized() override;

  ToolType getSelectedTool() const { return activeTool; }
  void setSelectedTool(ToolType newTool);

  std::function<void(ToolType)> onToolSelected;

private:
  class ToolButton : public juce::Button {
  public:
    ToolButton(ToolType type, const juce::String &name, const juce::String &tooltipText);
    ~ToolButton() override = default;

    void paintButton(juce::Graphics &g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
    ToolType getToolType() const { return toolType; }

    std::function<void()> onDoubleClicked;

    void mouseDoubleClick(const juce::MouseEvent &e) override {
      juce::Button::mouseDoubleClick(e);
      if (onDoubleClicked)
        onDoubleClicked();
    }

  private:
    ToolType toolType;

    void drawFreehandIcon(juce::Graphics &g, juce::Rectangle<float> bounds, juce::Colour iconColour);
    void drawRectangleIcon(juce::Graphics &g, juce::Rectangle<float> bounds, juce::Colour iconColour);
    void drawEraseIcon(juce::Graphics &g, juce::Rectangle<float> bounds, juce::Colour iconColour);
    void drawCopyIcon(juce::Graphics &g, juce::Rectangle<float> bounds, juce::Colour iconColour);
  };

  ToolType activeTool = ToolType::RectangleSelect;

  ToolButton freehandBtn{ToolType::Freehand, "Freehand", "Freehand  (Double-click to disable)"};
  ToolButton rectSelectBtn{ToolType::RectangleSelect, "Rectangle", "Rectangle (Double-click to disable)"};
  ToolButton eraserBtn{ToolType::Erase, "Erase", "Eraser (Double-click to disable)"};
  ToolButton copyBtn{ToolType::Copy, "Copy", "Duplicate (Double-click to disable)"};

  juce::TooltipWindow tooltipWindow{this, 400};

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ToolbarComponent)
};
