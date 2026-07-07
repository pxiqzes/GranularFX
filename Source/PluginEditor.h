#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "LookAndFeel.h"

class GranularFXAudioProcessorEditor : public juce::AudioProcessorEditor,
                                        private juce::Timer
{
public:
    explicit GranularFXAudioProcessorEditor (GranularFXAudioProcessor&);
    ~GranularFXAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    GranularFXAudioProcessor& audioProcessor;
    GranularixLookAndFeel granularixLookAndFeel;

    juce::Slider grainSizeSlider, densitySlider, pitchSlider, scatterSlider, mixSlider;
    juce::Label  grainSizeLabel,  densityLabel,  pitchLabel,  scatterLabel,  mixLabel;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<SliderAttachment> grainSizeAttachment, densityAttachment,
                                       pitchAttachment, scatterAttachment, mixAttachment;

    // rounded card background drawn behind each knob, computed in resized()
    std::array<juce::Rectangle<int>, 5> knobCardBounds;

    // --- animation state, updated once per frame in timerCallback() ---
    float glowPhase = 0.0f;                 // drives the pulsing background/knob glow
    float starBrightness = 1.0f;            // current twinkle brightness (eased)
    float starTargetBrightness = 1.0f;      // twinkle randomly picks a new target
    juce::Random random;

    // --- stereometer state ---
    float leftDisplay = 0.0f, rightDisplay = 0.0f;
    float leftPeakHold = 0.0f, rightPeakHold = 0.0f;
    int leftPeakHoldCounter = 0, rightPeakHoldCounter = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GranularFXAudioProcessorEditor)
};
