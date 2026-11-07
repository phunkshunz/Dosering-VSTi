#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "KoldEngine.h"
#include <array>
#include <atomic>

// DEMO: byggs med -DKOLD_DEMO=ON. Samma kod, men bara några rattar öppna.
#ifndef KOLD_DEMO
 #define KOLD_DEMO 0
#endif

namespace ParamIDs
{
    inline constexpr const char* kalla      = "kalla";       // TAKT / SIDECHAIN / SJÄLV
    inline constexpr const char* form       = "form";        // kurvans form
    inline constexpr const char* djup       = "djup";
    inline constexpr const char* ducktid    = "ducktid";     // 1/16 1/8 1/4 1/2
    inline constexpr const char* duckmal    = "duckmal";     // BARA BOTTEN / ALLT
    inline constexpr const char* kanslighet = "kanslighet";
    inline constexpr const char* kicklangd  = "kicklangd";
    inline constexpr const char* anslag     = "anslag";
    inline constexpr const char* svans      = "svans";
    inline constexpr const char* subton     = "subton";
    inline constexpr const char* subniva    = "subniva";
    inline constexpr const char* subfall    = "subfall";
    inline constexpr const char* lage       = "lage";        // SLÄP / SPIKRAKT
    inline constexpr const char* brytpunkt  = "brytpunkt";
    inline constexpr const char* subdist    = "subdist";
    inline constexpr const char* ton        = "ton";
    inline constexpr const char* monobas    = "monobas";
    inline constexpr const char* blandning  = "blandning";
    inline constexpr const char* niva       = "niva";
}

// Kurvans former. EGEN = det du har ritat.
namespace Former
{
    inline const juce::StringArray& names()
    {
        static const juce::StringArray n { "PUMP", juce::String::fromUTF8 ("HÅRD"), "MJUK", "TRAPPA", "ANDNING", "DUBBEL", "GRIND", "EGEN" };
        return n;
    }
    constexpr int kEgen = 7;
    void fill (int form, std::array<float, kold::kCurvePoints>& c);
}

class KoldProcessor : public juce::AudioProcessor, private juce::AudioProcessorValueTreeState::Listener
{
public:
    KoldProcessor();
    ~KoldProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    static constexpr bool isDemo = KOLD_DEMO != 0;
    const juce::String getName() const override { return isDemo ? juce::String::fromUTF8 ("KÖLDBÄRAREN DEMO") : juce::String::fromUTF8 ("KÖLDBÄRAREN"); }
    bool acceptsMidi() const override  { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 1.5; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    // Den ritade kurvan. Panelen skriver, ljudtråden läser.
    std::array<std::atomic<float>, kold::kCurvePoints> curve;
    void setCurvePoint (int i, float v);       // ritning: gör formen till EGEN
    void applyForm (int form);                 // fyll kurvan från en form

    // För panelen
    std::atomic<float> meterDuck { 1.f }, meterSub { 0.f }, curvePos { 2.f };
    std::atomic<bool> meterKick { false }, sidechainConnected { false };

private:
    void parameterChanged (const juce::String& id, float value) override;
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    kold::Params readParams() const;

    kold::Engine engine;
    std::vector<float> bufL, bufR;
    bool drawing = false;   // sant medan setCurvePoint ändrar formen till EGEN

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KoldProcessor)
};
