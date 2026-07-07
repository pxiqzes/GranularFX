#pragma once
#include <JuceHeader.h>

/**
    Custom LookAndFeel for GRANULARIX, styled after the pxiqzes brand:
    near-black background, a violet-to-magenta gradient accent, a
    six-pointed star (star) motif, and clean geometric type.

    This is the ONE place all the plugin's visual styling lives - change
    the colours/font below and everything (knobs, text, background) updates.
*/
class GranularixLookAndFeel : public juce::LookAndFeel_V4
{
public:
    // Core palette, pulled from pxiqzes.com
    inline static const juce::Colour backgroundColour { 0xFF09090B }; // page background
    inline static const juce::Colour panelColour       { 0xFF141418 }; // header / knob track
    inline static const juce::Colour borderColour      { 0xFF27272A }; // pill borders
    inline static const juce::Colour accentViolet      { 0xFF8B5CF6 }; // gradient start
    inline static const juce::Colour accentMagenta     { 0xFFE879F9 }; // gradient end
    inline static const juce::Colour mutedTextColour   { 0xFF9A9AA2 }; // secondary labels

    // Approximating the clean geometric-sans feel of modern plugin UIs
    // (Century Gothic ships with Windows). Swap this name for a licensed
    // font file's family name if you have one you want to embed instead.
    inline static const juce::String brandFontName { "Century Gothic" };

    GranularixLookAndFeel()
    {
        setColour (juce::ResizableWindow::backgroundColourId, backgroundColour);
        setColour (juce::Slider::textBoxTextColourId, juce::Colours::white);
        setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
        setColour (juce::Label::textColourId, mutedTextColour);
    }

    static juce::Font getBrandFont (float size, bool bold = true)
    {
        return juce::Font (brandFontName, size, bold ? juce::Font::bold : juce::Font::plain);
    }

    // Called once per animation frame from the editor's Timer so the knob
    // glow can pulse in sync with the rest of the UI.
    void setGlowPhase (float newPhase) { glowPhase = newPhase; }

    juce::Font getLabelFont (juce::Label& label) override
    {
        // Slider's built-in value read-out is created editable-on-double-click;
        // our own plain name labels never call setEditable, so this tells
        // the two apart and gives the read-out a slightly different size.
        bool isValueReadout = label.isEditableOnDoubleClick();
        return getBrandFont (isValueReadout ? 13.0f : 12.0f);
    }

    void drawLabel (juce::Graphics& g, juce::Label& label) override
    {
        if (label.isBeingEdited())
            return; // let the default text-editor draw itself while typing a value

        auto bounds = label.getLocalBounds().toFloat();
        bool isValueReadout = label.isEditableOnDoubleClick();

        if (isValueReadout)
        {
            // small rounded pill, no border
            auto pillBounds = bounds.reduced (bounds.getWidth() * 0.10f, 2.0f);
            g.setColour (panelColour.brighter (0.08f));
            g.fillRoundedRectangle (pillBounds, pillBounds.getHeight() * 0.5f);
            g.setColour (juce::Colours::white);
        }
        else
        {
            g.setColour (label.findColour (juce::Label::textColourId));
        }

        g.setFont (getLabelFont (label));
        g.drawFittedText (label.getText(), bounds.toNearestInt(), label.getJustificationType(), 1);
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPosProportional, float rotaryStartAngle,
                           float rotaryEndAngle, juce::Slider&) override
    {
        auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height).reduced (6.0f);
        auto radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) / 2.0f;
        auto centre = bounds.getCentre();
        auto angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

        juce::Path track;
        track.addCentredArc (centre.x, centre.y, radius, radius, 0.0f,
                              rotaryStartAngle, rotaryEndAngle, true);
        g.setColour (panelColour);
        g.strokePath (track, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved,
                                                    juce::PathStrokeType::rounded));

        juce::Path activeArc;
        activeArc.addCentredArc (centre.x, centre.y, radius, radius, 0.0f,
                                  rotaryStartAngle, angle, true);

        float glowAlpha = 0.18f + 0.18f * (0.5f + 0.5f * std::sin (glowPhase));
        juce::ColourGradient glowGradient (accentViolet.withAlpha (glowAlpha), bounds.getX(), bounds.getY(),
                                            accentMagenta.withAlpha (glowAlpha), bounds.getRight(), bounds.getBottom(), false);
        g.setGradientFill (glowGradient);
        g.strokePath (activeArc, juce::PathStrokeType (10.0f, juce::PathStrokeType::curved,
                                                        juce::PathStrokeType::rounded));

        juce::ColourGradient gradient (accentViolet, bounds.getX(), bounds.getY(),
                                        accentMagenta, bounds.getRight(), bounds.getBottom(), false);
        g.setGradientFill (gradient);
        g.strokePath (activeArc, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved,
                                                        juce::PathStrokeType::rounded));

        auto knobRadius = radius * 0.68f;
        g.setColour (juce::Colour (0xFF1A1A20));
        g.fillEllipse (centre.x - knobRadius, centre.y - knobRadius, knobRadius * 2.0f, knobRadius * 2.0f);
        g.setColour (borderColour);
        g.drawEllipse (centre.x - knobRadius, centre.y - knobRadius, knobRadius * 2.0f, knobRadius * 2.0f, 1.0f);

        juce::Path pointer;
        auto pointerLength = knobRadius * 0.85f;
        auto pointerThickness = 2.5f;
        pointer.addRectangle (-pointerThickness * 0.5f, -pointerLength, pointerThickness, pointerLength * 0.6f);
        pointer.applyTransform (juce::AffineTransform::rotation (angle).translated (centre.x, centre.y));
        g.setColour (juce::Colours::white);
        g.fillPath (pointer);
    }

private:
    float glowPhase = 0.0f;
};
