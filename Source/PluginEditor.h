#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"

namespace PeakColours
{
    static juce::Colour getColour (int index)
    {
        static const juce::Colour palette[8] = {
            juce::Colour (0xffff5252), // P1: レッド / ピンク
            juce::Colour (0xffff9100), // P2: オレンジ
            juce::Colour (0xffffea00), // P3: イエロー
            juce::Colour (0xff00e676), // P4: グリーン
            juce::Colour (0xff00e5ff), // P5: シアン
            juce::Colour (0xff448aff), // P6: ブルー
            juce::Colour (0xffe040fb), // P7: パープル
            juce::Colour (0xffff4081)  // P8: マゼンタ
        };
        return palette[juce::jlimit (0, 7, index)];
    }
}

class CustomLookAndFeel : public juce::LookAndFeel_V4
{
public:
    CustomLookAndFeel()
    {
        setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xffe0e0e0));
        setColour(juce::Label::textColourId, juce::Colour(0xff9e9e9e));
        setColour (juce::Slider::rotarySliderFillColourId, juce::Colour (0xff00e676));
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
            valueArc.addCentredArc(centreX, centreY, arcRadius, arcRadius, 0.0f, rotaryStartAngle, toAngle, true);
            g.setColour(slider.findColour(juce::Slider::rotarySliderFillColourId));
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

class SpectrumVisualizer : public juce::Component, private juce::Timer
{
public:
    SpectrumVisualizer(HarmonicGlitchAudioProcessor& p) : processor (p){
        startTimerHz(30);
    }

    void timerCallback() override{repaint();}

    void paint(juce::Graphics& g) override {
        auto bounds = getLocalBounds().toFloat();
        g.fillAll(juce::Colour(0xff141518));

        const int numBins = HarmonicGlitchAudioProcessor::scopeSize;
        const float w = bounds.getWidth();
        const float h = bounds.getHeight();

        if (w <= 10.0f || h <= 10.0f) return;
        if (numBins <= 1) return;

        juce::Path wavePath;
        wavePath.startNewSubPath(0.0f, h);

        const float minDb = -60.0f;
        const float maxDb = 6.0f;

        bool isFirst = true;

        for (int pixelX = 0; pixelX < getWidth(); pixelX++) {
            float normX = static_cast<float>(pixelX) / w;
            int bin = static_cast<int>(normX * static_cast<float>(numBins - 1));

            float mag = processor.scopeMagnitudes[bin].load(std::memory_order_relaxed);
            if (std::isnan(mag) || std::isinf(mag) || mag < 0.0) mag = 0.0f;

            float db = juce::Decibels::gainToDecibels(mag, minDb);
            float y = juce::jmap(db, minDb, maxDb, h, 0.0f);

            float x = static_cast<float>(pixelX);
            y = juce::jlimit(0.0f, h, y);

            if (isFirst) {
                wavePath.startNewSubPath(x, y);
                isFirst = false;
            }
            else {
                wavePath.lineTo(x, y);
            }

        }

        g.setColour(juce::Colour(0xff00e676).withAlpha(0.15f));
        juce::Path strokePath = wavePath;
        strokePath.lineTo(w, h);
        strokePath.lineTo(0.0f, h);
        strokePath.closeSubPath();
        g.fillPath(strokePath);

        g.setColour(juce::Colour(0xff00e676));
        g.strokePath(wavePath, juce::PathStrokeType(1.5f));

        int numPeaks = processor.scopeNumPeaks.load(std::memory_order_relaxed);
        numPeaks = juce::jlimit(0, 16, numPeaks);


        for (int i = 0;i < numPeaks && i < 16; i++) {
            int srcBin = processor.scopePeakBins[i].load(std::memory_order_relaxed);
            float dstBin = processor.scopeTargetBins[i].load(std::memory_order_relaxed);

            if (std::isnan(dstBin) || std::isinf(dstBin)) dstBin = 0.0f;

            float srcX = juce::jmap(static_cast<float>(srcBin), 0.0f, static_cast<float>(numBins), 0.0f, w);
            float dstX = juce::jmap(dstBin, 0.0f, static_cast<float>(numBins), 0.0f, w);

            const auto peakColour = PeakColours::getColour(i);

            g.setColour (peakColour.withAlpha(0.4f));
            for (float dashY = 0.0f; dashY < h; dashY += 8.0f)
            {
                g.fillRect (srcX, dashY, 1.0f, 4.0f);
            }

            g.setColour(peakColour);
            g.drawLine(dstX, 0, dstX, h, 1.5f);

            g.setFont(10.0f);
            g.drawText("P" + juce::String(i + 1), static_cast<int>(dstX) - 10, 2, 20, 12, juce::Justification::centred);
        }

        g.setColour(juce::Colour(0xff32363e));
        g.drawRect(getLocalBounds(), 1);
    }
private:
    HarmonicGlitchAudioProcessor& processor;

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

    SpectrumVisualizer spectrumVisualizer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HarmonicGlitchAudioProcessorEditor)
};


