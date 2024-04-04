/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "Analyser.h"
#include "PluginVisualiser.h"

using namespace juce;
typedef AudioProcessorValueTreeState::SliderAttachment SliderAttachment;
typedef AudioProcessorValueTreeState::ButtonAttachment ButtonAttachment;
typedef Slider::SliderStyle SliderStyle;


//==============================================================================
/**
*/
class ResonanceSuppressorAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    ResonanceSuppressorAudioProcessorEditor (ResonanceSuppressorAudioProcessor&, AudioProcessorValueTreeState&);
    ~ResonanceSuppressorAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
    void addSlider(String name, String labelText, Slider& slider, Label& label, Slider::SliderStyle style, bool onLeft, Slider::TextEntryBoxPosition textPos, int width, int height, std::unique_ptr<SliderAttachment>& attachment);
    
    
    
private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.

    PluginVisualiser visualiser;
    
    
    ResonanceSuppressorAudioProcessor& audioProcessor;
    
    AudioProcessorValueTreeState& params;
    
    Slider ratio, attack, release, thresh, freqLow, freqHi, speed, quality, peaks;
    Label ratioL, attackL, releaseL, threshL, freqLowL, freqHiL, speedL, qualityL, peaksL;
    
    Slider mainGain;
    Label mainGainLabel;
    
    Slider qSlider;
    Label qLabel;
    
    Label title, dwLabel;
    
    Slider drywet;
    
    juce::TextButton bypass{ TRANS ("Bypass") };
    juce::TextButton delta{ TRANS ("Delta") };
    
    // 
    std::unique_ptr<SliderAttachment> mainGainAttachment,qAttachment,dwAttachment;
    std::unique_ptr<ButtonAttachment> bpAttachment,dtAttachment;
    std::unique_ptr<SliderAttachment> ratioAttachment, attackAttachment, releaseAttachment, threshAttachment, freqLowAttachment, freqHiAttachment, speedAttachment, qualityAttachment, peaksAttachment;
    
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ResonanceSuppressorAudioProcessorEditor)
};
