#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"

class HarmonicGlitchAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    HarmonicGlitchAudioProcessorEditor (HarmonicGlitchAudioProcessor&);
    ~HarmonicGlitchAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    HarmonicGlitchAudioProcessor& audioProcessor;

    static constexpr int numPeaks = 8;

    juce::Slider dryWetSlider, topNSlider, noiseVolSlider;
    juce::Label dryWetLabel, topNLabel, noiseVolLabel;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> dryWetAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> topNAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> noiseVolAttachment;

    std::array<juce::Slider, numPeaks> peakPitchSliders;
    std::array<juce::Label, numPeaks> peakPitchLabels;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, numPeaks> peakPitchAttachments;

    std::array<juce::Slider, numPeaks> peakVolSliders;
    std::array<juce::Label, numPeaks> peakVolLabels;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, numPeaks> peakVolAttachments;

    void setupSlider(juce::Slider& slider, juce::Label& label, const juce::String& labelText);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HarmonicGlitchAudioProcessorEditor)
};