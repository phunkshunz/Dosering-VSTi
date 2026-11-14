#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "Dose.h"

// DOSERING · one knob för botten och pump (MASKINHALLEN / Phunkshunz AUDIO)
class DoseringProcessor : public juce::AudioProcessor
{
public:
    DoseringProcessor();
    ~DoseringProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "DOSERING"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 1.5; }

    int getNumPrograms() override { return dos::kNumPresets; }
    int getCurrentProgram() override { return preset.load(); }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // SLUMPA: ny karaktär. Ratten och läget står kvar.
    void slumpa();

    juce::AudioProcessorValueTreeState apvts;
    std::atomic<int> preset { 0 };
    // för panelen
    std::atomic<float> mDuck { 0.0f }, mSub { 0.0f }, mDist { 0.0f };
    std::atomic<bool> hostPlaying { false };

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout layout();
    void setParam (const char* id, float value);
    dos::Settings read() const;

    kold::Engine engine;
    std::vector<float> bufL, bufR;
    float holdDuck = 0.0f, holdSub = 0.0f;

    std::atomic<float>* v[19] {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DoseringProcessor)
};
