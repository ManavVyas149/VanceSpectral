/*
  ==============================================================================

    ToolbarComponent.h
    Created: 24 Jul 2026 2:36:05pm
    Author:  MANAV VYAS

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

enum class ToolType { RectangleSelect, Copy, Eraser, Paste, None };

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
    ToolButton(ToolType type, const juce::String &name,
               const juce::String &tooltipText);
    ~ToolButton() override = default;

    void paintButton(juce::Graphics &g, bool shouldDrawButtonAsHighlighted,
                     bool shouldDrawButtonAsDown) override;

    ToolType getToolType() const { return toolType; }
    void setSelected(bool selected);
    bool isSelected() const { return active; }

  private:
    ToolType toolType;
    bool active = false;

    void drawRectangleSelectIcon(juce::Graphics &g,
                                 juce::Rectangle<float> bounds,
                                 juce::Colour iconColour);
    void drawCopyIcon(juce::Graphics &g, juce::Rectangle<float> bounds,
                      juce::Colour iconColour);
    void drawEraserIcon(juce::Graphics &g, juce::Rectangle<float> bounds,
                        juce::Colour iconColour);
    void drawPasteIcon(juce::Graphics &g, juce::Rectangle<float> bounds,
                       juce::Colour iconColour);
  };

  ToolType activeTool = ToolType::RectangleSelect;

  ToolButton rectSelectBtn{ToolType::RectangleSelect, "Select", "Selection"};
  ToolButton copyBtn{ToolType::Copy, "Copy", "Copy"};
  ToolButton eraserBtn{ToolType::Eraser, "Eraser", "Eraser"};
  ToolButton pasteBtn{ToolType::Paste, "Paste", "Paste"};

  juce::TooltipWindow tooltipWindow{this, 400};

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ToolbarComponent)
};
