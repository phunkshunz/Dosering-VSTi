#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"
#include "Sprak.h"
#if __has_include ("BinaryData.h")
 #include "BinaryData.h"
 #define DOS_HAS_FONTS 1
#else
 #define DOS_HAS_FONTS 0
#endif

// DOSERING · MASKINHALLEN ONE KNOB. Smal panel 300 x 580: en stor ratt som doserar botten, lampor som visar
// vad som händer, fyra lägen, två små reglage och presets. Grundschemat: kolgrå, natriumorange och turkos.
namespace stil
{
const juce::Colour ground { 0xff121314 }, panel { 0xff1a1c1e }, raised { 0xff232527 }, cell { 0xff26292c },
                   line { 0xff2c2f32 }, line2 { 0xff3d4246 }, rim { 0xff4a4f54 },
                   ink { 0xffe6e3dc }, ink2 { 0xffb9b6ae }, ink3 { 0xff8b8e91 },
                   sodium { 0xffff8a1f }, cold { 0xff4fd1e0 };

inline juce::Typeface::Ptr load (const char* file)
{
   #if DOS_HAS_FONTS
    for (int i = 0; i < BinaryData::namedResourceListSize; ++i)
        if (juce::String (BinaryData::originalFilenames[i]) == file)
        {
            int size = 0;
            if (auto* data = BinaryData::getNamedResource (BinaryData::namedResourceList[i], size))
                return juce::Typeface::createSystemTypefaceFor (data, (size_t) size);
        }
   #endif
    juce::ignoreUnused (file);
    return nullptr;
}
inline juce::Typeface::Ptr sans()     { static auto t = load ("IBMPlexSansCondensed-Medium.ttf"); return t; }
inline juce::Typeface::Ptr sansBold() { static auto t = load ("IBMPlexSansCondensed-SemiBold.ttf"); return t; }
inline juce::Typeface::Ptr monoFace() { static auto t = load ("IBMPlexMono-Medium.ttf"); return t; }

inline juce::Font text (float size, bool bold = false, float tracking = 0.08f)
{
    auto tf = bold ? sansBold() : sans();
    juce::FontOptions o = tf != nullptr ? juce::FontOptions (tf).withHeight (size) : juce::FontOptions (size, bold ? juce::Font::bold : juce::Font::plain);
    return juce::Font (o.withKerningFactor (tracking));
}
inline juce::Font mono (float size)
{
    auto tf = monoFace();
    return juce::Font (tf != nullptr ? juce::FontOptions (tf).withHeight (size) : juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), size, juce::Font::plain));
}
} // namespace stil

#include "Mini.h"

class DosLnF : public juce::LookAndFeel_V4
{
public:
    DosLnF();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos, float a0, float a1, juce::Slider&) override;
    void drawLinearSlider (juce::Graphics&, int x, int y, int w, int h, float pos, float minPos, float maxPos, juce::Slider::SliderStyle, juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool over, bool down) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool over, bool down) override;
    juce::Font getPopupMenuFont() override { return stil::text (14.0f, false, 0.02f); }
    void drawPopupMenuBackground (juce::Graphics&, int w, int h) override;
    void drawTooltip (juce::Graphics&, const juce::String&, int w, int h) override;
};

class DoseringPanel : public juce::Component, private juce::Timer
{
public:
    DoseringPanel (DoseringProcessor&, std::function<void()> bytSprak);
    ~DoseringPanel() override;

    void paint (juce::Graphics&) override;
    void paintOverChildren (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void setChoice (int mode);
    void visaPresetMeny();

    DoseringProcessor& proc;
    juce::Slider knob, mix, ut;
    mini::Knob minis[8];
    juce::TextButton lage[4], allt, slumpa, prev, next, namn;
    sprak::Knapp sprakKnapp;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> knobAtt, mixAtt, utAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> alltAtt;
    float lDuck = 0, lSub = 0, lDist = 0;
    int shownPreset = -1;
    juce::Rectangle<int> knobArea, lampArea, mixRow, utRow, miniArea;
};

class DoseringEditor : public juce::AudioProcessorEditor
{
public:
    explicit DoseringEditor (DoseringProcessor&);
    ~DoseringEditor() override;
    void paint (juce::Graphics& g) override { g.fillAll (stil::ground); }
    void resized() override;

private:
    void byggPanel();

    DoseringProcessor& proc;
    DosLnF lnf;
    juce::TooltipWindow tips { this, 700 };
    std::unique_ptr<DoseringPanel> panel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DoseringEditor)
};
