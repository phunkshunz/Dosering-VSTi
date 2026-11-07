#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "Gui.h"

// KÖLDBÄRAREN: panelen. KURVAN till vänster (rita med musen), botten-verktygen till höger.

// Den ritade duck-kurvan med spelhuvud och nivåmätare
class Kurvan : public juce::Component, public juce::SettableTooltipClient, private juce::Timer
{
public:
    explicit Kurvan (KoldProcessor& p);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override { lastIdx = -1; }
    void mouseDoubleClick (const juce::MouseEvent&) override;
private:
    void timerCallback() override { repaint(); }
    void drawAt (juce::Point<float> pos);
    juce::Rectangle<float> plot() const { return getLocalBounds().toFloat().reduced (12.f, 10.f).withTrimmedBottom (14.f); }
    KoldProcessor& proc;
    int lastIdx = -1;
    float lastVal = 1.f;
};

class KoldPanel : public juce::Component, private juce::Timer
{
public:
    explicit KoldPanel (KoldProcessor&);
    void paint (juce::Graphics&) override;
    void resized() override;
private:
    void timerCallback() override;
    Knob& knob (const char* id, const char* name, juce::Colour accent, const char* tip);
    juce::ComboBox& box (const char* id, const char* tip);

    KoldProcessor& proc;
    Kurvan kurvan { proc };
    std::vector<std::unique_ptr<Knob>> knobs;
    std::vector<std::unique_ptr<juce::ComboBox>> boxes;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>> boxAtts;
    juce::TextButton monoBtn { "MONO BAS" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> monoAtt;

    Knob *djup {}, *kanslighet {}, *kicklangd {}, *anslag {}, *svans {}, *subton {}, *subniva {}, *subfall {},
         *brytpunkt {}, *subdist {}, *ton {}, *blandning {}, *niva {};
    juce::ComboBox *kalla {}, *form {}, *ducktid {}, *duckmal {}, *lage {};
    juce::Rectangle<int> secKurva, secGilj, secTrans, secDist, secUt;
    float subMeter = 0.f, duckMeter = 1.f;
    bool kickLamp = false;
};

class KoldEditor : public juce::AudioProcessorEditor
{
public:
    explicit KoldEditor (KoldProcessor&);
    ~KoldEditor() override;
    void resized() override;
    void paint (juce::Graphics& g) override { g.fillAll (KoldColours::ground); }
private:
    void byggPanel();
    KoldProcessor& proc;
    KoldLnF lnf;
    std::unique_ptr<KoldPanel> panel;
    sprak::Knapp sprakKnapp;
    juce::TooltipWindow tips { this, 700 };
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KoldEditor)
};
