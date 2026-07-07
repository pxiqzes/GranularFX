#pragma once
#include <JuceHeader.h>
#include "GranularEngine.h"

class GranularFXAudioProcessor : public juce::AudioProcessor
{
public:
    GranularFXAudioProcessor();
    ~GranularFXAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // Holds all plugin parameters (grain size, density, pitch, scatter, mix)
    // and handles saving/loading them with the DAW project.
    juce::AudioProcessorValueTreeState apvts;

    // Updated every block with the per-channel peak output level, so the
    // editor's stereometer can react to real audio without touching the
    // audio thread directly (the editor just reads these atomically each frame).
    std::atomic<float> leftLevel { 0.0f };
    std::atomic<float> rightLevel { 0.0f };

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    GranularEngine granularEngine;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GranularFXAudioProcessor)
};
