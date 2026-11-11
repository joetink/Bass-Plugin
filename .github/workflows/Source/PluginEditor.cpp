#include "PluginEditor.h"
// JT Bass - BUILT FROM SCRATCH TO MOCKUP 2026-11-11 - FIRST BUILD, NOT A PATCH
// Chain: SansAmp V2 -> CAB IR -> EQ -> LA-2A -> Limiter (0-latency, NO lookahead)
// TRUE rules (same as Mix Bus TRUE / 500 Strip TRUE / Drum Bus TRUE):
//  1. No titleLabel. Paint draws top-bar + panel titles ONCE.
//  2. Needle math: x=cx+sin(a)*r, y=cy-cos(a)*r, pivot bottom-centre, a -0.9..+0.9 rad. No diagonal slash.
//  3. drawComboBox: text x=8, width w-28, arrow w-20, font 11, boxes 96-120 wide.
//  4. Knob labels: Label ONLY, LookAndFeel draws no text.
//  5. Meters bounds set immediately + resized() at end.
// Bypass: GLOBAL BYPASS (top bar, real ButtonAttachment) + per-module ON toggles (SANS_ON / CAB_ON / EQ_ON / LA_ON / LIM_ON).
// Limiter is 0-latency: instant per-sample gain, smoothed release only, no lookahead, no delay. It cannot overshoot
// by a buffer - it catches the sample that crosses thresh, in that sample.
struct KnobBass : juce::Slider { KnobBass(): juce::Slider(juce::Slider::RotaryVerticalDrag, juce::Slider::NoTextBox){ setTextBoxStyle(juce::Slider::NoTextBox,false,0,0);} };
struct SmallVUB : juce::Component {
  float db=-60; juce::String label="IN"; void setDb(float v){ db=v; repaint(); }
  void paint(juce::Graphics& g) override {
    g.fillAll(juce::Colour(0xfff5e8c8)); g.setColour(juce::Colours::black); g.drawRect(0,0,getWidth(),getHeight(),1);
    g.setColour(juce::Colours::black); g.setFont(9.0f); g.drawText(label,0,2,getWidth(),10,juce::Justification::centred);
    float cx=getWidth()*0.5f, cy=(float)getHeight()-4, r=getWidth()*0.42f;
    g.setColour(juce::Colours::black.withAlpha(0.5f));
    for(int i=0;i<=5;i++){ float a=juce::jmap((float)i,0.f,5.f,-0.9f,0.9f); g.drawLine(cx+std::sin(a)*r,cy-std::cos(a)*r,cx+std::sin(a)*(r+3),cy-std::cos(a)*(r+3),1); }
    float pos=juce::jlimit(-60.0f,3.0f,db); float ang=juce::jmap(pos,-60.0f,3.0f,-0.9f,0.9f);
    g.setColour(juce::Colours::red); g.drawLine(cx,cy,cx+std::sin(ang)*r,cy-std::cos(ang)*r,1.5f);
    g.setColour(juce::Colours::black); g.setFont(8.0f); g.drawText(juce::String(db,1)+" dB",0,getHeight()-11,getWidth(),10,juce::Justification::centred);
  }
};
struct GRVUB : juce::Component {
  float grDb=0; juce::String title="GR"; void setGR(float v){ grDb=v; repaint(); }
  void paint(juce::Graphics& g) override {
    g.fillAll(juce::Colour(0xfff5e8c8)); g.setColour(juce::Colours::black); g.drawRect(0,0,getWidth(),getHeight(),1);
    g.setColour(juce::Colours::black); g.setFont(10.0f); g.drawText(title,0,4,getWidth(),12,juce::Justification::centred);
    float cx=getWidth()*0.5f, cy=(float)getHeight()-6, r=getWidth()*0.42f;
    g.setColour(juce::Colours::black.withAlpha(0.5f));
    for(int i=0;i<=10;i++){ float a=juce::jmap((float)i,0.f,10.f,-0.9f,0.9f); g.drawLine(cx+std::sin(a)*r,cy-std::cos(a)*r,cx+std::sin(a)*(r+4),cy-std::cos(a)*(r+4),1); }
    g.setFont(9.0f); g.drawText("0",cx+r*0.75f,cy-16,18,10,juce::Justification::centred); g.drawText("-20",cx-r*0.75f-8,cy-16,22,10,juce::Justification::centred);
    float pos=juce::jlimit(-20.0f,0.0f,grDb); float ang=juce::jmap(pos,-20.0f,0.0f,-0.9f,0.9f);
    g.setColour(juce::Colours::red); g.drawLine(cx,cy,cx+std::sin(ang)*r,cy-std::cos(ang)*r,2.0f);
    g.setColour(juce::Colours::black); g.setFont(11.0f); g.drawText(juce::String(grDb,1)+" dB",0,getHeight()-16,getWidth(),14,juce::Justification::centred);
  }
};
struct LookBass : juce::LookAndFeel_V4 {
  void drawRotarySlider(juce::Graphics& g,int x,int y,int w,int h,float pos,float, float, juce::Slider&) override {
    float cx=x+w*0.5f, cy=y+h*0.5f, r=juce::jmin(w,h)*0.42f;
    g.setColour(juce::Colours::white.withAlpha(0.45f));
    for(int i=0;i<11;i++){ float a=juce::jmap((float)i,0.f,10.f,-2.356f,2.356f); g.drawLine(cx+std::sin(a)*(r+3),cy-std::cos(a)*(r+3),cx+std::sin(a)*(r+7),cy-std::cos(a)*(r+7),1); }
    g.setColour(juce::Colours::black.withAlpha(0.45f)); g.fillEllipse(cx-r+2,cy-r+3,r*2,r*2);
    juce::ColourGradient grad(juce::Colour(0xff3a3a3a),cx-r,cy-r,juce::Colour(0xff0a0a0a),cx+r,cy+r,false);
    g.setGradientFill(grad); g.fillEllipse(cx-r,cy-r,r*2,r*2);
    g.setColour(juce::Colour(0xff777777)); g.drawEllipse(cx-r,cy-r,r*2,r*2,1.5f);
    g.setColour(juce::Colours::white.withAlpha(0.18f)); g.fillEllipse(cx-r*0.7f,cy-r*0.7f,r*0.9f,r*0.6f);
    float ang=juce::jmap(pos,0.f,1.f,-2.356f,2.356f);
    g.setColour(juce::Colours::white); g.drawLine(cx,cy,cx+std::sin(ang)*r*0.82f,cy-std::cos(ang)*r*0.82f,2.5f);
  }
  void drawComboBox(juce::Graphics& g,int w,int h,bool, int,int,int,int, juce::ComboBox& box) override {
    g.setColour(juce::Colour(0xff222222)); g.fillRect(0,0,w,h); g.setColour(juce::Colours::white); g.drawRect(0,0,w,h,1);
    g.setFont(11.0f); g.drawText(box.getText(),8,0,w-28,h,juce::Justification::centredLeft); g.drawText("v",w-20,0,14,h,juce::Justification::centred);
  }
  void drawButtonBackground(juce::Graphics& g, juce::Button& b, const juce::Colour&, bool, bool) override {
    if(b.getName()=="BYPASS"){ bool on=b.getToggleState(); g.setColour(on?juce::Colour(0xff5a0f0f):juce::Colour(0xffc0272d)); g.fillRect(0,0,b.getWidth(),b.getHeight()); g.setColour(juce::Colours::white); g.drawRect(0,0,b.getWidth(),b.getHeight(),1); g.setFont(juce::Font(12.0f,juce::Font::bold)); g.drawText("BYPASS",0,0,b.getWidth(),b.getHeight(),juce::Justification::centred); return; }
    LookAndFeel_V4::drawButtonBackground(g,b,juce::Colours::grey,false,false);
  }
};
static LookBass lookBass;
BassEditor::BassEditor(BassAudioProcessor& proc): AudioProcessorEditor(proc), p(proc){
  setSize(1480, 680); setLookAndFeel(&lookBass);
  bypassBtn = std::make_unique<juce::TextButton>("BYPASS"); bypassBtn->setName("BYPASS"); bypassBtn->setBounds(680,14,110,32); bypassBtn->setClickingTogglesState(true); addAndMakeVisible(bypassBtn.get());
  bypassAtt = std::make_unique<BAtt>(p.apvts,"BYPASS",*bypassBtn);
  inVU = std::make_unique<SmallVUB>(); inVU->label="IN"; addAndMakeVisible(inVU.get()); inVU->setBounds(820,10,110,50);
  outVU = std::make_unique<SmallVUB>(); outVU->label="OUT"; addAndMakeVisible(outVU.get()); outVU->setBounds(950,10,110,50);
  // SANSAMP V2 - burnt orange, Mid Shift selector
  addToggle("SANS_ON","SANSAMP ON",20,102,120,18);
  addKnob("SANS_DRIVE","DRIVE",20,130,80,80); addKnob("SANS_BASS","BASS",110,130,80,80); addKnob("SANS_MID","MID",200,130,80,80);
  addKnob("SANS_TREBLE","TREBLE",20,230,80,80); addKnob("SANS_LEVEL","LEVEL",110,230,80,80); addKnob("SANS_BLEND","BLEND",200,230,80,80);
  addChoice("SANS_MIDSHIFT",20,330,120,24);
  // CAB IR - dark, 8x10 / 4x10 / 1x15 / CUSTOM + LOAD + Blend
  addToggle("CAB_ON","CAB ON",330,102,80,18);
  addChoice("CAB_SELECT",330,130,120,24);
  loadBtn = std::make_unique<juce::TextButton>("LOAD IR..."); loadBtn->setBounds(330,165,120,24); loadBtn->onClick=[this]{ juce::FileChooser fc("Load Bass Cab IR (WAV)", juce::File(), "*.wav"); if(fc.browseForFileToOpen()){ auto f=fc.getResult(); p.loadIRFile(f); } }; addAndMakeVisible(loadBtn.get());
  addKnob("CAB_BLEND","BLEND",330,210,80,80);
  // EQ - black
  addToggle("EQ_ON","EQ ON",520,102,80,18);
  addKnob("EQ_HPF","HPF",520,130,70,70); addKnob("EQ_LPF","LPF",600,130,70,70);
  addKnob("EQ_LOW","LOW",520,230,70,70); addKnob("EQ_MID","MID",600,230,70,70); addKnob("EQ_MIDFREQ","MID FREQ",680,230,70,70); addKnob("EQ_HIGH","HIGH",760,230,70,70);
  // LA-2A - cream
  addToggle("LA_ON","LA ON",880,102,80,18);
  addKnob("LA_PEAKRED","PEAK RED",880,130,80,80); addKnob("LA_GAIN","GAIN",970,130,80,80); addKnob("LA_MIX","MIX",1060,130,80,80);
  laVU = std::make_unique<GRVUB>(); laVU->title="LA GR"; addAndMakeVisible(laVU.get()); laVU->setBounds(880,230,260,130);
  // LIMITER - grey, 0-latency
  addToggle("LIM_ON","LIMITER ON",1180,102,110,18);
  addKnob("LIM_THRESH","THRESH",1180,130,80,80); addKnob("LIM_CEIL","CEIL",1270,130,80,80); addKnob("LIM_MIX","MIX",1360,130,80,80);
  limVU = std::make_unique<GRVUB>(); limVU->title="LIM GR"; addAndMakeVisible(limVU.get()); limVU->setBounds(1180,230,260,130);
  startTimer(30);
  resized();
}
BassEditor::~BassEditor(){ stopTimer(); setLookAndFeel(nullptr); }
void BassEditor::timerCallback(){ if(laVU) laVU->setGR(p.bus.grLA); if(limVU) limVU->setGR(p.bus.limGR); if(inVU) inVU->setDb(p.bus.inLevelDb); if(outVU) outVU->setDb(p.bus.outLevelDb); }
juce::Slider* BassEditor::addKnob(const juce::String& id,const juce::String& label,int x,int y,int w,int h){ return addKnob(id,label,x,y,w,h,juce::Colours::white); }
juce::Slider* BassEditor::addKnob(const juce::String& id,const juce::String& label,int x,int y,int w,int h,juce::Colour lc){
  auto s=std::make_unique<KnobBass>(); s->setBounds(x,y,w,h); addAndMakeVisible(s.get());
  auto a=std::make_unique<Att>(p.apvts,id,*s); sliders.push_back(std::move(s)); satts.push_back(std::move(a));
  auto lb=std::make_unique<juce::Label>(); lb->setText(label,juce::dontSendNotification); lb->setFont(juce::Font(11.0f,juce::Font::bold)); lb->setColour(juce::Label::textColourId,lc); lb->setJustificationType(juce::Justification::centred); lb->setBounds(x-10,y+h,w+20,16); addAndMakeVisible(lb.get()); knobLabels.push_back(std::move(lb));
  return sliders.back().get();
}
juce::ToggleButton* BassEditor::addToggle(const juce::String& id,const juce::String& label,int x,int y,int w,int h){ return addToggle(id,label,x,y,w,h,juce::Colours::white); }
juce::ToggleButton* BassEditor::addToggle(const juce::String& id,const juce::String& label,int x,int y,int w,int h,juce::Colour tc){ auto t=std::make_unique<juce::ToggleButton>(label); t->setBounds(x,y,w,h); t->setColour(juce::ToggleButton::textColourId,tc); t->setColour(juce::ToggleButton::tickColourId,tc); addAndMakeVisible(t.get()); auto a=std::make_unique<BAtt>(p.apvts,id,*t); toggles.push_back(std::move(t)); batts.push_back(std::move(a)); return toggles.back().get(); }
juce::ComboBox* BassEditor::addChoice(const juce::String& id,int x,int y,int w,int h){
  auto c=std::make_unique<juce::ComboBox>(); c->setBounds(x,y,w,h);
  if(id=="SANS_MIDSHIFT"){ c->addItem("200 Hz",1); c->addItem("500 Hz",2); c->addItem("1 kHz",3); }
  else if(id=="CAB_SELECT"){ c->addItem("8x10",1); c->addItem("4x10",2); c->addItem("1x15",3); c->addItem("CUSTOM",4); c->addItem("OFF",5); }
  addAndMakeVisible(c.get()); auto a=std::make_unique<CAtt>(p.apvts,id,*c); combos.push_back(std::move(c)); catts.push_back(std::move(a)); return combos.back().get();
}
void BassEditor::paint(juce::Graphics& g){
  g.fillAll(juce::Colour(0xff141414));
  { // Top bar - title ONCE
    juce::ColourGradient steel(juce::Colour(0xff4a4a4a),0,0,juce::Colour(0xff1a1a1a),0,70,false);
    g.setGradientFill(steel); g.fillRect(0,0,1480,70);
    g.setColour(juce::Colours::white.withAlpha(0.08f)); for(int y=2;y<70;y+=3) g.drawLine(0,y,1480,y,1);
    g.setColour(juce::Colour(0xffdddddd)); g.setFont(juce::Font(22.0f,juce::Font::bold)); g.drawText("JT BASS  -  SANSAMP  -  CAB IR  -  500 SERIES",20,12,620,30,juce::Justification::left);
    g.setFont(11.0f); g.drawText("SANSAMP  ->  CAB IR  ->  EQ  ->  LA-2A  ->  LIMITER",20,40,620,16,juce::Justification::left);
    g.setColour(juce::Colour(0xffdddddd)); g.setFont(juce::Font(18.0f,juce::Font::bold)); g.drawText("JT",1360,16,60,30,juce::Justification::centred);
  }
  auto panel=[&](int x,int y,int w,int h,juce::Colour col,juce::String title,juce::Colour tc){
    juce::ColourGradient grad(col.brighter(0.08f),(float)x,(float)y,col.darker(0.12f),(float)x,(float)(y+h),false);
    g.setGradientFill(grad); g.fillRect(x,y,w,h);
    g.setColour(juce::Colours::white.withAlpha(0.10f)); g.fillRect(x,y,w,3);
    g.setColour(juce::Colours::black.withAlpha(0.55f)); g.drawRect(x,y,w,h,2);
    g.setColour(juce::Colour(0xff888888)); for(auto [sx,sy] : {std::pair<int,int>{x+6,y+6},{x+w-10,y+6},{x+6,y+h-10},{x+w-10,y+h-10}}){ g.fillEllipse(sx,sy,6,6); }
    g.setColour(tc); g.setFont(juce::Font(12.0f,juce::Font::bold)); g.drawText(title,x+10,y+6,w-20,16,juce::Justification::left);
  };
  panel(10,80,300,580,juce::Colour(0xffc65a1e),"SANSAMP V2",juce::Colours::white);
  panel(320,80,180,580,juce::Colour(0xff2b2b2b),"CAB IR",juce::Colours::white);
  panel(510,80,340,580,juce::Colour(0xff1a1a1a),"EQ",juce::Colours::white);
  panel(870,80,290,580,juce::Colour(0xfff0e6d0),"LA-2A",juce::Colours::black);
  panel(1170,80,300,580,juce::Colour(0xff4a5a6a),"LIMITER - 0 LATENCY",juce::Colours::white);
  g.setColour(juce::Colours::white); g.setFont(11.0f);
  g.drawText("MID SHIFT",20,356,120,14,juce::Justification::centred);
  g.drawText("CAB",330,156,120,14,juce::Justification::centred);
  g.drawText("LOAD A WAV TO USE CUSTOM",330,195,160,14,juce::Justification::left);
  g.drawText("LA GR",880,214,260,14,juce::Justification::centred);
  g.drawText("LIM GR - NO LOOKAHEAD",1180,214,260,14,juce::Justification::centred);
  g.setColour(juce::Colours::black); g.drawText("DRIVE + BLEND ONLY - NO TYPE",880,380,260,14,juce::Justification::centred);
}
void BassEditor::resized(){
  if(bypassBtn) bypassBtn->setBounds(680,14,110,32);
  if(inVU) inVU->setBounds(820,10,110,50);
  if(outVU) outVU->setBounds(950,10,110,50);
  if(laVU) laVU->setBounds(880,230,260,130);
  if(limVU) limVU->setBounds(1180,230,260,130);
  if(loadBtn) loadBtn->setBounds(330,165,120,24);
}
juce::AudioProcessorEditor* BassAudioProcessor::createEditor(){ return new BassEditor(*this); }
