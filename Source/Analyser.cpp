/*
 Analyser mainly does the FFT analyse and calculate the frequency with highest magnitude
 
 The threading part reuses code from Frequalizer by Daniel Walz https://github.com/ffAudio/Frequalizer/blob/master/Source/Analyser.h
 
 */


#include <iostream>
#include <vector>
#include <algorithm>
#include <utility>
#include <cmath>

#include "Analyser.h"


template<> Analyser<float>::Analyser() : juce::Thread ("Frequaliser-Analyser")
{
    averager.clear();
    fftBuffer.clear();
    
    fftSize = fft.getSize();
}

template<> void Analyser<float>::addAudioData (const juce::AudioBuffer<float>& buffer, int startChannel, int numChannels)
{
    if (abstractFifo.getFreeSpace() < buffer.getNumSamples())
        return;

    int start1, block1, start2, block2;
    abstractFifo.prepareToWrite (buffer.getNumSamples(), start1, block1, start2, block2);
    audioFifo.copyFrom (0, start1, buffer.getReadPointer (startChannel), block1);
    if (block2 > 0)
        audioFifo.copyFrom (0, start2, buffer.getReadPointer (startChannel, block1), block2);

    for (int channel = startChannel + 1; channel < startChannel + numChannels; ++channel)
    {
        if (block1 > 0) audioFifo.addFrom (0, start1, buffer.getReadPointer (channel), block1);
        if (block2 > 0) audioFifo.addFrom (0, start2, buffer.getReadPointer (channel, block1), block2);
    }
    abstractFifo.finishedWrite (block1 + block2);
    waitForData.signal();
}

template<> void Analyser<float>::getMaxBin(const juce::AudioBuffer<float>& buffer){
    
    const auto* fftData = buffer.getReadPointer(0);
    
    int idxLow = FreqToIndex(*freqLowParam);
    int idxHi = FreqToIndex(*freqHiParam);
    
    if (idxLow < idxHi){
        maxIdx = idxLow;
        maxMag = fftData[idxLow];
        for (int i = idxLow + 1; i < idxHi; i++){
            if (fftData[i] > maxMag){
                maxMag = fftData[i];
                maxIdx = i;
            }
        }
        maxFreq = indexToFreq(maxIdx, sr);
    } else {
        // TODO
    }
}


template<> void Analyser<float>::setupAnalyser (int audioFifoSize, float sampleRateToUse){
    sampleRate = sampleRateToUse;
    audioFifo.setSize (1, audioFifoSize);
    abstractFifo.setTotalSize (audioFifoSize);

//    startThread(5);
    startThread(juce::Thread::Priority::normal); // break change in JUCE 7.0.3
}


template<> void Analyser<float>::run(){
    
    while (! threadShouldExit()){
        
        if (abstractFifo.getNumReady() >= fft.getSize()){
            fftBuffer.clear();

            int start1, block1, start2, block2;
            abstractFifo.prepareToRead (fft.getSize(), start1, block1, start2, block2);
            if (block1 > 0) fftBuffer.copyFrom (0, 0, audioFifo.getReadPointer (0, start1), block1);
            if (block2 > 0) fftBuffer.copyFrom (0, block1, audioFifo.getReadPointer (0, start2), block2);
            abstractFifo.finishedRead ((block1 + block2) / 2);

            windowing.multiplyWithWindowingTable (fftBuffer.getWritePointer (0), size_t (fft.getSize()));
            fft.performFrequencyOnlyForwardTransform (fftBuffer.getWritePointer (0));

            juce::ScopedLock lockedForWriting (pathCreationLock);
            averager.addFrom (0, 0, averager.getReadPointer (averagerPtr), averager.getNumSamples(), -1.0f);
            averager.copyFrom (averagerPtr, 0, fftBuffer.getReadPointer(0), averager.getNumSamples(), 1.0f / (averager.getNumSamples() * (averager.getNumChannels() - 1)));
            averager.addFrom (0, 0, averager.getReadPointer (averagerPtr), averager.getNumSamples());
            if (++averagerPtr == averager.getNumChannels()) averagerPtr = 1;

            newDataAvailable = true;
            
            getMaxBin(averager);
        }

        if (abstractFifo.getNumReady() < fft.getSize())
            waitForData.wait (100);
    }
}

template<> bool Analyser<float>::checkForNewData() {
    auto available = newDataAvailable.load();
    newDataAvailable.store (false);
    return available;
}

