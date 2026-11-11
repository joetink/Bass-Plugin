#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "DSP.h"
class BassAudioProcessor : public juce::AudioProcessor {
public:
  BassAudioProcessor(); ~BassAudioProcessor() override;
  void prepareToPlay(double sr, int) override;
  void releaseResources() override;
  void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
  juce::AudioProcessorEditor* createEditor() override;
  bool hasEditor() const override { return true; }
  const juce::String getName() const override { return "JT Bass"; }
  bool acceptsMidi() const override { return false; }
  bool producesMidi() const override { return false; }
  double getTailLengthSeconds() const override { return 0; }
  int getNumPrograms() override { return 1; }
  int getCurrentProgram() override { return 0; }
  void setCurrentProgram(int) override {}
  const juce::String getProgramName(int) override { return {}; }
  void changeProgramName(int, const juce::String&) override {}
  void getStateInformation(juce::MemoryBlock&) override {}
  void setStateInformation(const void*, int) override {}
  void loadIRFile(const juce::File& file);
  juce::AudioProcessorValueTreeState apvts;
  BassBus bus;
private:
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BassAudioProcessor)
};
