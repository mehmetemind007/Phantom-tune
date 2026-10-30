#pragma once
#include <JuceHeader.h>
#include <array>
#include <atomic>
#include <vector>

class PhantomTuneAudioProcessor final : public juce::AudioProcessor
{
public:
    PhantomTuneAudioProcessor();
    void prepareToPlay(double, int) override;
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Phantom Tune"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;
    juce::AudioProcessorValueTreeState state;
    std::atomic<float> detectedHz { 0.0f }, targetHz { 0.0f };
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
private:
    struct VoiceState
    {
        std::vector<float> ring;
        int write = 0;
        double phase = 0.0;
        float smoothedRatio = 1.0f;
        double harmonyPhase[2] {0.0, 0.5};
        double doublePhase = 0.25;
    };
    std::array<VoiceState, 2> voices;
    std::vector<float> detector;
    int detectorWrite = 0, hopCounter = 0, analysisSize = 2048, hopSize = 256, ringSize = 16384;
    double sampleRateHz = 44100.0;
    float correctionRatio = 1.0f;
    float estimatePitch() const;
    static float sampleAt(const VoiceState&, double);
    static float closestNote(float, int, int);
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PhantomTuneAudioProcessor)
};
