/*
  ==============================================================================

    ToolbarComponent.cpp
    Created: 24 Jul 2026 2:36:05pm
    Author:  MANAV VYAS

  ==============================================================================
*/

#include "ToolbarComponent.h"

//==============================================================================
ToolbarComponent::ToolButton::ToolButton(ToolType type,
                                         const juce::String &name,
                                         const juce::String &tooltipText)
    : juce::Button(name), toolType(type) {
  setTooltip(tooltipText);
}

void ToolbarComponent::ToolButton::setSelected(bool selected) {
  if (active != selected) {
    active = selected;
    repaint();
  }
}

void ToolbarComponent::ToolButton::paintButton(
    juce::Graphics &g, bool shouldDrawButtonAsHighlighted,
    bool shouldDrawButtonAsDown) {
  auto bounds = getLocalBounds().toFloat().reduced(1.5f);
  float cornerRadius = 6.0f;

  juce::Colour bgColour;
  juce::Colour borderColour;
  juce::Colour iconColour;

  if (active) {
    bgColour = juce::Colour(0, 140, 220).withAlpha(0.25f);
    borderColour = juce::Colour(0, 180, 255);
    iconColour = juce::Colour(0, 210, 255);
  } else if (shouldDrawButtonAsHighlighted || shouldDrawButtonAsDown) {
    bgColour = juce::Colour(60, 60, 68);
    borderColour = juce::Colour(90, 90, 100);
    iconColour = juce::Colour(240, 240, 245);
  } else {
    bgColour = juce::Colour(40, 40, 46);
    borderColour = juce::Colour(55, 55, 62);
    iconColour = juce::Colour(170, 175, 185);
  }

  g.setColour(bgColour);
  g.fillRoundedRectangle(bounds, cornerRadius);

  g.setColour(borderColour);
  g.drawRoundedRectangle(bounds, cornerRadius, active ? 1.8f : 1.0f);

  // Reduced icon padding (from 9.0f to 6.0f) for compact icon rendering
  auto iconArea = bounds.reduced(6.0f);

  switch (toolType) {
  case ToolType::RectangleSelect:
    drawRectangleSelectIcon(g, iconArea, iconColour);
    break;
  case ToolType::Copy:
    drawCopyIcon(g, iconArea, iconColour);
    break;
  case ToolType::Eraser:
    drawEraserIcon(g, iconArea, iconColour);
    break;
  case ToolType::Paste:
    drawPasteIcon(g, iconArea, iconColour);
    break;
  case ToolType::None:
    break;
  }
}

void ToolbarComponent::ToolButton::drawRectangleSelectIcon(
    juce::Graphics &g, juce::Rectangle<float> bounds, juce::Colour iconColour) {
  auto rect = bounds.reduced(1.0f);

  g.setColour(iconColour.withAlpha(0.15f));
  g.fillRect(rect);

  juce::Path p;
  p.addRectangle(rect);

  juce::PathStrokeType stroke(1.4f);
  float strokeLengths[] = {2.5f, 2.0f};
  stroke.createDashedStroke(p, p, strokeLengths, 2);

  g.setColour(iconColour);
  g.fillPath(p);

  // Corner handle ticks
  float handleSize = 2.5f;
  g.fillRect(rect.getX() - 1.0f, rect.getY() - 1.0f, handleSize, handleSize);
  g.fillRect(rect.getRight() - handleSize + 1.0f, rect.getY() - 1.0f,
             handleSize, handleSize);
  g.fillRect(rect.getX() - 1.0f, rect.getBottom() - handleSize + 1.0f,
             handleSize, handleSize);
  g.fillRect(rect.getRight() - handleSize + 1.0f,
             rect.getBottom() - handleSize + 1.0f, handleSize, handleSize);
}

void ToolbarComponent::ToolButton::drawCopyIcon(juce::Graphics &g,
                                                juce::Rectangle<float> bounds,
                                                juce::Colour iconColour) {
  float w = bounds.getWidth() * 0.62f;
  float h = bounds.getHeight() * 0.70f;

  // Back document card
  auto backCard = juce::Rectangle<float>(bounds.getX(), bounds.getY(), w, h);
  g.setColour(iconColour.withAlpha(0.6f));
  g.drawRoundedRectangle(backCard, 2.0f, 1.3f);

  // Front document card
  auto frontCard = juce::Rectangle<float>(bounds.getRight() - w,
                                          bounds.getBottom() - h, w, h);
  g.setColour(juce::Colour(35, 35, 40));
  g.fillRoundedRectangle(frontCard, 2.0f);

  g.setColour(iconColour);
  g.drawRoundedRectangle(frontCard, 2.0f, 1.4f);

  // Content lines inside front card
  float lineY1 = frontCard.getY() + frontCard.getHeight() * 0.35f;
  float lineY2 = frontCard.getY() + frontCard.getHeight() * 0.65f;
  g.drawLine(frontCard.getX() + 2.0f, lineY1, frontCard.getRight() - 2.0f,
             lineY1, 1.1f);
  g.drawLine(frontCard.getX() + 2.0f, lineY2, frontCard.getRight() - 4.0f,
             lineY2, 1.1f);
}

