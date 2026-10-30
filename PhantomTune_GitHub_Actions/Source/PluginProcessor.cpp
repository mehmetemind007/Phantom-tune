#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>
#include <limits>

PhantomTuneAudioProcessor::PhantomTuneAudioProcessor()
 : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
                                    .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
   state(*this, nullptr, "PARAMETERS", createLayout()) {}

juce::AudioProcessorValueTreeState::ParameterLayout PhantomTuneAudioProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout p;
    p.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"key",1}, "Key",
        juce::StringArray{"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"}, 0));
    p.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"scale",1}, "Scale",
        juce::StringArray{"Chromatic","Major","Minor"}, 2));
    p.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"retune",1}, "Retune ms",
        juce::NormalisableRange<float>(1.0f, 250.0f, 0.1f, 0.4f), 20.0f));
    p.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"humanize",1}, "Humanize",
        juce::NormalisableRange<float>(0.0f, 100.0f), 10.0f));
    p.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"mix",1}, "Mix",
        juce::NormalisableRange<float>(0.0f, 100.0f), 100.0f));
    p.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"harmony",1}, "Harmony", juce::NormalisableRange<float>(0.f,100.f), 0.f));
    p.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"harmonyMode",1}, "Harmony Interval", juce::StringArray{"Third Above","Fifth Above","Octave Below","Third Below"}, 0));
    p.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"width",1}, "Stereo Width", juce::NormalisableRange<float>(0.f,100.f), 60.f));
    p.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"double",1}, "Vocal Double", juce::NormalisableRange<float>(0.f,100.f), 0.f));
    p.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"body",1}, "Body", juce::NormalisableRange<float>(0.f,100.f), 0.f));
    return p;
}

void PhantomTuneAudioProcessor::prepareToPlay(double sr, int)
{
    sampleRateHz = sr;
    analysisSize = sr >= 88200.0 ? 4096 : 2048;
    hopSize = analysisSize / 8;
    detector.assign(static_cast<size_t>(analysisSize), 0.0f);
    detectorWrite = hopCounter = 0;
    ringSize = static_cast<int>(sr * 0.5) + analysisSize + 16;
    for (auto& v : voices)
    {
        v.ring.assign(static_cast<size_t>(ringSize), 0.0f);
        v.write = 0;
        v.phase = 0.0;
        v.smoothedRatio = 1.0f;
        v.harmonyPhase[0] = 0.0; v.harmonyPhase[1] = 0.5; v.doublePhase = 0.25;
    }
    correctionRatio = 1.0f;
    detectedHz.store(0.0f);
    targetHz.store(0.0f);
    setLatencySamples(static_cast<int>(sr * 0.045));
}

float PhantomTuneAudioProcessor::estimatePitch() const
{
    // Normalized difference-function search; designed for monophonic vocals.
    const int n = analysisSize;
    const int minLag = juce::jmax(2, static_cast<int>(sampleRateHz / 950.0));
    const int maxLag = juce::jmin(n / 2, static_cast<int>(sampleRateHz / 75.0));
    double energy = 0.0;
    for (float x : detector) energy += static_cast<double>(x) * x;
    if (energy / n < 0.000003) return 0.0f;
    float best = -1.0f;
    int bestLag = 0;
    for (int lag = minLag; lag <= maxLag; ++lag)
    {
        double xy = 0.0, xx = 0.0, yy = 0.0;
        // Analyze the most recent contiguous logical window in circular buffer.
        for (int i = 0; i < n - maxLag; i += 2)
        {
            const float a = detector[static_cast<size_t>((detectorWrite + i) % n)];
            const float b = detector[static_cast<size_t>((detectorWrite + i + lag) % n)];
            xy += static_cast<double>(a) * b;
            xx += static_cast<double>(a) * a;
            yy += static_cast<double>(b) * b;
        }
        const float score = static_cast<float>(xy / std::sqrt(xx * yy + 1.0e-15));
        if (score > best) { best = score; bestLag = lag; }
        if (score > 0.94f && lag > minLag + 2) break;
    }
    return best > 0.65f && bestLag > 0 ? static_cast<float>(sampleRateHz / bestLag) : 0.0f;
}

