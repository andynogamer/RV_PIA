"""Original synthesized prototype audio. Python stdlib only; no downloaded recordings."""
from array import array
import json
import math
from pathlib import Path
import random
import wave

RATE=48000
ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'SourceAudio'
OUT.mkdir(exist_ok=True)
rng=random.Random(4312)
report=[]

def save(name,samples,loop=False):
    samples=list(samples)
    if loop:
        overlap=int(.04*RATE)
        for i in range(overlap):
            a=i/overlap
            samples[i]=samples[i]*a+samples[-overlap+i]*(1-a)
        samples=samples[:-overlap]
    peak=max(abs(x) for x in samples) or 1
    gain=min(1,.75/peak)
    pcm=array('h',(round(max(-.95,min(.95,x*gain))*32767) for x in samples))
    with wave.open(str(OUT/(name+'.wav')),'wb') as f:
        f.setnchannels(1);f.setsampwidth(2);f.setframerate(RATE);f.writeframes(pcm.tobytes())
    report.append({'name':name,'seconds':len(samples)/RATE,'loop':loop,'peak':peak*gain})

def engine():
    low=0
    for i in range(RATE*3):
        t=i/RATE;noise=rng.uniform(-1,1);low=.96*low+.04*noise
        pulses=.57*math.sin(math.tau*48*t)+.22*math.sin(math.tau*96*t+.15)+.12*math.sin(math.tau*144*t)
        rough=.04*math.sin(math.tau*343*t)*(1+.5*math.sin(math.tau*16*t))
        yield .52*pulses+.38*low+rough

def road():
    low=0;slow=0
    for i in range(RATE*4):
        t=i/RATE;n=rng.uniform(-1,1);low=.78*low+.22*n;slow=.985*slow+.015*n
        yield .5*(low-slow)+.18*slow+.025*math.sin(math.tau*31*t)

def city():
    low=0
    for i in range(RATE*16):
        t=i/RATE;low=.99*low+.01*rng.uniform(-1,1)
        passing=(.5+.5*math.cos(math.tau*t/8))**6
        yield .7*low+.035*passing*(math.sin(math.tau*(65*t+4*math.sin(math.tau*t/16)))+.25*math.sin(math.tau*130*t))

def tone(duration,frequencies):
    for i in range(int(RATE*duration)):
        t=i/RATE;envelope=min(1,t/.025,(duration-t)/.09)
        yield max(0,envelope)*sum(a*math.sin(math.tau*f*t) for f,a in frequencies)

def impact():
    low=0
    for i in range(int(RATE*.42)):
        t=i/RATE;low=.8*low+.2*rng.uniform(-1,1)
        yield min(1,t/.004)*math.exp(-t*13)*(.65*low+.3*math.sin(math.tau*74*t))

save('S_EngineLoop',engine(),True)
save('S_RoadLoop',road(),True)
save('S_CityLoop',city(),True)
save('S_Horn',tone(.42,[(349,.25),(440,.22),(698,.05),(880,.035)]))
save('S_Impact',impact())
save('S_Reward',list(tone(.13,[(523.25,.18),(1046.5,.025)]))+[0.0]*int(RATE*.03)+list(tone(.23,[(659.25,.16),(1318.5,.02)])))
(OUT/'audio_manifest.json').write_text(json.dumps({'authoring':'Original mathematical synthesis, prototype timbres.','sample_rate':RATE,'sounds':report},indent=2),encoding='utf-8')
print(json.dumps(report,indent=2))