void ToolbarComponent::ToolButton::drawEraserIcon(juce::Graphics &g,
                                                  juce::Rectangle<float> bounds,
                                                  juce::Colour iconColour) {
  juce::Path p;
  float x = bounds.getX();
  float y = bounds.getY();
  float w = bounds.getWidth();
  float h = bounds.getHeight();

  // Draw an angled eraser block
  p.startNewSubPath(x + w * 0.25f, y + h * 0.70f);
  p.lineTo(x + w * 0.65f, y + h * 0.15f);
  p.lineTo(x + w * 0.90f, y + h * 0.35f);
  p.lineTo(x + w * 0.50f, y + h * 0.90f);
  p.closeSubPath();

  g.setColour(iconColour.withAlpha(0.2f));
  g.fillPath(p);

  g.setColour(iconColour);
  g.strokePath(p, juce::PathStrokeType(1.4f));

  // Eraser band divider
  juce::Line<float> bandLine(x + w * 0.45f, y + h * 0.43f, x + w * 0.70f,
                             y + h * 0.63f);
  g.drawLine(bandLine, 1.2f);

  // Eraser bottom smudge line
  g.setColour(iconColour.withAlpha(0.5f));
  g.drawLine(x + w * 0.10f, y + h * 0.88f, x + w * 0.40f, y + h * 0.95f, 1.1f);
}

void ToolbarComponent::ToolButton::drawPasteIcon(juce::Graphics &g,
                                                 juce::Rectangle<float> bounds,
                                                 juce::Colour iconColour) {
  float w = bounds.getWidth() * 0.68f;
  float h = bounds.getHeight() * 0.80f;

  // Clipboard base
  auto board = juce::Rectangle<float>(bounds.getCentreX() - w * 0.5f,
                                      bounds.getBottom() - h, w, h);

  g.setColour(iconColour.withAlpha(0.2f));
  g.fillRoundedRectangle(board, 2.0f);

  g.setColour(iconColour);
  g.drawRoundedRectangle(board, 2.0f, 1.4f);

  // Document lines on board
  float lineY1 = board.getY() + board.getHeight() * 0.40f;
  float lineY2 = board.getY() + board.getHeight() * 0.65f;
  g.drawLine(board.getX() + 2.0f, lineY1, board.getRight() - 2.0f, lineY1,
             1.1f);
  g.drawLine(board.getX() + 2.0f, lineY2, board.getRight() - 4.0f, lineY2,
             1.1f);

  // Clipboard clip tab at top
  float clipW = w * 0.50f;
  float clipH = 3.5f;
  auto clip = juce::Rectangle<float>(bounds.getCentreX() - clipW * 0.5f,
                                     board.getY() - 1.8f, clipW, clipH);
  g.fillRoundedRectangle(clip, 1.2f);
}

//==============================================================================
ToolbarComponent::ToolbarComponent() {
  addAndMakeVisible(rectSelectBtn);
  addAndMakeVisible(copyBtn);
  addAndMakeVisible(eraserBtn);
  addAndMakeVisible(pasteBtn);

  auto setupButton = [this](ToolButton &btn) {
    btn.onClick = [this, &btn]() { setSelectedTool(btn.getToolType()); };
  };

  setupButton(rectSelectBtn);
  setupButton(copyBtn);
  setupButton(eraserBtn);
  setupButton(pasteBtn);

  setSelectedTool(ToolType::RectangleSelect);
}

void ToolbarComponent::setSelectedTool(ToolType newTool) {
  activeTool = newTool;

  rectSelectBtn.setSelected(activeTool == ToolType::RectangleSelect);
  copyBtn.setSelected(activeTool == ToolType::Copy);
  eraserBtn.setSelected(activeTool == ToolType::Eraser);
  pasteBtn.setSelected(activeTool == ToolType::Paste);

  if (onToolSelected)
    onToolSelected(activeTool);
}

void ToolbarComponent::paint(juce::Graphics &g) {
  // Toolbar Panel Background
  auto bounds = getLocalBounds().toFloat();

  g.setColour(juce::Colour(32, 32, 36));
  g.fillRoundedRectangle(bounds, 8.0f);

  g.setColour(juce::Colour(55, 55, 62));
  g.drawRoundedRectangle(bounds, 8.0f, 1.0f);

  // Header Label "TOOLS"
  g.setColour(juce::Colour(140, 145, 155));
  g.setFont(juce::Font(9.0f, juce::Font::bold));
  auto headerArea = getLocalBounds().removeFromTop(20);
  g.drawText("TOOLS", headerArea, juce::Justification::centred, false);

  // Subtle header line separator
  g.setColour(juce::Colour(50, 50, 56));
  g.drawLine(5.0f, 20.0f, (float)getWidth() - 5.0f, 20.0f, 1.0f);
}

void ToolbarComponent::resized() {
  auto area = getLocalBounds().reduced(4, 4);
  area.removeFromTop(20); // Title space

  constexpr int buttonHeight = 34;
  constexpr int gap = 4;

  rectSelectBtn.setBounds(area.removeFromTop(buttonHeight));
  area.removeFromTop(gap);

  copyBtn.setBounds(area.removeFromTop(buttonHeight));
  area.removeFromTop(gap);

  eraserBtn.setBounds(area.removeFromTop(buttonHeight));
  area.removeFromTop(gap);

  pasteBtn.setBounds(area.removeFromTop(buttonHeight));
}
