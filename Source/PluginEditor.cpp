#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

namespace
{
    void setupSlider (juce::Slider& slider, juce::Label& label, const juce::String& text,
                       juce::Component& parent)
    {
        slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 20);
        parent.addAndMakeVisible (slider);

        label.setText (text, juce::dontSendNotification);
        label.setJustificationType (juce::Justification::centred);
        parent.addAndMakeVisible (label);
    }
}

GranularFXAudioProcessorEditor::GranularFXAudioProcessorEditor (GranularFXAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setLookAndFeel (&granularixLookAndFeel);

    setupSlider (grainSizeSlider, grainSizeLabel, "GRAIN SIZE", *this);
    setupSlider (densitySlider,   densityLabel,   "DENSITY",    *this);
    setupSlider (pitchSlider,     pitchLabel,     "PITCH",      *this);
    setupSlider (scatterSlider,   scatterLabel,   "SCATTER",    *this);
    setupSlider (mixSlider,       mixLabel,       "MIX",        *this);

    auto& apvts = audioProcessor.apvts;
    grainSizeAttachment = std::make_unique<SliderAttachment> (apvts, "grainSize", grainSizeSlider);
    densityAttachment   = std::make_unique<SliderAttachment> (apvts, "density",   densitySlider);
    pitchAttachment     = std::make_unique<SliderAttachment> (apvts, "pitch",     pitchSlider);
    scatterAttachment   = std::make_unique<SliderAttachment> (apvts, "scatter",   scatterSlider);
    mixAttachment       = std::make_unique<SliderAttachment> (apvts, "mix",       mixSlider);

    setSize (620, 360);

    startTimerHz (30); // drives all animation - glow, twinkle, and the stereometer
}

GranularFXAudioProcessorEditor::~GranularFXAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void GranularFXAudioProcessorEditor::timerCallback()
{
    // pulsing glow phase (background wash + knob glow)
    glowPhase += 0.07f;
    granularixLookAndFeel.setGlowPhase (glowPhase);

    // twinkle: ease current brightness toward a target, and occasionally
    // (randomly) pick a new target - gives an irregular flicker rather
    // than a smooth predictable pulse
    starBrightness += (starTargetBrightness - starBrightness) * 0.15f;
    if (random.nextFloat() < 0.03f)
        starTargetBrightness = juce::jmap (random.nextFloat(), 0.4f, 1.0f);

    // stereometer: fast attack, slower release, plus a held peak line
    auto smooth = [] (float& display, float target)
    {
        if (target > display) display = target;
        else                  display += (target - display) * 0.15f;
    };
    smooth (leftDisplay,  audioProcessor.leftLevel.load());
    smooth (rightDisplay, audioProcessor.rightLevel.load());

    auto updatePeak = [] (float& peak, int& counter, float display)
    {
        if (display >= peak) { peak = display; counter = 0; }
        else if (++counter > 20) peak = juce::jmax (0.0f, peak - 0.01f);
    };
    updatePeak (leftPeakHold,  leftPeakHoldCounter,  leftDisplay);
    updatePeak (rightPeakHold, rightPeakHoldCounter, rightDisplay);

    repaint();
}

