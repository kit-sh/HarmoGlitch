#pragma once

#if defined (__APPLE__)
 #include <dispatch/dispatch.h>
#endif

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <vector>

class HarmonicGlitchAudioProcessor : public juce::AudioProcessor
{
public:
  HarmonicGlitchAudioProcessor();
  ~HarmonicGlitchAudioProcessor() override;

  void prepareToPlay (double sampleRate, int samplesPerBlock) override;
  void releaseResources() override;
  bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
  void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

  juce::AudioProcessorEditor* createEditor() override;
  bool hasEditor() const override { return true; }

  const juce::String getName() const override { return JucePlugin_Name; }
  bool acceptsMidi() const override { return false; }
  bool producesMidi() const override { return false; }
  bool isMidiEffect() const override { return false; }
  double getTailLengthSeconds() const override { return 0.0; }

  int getNumPrograms() override { return 1; }
  int getCurrentProgram() override { return 0; }
  void setCurrentProgram (int) override {}
  const juce::String getProgramName (int) override { return {}; }
  void changeProgramName (int, const juce::String&) override {}

  void getStateInformation (juce::MemoryBlock& destData) override;
  void setStateInformation (const void* data, int sizeInBytes) override;

  juce::AudioProcessorValueTreeState apvts;



private:
  static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

  static constexpr int MAX_PEAKS = 8;


  std::atomic<float>* dryWetParam = nullptr;
  std::atomic<float>* topNParam = nullptr;
  std::atomic<float>* noiseVolParam = nullptr;

  std::array<std::atomic<float>*, MAX_PEAKS> peakPitchParams {};
  std::array<std::atomic<float>*, MAX_PEAKS> peakVolParams {};

  juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedDryWet;

  static constexpr int fftOrder = 11;
  static constexpr int fftSize = 1 << fftOrder;
  static constexpr int hopSize = 512;

  juce::dsp::FFT fft { fftOrder};
  juce::dsp::WindowingFunction<float> window { static_cast<size_t>(fftSize), juce::dsp::WindowingFunction<float>::hann };

  struct ChannelSTFT
  {
      std::vector<float> inputBuffer;
      int writePos = 0;

      std::vector<float> outputBuffer;
      int readPos = 0;

      std::vector<float> fftData;

      std::vector<float> harmonicData;
      std::vector<float> noiseData;

      void prepare() {
          inputBuffer.assign(fftSize, 0.0f);
          outputBuffer.assign(fftSize * 2, 0.0f);
          fftData.assign(fftSize * 2, 0.0f);

          harmonicData.assign(fftSize * 2, 0.0f);
          noiseData.assign(fftSize * 2, 0.0f);

          writePos = 0;
          readPos = 0;
      }
  };

  std::vector<ChannelSTFT> stftChannels;

  void processSTFTFrame(ChannelSTFT& stft);

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HarmonicGlitchAudioProcessor)


};