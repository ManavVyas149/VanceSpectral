#include "ToolbarComponent.h"

ToolbarComponent::ToolButton::ToolButton(ToolType type, const juce::String &name, const juce::String &tooltipText)
    : juce::Button(name), toolType(type) {
  setTooltip(tooltipText);
}

void ToolbarComponent::ToolButton::paintButton(juce::Graphics &g, bool isHighlighted, bool isDown) {
  juce::ignoreUnused(isDown);
  auto bounds = getLocalBounds().toFloat().reduced(2.0f);
  bool isActive = getToggleState();
  bool enabled = isEnabled();

  if (isActive && enabled) {
    g.setColour(SpectralUILookAndFeel::accentColour.withAlpha(0.15f));
    g.fillRoundedRectangle(bounds, 5.0f);

    g.setColour(SpectralUILookAndFeel::accentColour);
    g.drawRoundedRectangle(bounds, 5.0f, 1.2f);
  } else if (isHighlighted && enabled) {
    g.setColour(juce::Colour::fromRGB(0xEA, 0xE5, 0xDA));
    g.fillRoundedRectangle(bounds, 5.0f);

    g.setColour(SpectralUILookAndFeel::dividerColour);
    g.drawRoundedRectangle(bounds, 5.0f, 1.0f);
  }

  juce::Colour iconCol;
  if (!enabled)
    iconCol = SpectralUILookAndFeel::textMutedColour.withAlpha(0.25f);
  else if (isActive)
    iconCol = SpectralUILookAndFeel::accentColour;
  else if (isHighlighted)
    iconCol = SpectralUILookAndFeel::textMainColour;
  else
    iconCol = SpectralUILookAndFeel::textMutedColour;

  auto iconBounds = bounds.reduced(6.0f);

  switch (toolType) {
  case ToolType::Freehand:
    drawFreehandIcon(g, iconBounds, iconCol);
    break;
  case ToolType::RectangleSelect:
    drawRectangleIcon(g, iconBounds, iconCol);
    break;
  case ToolType::Erase:
    drawEraseIcon(g, iconBounds, iconCol);
    break;
  case ToolType::Copy:
    drawCopyIcon(g, iconBounds, iconCol);
    break;
  case ToolType::None:
    break;
  }
}

void ToolbarComponent::ToolButton::drawFreehandIcon(juce::Graphics &g, juce::Rectangle<float> bounds, juce::Colour iconColour) {
  juce::Path p;
  float x = bounds.getX();
  float y = bounds.getY();
  float w = bounds.getWidth();
  float h = bounds.getHeight();

  p.startNewSubPath(x + w * 0.2f, y + h * 0.7f);
  p.cubicTo(x + w * 0.1f, y + h * 0.2f, x + w * 0.6f, y + h * 0.1f, x + w * 0.8f, y + h * 0.4f);
  p.cubicTo(x + w * 0.9f, y + h * 0.7f, x + w * 0.5f, y + h * 0.9f, x + w * 0.2f, y + h * 0.7f);
  p.closeSubPath();

  g.setColour(iconColour);
  g.strokePath(p, juce::PathStrokeType(1.4f));
}

void ToolbarComponent::ToolButton::drawRectangleIcon(juce::Graphics &g, juce::Rectangle<float> bounds, juce::Colour iconColour) {
  auto rect = bounds.reduced(2.0f);
  g.setColour(iconColour);
  g.drawRoundedRectangle(rect, 2.0f, 1.4f);

  float hs = 2.5f;
  g.fillRect(rect.getX() - hs * 0.5f, rect.getY() - hs * 0.5f, hs, hs);
  g.fillRect(rect.getRight() - hs * 0.5f, rect.getY() - hs * 0.5f, hs, hs);
  g.fillRect(rect.getX() - hs * 0.5f, rect.getBottom() - hs * 0.5f, hs, hs);
  g.fillRect(rect.getRight() - hs * 0.5f, rect.getBottom() - hs * 0.5f, hs, hs);
}

