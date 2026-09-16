#include "PluginProcessor.h"
#include "PluginEditor.h"

HarmonicGlitchAudioProcessor::HarmonicGlitchAudioProcessor()
    : AudioProcessor (BusesProperties().withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", createParameterLayout())
{
}

HarmonicGlitchAudioProcessor::~HarmonicGlitchAudioProcessor() {}

juce::AudioProcessorValueTreeState::ParameterLayout HarmonicGlitchAudioProcessor::createParameterLayout()
{
  juce::AudioProcessorValueTreeState::ParameterLayout layout;
  layout.add (std::make_unique<juce::AudioParameterFloat> (
      juce::ParameterID { "drywet", 1 },
      "Dry/Wet",
      juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f),
      0.5f
  ));
  return layout;
}

void HarmonicGlitchAudioProcessor::prepareToPlay (double, int) {}
void HarmonicGlitchAudioProcessor::releaseResources() {}

bool HarmonicGlitchAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
   && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
    return false;

  if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
    return false;

  return true;
}

void HarmonicGlitchAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
  juce::ScopedNoDenormals noDenormals;
  // 現時点ではオーディオ信号をそのままスルー通過
}

juce::AudioProcessorEditor* HarmonicGlitchAudioProcessor::createEditor()
{
  return new HarmonicGlitchAudioProcessorEditor (*this);
}

void HarmonicGlitchAudioProcessor::getStateInformation (juce::MemoryBlock&) {}
void HarmonicGlitchAudioProcessor::setStateInformation (const void*, int) {}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
  return new HarmonicGlitchAudioProcessor();
}