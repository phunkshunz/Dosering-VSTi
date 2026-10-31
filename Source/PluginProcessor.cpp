#include "PluginProcessor.h"
#include "PluginEditor.h"

DoseringAudioProcessor::DoseringAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ), apvts(*this, nullptr, "PARAMETERS", createParameters())
#endif
{
}

DoseringAudioProcessor::~DoseringAudioProcessor() {}

juce::AudioProcessorValueTreeState::ParameterLayout DoseringAudioProcessor::createParameters()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    params.push_back(std::make_unique<juce::AudioParameterFloat>("SUB_FREQ", "Sub Frequency", 30.0f, 90.0f, 45.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("DECAY", "Sub Decay", 0.1f, 1.5f, 0.4f));
    // 0 = Släp (Techno), 1 = Spikrakt (Psy)
    params.push_back(std::make_unique<juce::AudioParameterChoice>("MODE", "Mode", juce::StringArray{"Släp (Techno)", "Spikrakt (Psy)"}, 1));
    return { params.begin(), params.end() };
}

void DoseringAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    currentPhase = 0.0f;
    subEnvelope = 0.0f;
    kickEnvelope = 0.0f;
    kickWasActive = false;
}

void DoseringAudioProcessor::releaseResources() {}

void DoseringAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    float subFreq = apvts.getRawParameterValue("SUB_FREQ")->load();
    float decayTime = apvts.getRawParameterValue("DECAY")->load();
    int mode = (int)apvts.getRawParameterValue("MODE")->load(); 
    
    phaseDelta = juce::MathConstants<float>::twoPi * subFreq / currentSampleRate;
    float envelopeDecay = std::exp(-1.0f / (currentSampleRate * decayTime));
    
    auto* channelDataL = buffer.getWritePointer (0);
    auto* channelDataR = buffer.getWritePointer (1);

    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        // Enkel och rå env-follower på inkommande kick
        float inputSample = std::abs(channelDataL[sample]);
        kickEnvelope = std::max(inputSample, kickEnvelope * 0.999f); 
        
        bool kickIsActive = (kickEnvelope > 0.05f); // Tröskelvärde, justerbart i nästa version
        
        // Giljotinen: Faslåsning exakt när kicken faller
        if (kickWasActive && !kickIsActive)
        {
            // ABSOLUT NOLLSTÄLLNING - Här sker magin. Ingen överlappning.
            currentPhase = 0.0f;
            subEnvelope = 1.0f; 
        }
        kickWasActive = kickIsActive;
        
        float subSample = 0.0f;
        
        if (subEnvelope > 0.001f && !kickIsActive) 
        {
            subSample = std::sin(currentPhase) * subEnvelope;
            currentPhase += phaseDelta;
            if (currentPhase >= juce::MathConstants<float>::twoPi)
                currentPhase -= juce::MathConstants<float>::twoPi;
                
            subEnvelope *= envelopeDecay;
            
            // Psy-läge: Kirurgisk amputering av svansen
            if (mode == 1 && subEnvelope < 0.15f) {
                subEnvelope *= 0.8f; 
            }
        }
        else if (kickIsActive)
        {
            // Kicken spelar? Då mördas subben direkt för att eliminera fasfel.
            subEnvelope = 0.0f;
        }

        channelDataL[sample] += subSample;
        channelDataR[sample] += subSample;
    }
}

juce::AudioProcessorEditor* DoseringAudioProcessor::createEditor()
{
    return new DoseringAudioProcessorEditor (*this);
}
