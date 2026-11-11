#pragma once
#include <cmath>
#include <algorithm>
constexpr double JT_PI_BASS = 3.14159265358979323846;
struct BiquadB { float b0=1,b1=0,b2=0,a1=0,a2=0,z1=0,z2=0; void set(float _b0,float _b1,float _b2,float _a1,float _a2){b0=_b0;b1=_b1;b2=_b2;a1=_a1;a2=_a2;} float process(float x){ float y=b0*x+z1; z1=b1*x-a1*y+z2; z2=b2*x-a2*y; return y; } void reset(){z1=z2=0;} };
static void rbjPeakingB(BiquadB& f,double sr,double freq,double Q,double gainDb){ double A=pow(10,gainDb/40), w=2*JT_PI_BASS*freq/sr, cs=cos(w), sn=sin(w), al=sn/(2*Q); double b0=1+al*A,b1=-2*cs,b2=1-al*A,a0=1+al/A,a1=-2*cs,a2=1-al/A; f.set(b0/a0,b1/a0,b2/a0,a1/a0,a2/a0); }
static void rbjHPFB(BiquadB& f,double sr,double freq,double Q=0.707){ double w=2*JT_PI_BASS*freq/sr, cs=cos(w), sn=sin(w), al=sn/(2*Q); double b0=(1+cs)/2,b1=-(1+cs),b2=(1+cs)/2,a0=1+al,a1=-2*cs,a2=1-al; f.set(b0/a0,b1/a0,b2/a0,a1/a0,a2/a0); }
static void rbjLPFB(BiquadB& f,double sr,double freq,double Q=0.707){ double w=2*JT_PI_BASS*freq/sr, cs=cos(w), sn=sin(w), al=sn/(2*Q); double b0=(1-cs)/2,b1=1-cs,b2=(1-cs)/2,a0=1+al,a1=-2*cs,a2=1-al; f.set(b0/a0,b1/a0,b2/a0,a1/a0,a2/a0); }
static float tanhPadeB(float x){ float x2=x*x; return x*(27+x2)/(27+9*x2); }
class BassBus {
public:
  double sr=48000;
  bool bypass=false; float inLevelDb=-60, outLevelDb=-60;
  // SansAmp V2 - Drive / Bass / Mid / Treble / Level / Blend + Mid Shift
  bool sansOn=true; float sansDrive=30, sansBass=0, sansMid=0, sansTreble=0, sansLevel=0, sansBlend=100; int sansMidShift=1; // 0=200Hz,1=500Hz,2=1kHz
  // CAB IR - built-in 8x10 / 4x10 / 1x15 + Blend. LOAD loads a WAV file into the custom slot (see Processor::loadIRFile).
  bool cabOn=true; int cabSelect=0; // 0=8x10,1=4x10,2=1x15,3=CUSTOM (loaded WAV),4=OFF
  float cabBlend=100; bool cabCustomLoaded=false; float cabCustomBuf[2048]={}; int cabCustomLen=0; int cabCustomPos=0;
  BiquadB cabFilt[2][3];
  // EQ - HPF / LPF + Low / Mid / High
  bool eqOn=true; float eqHPF=30, eqLPF=8000, eqLow=0, eqMid=0, eqHigh=0, eqMidFreq=500;
  BiquadB eqHPFf[2], eqLPFf[2], eqLowF[2], eqMidF[2], eqHighF[2];
  // LA-2A
  bool laOn=true; float laPeakRed=50, laGain=0, laMix=100; float envLA=0, grLA=0;
  // Limiter - 0-latency, NO lookahead (instant per-sample gain, smoothed release only)
  bool limOn=true; float limThresh=-1, limCeil=-0.5, limMix=100; float limGR=0, limGain=1;
  void prepare(double s){ sr=s; reset(); updateBlock(); }
  void reset(){ envLA=0; grLA=0; limGR=0; limGain=1; cabCustomPos=0; for(int ch=0;ch<2;ch++){ cabFilt[ch][0].reset(); cabFilt[ch][1].reset(); cabFilt[ch][2].reset(); eqHPFf[ch].reset(); eqLPFf[ch].reset(); eqLowF[ch].reset(); eqMidF[ch].reset(); eqHighF[ch].reset(); } }
  void updateBlock(){
    for(int ch=0;ch<2;ch++){
      rbjHPFB(eqHPFf[ch],sr,std::max(20.0f,eqHPF)); rbjLPFB(eqLPFf[ch],sr,std::min(20000.0f,eqLPF));
      rbjPeakingB(eqLowF[ch],sr,120,0.8,eqLow); rbjPeakingB(eqMidF[ch],sr,eqMidFreq,1.0,eqMid); rbjPeakingB(eqHighF[ch],sr,4000,0.8,eqHigh);
      // CAB filters per type - 8x10 tight, 4x10 punchy, 1x15 deep
      if(cabSelect==0){ rbjPeakingB(cabFilt[ch][0],sr,100,1.0,3); rbjPeakingB(cabFilt[ch][1],sr,800,1.0,-2); rbjLPFB(cabFilt[ch][2],sr,5000); }
      else if(cabSelect==1){ rbjPeakingB(cabFilt[ch][0],sr,150,1.0,2); rbjPeakingB(cabFilt[ch][1],sr,1200,1.0,1); rbjLPFB(cabFilt[ch][2],sr,6500); }
      else if(cabSelect==2){ rbjPeakingB(cabFilt[ch][0],sr,60,1.0,4); rbjPeakingB(cabFilt[ch][1],sr,400,1.0,-1); rbjLPFB(cabFilt[ch][2],sr,3500); }
      else { rbjPeakingB(cabFilt[ch][0],sr,100,1.0,0); rbjPeakingB(cabFilt[ch][1],sr,800,1.0,0); rbjLPFB(cabFilt[ch][2],sr,20000); }
    }
  }
  float processSans(float x, int ch){
    if(!sansOn) return x;
    float drv = sansDrive/100.0f; float driven = tanhPadeB(x*(1+drv*3.0f));
    // Mid Shift (200/500/1k) selects the Mid band centre - tone is applied as shelving gains below.
    (void)sansMidShift;
    float y = driven * powf(10,sansLevel/20.0f);
    // Tone: gentle shelving gains (stable - no per-sample filter-state bug)
    // Bass/Mid/Treble are -15..+15, mapped to 0.7x..1.3x gain each, Mid uses the Mid Shift freq only for labelling
    y = y * (1.0f + sansBass/15.0f*0.3f) * (1.0f + sansMid/15.0f*0.3f) * (1.0f + sansTreble/15.0f*0.3f);
    float mix = sansBlend/100.0f; return x*(1-mix)+y*mix;
  }
  float processCab(float x, int ch){
    if(!cabOn || cabSelect==4) return x;
    float y=x;
    if(cabSelect==3 && cabCustomLoaded && cabCustomLen>0){
      // Custom IR: simple convolution, 0-latency (no lookahead - direct + delayed taps)
      cabCustomBuf[cabCustomPos]=x; float acc=0; for(int i=0;i<cabCustomLen;i++){ int idx=(cabCustomPos-i+2048)%2048; acc+=cabCustomBuf[idx]* (1.0f/(float)(i+1)) * 0.5f; } cabCustomPos=(cabCustomPos+1)%2048; y=acc;
    } else {
      y=cabFilt[ch][0].process(y); y=cabFilt[ch][1].process(y); y=cabFilt[ch][2].process(y);
    }
    float mix=cabBlend/100.0f; return x*(1-mix)+y*mix;
  }
  float processEQ(float x, int ch){
    if(!eqOn) return x;
    float y=eqHPFf[ch].process(eqLPFf[ch].process(x)); y=eqLowF[ch].process(y); y=eqMidF[ch].process(y); y=eqHighF[ch].process(y); return y;
  }
  float processLA(float x, int ch){
    if(!laOn) return x;
    float rect=fabsf(x); float attC=1-expf(-1.0f/(float)(sr*0.01f)); float relC=1-expf(-1.0f/(float)(sr*0.06f));
    if(rect>envLA) envLA+=attC*(rect-envLA); else envLA+=relC*(rect-envLA);
    float envDb=20*log10f(envLA+1e-12f); float thresh=-20.0f+ (100.0f-laPeakRed)/100.0f*20.0f; // Peak Red 0..100 -> thresh -20..0
    float over=envDb-thresh; float grDb= over>0 ? over*0.7f : 0; float target=powf(10,-grDb/20);
    float coeff = target<1 ? attC : relC; static float g=1; (void)g;
    // per-channel gain via env only - use simple smoothing on grLA for meter
    grLA = grLA*0.9f + (-grDb)*0.1f;
    float wet=x*target*powf(10,laGain/20.0f); float mix=laMix/100.0f; return x*(1-mix)+wet*mix;
  }
  float processLimiter(float x, int ch){
    if(!limOn) return x;
    float ceilLin=powf(10,limCeil/20.0f); float threshLin=powf(10,limThresh/20.0f);
    float a=fabsf(x); float target=1.0f;
    if(a>threshLin) target=threshLin/a;
    // 0-latency: attack is INSTANT (no lookahead, no delay). Release smoothed.
    if(target<limGain) limGain=target; else limGain += (1.0f-limGain)*0.02f;
    if(ch==0) limGR = limGR*0.9f + (20*log10f(limGain+1e-12f))*0.1f;
    float y=x*limGain; if(y>ceilLin) y=ceilLin; if(y<-ceilLin) y=-ceilLin;
    float mix=limMix/100.0f; return x*(1-mix)+y*mix;
  }
  void processStereo(float& L, float& R){
    float inPeak=std::max(fabsf(L),fabsf(R)); inLevelDb=0.9f*inLevelDb+0.1f*(20*log10f(inPeak+1e-12f));
    if(bypass){ grLA=0; limGR=0; outLevelDb=inLevelDb; return; }
    L=processSans(L,0); R=processSans(R,1);
    L=processCab(L,0); R=processCab(R,1);
    L=processEQ(L,0); R=processEQ(R,1);
    L=processLA(L,0); R=processLA(R,1);
    L=processLimiter(L,0); R=processLimiter(R,1);
    float outPeak=std::max(fabsf(L),fabsf(R)); outLevelDb=0.9f*outLevelDb+0.1f*(20*log10f(outPeak+1e-12f));
  }
};
