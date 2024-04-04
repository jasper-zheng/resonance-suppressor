/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
//#include <juce_Justification.h>

static float maxDB = 24.0f;

//==============================================================================
ResonanceSuppressorAudioProcessorEditor::ResonanceSuppressorAudioProcessorEditor (ResonanceSuppressorAudioProcessor& p, AudioProcessorValueTreeState& state)
    : AudioProcessorEditor (&p), visualiser(p), audioProcessor (p), params(state)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    
    setSize(870, 500);
    
    title.setText("Adaptive Resonance\nSuppressor", dontSendNotification);
    addAndMakeVisible(title);
    title.setBounds(25, 20, 150, 40);
    
    addSlider("ratio", "Ratio", ratio, ratioL, SliderStyle::RotaryVerticalDrag, false, Slider::TextBoxBelow, 60, 15, ratioAttachment);
    
    addSlider("freqLow", "FreqLow", freqLow, freqLowL, SliderStyle::LinearHorizontal, true, Slider::TextBoxBelow, 80, 15, freqLowAttachment);
    freqLow.onValueChange = [this]{visualiser.freqLowParam = freqLow.getValue();};
    visualiser.freqLowParam = freqLow.getValue();
    
    addSlider("freqHi", "FreqHi", freqHi, freqHiL, SliderStyle::LinearHorizontal, true, Slider::TextBoxBelow, 80, 15, freqHiAttachment);
    freqHi.onValueChange = [this]{visualiser.freqHiParam = freqHi.getValue();};
    visualiser.freqHiParam = freqHi.getValue();
    
    addSlider("speed", "Pitch Track", speed, speedL, SliderStyle::RotaryVerticalDrag, false, Slider::TextBoxBelow, 60, 15, speedAttachment);
    addSlider("quality", "Q", quality, qualityL, SliderStyle::RotaryVerticalDrag, false, Slider::TextBoxBelow, 60, 15, qualityAttachment);
    addSlider("peaks", "Analysers", peaks, peaksL, SliderStyle::RotaryVerticalDrag, false, Slider::TextBoxBelow, 60, 15, peaksAttachment);
//    peaks.onValueChange = [this]{visualiser.peaksParam = 2.0f;};
//    visualiser.peaksParam = 2.0f;
        peaks.onValueChange = [this]{visualiser.peaksParam = peaks.getValue();};
        visualiser.peaksParam = peaks.getValue();
    
    
    addSlider("thresh", "Thresh", thresh, threshL, SliderStyle::LinearVertical, false, Slider::TextBoxBelow, 80, 15, threshAttachment);
    thresh.onValueChange = [this]{visualiser.threshParam = thresh.getValue();};
    visualiser.threshParam = thresh.getValue();
    
    addSlider("attack", "Attack", attack, attackL, SliderStyle::RotaryVerticalDrag, false, Slider::TextBoxBelow, 60, 15, attackAttachment);
    addSlider("release", "Release", release, releaseL, SliderStyle::RotaryVerticalDrag, false, Slider::TextBoxBelow, 60, 15, releaseAttachment);
    
    addSlider("gainMain", "Gain", mainGain, mainGainLabel, SliderStyle::LinearVertical, false, Slider::TextBoxBelow, 50, 15, mainGainAttachment);
    
    
    addAndMakeVisible(bypass);
    bpAttachment.reset(new ButtonAttachment(params, "bypass", bypass));
    bypass.setClickingTogglesState(true);
    
    addAndMakeVisible(delta);
    dtAttachment.reset(new ButtonAttachment(params, "delta", delta));
    delta.setClickingTogglesState(true);
    
    addAndMakeVisible(visualiser);
    
}

ResonanceSuppressorAudioProcessorEditor::~ResonanceSuppressorAudioProcessorEditor()
{
}

//==============================================================================
void ResonanceSuppressorAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

//    g.setColour (juce::Colours::white);
    
    g.setFont (12.0f);

}

void ResonanceSuppressorAudioProcessorEditor::resized(){
    
    // position the components
    ratio.setBounds(45, 140, 60, 80);
    freqLow.setBounds(275, 25, 210, 45);
    freqHi.setBounds(325, 80, 210, 45);
    
    speed.setBounds(530, 45, 100, 80);
    quality.setBounds(605, 45, 100, 80);
    peaks.setBounds(680, 45, 100, 80);
    thresh.setBounds(130, 140, 60, 320);
    
    attack.setBounds(45, 260, 60, 80);
    release.setBounds(45, 380, 60, 80);
    
    
    bypass.setBounds(785, 70, 60, 30);
    delta.setBounds(785, 30, 60, 30);
    
    mainGain.setBounds(790, 140, 50, 320);
    visualiser.setBounds(220, 145, 540, 316);
    
    visualiser.plotFrame = getLocalBounds().reduced(3, 3);
    visualiser.plotFrame.setBounds(0, 0, 540, 316);
    
    
    visualiser.pixelsPerDouble = 2.0f * visualiser.plotFrame.getHeight() / juce::Decibels::decibelsToGain (maxDB);
}

// adapted from 03b - SaveState2 - ValueTreeState and AudioParam:
// adding sliders and their labels to the UI
//void ResonanceSuppressorAudioProcessorEditor::addSlider(String name, String labelText, Slider& slider, Label& label, bool isVertical, std::unique_ptr<SliderAttachment>& attachment) {
//
//    // put the slider in vertical
//    if (isVertical) slider.setSliderStyle(Slider::SliderStyle::LinearVertical);
//
//    slider.setTextBoxStyle(Slider::TextBoxBelow, false, 60, 30);
//    addAndMakeVisible(slider);
//    attachment.reset(new SliderAttachment(params, name, slider));
//    label.setText(labelText, dontSendNotification);
//    label.attachToComponent(&slider, !isVertical);
//    addAndMakeVisible(label);
//}
void ResonanceSuppressorAudioProcessorEditor::addSlider(String name, String labelText, Slider& slider, Label& label, SliderStyle style, bool onLeft, Slider::TextEntryBoxPosition textPos, int width, int height, std::unique_ptr<SliderAttachment>& attachment) {
    
    // put the slider in vertical
    slider.setSliderStyle(style);
    
    slider.setTextBoxStyle(textPos, false, width, height);
    addAndMakeVisible(slider);
    attachment.reset(new SliderAttachment(params, name, slider));
    label.setText(labelText, dontSendNotification);
    label.setJustificationType(Justification::Flags::horizontallyCentred);
    label.attachToComponent(&slider, onLeft);
    addAndMakeVisible(label);
    
}

