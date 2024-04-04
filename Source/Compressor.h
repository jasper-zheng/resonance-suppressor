/*
 Compressor contains four dynamic filters working in a circular mode,
 It controls these four filter's gain and frequencies in response to the detected resonance
 The pointed filter is applied to the resonance
 
 The timing function calculateGain() borrowed from the tutorial https://github.com/joshreiss/Audio-programming-with-VST-and-JUCE/tree/main/06%20-%20Dynamics%20Processing/A-%20Compressor 
 */

#pragma once
class Compressor {
public:
    Compressor();
    bool updateGain(float magnitude);
    bool updateGain();
    void updateFreqAndIdx(float f, int idx);
    int updateIdx(int idx, int fftSize);
    float calculateGain(float mag, float oldGain);
    float freqShift(float data);
    void setSpec(double sr,int samplesPerBlock, int numChannels);
    void createMagnitudes(int responseResolution);
    void getMagnitudes(const double* frequencies, size_t numSamples);
    
    float indexToFreq(int idx, int fftSize);
    
    float freqToX (float freq){
        float a = std::log (freq / 20.0f);
        float b = std::log (2.0f);
        float x = 0.0f;
        if (freq > 0.01f){
            x = a/b;
        }
        return x/10.0f;
    }
    
    std::atomic<float> *threshParam;
    std::atomic<float> *ratioParam;
    std::atomic<float> *attackParam;
    std::atomic<float> *releaseParam;
    std::atomic<float> *speedParam;
    std::atomic<float> *qParam;
    
    float thresh = -30.0f; // -40.0 - 0.0
    float ratio = 4.0f; // 1.0 - 20.0
    float attack = 0.1f; // 0.01 - 0.99, higher the quicker
    float release = 0.1f;
    
    int index = 100;
    float lastFreq = 200.0f;
    float freqSensitivity = 0.3f; // 0.01 - 0.99, higher the quicker  (1-f)*10 -> 0.1 - 9.9
    
    float q = 10.0f; // 2 - 20
    
    float gain = 1.0f;
    
    juce::Colour debugColour;
    
    
    std::vector<double> magnitudes;
    
    std::vector<double> magnitude1;
    std::vector<double> magnitude2;
    std::vector<double> magnitude3;
    std::vector<double> magnitude4;
    
    std::vector<double> magnitudeRegular;
    
    juce::Array<juce::dsp::IIR::Coefficients<float>::Ptr> coefficients;
    
    
    juce::dsp::IIR::Coefficients<float>::Ptr coefficient1;
    juce::dsp::IIR::Coefficients<float>::Ptr coefficient2;
    juce::dsp::IIR::Coefficients<float>::Ptr coefficient3;
    juce::dsp::IIR::Coefficients<float>::Ptr coefficient4;
    
    // four peak filters work in a circular mode
    using StereoFilter = juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>>;
    using FiltersChain = juce::dsp::ProcessorChain<StereoFilter,StereoFilter,StereoFilter,StereoFilter>;
    
    FiltersChain filters;
    
    int totalFilters = 4;
    int filterPtr = 0; // index of the pointed filter
    
    std::vector<float> filterGains;
    std::vector<float> filterFreqs;
    std::vector<int> filterIdxes;
    
    const float* fftDataPt;
    
    
private:
    
    double sampleRate;
    int numInputChannels;
    
    
    
};

