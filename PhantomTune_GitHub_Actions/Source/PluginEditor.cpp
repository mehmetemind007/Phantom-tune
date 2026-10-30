#include "PluginEditor.h"
PhantomTuneEditor::PhantomTuneEditor(PhantomTuneAudioProcessor& p)
 : AudioProcessorEditor(&p), processor(p)
{
    setSize(740, 590);
    for (auto* c : {&key, &scale, &harmonyMode}) { addAndMakeVisible(*c); c->setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff272735)); c->setColour(juce::ComboBox::textColourId, juce::Colours::white); }
    const juce::StringArray keys{"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};
    for (int i = 0; i < keys.size(); ++i) key.addItem(keys[i], i + 1);
    scale.addItemList({"Chromatic","Major","Minor"}, 1);
    harmonyMode.addItemList({"Third Above","Fifth Above","Octave Below","Third Below"}, 1);
    for (auto* s : {&retune, &humanize, &mix, &harmony, &width, &doubling, &body})
    {
        addAndMakeVisible(*s);
        s->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        s->setTextBoxStyle(juce::Slider::TextBoxBelow, false, 95, 22);
        s->setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xffad83ff));
        s->setColour(juce::Slider::textBoxTextColourId, juce::Colours::white);
    }
    addAndMakeVisible(pitchLabel);
    pitchLabel.setJustificationType(juce::Justification::centred);
    pitchLabel.setColour(juce::Label::textColourId, juce::Colour(0xffc8a8ff));
    pitchLabel.setFont(juce::Font(juce::FontOptions(21.0f)));
    keyAttachment = std::make_unique<ComboAttachment>(p.state, "key", key);
    scaleAttachment = std::make_unique<ComboAttachment>(p.state, "scale", scale);
    retuneAttachment = std::make_unique<SliderAttachment>(p.state, "retune", retune);
    humanizeAttachment = std::make_unique<SliderAttachment>(p.state, "humanize", humanize);
    mixAttachment = std::make_unique<SliderAttachment>(p.state, "mix", mix);
    harmonyModeAttachment = std::make_unique<ComboAttachment>(p.state, "harmonyMode", harmonyMode);
    harmonyAttachment = std::make_unique<SliderAttachment>(p.state, "harmony", harmony);
    widthAttachment = std::make_unique<SliderAttachment>(p.state, "width", width);
    doubleAttachment = std::make_unique<SliderAttachment>(p.state, "double", doubling);
    bodyAttachment = std::make_unique<SliderAttachment>(p.state, "body", body);
    startTimerHz(15);
}
void PhantomTuneEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff111119));
    g.setColour(juce::Colour(0xffb28aff));
    g.setFont(juce::Font(juce::FontOptions(30.0f, juce::Font::bold)));
    g.drawText("PHANTOM TUNE", 24, 18, 480, 44, juce::Justification::centredLeft);
    g.setColour(juce::Colour(0xff9c9cac));
    g.setFont(juce::Font(juce::FontOptions(12.0f)));
    g.drawText("PHANTOM CULTURE  /  REAL-TIME VOCAL CORRECTION", 26, 61, 560, 20, juce::Justification::centredLeft);
    g.setColour(juce::Colour(0xff222231));
    g.fillRoundedRectangle(22.0f, 99.0f, 696.0f, 105.0f, 12.0f);
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(juce::FontOptions(14.0f)));
    g.drawText("KEY", 45, 112, 110, 24, juce::Justification::centredLeft);
    g.drawText("SCALE", 230, 112, 130, 24, juce::Justification::centredLeft);
    g.drawText("DETECTED NOTE", 425, 112, 165, 24, juce::Justification::centredLeft);
    g.drawText("RETUNE SPEED", 57, 332, 150, 24, juce::Justification::centred);
    g.drawText("HARMONY / VOCAL WIDTH", 30, 365, 450, 25, juce::Justification::centredLeft);
    g.drawText("INTERVAL", 560, 365, 145, 25, juce::Justification::centredLeft);
    const char* labels[] = {"HARMONY", "WIDTH", "DOUBLE", "BODY"};
    for (int i = 0; i < 4; ++i) g.drawText(labels[i], 27 + i * 173, 526, 145, 24, juce::Justification::centred);
    g.drawText("HUMANIZE", 246, 332, 150, 24, juce::Justification::centred);
    g.drawText("DRY / WET", 435, 332, 150, 24, juce::Justification::centred);
    g.setColour(juce::Colour(0xff77778a));
    g.drawText("v0.2  •  EXPERIMENTAL HARMONIZER", 24, 566, 680, 17, juce::Justification::centredRight);
}
void PhantomTuneEditor::resized()
{
    key.setBounds(45, 145, 150, 38);
    scale.setBounds(230, 145, 155, 38);
    pitchLabel.setBounds(430, 145, 155, 38);
    retune.setBounds(55, 222, 150, 109);
    humanize.setBounds(245, 222, 150, 109);
    mix.setBounds(435, 222, 150, 109);
    harmonyMode.setBounds(550, 391, 165, 32);
    harmony.setBounds(27, 426, 145, 100);
    width.setBounds(200, 426, 145, 100);
    doubling.setBounds(373, 426, 145, 100);
    body.setBounds(546, 426, 145, 100);
}
void PhantomTuneEditor::timerCallback()
{
    const float hz = processor.detectedHz.load(std::memory_order_relaxed);
    if (hz <= 0.0f) { pitchLabel.setText("—", juce::dontSendNotification); return; }
    const int note = juce::jlimit(0, 127, static_cast<int>(std::round(69.0 + 12.0 * std::log2(hz / 440.0))));
    pitchLabel.setText(juce::MidiMessage::getMidiNoteName(note, true, true, 3), juce::dontSendNotification);
}
juce::AudioProcessorEditor* PhantomTuneAudioProcessor::createEditor() { return new PhantomTuneEditor(*this); }
