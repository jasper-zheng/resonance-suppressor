/*
 Visualiser handles visualisations of the spectrum and the compressor's response curves.
 
 The grid drawing part is borrowed from Frequalizer by Daniel Walz https://github.com/ffAudio/Frequalizer/blob/master/Source/Analyser.h
 
 */

 
#include <JuceHeader.h>
#include "PluginProcessor.h"


class PluginVisualiser : public juce::Component, public juce::Timer{
public:
    PluginVisualiser(ResonanceSuppressorAudioProcessor& p);
    
    void timerCallback() override;
    
    void paint(juce::Graphics& g) override;
    
    float getFrequencyForPosition (float pos);
    juce::Rectangle<int> plotFrame;
    juce::Path analyserPath;
    juce::Path roundedPath;
    bool smoothLines = false;
    
    float pixelsPerDouble;
    
    float freqLowParam = 320.0f;
    float freqHiParam = 10200.0f;
    float threshParam = 0.0f;
    float peaksParam = 1.0f;
    
    const int responseResolution = 256;
    
    int fftSize = 2048;
    
private:
    ResonanceSuppressorAudioProcessor& processor;
    juce::CriticalSection pathCreationLock;
    
    int fps = 30;
    
    
    float freqToX (float freq, float minFreq){
        float a = std::log (freq / minFreq);
        float b = std::log (2.0f);
        float x = 0.0f;
        if (freq > 0.01f){
            x = a/b;
        }
        return x;
    }
    
    float magToY(float mag, const juce::Rectangle<int> bounds){
        // TODO
        float a = juce::jmap(mag, 0.0f, 1.0f, 1.0f, 0.0f);
        double y = juce::mapToLog10(double(a),0.1,1.0)-0.1;
        return juce::jmap(float(y), 0.0f, 1.0f, float(bounds.getY()), float(bounds.getBottom()));
    }
    
    float dbToY(float db, int maxY, const juce::Rectangle<int> bounds){
        
        return juce::jmap(db, -64.0f, 6.0f, float(bounds.getBottom()-maxY), float(bounds.getY()));
    }
    
    float binToY (float bin, const juce::Rectangle<float> bounds) const{
        const float infinity = -80.0f;
        return juce::jmap (juce::Decibels::gainToDecibels (bin, infinity),
                           infinity, 0.0f, bounds.getBottom(), bounds.getY());
    }
    
    float indexToX (float index, float minFreq, int fftSize) const{
        const auto freq = (processor.sampleRate * index) / fftSize;
        return (freq > 0.01f) ? std::log (freq / minFreq) / std::log(2.0f) : 0.0f;
    }
    

    void createFrequencyPlot(juce::Path& p, const juce::Rectangle<int> bounds);
    void createResponsePlot(juce::Path& p, const juce::Rectangle<int> bounds, int idx);
    void createPath(juce::Path& p, const float* averager, int numSamples, const juce::Rectangle<float> bounds, float minFreq);
    
    juce::OwnedArray<juce::Rectangle<float>> pointAreas;
    juce::OwnedArray<juce::Rectangle<float>> pointAreas2;

    
    std::vector<double> frequencies;
    std::vector<double> magnitudes;
    std::vector<double> lastMagnitudes;
    juce::Path frequencyResponse;
    juce::Path frequencyResponse2;
    juce::Path frequencyResponse3;
    juce::Path frequencyResponse4;
    
};