float PhantomTuneAudioProcessor::closestNote(float hz, int key, int scale)
{
    const float midi = 69.0f + 12.0f * std::log2(hz / 440.0f);
    constexpr int major[] = {0,2,4,5,7,9,11};
    constexpr int minor[] = {0,2,3,5,7,8,10};
    float bestMidi = std::round(midi), bestDistance = std::numeric_limits<float>::max();
    for (int note = juce::jmax(0, static_cast<int>(midi) - 14);
         note <= juce::jmin(127, static_cast<int>(midi) + 14); ++note)
    {
        const int degree = (note - key + 120) % 12;
        bool allowed = scale == 0;
        for (int d = 0; d < 7 && !allowed; ++d)
            allowed = degree == (scale == 1 ? major[d] : minor[d]);
        const float dist = std::abs(static_cast<float>(note) - midi);
        if (allowed && dist < bestDistance) { bestDistance = dist; bestMidi = static_cast<float>(note); }
    }
    return 440.0f * std::pow(2.0f, (bestMidi - 69.0f) / 12.0f);
}

float PhantomTuneAudioProcessor::sampleAt(const VoiceState& v, double pos)
{
    pos = std::fmod(pos + static_cast<double>(v.ring.size()) * 2.0, static_cast<double>(v.ring.size()));
    const int a = static_cast<int>(pos);
    const int b = (a + 1) % static_cast<int>(v.ring.size());
    const float frac = static_cast<float>(pos - a);
    return v.ring[static_cast<size_t>(a)] * (1.0f - frac) + v.ring[static_cast<size_t>(b)] * frac;
}

void PhantomTuneAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int channels = juce::jmin(buffer.getNumChannels(), 2);
    if (channels == 0 || detector.empty()) return;
    const int key = static_cast<int>(state.getRawParameterValue("key")->load());
    const int scale = static_cast<int>(state.getRawParameterValue("scale")->load());
    const float retune = state.getRawParameterValue("retune")->load();
    const float human = state.getRawParameterValue("humanize")->load() / 100.0f;
    const float mix = state.getRawParameterValue("mix")->load() / 100.0f;
    const float harmony = state.getRawParameterValue("harmony")->load() / 100.0f;
    const int harmonyMode = static_cast<int>(state.getRawParameterValue("harmonyMode")->load());
    const float width = state.getRawParameterValue("width")->load() / 100.0f;
    const float doubling = state.getRawParameterValue("double")->load() / 100.0f;
    const float body = state.getRawParameterValue("body")->load() / 100.0f;
    const int intervals[] = {4, 7, -12, -3};
    const float harmonyRatio = std::pow(2.0f, intervals[juce::jlimit(0,3,harmonyMode)] / 12.0f);
    const float alpha = std::exp(-1.0f / (juce::jmax(1.0f, retune) * 0.001f * static_cast<float>(sampleRateHz)));
    const int grain = static_cast<int>(sampleRateHz * 0.04);
    const int delay = static_cast<int>(sampleRateHz * 0.045);
    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        float mono = 0.0f;
        for (int c = 0; c < channels; ++c) mono += buffer.getSample(c, i);
        mono /= static_cast<float>(channels);
        detector[static_cast<size_t>(detectorWrite)] = mono;
        detectorWrite = (detectorWrite + 1) % analysisSize;
        if (++hopCounter >= hopSize)
        {
            hopCounter = 0;
            const float pitch = estimatePitch();
            detectedHz.store(pitch, std::memory_order_relaxed);
            if (pitch > 0.0f)
            {
                const float target = closestNote(pitch, key, scale);
                targetHz.store(target, std::memory_order_relaxed);
                correctionRatio = juce::jlimit(0.5f, 2.0f, std::pow(target / pitch, 1.0f - human));
            }
            else { targetHz.store(0.0f, std::memory_order_relaxed); correctionRatio = 1.0f; }
        }
        for (int c = 0; c < channels; ++c)
        {
            auto& v = voices[static_cast<size_t>(c)];
            const float dryInput = buffer.getSample(c, i);
            v.ring[static_cast<size_t>(v.write)] = dryInput;
            v.smoothedRatio = alpha * v.smoothedRatio + (1.0f - alpha) * correctionRatio;
            // Dual overlapping delay-line grains, Hann windows, independent of host block size.
            v.phase += (1.0 - static_cast<double>(v.smoothedRatio)) / grain;
            v.phase -= std::floor(v.phase);
            const double p1 = v.phase;
            const double p2 = std::fmod(p1 + 0.5, 1.0);
            auto grainSample = [&](double phase)
            {
                const double offset = delay + (phase - 0.5) * grain;
                const double pos = v.write - offset;
                const float window = static_cast<float>(0.5 - 0.5 * std::cos(2.0 * juce::MathConstants<double>::pi * phase));
                return window * sampleAt(v, pos);
            };
            const float wet = grainSample(p1) + grainSample(p2);
            const float dry = sampleAt(v, v.write - delay);
            // Extra harmonizer grains, sharing the same input history as the main shifter.
            // Fixed chromatic intervals are intentional for this first prototype.
            auto shifted = [&](double& phase, float ratio, float extraDelay)
            {
                phase += (1.0 - static_cast<double>(ratio)) / grain;
                phase -= std::floor(phase);
                auto readGrain = [&](double ph)
                {
                    const float window = static_cast<float>(0.5 - 0.5 * std::cos(2.0 * juce::MathConstants<double>::pi * ph));
                    return window * sampleAt(v, v.write - (delay + extraDelay + (ph - 0.5) * grain));
                };
                return readGrain(phase) + readGrain(std::fmod(phase + 0.5, 1.0));
            };
            float output = (1.0f - mix) * dry + mix * wet;
            if (harmony > 0.0001f)
            {
                const float sideGain = channels == 2 ? (c == 0 ? 1.0f + width * 0.5f : 1.0f - width * 0.5f) : 1.0f;
                const float interval = harmonyRatio * v.smoothedRatio;
                const float harmonyWet = shifted(v.harmonyPhase[0], interval * (c == 0 ? 0.998f : 1.002f), 0.0f);
                output += harmony * 0.38f * sideGain * harmonyWet;
            }
            if (doubling > 0.0001f)
            {
                const float detune = c == 0 ? 1.006f : 0.994f;
                const float doubleWet = shifted(v.doublePhase, v.smoothedRatio * detune, static_cast<float>(sampleRateHz * (c == 0 ? 0.009 : 0.016)));
                output += doubling * 0.3f * (0.5f + width * 0.5f) * doubleWet;
            }
            // Soft saturation gives perceived density; final limiter prevents extreme peaks.
            const float drive = 1.0f + body * 2.0f;
            output = std::tanh(output * drive) / std::tanh(drive);
            buffer.setSample(c, i, juce::jlimit(-1.0f, 1.0f, output));
            v.write = (v.write + 1) % ringSize;
        }
    }
    for (int c = channels; c < buffer.getNumChannels(); ++c) buffer.clear(c, 0, buffer.getNumSamples());
}

void PhantomTuneAudioProcessor::getStateInformation(juce::MemoryBlock& dest)
{
    if (auto xml = state.copyState().createXml()) copyXmlToBinary(*xml, dest);
}
void PhantomTuneAudioProcessor::setStateInformation(const void* data, int size)
{
    if (auto xml = getXmlFromBinary(data, size))
        if (xml->hasTagName(state.state.getType())) state.replaceState(juce::ValueTree::fromXml(*xml));
}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new PhantomTuneAudioProcessor(); }
