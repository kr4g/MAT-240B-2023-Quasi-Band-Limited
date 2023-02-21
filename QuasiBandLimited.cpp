// Karl Yerkes
// 2023-01-17
// MAT 240B ~ Audio Programming
// Assignment 2 ~ Quasi Band-Limited Sawtooth and Pulse Generator
// Ryan Millett

#include <juce_audio_processors/juce_audio_processors.h>

#include "quasi.hpp"

template <typename T>
T mtof(T m) {
  return T(440) * pow(T(2), (m - T(69)) / T(12));
}

template <typename T>
T dbtoa(T db) {
  return pow(T(10), db / T(20));
}

template <class T>
inline T sine(T n) {
  T nn = n*n;
	return n * (T(3.138982) + nn * (T(-5.133625) +
                          nn * (T(2.428288) - nn * T(0.433645))));
}

float softclip(float x) {
  if (x >= 1.0f) x = 1.0f;
  if (x <= -1.0f) x = -1.0f;
  return (3 * x - x * x * x) / 2.0f;
}

struct Cycle
{
  float phase = 0;
  float operator()(float hertz) {
    float value = sine(phase); // output sample;
    phase += hertz / 48000;
    if (phase > 1.0f) {
      phase -= 2.0f;
    }
    return value;
  }
};

using namespace juce;

// http://scp.web.elte.hu/papers/synthesis1.pdf

struct QuasiBandLimited : public AudioProcessor {
  AudioParameterFloat* gain;
  AudioParameterFloat* note;
  AudioParameterFloat* oscMix;
  AudioParameterFloat* filter;
  AudioParameterFloat* pulseWidth;
  // Cycle carrier, modulator;
  /// add parameters here ///////////////////////////////////////////////////
  /// add your objects here /////////////////////////////////////////////////
  QuasiSaw qSaw;
  QuasiPulse qPulse;

  QuasiBandLimited()
      : AudioProcessor(BusesProperties()
                           .withInput("Input", AudioChannelSet::stereo())
                           .withOutput("Output", AudioChannelSet::stereo())) {
    addParameter(gain = new AudioParameterFloat(
                     {"gain", 1}, "Gain",
                     NormalisableRange<float>(-65, -1, 0.01f), -65));
    /// add parameters here /////////////////////////////////////////////////
    addParameter(note = new AudioParameterFloat(
                     {"note", 1}, "Pitch (MIDI)",
                     NormalisableRange<float>(-2, 129, 0.01f), 33));
    addParameter(filter = new AudioParameterFloat(
                     {"filter", 1}, "Filter",
                     NormalisableRange<float>(0, 1.0f, 0.001f), 1.0f));
    addParameter(pulseWidth = new AudioParameterFloat(
                     {"pulseWidth", 1}, "qPulse Width",
                     NormalisableRange<float>(0.1f, 0.9f, 0.0001f), 0.1f));
    addParameter(oscMix = new AudioParameterFloat(
                     {"quasiMix", 1}, "qSaw <--> qPulse",
                     NormalisableRange<float>(0, 1, 0.001f), 0.5f));
  }

  /// this function handles the audio ///////////////////////////////////////
  void processBlock(AudioBuffer<float>& buffer, MidiBuffer&) override {
    /// put your own code here instead of this code /////////////////////////
    buffer.clear(0, 0, buffer.getNumSamples());
    buffer.clear(1, 0, buffer.getNumSamples());
    auto left = buffer.getWritePointer(0, 0);
    auto right = buffer.getWritePointer(1, 0);
    // left[0] = right[0] = dbtoa(gain->get());  // click!
    for (int i = 0; i < buffer.getNumSamples(); ++i) {
        float A = dbtoa(gain->get());
        float freq = mtof(note->get());
        
        qSaw.set(freq);
        qSaw.updateFilter(filter->get());

        qPulse.set(freq);
        qPulse.updateFilter(filter->get());
        qPulse.pw = pulseWidth->get();
        
        // mix between QuasiPulse and QuasiSaw
        left[i] = right[i] = A * (oscMix->get() * qPulse() + (1 - oscMix->get()) * qSaw());
    }
  }

  /// start and shutdown callbacks///////////////////////////////////////////
  void prepareToPlay(double, int) override {}
  void releaseResources() override {}

  /// maintaining persistant state on suspend ///////////////////////////////
  void getStateInformation(MemoryBlock& destData) override {
    MemoryOutputStream(destData, true).writeFloat(*gain);
    /// add parameters here /////////////////////////////////////////////////
  }

  void setStateInformation(const void* data, int sizeInBytes) override {
    gain->setValueNotifyingHost(
        MemoryInputStream(data, static_cast<size_t>(sizeInBytes), false)
            .readFloat());
    /// add parameters here /////////////////////////////////////////////////
  }

  /// do not change anything below this line, probably //////////////////////

  /// general configuration /////////////////////////////////////////////////
  const String getName() const override { return "Quasi Band Limited"; }
  double getTailLengthSeconds() const override { return 0; }
  bool acceptsMidi() const override { return false; }
  bool producesMidi() const override { return false; }

  /// for handling presets //////////////////////////////////////////////////
  int getNumPrograms() override { return 1; }
  int getCurrentProgram() override { return 0; }
  void setCurrentProgram(int) override {}
  const String getProgramName(int) override { return "None"; }
  void changeProgramName(int, const String&) override {}

  /// ?????? ////////////////////////////////////////////////////////////////
  bool isBusesLayoutSupported(const BusesLayout& layouts) const override {
    const auto& mainInLayout = layouts.getChannelSet(true, 0);
    const auto& mainOutLayout = layouts.getChannelSet(false, 0);

    return (mainInLayout == mainOutLayout && (!mainInLayout.isDisabled()));
  }

  /// automagic user interface //////////////////////////////////////////////
  AudioProcessorEditor* createEditor() override {
    return new GenericAudioProcessorEditor(*this);
  }
  bool hasEditor() const override { return true; }

 private:
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(QuasiBandLimited)
};

AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
  return new QuasiBandLimited();
}