void GranularFXAudioProcessorEditor::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // near-black base, matching pxiqzes.com's page background
    g.fillAll (GranularixLookAndFeel::backgroundColour);

    // pulsing violet glow bleeding down from the top
    float bgGlowAlpha = 0.16f + 0.10f * (0.5f + 0.5f * std::sin (glowPhase));
    juce::ColourGradient glow (GranularixLookAndFeel::accentViolet.withAlpha (bgGlowAlpha),
                                bounds.getCentreX(), 0.0f,
                                juce::Colours::transparentBlack,
                                bounds.getCentreX(), bounds.getHeight() * 0.7f, false);
    g.setGradientFill (glow);
    g.fillRect (bounds);

    // header pill
    auto headerArea = bounds.removeFromTop (56.0f).reduced (10.0f, 8.0f);
    g.setColour (GranularixLookAndFeel::panelColour);
    g.fillRoundedRectangle (headerArea, 16.0f);
    g.setColour (GranularixLookAndFeel::borderColour);
    g.drawRoundedRectangle (headerArea, 16.0f, 1.0f);

    juce::String starGlyph (juce::CharPointer_UTF8 ("\xE2\x9C\xB6"));
    g.setColour (juce::Colours::white.withAlpha (starBrightness));
    g.setFont (juce::Font (20.0f));
    g.drawText (starGlyph, headerArea.removeFromLeft (36.0f), juce::Justification::centred);

    auto titleArea = headerArea.removeFromLeft (230.0f);
    juce::ColourGradient titleGradient (GranularixLookAndFeel::accentViolet,
                                        titleArea.getX(), titleArea.getY(),
                                        GranularixLookAndFeel::accentMagenta,
                                        titleArea.getRight(), titleArea.getBottom(), false);
    g.setGradientFill (titleGradient);
    g.setFont (GranularixLookAndFeel::getBrandFont (26.0f));
    g.drawFittedText ("GRANULARIX", titleArea.toNearestInt(), juce::Justification::centredLeft, 1);

    g.setColour (GranularixLookAndFeel::mutedTextColour);
    g.setFont (GranularixLookAndFeel::getBrandFont (13.0f, false));
    g.drawFittedText ("by pxiqzes", headerArea.toNearestInt(), juce::Justification::centredLeft, 1);

    // rounded card behind each knob, breaks up the "flat row" look
    g.setColour (GranularixLookAndFeel::panelColour.withAlpha (0.55f));
    for (auto& cardBounds : knobCardBounds)
        g.fillRoundedRectangle (cardBounds.toFloat(), 12.0f);

    // --- stereometer panel ---
    auto meterPanel = juce::Rectangle<float> (bounds.getX() + 10.0f, 66.0f, bounds.getWidth() - 20.0f, 76.0f);
    g.setColour (GranularixLookAndFeel::panelColour);
    g.fillRoundedRectangle (meterPanel, 14.0f);
    g.setColour (GranularixLookAndFeel::borderColour);
    g.drawRoundedRectangle (meterPanel, 14.0f, 1.0f);

    auto meterContent = meterPanel.reduced (16.0f, 10.0f);

    g.setColour (GranularixLookAndFeel::mutedTextColour);
    g.setFont (GranularixLookAndFeel::getBrandFont (11.0f));
    g.drawText ("STEREO", meterContent.removeFromLeft (70.0f), juce::Justification::topLeft);

    auto labelStrip = meterContent.removeFromBottom (14.0f);
    auto barsArea = meterContent;

    const float barWidth = 20.0f;
    const float barGap = 16.0f;
    auto leftTrack  = juce::Rectangle<float> (barsArea.getX(), barsArea.getY(), barWidth, barsArea.getHeight());
    auto rightTrack = juce::Rectangle<float> (barsArea.getX() + barWidth + barGap, barsArea.getY(), barWidth, barsArea.getHeight());
    auto leftLabelArea  = labelStrip.withX (leftTrack.getX()).withWidth (barWidth);
    auto rightLabelArea = labelStrip.withX (rightTrack.getX()).withWidth (barWidth);

    auto drawBar = [&] (juce::Rectangle<float> trackBounds, float level, float peak,
                         juce::Rectangle<float> labelArea, const juce::String& labelText)
    {
        g.setColour (juce::Colour (0xFF1A1A20));
        g.fillRoundedRectangle (trackBounds, barWidth * 0.3f);

        float boosted = juce::jlimit (0.0f, 1.0f, level * 4.0f);
        float fillHeight = boosted * trackBounds.getHeight();
        auto fillBounds = trackBounds.withY (trackBounds.getBottom() - fillHeight).withHeight (fillHeight);

        juce::ColourGradient barGradient (GranularixLookAndFeel::accentViolet, fillBounds.getX(), fillBounds.getBottom(),
                                            GranularixLookAndFeel::accentMagenta, fillBounds.getX(), trackBounds.getY(), false);
        g.setGradientFill (barGradient);
        g.fillRoundedRectangle (fillBounds, barWidth * 0.3f);

        float peakBoosted = juce::jlimit (0.0f, 1.0f, peak * 4.0f);
        float peakY = trackBounds.getBottom() - peakBoosted * trackBounds.getHeight();
        g.setColour (juce::Colours::white);
        g.fillRect (trackBounds.getX(), peakY - 1.0f, trackBounds.getWidth(), 2.0f);

        g.setColour (GranularixLookAndFeel::mutedTextColour);
        g.setFont (GranularixLookAndFeel::getBrandFont (10.0f));
        g.drawText (labelText, labelArea, juce::Justification::centred);
    };

    drawBar (leftTrack,  leftDisplay,  leftPeakHold,  leftLabelArea,  "L");
    drawBar (rightTrack, rightDisplay, rightPeakHold, rightLabelArea, "R");
}

void GranularFXAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (10);
    area.removeFromTop (56);  // header
    area.removeFromTop (10);  // gap
    area.removeFromTop (76);  // stereometer panel (drawn in paint())
    area.removeFromTop (10);  // gap

    const int numKnobs = 5;
    const int knobWidth = area.getWidth() / numKnobs;
    const int staggerOffset = 14; // alternating vertical offset - breaks up the flat row

    auto layoutKnob = [&] (juce::Slider& slider, juce::Label& label, int index)
    {
        auto knobArea = area.withX (area.getX() + index * knobWidth).withWidth (knobWidth);
        if (index % 2 == 1)
            knobArea.translate (0, staggerOffset);

        knobCardBounds[(size_t) index] = knobArea.expanded (4);

        label.setBounds (knobArea.removeFromTop (20));
        slider.setBounds (knobArea.reduced (5));
    };

    layoutKnob (grainSizeSlider, grainSizeLabel, 0);
    layoutKnob (densitySlider,   densityLabel,   1);
    layoutKnob (pitchSlider,     pitchLabel,     2);
    layoutKnob (scatterSlider,   scatterLabel,   3);
    layoutKnob (mixSlider,       mixLabel,       4);
}
