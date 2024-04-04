/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
ResonanceSuppressorAudioProcessor::ResonanceSuppressorAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
    , state(*this, nullptr, "params", {
    std::make_unique<juce::AudioParameterFloat>("ratio", "Ratio", NormalisableRange<float>(0.01f, 4.0f, 0.01f, std::log (0.5f) / std::log (1.99f / 3.99f)), 1.0f, juce::String(), juce::AudioProcessorParameter::genericParameter, [](float value, int) { return "1 : " + juce::String(value, 2); }, [](juce::String text) { return text.replace("1 : ","").getFloatValue(); }),
                
    std::make_unique<juce::AudioParameterFloat>("freqLow", "FreqLow", NormalisableRange<float>{20.0f, 10000.0f, 0.1f, std::log (0.5f) / std::log (480.0f / 9980.0f)}, 320.0f, juce::String(), juce::AudioProcessorParameter::genericParameter, [](float value, int) { return (value < 1000.0f) ? juce::String (value, 1) + " Hz" : juce::String (value / 1000.0f, 2) + " kHz"; }, [](juce::String text) { return text.endsWith(" kHz") ? text.dropLastCharacters (4).getFloatValue() * 1000.0f : text.dropLastCharacters (3).getFloatValue(); }),
        
    std::make_unique<juce::AudioParameterFloat>("freqHi", "FreqHi", NormalisableRange<float>{80.0f, 20000.0f, 1.0f, std::log (0.5f) / std::log (1920.0f / 18000.0f)}, 10200.0f, juce::String(), juce::AudioProcessorParameter::genericParameter, [](float value, int) { return (value < 1000.0f) ? juce::String (value, 1) + " Hz" : juce::String (value / 1000.0f, 2) + " kHz"; }, [](juce::String text) { return text.endsWith(" kHz") ? text.dropLastCharacters (4).getFloatValue() * 1000.0f : text.dropLastCharacters (3).getFloatValue(); }),
        
    std::make_unique<juce::AudioParameterFloat>("speed", "Pitch Track", NormalisableRange<float>(0.01f, 0.1f), 0.05f, juce::String(), juce::AudioProcessorParameter::genericParameter, [](float value, int) { return "x "+juce::String (value, 2); }, [](juce::String text) { return text.replace("x ","").getFloatValue(); }),
              
    std::make_unique<juce::AudioParameterFloat>("thresh", "Thresh", NormalisableRange<float>(-64.0f, 6.0f), 0.0f, juce::String(), juce::AudioProcessorParameter::genericParameter, [](float value, int) { return juce::String (value, 1) + " dB"; }, [](juce::String text) { return text.dropLastCharacters(3).getFloatValue(); }),
    
    std::make_unique<juce::AudioParameterFloat>("attack", "Attack", NormalisableRange<float>(0.1f, 9.9f, 0.01f), 7.0f, juce::String(), juce::AudioProcessorParameter::genericParameter, [](float value, int) { return juce::String (value*10, 1) + " ms"; }, [](juce::String text) { return text.dropLastCharacters(3).getFloatValue()/10.0f; }),
        
    std::make_unique<juce::AudioParameterFloat>("release", "Release", NormalisableRange<float>(0.1f, 9.9f, 0.01f), 9.0f, juce::String(), juce::AudioProcessorParameter::genericParameter, [](float value, int) { return juce::String (value*10, 1) + " ms"; }, [](juce::String text) { return text.dropLastCharacters(3).getFloatValue()/10.0f; }),
        
    std::make_unique<juce::AudioParameterFloat>("quality", "Q", NormalisableRange<float>(4.0f, 20.0f), 12.0f, juce::String(), juce::AudioProcessorParameter::genericParameter, [](float value, int) { return juce::String (value, 2); }, [](juce::String text) { return text.getFloatValue(); }),
        
    std::make_unique<juce::AudioParameterFloat>("peaks", "Analysers", NormalisableRange<float>(1.0f, 4.0f, 1.0f), 2.0f, juce::String(), juce::AudioProcessorParameter::genericParameter, [](float value, int) { return juce::String (value, 2); }, [](juce::String text) { return text.getFloatValue(); }),
            
    std::make_unique<juce::AudioParameterFloat>("gainMain", "Gain", -64.0f, 6.0f, 0.0f),
    std::make_unique<juce::AudioParameterFloat>("bypass", "Bypass", 0.0f, 1.0f, 0.0f),
    std::make_unique<juce::AudioParameterFloat>("delta", "Delta", 0.0f, 1.0f, 0.0f)
     })
#endif
{
}

ResonanceSuppressorAudioProcessor::~ResonanceSuppressorAudioProcessor()
{
//    for (int i = 0; i < numberOfAnalysers; i++) {
//        analysers[i]->stopThread(1000);
//    }
}

//==============================================================================
const juce::String ResonanceSuppressorAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool ResonanceSuppressorAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool ResonanceSuppressorAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool ResonanceSuppressorAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double ResonanceSuppressorAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int ResonanceSuppressorAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int ResonanceSuppressorAudioProcessor::getCurrentProgram()
{
    return 0;
}

void ResonanceSuppressorAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String ResonanceSuppressorAudioProcessor::getProgramName (int index)
{
    return {};
}

void ResonanceSuppressorAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void ResonanceSuppressorAudioProcessor::prepareToPlay (double sr, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
//    audioFifo.setSize (1, sr);
//    abstractFifo.setTotalSize (sr);
//    analyser.setupAnalyser(int(sr), float(sr));
    
    numOutputChannels = getTotalNumOutputChannels();
    numInputChannels = getTotalNumInputChannels();
    numSamples = samplesPerBlock;
    
    delta.setSize(numInputChannels, numSamples);
    
    analysers.push_back(&analyser1);
    analysers.push_back(&analyser2);
    analysers.push_back(&analyser3);
    analysers.push_back(&analyser4);
    
    for (int i = 0; i < numberOfAnalysers; i++) {
//        Analyser<float>* ana;
//        analysers.add(ana = new Analyser<float>());
////        Analyser<float>* ana = &analysers.getReference(i);
//        ana->setupAnalyser(int(sr), float(sr));
        analysers[i]->setupAnalyser(int(sr), float(sr));
//        analyser.setupAnalyser(int(sr), float(sr));
        allCompressors.add(Compressor());
        Compressor* comp = &allCompressors.getReference(i);
        
        comp->fftDataPt = analysers[i]->getFftPointer(0);
//        comp->fftDataPt = analyser.getFftPointer(i);
        comp->debugColour = juce::Colour(30,122,51*5);
        comp->setSpec(sr,samplesPerBlock,numInputChannels);
        comp->threshParam = state.getRawParameterValue("thresh");
        comp->ratioParam = state.getRawParameterValue("ratio");
        comp->attackParam = state.getRawParameterValue("attack");
        comp->releaseParam = state.getRawParameterValue("release");
        comp->speedParam = state.getRawParameterValue("speed");
        comp->qParam = state.getRawParameterValue("quality");
        
//        analyser.freqLowParam = state.getRawParameterValue("freqLow");
//        analyser.freqHiParam = state.getRawParameterValue("freqHi");
        analysers[i]->freqLowParam = state.getRawParameterValue("freqLow");
        analysers[i]->freqHiParam = state.getRawParameterValue("freqHi");
    }
    
    
//    analyser.freqLowParam = state.getRawParameterValue("freqLow");
//    analyser.freqHiParam = state.getRawParameterValue("freqHi");
    
    gainMainParam = state.getRawParameterValue("gainMain");
    
    
    bpParam = state.getRawParameterValue("bypass");
    dtParam = state.getRawParameterValue("delta");
    peaksParam = state.getRawParameterValue("peaks");
    
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sr;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = numInputChannels;
    
    sampleRate = sr;

}

void ResonanceSuppressorAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
//    analyser.stopThread(1000);
    
    for (int i = 0; i < analysers.size(); i++){
        analysers[i]->stopThread(1000);
//        delete analysers[i];
    }
//    for (int i = 0; i < numberOfAnalysers; i++) {
//        analysers[i]->stopThread(1000);
//    }
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool ResonanceSuppressorAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void ResonanceSuppressorAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    
    bool isDelta = (*dtParam < 0.5f) ? false : true;
    
    if (isDelta){
        delta.copyFrom(0, 0, buffer.getReadPointer(0), numSamples);
        delta.copyFrom(1, 0, buffer.getReadPointer(1), numSamples);
    }
    
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    
    for (int i = 0; i < *peaksParam; i++){
    
        if (getActiveEditor() != nullptr)
            analysers[i]->addAudioData(buffer, 0, getTotalNumInputChannels());
        
        Compressor* comp = &allCompressors.getReference(i);
        
        comp->updateFreqAndIdx(analysers[i]->maxFreq, analysers[i]->maxIdx);
        comp->updateGain();

        if (*bpParam < 0.5f){
            comp->filters.process(context);
        }
    }
    
    
    
    
    for (int channel = 0; channel < numInputChannels; ++channel) {
        float* channelData = buffer.getWritePointer(channel);
//        for (int i = 0; i < numSamples; ++i) channelData[i] = 2.0f * rand() / (float)RAND_MAX - 1.0f;
        auto* deltaData = delta.getReadPointer(channel);
        
        for (int i = 0; i < numSamples; ++i) {
            // calculate the delta (output the signal that has been suppressed)
            if (isDelta) {
                channelData[i] = deltaData[i] - channelData[i];
            }
            
            channelData[i] = pow(10.0f, *gainMainParam / 20.0f) * channelData[i];
        }
    }
    
    
    for (auto i = numInputChannels; i < numOutputChannels; ++i) buffer.clear (i, 0, buffer.getNumSamples());
    
    
}

//==============================================================================
bool ResonanceSuppressorAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* ResonanceSuppressorAudioProcessor::createEditor()
{
    return new ResonanceSuppressorAudioProcessorEditor(*this, state);
}

//==============================================================================

// taken from 03b - SaveState2 - ValueTreeState and AudioParam
// save and load the parameters from ValueTreeState
void ResonanceSuppressorAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
    auto stateTree = state.copyState();
    std::unique_ptr<XmlElement> xml(stateTree.createXml());
    copyXmlToBinary(*xml, destData);
}

void ResonanceSuppressorAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    std::unique_ptr<XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState.get() != nullptr && xmlState->hasTagName(state.state.getType()))
      state.replaceState(ValueTree::fromXml(*xmlState));
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ResonanceSuppressorAudioProcessor();
}
