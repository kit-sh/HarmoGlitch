#include "PluginProcessor.h"

#include <dispatch/data.h>

#include "PluginEditor.h"

HarmonicGlitchAudioProcessor::HarmonicGlitchAudioProcessor()
    : AudioProcessor (BusesProperties().withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", createParameterLayout())
{
  dryWetParam = apvts.getRawParameterValue("drywet");
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

void HarmonicGlitchAudioProcessor::prepareToPlay (double sampleRate, int) {
  smoothedDryWet.reset(sampleRate, 0.05);
  smoothedDryWet.setCurrentAndTargetValue(dryWetParam->load());

  const int numChannels = getTotalNumInputChannels();
  stftChannels.resize(static_cast<size_t> (numChannels));
  for (auto& stft : stftChannels) {
    stft.prepare();
  }
}
void HarmonicGlitchAudioProcessor::releaseResources() {
}

bool HarmonicGlitchAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
   && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
    return false;

  if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
    return false;

  return true;
}

void HarmonicGlitchAudioProcessor::processSTFTFrame(ChannelSTFT &stft) {

  std::fill(stft.fftData.begin(), stft.fftData.end(), 0.0f);
  std::copy(stft.inputBuffer.begin(), stft.inputBuffer.end(), stft.fftData.begin());

  window.multiplyWithWindowingTable(stft.fftData.data(), fftSize);

  fft.performRealOnlyForwardTransform(stft.fftData.data());


  fft.performRealOnlyInverseTransform(stft.fftData.data());

  const float overlapSclae = 1.0f / 1.5f;
  window.multiplyWithWindowingTable(stft.fftData.data(), fftSize);

  const int outputBufferSize = static_cast<int>(stft.outputBuffer.size());
  const int currentWritePos = stft.readPos;

  for (int i = 0; i < fftSize; i++) {
    const int outIdx = (currentWritePos + i) % outputBufferSize;
    stft.outputBuffer[static_cast<size_t>(outIdx)] += stft.fftData[static_cast<size_t>(i)] * overlapSclae;
  }

  std::copy(stft.inputBuffer.begin() + hopSize, stft.inputBuffer.end(), stft.inputBuffer.begin());
  stft.writePos = fftSize - hopSize;
}

void HarmonicGlitchAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
  juce::ScopedNoDenormals noDenormals;

  const int numChannels = buffer.getNumChannels();
  const int numSamples = buffer.getNumSamples();

  if (stftChannels.size() != static_cast<size_t>(numChannels)) {
    stftChannels.resize(static_cast<size_t>(numChannels));
    for (auto& stft : stftChannels) {
      stft.prepare();
    }
  }

  juce::AudioBuffer<float> dryBuffer;
  dryBuffer.makeCopyOf(buffer);

  smoothedDryWet.setTargetValue(dryWetParam->load());


  for (int i = 0; i < numSamples; i++) {
    const float mix = smoothedDryWet.getNextValue();

    const float dryGain = std::cos(mix * juce::MathConstants<float>::halfPi);
    const float wetGain = std::sin(mix * juce::MathConstants<float>::halfPi);

    for (int ch = 0; ch < numChannels; ch++) {
      auto& stft = stftChannels[static_cast<size_t>(ch)];

      const float inSample = buffer.getSample(ch, i);
      stft.inputBuffer[static_cast<size_t>(stft.writePos)] = inSample;
      stft.writePos++;

      if (stft.writePos >= fftSize) {
        processSTFTFrame(stft);
      }

      const int outputBufferSize = static_cast<int>(stft.outputBuffer.size());
      const float wetSample = stft.outputBuffer[static_cast<size_t>(stft.readPos)];
      stft.outputBuffer[static_cast<size_t>(stft.readPos)] = 0.0f;
      stft.readPos = (stft.readPos + 1) % outputBufferSize;

      const float drySample = dryBuffer.getSample(ch, i);
      buffer.setSample(ch, i, (drySample * dryGain) + (wetSample * wetGain));
    }
  }
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