void ToolbarComponent::ToolButton::drawEraseIcon(juce::Graphics &g, juce::Rectangle<float> bounds, juce::Colour iconColour) {
  juce::Path p;
  float cx = bounds.getCentreX();
  float cy = bounds.getCentreY();
  float sz = bounds.getWidth() * 0.35f;

  p.addRectangle(cx - sz * 0.8f, cy - sz * 0.8f, sz * 1.6f, sz * 1.6f);

  g.setColour(iconColour);
  g.strokePath(p, juce::PathStrokeType(1.4f));

  g.drawLine(cx - sz, cy + sz, cx + sz, cy - sz, 1.4f);
}

void ToolbarComponent::ToolButton::drawCopyIcon(juce::Graphics &g, juce::Rectangle<float> bounds, juce::Colour iconColour) {
  float w = bounds.getWidth() * 0.55f;
  float h = bounds.getHeight() * 0.55f;

  auto r1 = juce::Rectangle<float>(bounds.getX(), bounds.getY() + bounds.getHeight() - h, w, h);
  auto r2 = juce::Rectangle<float>(bounds.getX() + bounds.getWidth() - w, bounds.getY(), w, h);

  g.setColour(iconColour.withAlpha(0.6f));
  g.drawRoundedRectangle(r1, 1.5f, 1.2f);

  g.setColour(iconColour);
  g.drawRoundedRectangle(r2, 1.5f, 1.4f);
}

ToolbarComponent::ToolbarComponent() {
  addAndMakeVisible(freehandBtn);
  addAndMakeVisible(rectSelectBtn);
  addAndMakeVisible(eraserBtn);
  addAndMakeVisible(copyBtn);

  auto setupBtn = [this](ToolButton &btn, ToolType type) {
    btn.setRadioGroupId(1001);
    btn.setClickingTogglesState(true);

    btn.onClick = [this, type, &btn]() {
      if (!isEnabled()) return;
      if (btn.getToggleState()) {
        activeTool = type;
        if (onToolSelected)
          onToolSelected(activeTool);
      }
    };

    // Double-click deselects the tool -> sets activeTool = ToolType::None
    btn.onDoubleClicked = [this, &btn]() {
      if (!isEnabled()) return;
      setSelectedTool(ToolType::None);
      if (onToolSelected)
        onToolSelected(ToolType::None);
    };
  };

  setupBtn(freehandBtn, ToolType::Freehand);
  setupBtn(rectSelectBtn, ToolType::RectangleSelect);
  setupBtn(eraserBtn, ToolType::Erase);
  setupBtn(copyBtn, ToolType::Copy);

  rectSelectBtn.setToggleState(true, juce::dontSendNotification);
}

void ToolbarComponent::setSelectedTool(ToolType newTool) {
  activeTool = newTool;
  freehandBtn.setToggleState(activeTool == ToolType::Freehand, juce::dontSendNotification);
  rectSelectBtn.setToggleState(activeTool == ToolType::RectangleSelect, juce::dontSendNotification);
  eraserBtn.setToggleState(activeTool == ToolType::Erase, juce::dontSendNotification);
  copyBtn.setToggleState(activeTool == ToolType::Copy, juce::dontSendNotification);
  repaint();
}

void ToolbarComponent::paint(juce::Graphics &g) {
  auto bounds = getLocalBounds().toFloat();

  g.setColour(SpectralUILookAndFeel::panelBgColour.withAlpha(isEnabled() ? 1.0f : 0.45f));
  g.fillRoundedRectangle(bounds, 6.0f);

  g.setColour(SpectralUILookAndFeel::dividerColour.withAlpha(isEnabled() ? 1.0f : 0.3f));
  g.drawRoundedRectangle(bounds, 6.0f, 1.0f);
}

void ToolbarComponent::resized() {
  auto area = getLocalBounds().reduced(4);
  int btnSize = area.getWidth();

  freehandBtn.setBounds(area.removeFromTop(btnSize));
  area.removeFromTop(4);
  rectSelectBtn.setBounds(area.removeFromTop(btnSize));
  area.removeFromTop(4);
  eraserBtn.setBounds(area.removeFromTop(btnSize));
  area.removeFromTop(4);
  copyBtn.setBounds(area.removeFromTop(btnSize));
}
