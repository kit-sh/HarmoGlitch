#include "PluginProcessor.h"
#include "PluginEditor.h"

HarmonicGlitchAudioProcessorEditor::HarmonicGlitchAudioProcessorEditor (HarmonicGlitchAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    dryWetSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    dryWetSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 20);
    addAndMakeVisible (dryWetSlider);

    dryWetAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.apvts, "drywet", dryWetSlider);

    setSize (400, 300);
}

HarmonicGlitchAudioProcessorEditor::~HarmonicGlitchAudioProcessorEditor() {}

void HarmonicGlitchAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::darkslategrey);

    g.setColour (juce::Colours::white);
    g.setFont (20.0f);
    g.drawFittedText ("HarmoGlitch", getLocalBounds().removeFromTop (40), juce::Justification::centred, 1);
}

void HarmonicGlitchAudioProcessorEditor::resized()
{
    dryWetSlider.setBounds (150, 100, 100, 120);
}