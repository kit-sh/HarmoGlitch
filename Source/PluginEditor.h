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

    juce::Slider dryWetSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> dryWetAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HarmonicGlitchAudioProcessorEditor)
};