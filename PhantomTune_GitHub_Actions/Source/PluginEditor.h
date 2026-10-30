#pragma once
#include "PluginProcessor.h"
class PhantomTuneEditor final : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit PhantomTuneEditor(PhantomTuneAudioProcessor&);
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    void timerCallback() override;
    PhantomTuneAudioProcessor& processor;
    juce::ComboBox key, scale, harmonyMode;
    juce::Slider retune, humanize, mix, harmony, width, doubling, body;
    juce::Label pitchLabel;
    using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<ComboAttachment> keyAttachment, scaleAttachment, harmonyModeAttachment;
    std::unique_ptr<SliderAttachment> retuneAttachment, humanizeAttachment, mixAttachment, harmonyAttachment, widthAttachment, doubleAttachment, bodyAttachment;
};
