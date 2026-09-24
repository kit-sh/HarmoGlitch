#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"


class CustomLookAndFeel : public juce::LookAndFeel_V4
{
public:
    CustomLookAndFeel()
    {
        setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xffe0e0e0));
        setColour(juce::Label::textColourId, juce::Colour(0xff9e9e9e));
    }

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
        float sliderPosProportional, float rotaryStartAngle, float rotaryEndAngle,
        juce::Slider& slider) override
    {
        auto bounds = juce::Rectangle<float>(x, y, width, height).reduced(3.0f);
        auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
        auto toAngle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);
        auto lineW = 3.2f;
        auto arcRadius = radius - lineW * 1.5f;

        auto centreX = bounds.getCentreX();
        auto centreY = bounds.getCentreY();

        juce::Path backgroundArc;
        backgroundArc.addCentredArc(centreX, centreY, arcRadius, arcRadius, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
        g.setColour(juce::Colour(0xff32363e));
        g.strokePath(backgroundArc, juce::PathStrokeType(lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        if (slider.isEnabled())
        {
            juce::Path valueArc;
            valueArc.addCentredArc(centreX, centreY, arcRadius, arcRadius, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
            g.setColour(juce::Colour(0xff00e676));
            g.strokePath(valueArc, juce::PathStrokeType(lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        auto thumbRadius = arcRadius - 5.0f;
        if (thumbRadius > 0.0f) {
            juce::ColourGradient grad (juce::Colour (0xff525660), centreX, centreY - thumbRadius,
                                       juce::Colour (0xff222428), centreX, centreY + thumbRadius, false);
            g.setGradientFill(grad);
            g.fillEllipse(centreX - thumbRadius, centreY - thumbRadius, thumbRadius*2.0f, thumbRadius*2.0f);

            g.setColour(juce::Colour(0xff18191c));
            g.drawEllipse(centreX - thumbRadius, centreY - thumbRadius, thumbRadius*2.0f, thumbRadius*2.0f, 1.2f);

            juce::Path p;
            auto pointerLength = thumbRadius * 0.7f;
            auto pointerThickness = 2.0f;
            p.addRectangle(-pointerThickness * 0.5f, -thumbRadius + 2.0f, pointerThickness, pointerLength);
            p.applyTransform(juce::AffineTransform::rotation(toAngle).translated(centreX, centreY));

            g.setColour(juce::Colours::white);
            g.fillPath(p);

        }

    }

};

class HarmonicGlitchAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    HarmonicGlitchAudioProcessorEditor (HarmonicGlitchAudioProcessor&);
    ~HarmonicGlitchAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    HarmonicGlitchAudioProcessor& audioProcessor;
    CustomLookAndFeel customLaf;

    static constexpr int numPeaks = 8;

    juce::Slider dryWetSlider, topNSlider, noiseVolSlider, prominenceSlider, masterVolSlider;
    juce::Label dryWetLabel, topNLabel, noiseVolLabel, prominenceLabel, masterVolLabel;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> dryWetAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> topNAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> noiseVolAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> prominenceAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> masterVolAttachment;

    std::array<juce::Slider, numPeaks> peakPitchSliders;
    std::array<juce::Label, numPeaks> peakPitchLabels;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, numPeaks> peakPitchAttachments;

    std::array<juce::Slider, numPeaks> peakVolSliders;
    std::array<juce::Label, numPeaks> peakVolLabels;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, numPeaks> peakVolAttachments;

    void setupSlider(juce::Slider& slider, juce::Label& label, const juce::String& labelText);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HarmonicGlitchAudioProcessorEditor)
};

