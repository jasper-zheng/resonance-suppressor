/*
 Compressor contains four dynamic filters working in a circular mode,
 It controls these four filter's gain and frequencies in response to the detected resonance
 The pointed filter is applied to the resonance
 
 The timing function calculateGain() borrowed from the tutorial https://github.com/joshreiss/Audio-programming-with-VST-and-JUCE/tree/main/06%20-%20Dynamics%20Processing/A-%20Compressor
 */


#include <JuceHeader.h>
#include "Compressor.h"

Compressor::Compressor(){

    for (int i = 0; i < totalFilters; i++) {
        filterGains.push_back(1.0f);
        filterFreqs.push_back(400.0f);
        filterIdxes.push_back(10);
        
        coefficients.add(juce::dsp::IIR::Coefficients<float>::Ptr());
    }
}

void Compressor::setSpec(double sr,int samplesPerBlock, int numChannels){
    numInputChannels = numChannels;
    juce::dsp::ProcessSpec spec;
    sampleRate = sr;
    spec.sampleRate = sr;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = numChannels;
    filters.prepare(spec);
    
    
}

void Compressor::createMagnitudes(int responseResolution){
    magnitudes.resize(responseResolution, 1.0);
    magnitude1.resize(responseResolution, 1.0);
    magnitude2.resize(responseResolution, 1.0);
    magnitude3.resize(responseResolution, 1.0);
    magnitude4.resize(responseResolution, 1.0);
    magnitudeRegular.resize(responseResolution, 1.0);
}

float Compressor::calculateGain(float mag, float oldGain){
    //mag data range 0 - 1
    
    // taken from 06 - Dynamics Processing - Compressor look ahead VTS
    
    float dbRMS = 10 * std::log10(std::max(0.000001f,std::min(1.2f,mag))); // -50 - 0
    float dbGain = std::min(0.0f, (*ratioParam * (*threshParam - dbRMS))); // 0 - thresh
    float newGain = std::pow(10, dbGain / 20); // 1 - 0 (1: no  change)
    
    float coeff;
    if (newGain < oldGain) coeff = 1 - *attackParam/10.0f;
    else coeff = 1 - *releaseParam/10.0f;
    return (1 - coeff) * oldGain + coeff * newGain; 
    
}

void Compressor::updateFreqAndIdx(float f, int idx){
    
    // four peak filters work in a circular mode,
    float dist = std::abs(freqToX(f) - freqToX(lastFreq));
    lastFreq = f;
    if (dist > *speedParam){ // 0.01 - 0.1
        filterPtr = filterPtr + 1;
        if (filterPtr == totalFilters) {
            filterPtr = 0;
        }
        filterGains[filterPtr] = 1.0f;
        filterFreqs[filterPtr] = f;
    } else {
        filterFreqs[filterPtr] = (1 - 0.3) * filterFreqs[filterPtr] + 0.3 * f;
    }
    
    
    filterIdxes[filterPtr] = idx;
    
}


bool Compressor::updateGain(){
    
    for (int i = 0; i < totalFilters; i++) {
        float newGain = 0.0f;
        if (i == filterPtr){
            newGain = calculateGain(fftDataPt[filterIdxes[i]], filterGains[i]);
            filterGains[i] = newGain;
            
        } else {
            newGain = calculateGain(0.0f, filterGains[i]);
            filterGains[i] = newGain;
        }
        if (i == 0) {
            coefficient1 = juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate, filterFreqs[i], *qParam, newGain);
            *filters.get<0>().state = *coefficient1;
        }
        if (i == 1) {
            coefficient2 = juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate, filterFreqs[i], *qParam, newGain);
            *filters.get<1>().state = *coefficient2;
        }
        if (i == 2) {
            coefficient3 = juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate, filterFreqs[i], *qParam, newGain);
            *filters.get<2>().state = *coefficient3;
        }
        if (i == 3) {
            coefficient4 = juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate, filterFreqs[i], *qParam, newGain);
            *filters.get<3>().state = *coefficient4;
        }
    }

    return false;
}


float Compressor::indexToFreq(int idx, int fftSize){
    if (idx == 0){
        idx = 1;
    }
    return float((sampleRate * idx) / fftSize);
}


void Compressor::getMagnitudes(const double* frequencies, size_t numSamples){
    
    std::fill(magnitudes.begin(), magnitudes.end(), 1.0);
    
    coefficient1->getMagnitudeForFrequencyArray(frequencies, magnitude1.data(), numSamples, sampleRate);
    juce::FloatVectorOperations::multiply(magnitudes.data(), magnitude1.data(), static_cast<int> (magnitudes.size()));
    
    coefficient2->getMagnitudeForFrequencyArray(frequencies, magnitude2.data(), numSamples, sampleRate);
    juce::FloatVectorOperations::multiply(magnitudes.data(), magnitude2.data(), static_cast<int> (magnitudes.size()));
    
    coefficient3->getMagnitudeForFrequencyArray(frequencies, magnitude3.data(), numSamples, sampleRate);
    juce::FloatVectorOperations::multiply(magnitudes.data(), magnitude3.data(), static_cast<int> (magnitudes.size()));
    
    coefficient4->getMagnitudeForFrequencyArray(frequencies, magnitude4.data(), numSamples, sampleRate);
    juce::FloatVectorOperations::multiply(magnitudes.data(), magnitude4.data(), static_cast<int> (magnitudes.size()));
    
    for (int i = 0; i < numSamples; i++){
        if (magnitudes[i] <= 1.1 && magnitudes[i] >= 0.05){
            magnitudeRegular[i] = magnitudes[i];
        } else {
            magnitudes[i] = magnitudeRegular[i];
        }
    }
}
