#include "PluginProcessor.h"
#include "PluginEditor.h"

HarmonicGlitchAudioProcessorEditor::HarmonicGlitchAudioProcessorEditor (HarmonicGlitchAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setLookAndFeel(&customLaf);

    setSize(800, 580);
    setupSlider(dryWetSlider, dryWetLabel, "Dry / Wet");
    dryWetAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, "drywet", dryWetSlider);

    setupSlider(topNSlider, topNLabel, "Top N Peaks");
    topNSlider.setNumDecimalPlacesToDisplay(0);
    topNAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, "topN", topNSlider);

    setupSlider(noiseVolSlider, noiseVolLabel, "Noise Vol");
    noiseVolAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, "noiseVol", noiseVolSlider);

    setupSlider(prominenceSlider, prominenceLabel, "Sensitivity");
    prominenceSlider.setNumDecimalPlacesToDisplay(4);
    prominenceAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, "prominence", prominenceSlider);

    setupSlider(masterVolSlider, masterVolLabel, "Master Volume");
    masterVolSlider.setTextValueSuffix(" dB");
    masterVolAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, "masterVol", masterVolSlider);

    for (int i = 0; i < numPeaks; i++) {
        const juce::String pIdx = juce::String(i);

        setupSlider(peakPitchSliders[i], peakPitchLabels[i], "P" + juce::String(i + 1) + "Pitch");
        peakPitchSliders[i].setTextValueSuffix(" Cent");
        peakPitchAttachments[i] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            audioProcessor.apvts, "peak_" + pIdx + "_pitch", peakPitchSliders[i]);

        setupSlider(peakVolSliders[i], peakVolLabels[i], "P" + juce::String(i + 1) + "Gain");
        peakVolSliders[i].setTextValueSuffix(" dB");
        peakVolAttachments[i] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            audioProcessor.apvts, "peak_" + pIdx + "_vol", peakVolSliders[i]);
    }

}

HarmonicGlitchAudioProcessorEditor::~HarmonicGlitchAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

void HarmonicGlitchAudioProcessorEditor::setupSlider(juce::Slider& slider, juce::Label& label, const juce::String& labelText)
{
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 65, 20);
    slider.setDoubleClickReturnValue(true, 0.0);
    addAndMakeVisible(slider);

    label.setText(labelText, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.setFont(12.0f);
    addAndMakeVisible(label);

}

void HarmonicGlitchAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour(0xff18191c));

    g.setColour (juce::Colour(0xffffffff));
    g.setFont (18.0f);
    g.drawText ("HarmoGlitch", 20, 12, 300, 25, juce::Justification::left);

    g.setColour(juce::Colour(0xff222428));
    g.fillRoundedRectangle(15.0f, 45.0f, 790.0f, 135.0f, 8.0f);
    g.fillRoundedRectangle(15.0f, 185.0f, 790.0f, 375.0f, 8.0f);

    g.setColour(juce::Colour(0xff32363e));
    g.drawRoundedRectangle(15.0f, 45.0f, 790.0f, 135.f, 8.0f, 1.0f);
    g.drawRoundedRectangle(15.0f, 185.0f, 790.0f, 375.f, 8.0f, 1.0f);

    g.setColour(juce::Colour(0xff00e676));
    g.setFont(11.0f);
    g.drawText("GLOBAL CONTROLS", 25, 46, 150, 15, juce::Justification::left);
    g.drawText("PEAK PARAMETERS (P1 - P8)", 25, 190, 300, 15, juce::Justification::left);
}

void HarmonicGlitchAudioProcessorEditor::resized()
{
    auto globalArea = juce::Rectangle<int>(25, 62, 770, 105);
    const int globalWidth = globalArea.getWidth() / 5;

    auto setupCell = [](juce::Rectangle<int> area, juce::Label& label, juce::Slider& slider) {
        label.setBounds(area.removeFromTop(16));
        slider.setBounds(area);
    };

    setupCell(globalArea.removeFromLeft(globalWidth), dryWetLabel, dryWetSlider);
    setupCell(globalArea.removeFromLeft(globalWidth), topNLabel, topNSlider);
    setupCell(globalArea.removeFromLeft(globalWidth), noiseVolLabel, noiseVolSlider);
    setupCell(globalArea.removeFromLeft(globalWidth), prominenceLabel, prominenceSlider);
    setupCell(globalArea, masterVolLabel, masterVolSlider);


    auto peakArea = juce::Rectangle<int>(25, 210, 770, 340);
    const int colWidth = peakArea.getWidth() / numPeaks;
    const int rowHeight = peakArea.getHeight() / 2;

    auto pitchRow = peakArea.removeFromTop(rowHeight);
    for (int i = 0; i < numPeaks; i++) {
        auto cell = pitchRow.removeFromLeft(colWidth).reduced(2,0);

        cell.removeFromTop(6);
        peakPitchLabels[i].setBounds(cell.removeFromTop(16));
        cell.removeFromTop(2);
        peakPitchSliders[i].setBounds(cell);
    }

    auto volRow = peakArea;
    for (int i = 0; i< numPeaks; i++) {
        auto cell = volRow.removeFromLeft(colWidth).reduced(2, 0);

        cell.removeFromTop(6);
        peakVolLabels[i].setBounds(cell.removeFromTop(16));
        cell.removeFromTop(2);
        peakVolSliders[i].setBounds(cell);
    }
}