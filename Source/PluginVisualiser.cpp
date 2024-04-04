/*
 Visualiser handles visualisations of the spectrum and the compressor's response curves.
 
 The grid drawing part is borrowed from Frequalizer by Daniel Walz https://github.com/ffAudio/Frequalizer/blob/master/Source/Analyser.h
 
 */

#include <JuceHeader.h>
#include "PluginVisualiser.h"



PluginVisualiser::PluginVisualiser(ResonanceSuppressorAudioProcessor& p) : processor(p)
{
    startTimerHz(fps); // Refresh rate
    
    for (int i = 0; i < 1; i++){
        juce::Rectangle<float>* rect;
        pointAreas.add(rect = new juce::Rectangle<float>(8, 8));
    }
    
    
    for (int i = 0; i < processor.numberOfAnalysers; i++){
        Compressor* comp = &processor.allCompressors.getReference(i);
        comp->createMagnitudes(responseResolution);
    }
    
    frequencies.resize (responseResolution);
    for (size_t i=0; i < frequencies.size(); ++i) {
        frequencies [i] = 20.0 * std::pow (2.0, i / 25.6);
    }
    magnitudes.resize(frequencies.size(), 1.0);
    lastMagnitudes.resize(frequencies.size(), 1.0);
    
    fftSize = processor.analysers[0]->fftSize;
    
}

void PluginVisualiser::timerCallback(){
    if (processor.analyser1.checkForNewData()){
        repaint();
    }
}

void PluginVisualiser::paint(juce::Graphics& g){
    const auto factor = plotFrame.getWidth() / 10.0f;
    
    g.setColour(juce::Colours::white);
    
    g.setFont (12.0f);
    
    g.setColour(juce::Colour::fromFloatRGBA (1.0f, 1.0f, 1.0f, 0.05f));
    int startX = juce::roundToInt(factor * freqToX(freqLowParam, 20.0f));
    int endX = std::max(0, juce::roundToInt(factor * freqToX(freqHiParam, 20.0f)) - startX);
    g.fillRect(startX, 0, std::min(plotFrame.getWidth() - startX, endX) , plotFrame.getHeight());
    
    g.setColour (juce::Colours::silver);
    g.drawRect(plotFrame.toFloat(), 1.0f);
    
    
    // draw the spectrum grid
    for (int i=0; i < 10; ++i) {
        g.setColour (juce::Colours::silver.withAlpha (0.3f));
        auto x = plotFrame.getX() + plotFrame.getWidth() * i * 0.1f;
        if (i > 0) g.drawVerticalLine (juce::roundToInt (x), float (plotFrame.getY()), float (plotFrame.getBottom()));

        g.setColour (juce::Colours::silver);
        auto freq = getFrequencyForPosition (i * 0.1f);
        g.drawFittedText ((freq < 1000) ? juce::String (freq) + " Hz"
                                        : juce::String (freq / 1000, 1) + " kHz",
                          juce::roundToInt(x + 3), plotFrame.getBottom() - 18, 50, 15, juce::Justification::left, 1);
    }
    
    // create the spectrum visualisation
    createPath(analyserPath, processor.analysers[0]->getFftPointer(0), int(fftSize/2.0f), plotFrame.toFloat(), 20.0f);
    
    if (smoothLines){
        roundedPath = analyserPath.createPathWithRoundedCorners(8.0f);
        g.strokePath (roundedPath, juce::PathStrokeType (1.0));
    } else {
        g.strokePath (analyserPath, juce::PathStrokeType (1.0));
    }
    
    g.drawHorizontalLine(juce::roundToInt(dbToY(threshParam, 18, plotFrame)), plotFrame.getX(), plotFrame.getRight());
    g.drawHorizontalLine(plotFrame.getBottom()-18, plotFrame.getX(), plotFrame.getRight());
    
    


    // draw detected peaks
    
//    for (int i = 0; i < processor.numberOfAnalysers; i++){
//        int idx = processor.analyser.maxIdx;
//        float amplitude = processor.analyser.maxMag;
//        int x = 0;
//        float y = 0.0;
//        
//        x = juce::roundToInt(factor * processor.analyser.indexToX(idx, 20.0f));
//        y = processor.analyser.binToY(amplitude, plotFrame.toFloat());
//        pointAreas[i]->setCentre(plotFrame.getX() + x, plotFrame.getY() + y);
//        g.fillEllipse(*pointAreas[i]);
//    }
//    
//    for (int i = 0; i < comp->totalFilters; i++){
//        float freq = comp->filterFreqs[i];
//        float gain = comp->filterGains[i];
//        int x = juce::roundToInt(factor * freqToX(freq, 20.0f));
//        float y = juce::roundToInt(magToY(gain, plotFrame));
//        pointAreas2[i]->setCentre(plotFrame.getX() + x, plotFrame.getY() + y);
//        g.fillEllipse(*pointAreas2[i]);
//    }
    
    g.drawFittedText ("fps: " + juce::String(fps),
                      plotFrame.getWidth() - 100,
                      plotFrame.getY(), 95, 15, juce::Justification::right, 1);

    // draw compresser responses
    createResponsePlot(frequencyResponse, plotFrame, 0);
    g.strokePath (frequencyResponse, juce::PathStrokeType (1.0));
    
    if (peaksParam >= 2) {
        createResponsePlot(frequencyResponse2, plotFrame, 1);
        g.strokePath (frequencyResponse2, juce::PathStrokeType (1.0));
    }
    if (peaksParam >= 3) {
        createResponsePlot(frequencyResponse3, plotFrame, 2);
        g.strokePath (frequencyResponse3, juce::PathStrokeType (1.0));
    }
    if (peaksParam >= 4) {
        createResponsePlot(frequencyResponse4, plotFrame, 3);
        g.strokePath (frequencyResponse4, juce::PathStrokeType (1.0));
    }
}

void PluginVisualiser::createPath(juce::Path& p, const float* averager, int numSamples, const juce::Rectangle<float> bounds, float minFreq){
    p.clear();
    p.preallocateSpace (8 + numSamples * 3);
    juce::ScopedLock lockedForReading (pathCreationLock);
    const auto  factor  = bounds.getWidth() / 10.0f;
    
    p.startNewSubPath (bounds.getX() + factor * indexToX(0, minFreq, fftSize), binToY(averager[0], bounds));
    for (int i = 0; i < numSamples; ++i)
        p.lineTo (bounds.getX() + factor * indexToX(float(i), minFreq, fftSize), binToY(averager[i], bounds));
}

void PluginVisualiser::createResponsePlot(juce::Path& p, const juce::Rectangle<int> bounds, int idx){
    
    Compressor* comp = &processor.allCompressors.getReference(idx);
    comp->getMagnitudes(frequencies.data(), frequencies.size());
    p.clear();
    p.preallocateSpace (8 + int(frequencies.size()) * 3);
    juce::ScopedLock lockedForReading (pathCreationLock);
    const auto factor = (plotFrame.getWidth() / responseResolution) * 1.055f;
    
    p.startNewSubPath(bounds.getX(), magToY(comp->magnitudes[0], bounds));
    
    for (int i=1; i < responseResolution; ++i) {
        
        p.lineTo (bounds.getX() + factor * i, magToY(comp->magnitudes[i], bounds));
    }
    
}


float PluginVisualiser::getFrequencyForPosition (float pos)
{
    return 20.0f * std::pow (2.0f, pos * 10.0f);
}


