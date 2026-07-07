#include "PluginProcessor.h"
#include "PluginEditor.h"

GranularFXAudioProcessor::GranularFXAudioProcessor()
     : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
       apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{
}

GranularFXAudioProcessor::~GranularFXAudioProcessor() {}

juce::AudioProcessorValueTreeState::ParameterLayout GranularFXAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        "grainSize", "Grain Size (ms)",
        juce::NormalisableRange<float> (10.0f, 300.0f, 0.1f), 80.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        "density", "Density (grains/sec)",
        juce::NormalisableRange<float> (1.0f, 60.0f, 0.1f), 15.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        "pitch", "Pitch (semitones)",
        juce::NormalisableRange<float> (-24.0f, 24.0f, 0.01f), 0.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        "scatter", "Scatter (ms)",
        juce::NormalisableRange<float> (0.0f, 200.0f, 0.1f), 20.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        "mix", "Mix",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f));

    return { params.begin(), params.end() };
}

void GranularFXAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    granularEngine.prepare (sampleRate, samplesPerBlock, getTotalNumOutputChannels());
}

void GranularFXAudioProcessor::releaseResources() {}

bool GranularFXAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    return layouts.getMainOutputChannelSet() == layouts.getMainInputChannelSet();
}

void GranularFXAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    for (auto i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    float grainSizeMs = *apvts.getRawParameterValue ("grainSize");
    float density      = *apvts.getRawParameterValue ("density");
    float pitch        = *apvts.getRawParameterValue ("pitch");
    float scatter       = *apvts.getRawParameterValue ("scatter");
    float mix           = *apvts.getRawParameterValue ("mix");

    granularEngine.setParameters (grainSizeMs, density, pitch, scatter, mix);
    granularEngine.process (buffer);

    const int numSamples = buffer.getNumSamples();
    const int numCh = buffer.getNumChannels();
    leftLevel.store (buffer.getMagnitude (0, 0, numSamples));
    rightLevel.store (numCh > 1 ? buffer.getMagnitude (1, 0, numSamples) : leftLevel.load());
}

juce::AudioProcessorEditor* GranularFXAudioProcessor::createEditor()
{
    return new GranularFXAudioProcessorEditor (*this);
}

bool GranularFXAudioProcessor::hasEditor() const { return true; }

const juce::String GranularFXAudioProcessor::getName() const { return JucePlugin_Name; }

bool GranularFXAudioProcessor::acceptsMidi() const { return false; }
bool GranularFXAudioProcessor::producesMidi() const { return false; }
bool GranularFXAudioProcessor::isMidiEffect() const { return false; }
double GranularFXAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int GranularFXAudioProcessor::getNumPrograms() { return 1; }
int GranularFXAudioProcessor::getCurrentProgram() { return 0; }
void GranularFXAudioProcessor::setCurrentProgram (int) {}
const juce::String GranularFXAudioProcessor::getProgramName (int) { return {}; }
void GranularFXAudioProcessor::changeProgramName (int, const juce::String&) {}

void GranularFXAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void GranularFXAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState.get() != nullptr && xmlState->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
}

// This function tells JUCE how to create an instance of our plugin.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new GranularFXAudioProcessor();
}
