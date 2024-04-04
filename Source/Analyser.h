/*
 Analyser mainly does the FFT analyse and calculate the frequency with highest magnitude
 
 The threading part reuses code from Frequalizer by Daniel Walz https://github.com/ffAudio/Frequalizer/blob/master/Source/Analyser.h
 
 */

#pragma once

#include <juce_dsp/juce_dsp.h>
#include <juce_gui_basics/juce_gui_basics.h>

//==============================================================================
/*
*/
template<typename Type>
class Analyser : public juce::Thread
{
public:
    Analyser();
    ~Analyser() override = default;

    void addAudioData (const juce::AudioBuffer<Type>& buffer, int startChannel, int numChannels);
    void getMaxBin(const juce::AudioBuffer<Type>& buffer);
    
    void setupAnalyser(int audioFifoSize, Type sampleRateToUse);
    void run() override;

    void createPath (juce::Path& p, const juce::Rectangle<float> bounds, float minFreq);

    void createAnalyserPlot(juce::Path& p, const juce::Rectangle<int> bounds, float minFreq);
    
    bool checkForNewData();
    
    // current frequency with max magnitude, its fft index, magnitude value, and freqency
    int maxIdx = 100;
    float maxMag = 0.01f;
    float maxFreq = 400.0f;
    
    std::vector<float> reductions = {0.0};
    
    int totalNumberOfPeaks = 1;
    
    int filteredNumberOfPeaks = 1;

    int fftSize;
    
    std::atomic<float> *freqLowParam;
    std::atomic<float> *freqHiParam;
    
    inline float indexToFreq(int index, int sr) const{
        if (index == 0){
            index = 1;
        }
        const float freq = (sampleRate * index) / fft.getSize();
        return freq;
    }
    
    inline float FreqToIndex(int freq){
        int idx = juce::roundToInt(freq * fft.getSize() / sampleRate);
        return idx;
    }
    
    const float* getFftPointer(int channelIdx){
        return averager.getReadPointer(channelIdx);
    }
    

    
    
private:
    
    juce::WaitableEvent waitForData;
    juce::CriticalSection pathCreationLock;

    Type sampleRate {};

    juce::dsp::FFT fft                           { 11 };
    juce::dsp::WindowingFunction<Type> windowing { size_t (fft.getSize()), juce::dsp::WindowingFunction<Type>::hann, true };
    juce::AudioBuffer<float> fftBuffer           { 1, fft.getSize() * 2 }; // {1, 4096}

    juce::AudioBuffer<float> averager            { 5, fft.getSize() / 2 }; // {5, 1024}
    int averagerPtr = 1;
    
    int sr = 48000;
    float threshold = 0.0;
    juce::AbstractFifo abstractFifo              { 48000 };
    juce::AudioBuffer<Type> audioFifo;

    std::atomic<bool> newDataAvailable;
    
    

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Analyser)
};
