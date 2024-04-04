/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "Analyser.h"
#include "Compressor.h"

//==============================================================================
/**
*/
class ResonanceSuppressorAudioProcessor  : public juce::AudioProcessor
                            #if JucePlugin_Enable_ARA
                             , public juce::AudioProcessorARAExtension
                            #endif
{
public:
    //==============================================================================
    ResonanceSuppressorAudioProcessor();
    ~ResonanceSuppressorAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    void pushNextSampleIntoFifo(const juce::AudioBuffer<float>& buffer, int startChannel, int numChannels);
    void makeFFT();

    struct FFTData {
        int fftOrder = 11;
        int fftSize = 1 << fftOrder; //2048
        float fftData [2 * 2048];       
        int hopSize = 512;
        int writeIndex = 0;
        bool bufferReady = false;
    } fftData;
    float fifo [2048];
    float scopeData [512];
    bool nextFFTBlockReady = false;
    int fifoIndex = 0;
    float sampleRate;
    
    int numberOfAnalysers = 4;
    
    Analyser<float> analyser1;
    Analyser<float> analyser2;
    Analyser<float> analyser3;
    Analyser<float> analyser4;
    
    std::vector<Analyser<float>*> analysers;
    
    
    juce::Array<Compressor> allCompressors;
    juce::dsp::IIR::Coefficients<float>::Ptr coefficients2;
    
private:
    //==============================================================================
    
    juce::AudioProcessorValueTreeState state; // save the state in ValueTreeState
    
    juce::AudioBuffer<float> delta;
    
    int numSamples, numInputChannels, numOutputChannels;
    
    std::atomic<float> *gainMainParam;
    std::atomic<float> *bpParam;
    std::atomic<float> *dtParam;
    
    std::atomic<float> *peaksParam;

    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ResonanceSuppressorAudioProcessor)
};
