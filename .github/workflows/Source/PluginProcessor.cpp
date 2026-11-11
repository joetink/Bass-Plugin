#include "PluginProcessor.h"
#include "PluginEditor.h"
static juce::AudioProcessorValueTreeState::ParameterLayout layout(){
  using P = juce::AudioProcessorValueTreeState; std::vector<std::unique_ptr<juce::RangedAudioParameter>> v;
  auto F=[&](const char* id,const char* n,float mn,float mx,float d){ v.push_back(std::make_unique<juce::AudioParameterFloat>(id,n,mn,mx,d)); };
  auto B=[&](const char* id,const char* n,bool d){ v.push_back(std::make_unique<juce::AudioParameterBool>(id,n,d)); };
  auto C=[&](const char* id,const char* n,juce::StringArray opts,int d){ v.push_back(std::make_unique<juce::AudioParameterChoice>(id,n,opts,d)); };
  B("BYPASS","Bypass",false);
  // SansAmp V2
  B("SANS_ON","SansAmp ON",true); F("SANS_DRIVE","Sans Drive",0,100,30); F("SANS_BASS","Sans Bass",-15,15,0); F("SANS_MID","Sans Mid",-15,15,0); F("SANS_TREBLE","Sans Treble",-15,15,0); F("SANS_LEVEL","Sans Level",-20,20,0); F("SANS_BLEND","Sans Blend",0,100,100); C("SANS_MIDSHIFT","Mid Shift",juce::StringArray{"200 Hz","500 Hz","1 kHz"},1);
  // CAB IR
  B("CAB_ON","Cab ON",true); C("CAB_SELECT","Cab",juce::StringArray{"8x10","4x10","1x15","CUSTOM","OFF"},0); F("CAB_BLEND","Cab Blend",0,100,100);
  // EQ
  B("EQ_ON","EQ ON",true); F("EQ_HPF","EQ HPF",20,500,30); F("EQ_LPF","EQ LPF",1000,20000,8000); F("EQ_LOW","EQ Low",-15,15,0); F("EQ_MID","EQ Mid",-15,15,0); F("EQ_MIDFREQ","EQ Mid Freq",100,2000,500); F("EQ_HIGH","EQ High",-15,15,0);
  // LA
  B("LA_ON","LA ON",true); F("LA_PEAKRED","LA Peak Red",0,100,50); F("LA_GAIN","LA Gain",-20,20,0); F("LA_MIX","LA Mix",0,100,100);
  // Limiter - 0-latency, no lookahead
  B("LIM_ON","Limiter ON",true); F("LIM_THRESH","Lim Thresh",-20,0,-1); F("LIM_CEIL","Lim Ceil",-20,0,-0.5); F("LIM_MIX","Lim Mix",0,100,100);
  return juce::AudioProcessorValueTreeState::ParameterLayout(std::make_move_iterator(v.begin()), std::make_move_iterator(v.end()));
}
BassAudioProcessor::BassAudioProcessor(): AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true).withOutput("Output", juce::AudioChannelSet::stereo(), true)), apvts(*this, nullptr, "PARAMS", layout()) { bus.prepare(48000); }
BassAudioProcessor::~BassAudioProcessor(){}
void BassAudioProcessor::prepareToPlay(double sr,int){ bus.prepare(sr); }
void BassAudioProcessor::releaseResources(){}
void BassAudioProcessor::loadIRFile(const juce::File& file){
  juce::AudioFormatManager fm; fm.registerBasicFormats();
  std::unique_ptr<juce::AudioFormatReader> r(fm.createReaderFor(file));
  if(!r) return;
  int len=(int)std::min<int64_t>(2048, r->lengthInSamples);
  juce::AudioBuffer<float> buf((int)r->numChannels, len); r->read(&buf,0,len,0,true,false);
  for(int i=0;i<len;i++) bus.cabCustomBuf[i]=buf.getSample(0,i);
  bus.cabCustomLen=len; bus.cabCustomLoaded=true; bus.cabSelect=3;
  if(auto* p=apvts.getParameter("CAB_SELECT")) p->setValueNotifyingHost(3.0f/4.0f);
}
void BassAudioProcessor::processBlock(juce::AudioBuffer<float>& b, juce::MidiBuffer&){
  juce::ScopedNoDenormals d; auto G=[&](const char* id)->float{ return *apvts.getRawParameterValue(id); };
  bus.bypass=G("BYPASS")>0.5f;
  bus.sansOn=G("SANS_ON")>0.5f; bus.sansDrive=G("SANS_DRIVE"); bus.sansBass=G("SANS_BASS"); bus.sansMid=G("SANS_MID"); bus.sansTreble=G("SANS_TREBLE"); bus.sansLevel=G("SANS_LEVEL"); bus.sansBlend=G("SANS_BLEND"); bus.sansMidShift=(int)G("SANS_MIDSHIFT");
  bus.cabOn=G("CAB_ON")>0.5f; bus.cabSelect=(int)G("CAB_SELECT"); bus.cabBlend=G("CAB_BLEND");
  bus.eqOn=G("EQ_ON")>0.5f; bus.eqHPF=G("EQ_HPF"); bus.eqLPF=G("EQ_LPF"); bus.eqLow=G("EQ_LOW"); bus.eqMid=G("EQ_MID"); bus.eqMidFreq=G("EQ_MIDFREQ"); bus.eqHigh=G("EQ_HIGH");
  bus.laOn=G("LA_ON")>0.5f; bus.laPeakRed=G("LA_PEAKRED"); bus.laGain=G("LA_GAIN"); bus.laMix=G("LA_MIX");
  bus.limOn=G("LIM_ON")>0.5f; bus.limThresh=G("LIM_THRESH"); bus.limCeil=G("LIM_CEIL"); bus.limMix=G("LIM_MIX");
  bus.updateBlock();
  if(b.getNumChannels()>=2){ auto* dl=b.getWritePointer(0); auto* dr=b.getWritePointer(1); for(int i=0;i<b.getNumSamples();++i) bus.processStereo(dl[i],dr[i]); }
  else { for(int ch=0;ch<b.getNumChannels();++ch){ auto* data=b.getWritePointer(ch); for(int i=0;i<b.getNumSamples();++i){ float L=data[i], R=data[i]; bus.processStereo(L,R); data[i]=L; } } }
}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){ return new BassAudioProcessor(); }
