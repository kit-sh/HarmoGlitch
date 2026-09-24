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

  topNParam = apvts.getRawParameterValue("topN");
  noiseVolParam = apvts.getRawParameterValue("noiseVol");

  for (int i=0; i < MAX_PEAKS; i++) {
    const juce::String pIdx = juce::String(i);
    peakPitchParams[static_cast<size_t>(i)] = apvts.getRawParameterValue("peak_" + pIdx + "_pitch");
    peakVolParams[static_cast<size_t>(i)] = apvts.getRawParameterValue("peak_" + pIdx + "_vol");
  }
}

HarmonicGlitchAudioProcessor::~HarmonicGlitchAudioProcessor() {}

juce::AudioProcessorValueTreeState::ParameterLayout HarmonicGlitchAudioProcessor::createParameterLayout()
{
  std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

  params.push_back(std::make_unique<juce::AudioParameterFloat>(
    juce::ParameterID{"drywet", 1}, "Dry/Wet",
    juce::NormalisableRange<float>{0.0f, 1.0f, 0.01f},
    0.5f
    ));

  params.push_back(std::make_unique<juce::AudioParameterInt>(
    juce::ParameterID{"topN", 1}, "Active Peaks", 1, MAX_PEAKS, MAX_PEAKS
    ));

  params.push_back(std::make_unique<juce::AudioParameterFloat>(
    juce::ParameterID{"noiseVol", 1}, "Noise Level",
    juce::NormalisableRange<float>{0.0f, 2.0f, 0.01f}, 1.0f
    ));

  params.push_back(std::make_unique<juce::AudioParameterFloat>(
    juce::ParameterID{"masterVol", 1},
    "Master Vol",
    juce::NormalisableRange<float>(-48.0f, 12.0f, 0.1f, 0.5f),
    0.0f
    ));

  params.push_back(std::make_unique<juce::AudioParameterFloat>(
    juce::ParameterID{"prominence", 1},
    "Prominence",
    juce::NormalisableRange<float>{0.0001f, 0.05f, 0.0001, 0.4f},
    0.005f
    ));

  for (int i = 0; i < MAX_PEAKS; i++) {
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID{"peak_" + juce::String(i) + "_pitch", 1},
      "Peak " + juce::String(i+1) + " Pitch",
      juce::NormalisableRange<float>{-200.0f, 200.0f, 1.0f}, 0.0f
      ));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID{"peak_" + juce::String(i) + "_vol", 1},
      "Peak " + juce::String(i+1) + " Vol",
      juce::NormalisableRange<float>{-48.0f, 12.0f, 0.1f}, 0.0f
      ));
  }

  return {params.begin(), params.end()};
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
    const float imag = stft.fftData[static_cast<size_t>(2*i + 1)];
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
    for (int i=pIdx + 1; i < numBins; i++) {
      if (magnitudes[i] > pMag) break;
      minRight = std::min(minRight, magnitudes[i]);
    }

    const float base = std::max(minLeft, minRight);

    const float prominence = pMag - base;

    const float prominenceThreshold = *apvts.getRawParameterValue("prominence");

    if (prominence > prominenceThreshold) {
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

  const int activeTopN = *apvts.getRawParameterValue("topN");
  const size_t processCount = std::min(static_cast<size_t>(activeTopN), peaks.size());

  for (size_t i = 0; i < processCount; i++){
    const juce::String pIdx = juce::String(i);

    const float cents = *apvts.getRawParameterValue("peak_" + pIdx + "_pitch");
    const float peakDb = *apvts.getRawParameterValue("peak_" + pIdx + "_vol");
    const float peakGain = (peakDb <= -47.9f) ? 0.0f : juce::Decibels::decibelsToGain(peakDb);


    const float pitchRatio = std::pow(2.0f, cents / 1200.0f);

    const auto& peak = peaks[i];
    const int startBin = std::max(0, peak.bin -1);
    const int endBin = std::min(numBins - 1, peak.bin + 1);

    for (int b = startBin; b <= endBin; b++) {
      const int targetBin = static_cast<int>(std::round(b * pitchRatio));

      if (targetBin > 0 && targetBin < numBins) {
        stft.harmonicData[static_cast<size_t>(2 * targetBin)] += stft.fftData[static_cast<size_t>(2 * b)] * peakGain;
        stft.harmonicData[static_cast<size_t>(2 * targetBin + 1)] += stft.fftData[static_cast<size_t>(2 * b + 1)] * peakGain;
      }
    }
  }

  const float harmonicLevel = 1.0f;
  const float noiseLevel = 1.0f;

  for (size_t i = 0; i < stft.fftData.size(); i++) {
    stft.fftData[i] = (stft.harmonicData[i] * harmonicLevel) + (stft.noiseData[i] * noiseLevel);
  }


  fft.performRealOnlyInverseTransform(stft.fftData.data());

  const float overlapScale = 1.0f / (1.5f * 2.0f);
  window.multiplyWithWindowingTable(stft.fftData.data(), fftSize);

  const int outputBufferSize = static_cast<int>(stft.outputBuffer.size());
  const int currentWritePos = stft.readPos;

  for (int i = 0; i < fftSize; i++) {
    const int outIdx = (currentWritePos + i) % outputBufferSize;
    stft.outputBuffer[static_cast<size_t>(outIdx)] += stft.fftData[static_cast<size_t>(i)] * overlapScale;
  }

  std::copy(stft.inputBuffer.begin() + hopSize, stft.inputBuffer.end(), stft.inputBuffer.begin());
  stft.writePos = fftSize - hopSize;
}

void HarmonicGlitchAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
  juce::ScopedNoDenormals noDenormals;

  const float inputThreshold = 0.001f;
  if (buffer.getRMSLevel(0, 0, buffer.getNumSamples()) < inputThreshold) {
    buffer.clear();
    return;
  }

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

  const float masterDb = *apvts.getRawParameterValue("masterVol");
  const float masterGain = (masterDb <= -47.9) ? 0.0f : juce::Decibels::decibelsToGain(masterDb);

  for (int ch = 0; ch < numChannels; ch++) {
    float* channelData = buffer.getWritePointer(ch);

    for (int i = 0; i < numChannels; i++) {
      float s = channelData[i];

      if (std::isnan(s) || std::isinf(s)) s = 0.0f;

      s = std::tanh(s);

      s *= masterGain;

      channelData[i] = s;
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