#include "PluginProcessor.h"

#include <dispatch/data.h>

#include "PluginEditor.h"
#include "../../../../../../Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/System/Library/Frameworks/ApplicationServices.framework/Frameworks/PrintCore.framework/Headers/PMPrintingDialogExtensions.h"

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

  struct Peak {
    int bin;
    float magnitude;
    float prominence;
    float real;
    float imag;
  };

  const int numBins = fftSize / 2;
  std::vector<float> magnitudes(numBins, 0.0f);

  for (int i=0; i<numBins; i++) {
    const float real = stft.fftData[static_cast<size_t>(2*i)];
    const float imag = stft.fftData[static_cast<size_t>(2*1 + 1)];
    magnitudes[static_cast<size_t>(i)] = std::sqrt(real * real + imag * imag);
  }

  std::vector<int> peakIndices;
  peakIndices.reserve(static_cast<size_t>(numBins/2));

  int peakStart = -1;
  for (int i=1; i<numBins; i++) {
    if (magnitudes[i] > magnitudes[i - 1]) {
      peakStart = i;
    }
    else if (magnitudes[i] < magnitudes[i - 1]) {
      const int peakCenter = peakStart + (i - 1 - peakStart) / 2;
      peakIndices.push_back(peakCenter);
      peakStart = -1;
    }
  }

  std::vector<Peak> peaks;
  peaks.reserve(peakIndices.size());

  for (int pIdx : peakIndices) {
    const float pMag = magnitudes[pIdx];

    float minLeft = pMag;
    for (int i = pIdx - 1; i >= 0; i--) {
      if (magnitudes[i] > pMag) break;
      minLeft = std::min(minLeft, magnitudes[i]);
    }

    float minRight = pMag;
    for (int i=pIdx + 1; i < peaks.size(); i++) {
      if (magnitudes[i] > pMag) break;
      minRight = std::min(minRight, magnitudes[i]);
    }

    const float base = std::max(minLeft, minRight);

    const float prominence = pMag - base;

    if (prominence > 0.005f) {
      peaks.push_back({
        pIdx,
        pMag,
        prominence,
        stft.fftData[static_cast<size_t> (2 * pIdx)],
        stft.fftData[static_cast<size_t> (2 * pIdx + 1)]
      });
    }
  }

  const size_t topN = 16;
  if (peaks.size() > topN) {
    std::partial_sort(peaks.begin(), peaks.begin() + topN, peaks.end(), [](const Peak& a, const Peak& b) {return a.magnitude > b.magnitude;});
    peaks.resize(topN);
  }

  std::copy(stft.fftData.begin(), stft.fftData.end(), stft.noiseData.begin());

  for (const auto& peak : peaks) {
    const int startBin = std::max(0, peak.bin - 1);
    const int endBin = std::min(numBins - 1, peak.bin + 1);

    for (int b = startBin; b <= endBin; b++) {
      stft.noiseData[static_cast<size_t>(2*b)] = 0.0f;
      stft.noiseData[static_cast<size_t>(2*b + 1)] = 0.0f;
    }
  }

  std::fill(stft.harmonicData.begin(), stft.harmonicData.end(), 0.0f);

  const float pitchRatio = 1.5f;

  for (const auto& peak : peaks) {

    //仮
    const int startBin = std::max (0, peak.bin - 1);
    const int endBin   = std::min (numBins - 1, peak.bin + 1);

    for (int b = startBin; b <= endBin; ++b)
    {
      // ピッチシフト位置の計算
      int targetBin = static_cast<int> (std::round (b * pitchRatio));

      if (targetBin > 0 && targetBin < numBins)
      {
        stft.harmonicData[static_cast<size_t> (2 * targetBin)]     = stft.fftData[static_cast<size_t> (2 * b)];
        stft.harmonicData[static_cast<size_t> (2 * targetBin + 1)] = stft.fftData[static_cast<size_t> (2 * b + 1)];
      }
    }
  }

  const float harmonicLevel = 1.0f;
  const float noiseLevel = 1.0f;

  for (size_t i = 0; i < stft.fftData.size(); i++) {
    stft.fftData[i] = (stft.harmonicData[i] * harmonicLevel) + (stft.noiseData[i] * noiseLevel);
  }


  fft.performRealOnlyInverseTransform(stft.fftData.data());

  const float overlapSclae = 1.0f / (1.5f * 2.0f);
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