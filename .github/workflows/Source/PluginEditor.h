#pragma once
#include "PluginProcessor.h"
#include <juce_gui_basics/juce_gui_basics.h>
struct SmallVUB; struct GRVUB;
class BassEditor : public juce::AudioProcessorEditor, public juce::Timer {
public: explicit BassEditor(BassAudioProcessor&); ~BassEditor() override;
  void paint(juce::Graphics&) override; void resized() override; void timerCallback() override;
private:
  BassAudioProcessor& p;
  using Att = juce::AudioProcessorValueTreeState::SliderAttachment;
  using BAtt = juce::AudioProcessorValueTreeState::ButtonAttachment;
  using CAtt = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
  std::vector<std::unique_ptr<juce::Slider>> sliders; std::vector<std::unique_ptr<Att>> satts;
  std::vector<std::unique_ptr<juce::ToggleButton>> toggles; std::vector<std::unique_ptr<BAtt>> batts;
  std::vector<std::unique_ptr<juce::ComboBox>> combos; std::vector<std::unique_ptr<CAtt>> catts;
  std::vector<std::unique_ptr<juce::Label>> knobLabels;
  juce::Slider* addKnob(const juce::String& id, const juce::String& label, int x,int y,int w,int h);
  juce::Slider* addKnob(const juce::String& id, const juce::String& label, int x,int y,int w,int h, juce::Colour labelColour);
  juce::ToggleButton* addToggle(const juce::String& id,const juce::String& label,int x,int y,int w,int h);
  juce::ToggleButton* addToggle(const juce::String& id,const juce::String& label,int x,int y,int w,int h, juce::Colour textColour);
  juce::ComboBox* addChoice(const juce::String& id,int x,int y,int w,int h);
  std::unique_ptr<juce::TextButton> bypassBtn, loadBtn; std::unique_ptr<BAtt> bypassAtt;
  std::unique_ptr<SmallVUB> inVU, outVU; std::unique_ptr<GRVUB> laVU, limVU;
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BassEditor)
};
