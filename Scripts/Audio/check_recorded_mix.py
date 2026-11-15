"""Check real exported Unreal master recordings; does not claim human audibility."""
from array import array
import json
import math
from pathlib import Path
import wave
folder=Path(__file__).resolve().parents[2]/'Saved'/'AgentReports'
records={}
for name in ['audible','muted','paused']:
    with wave.open(str(folder/('audio_'+name+'.wav')),'rb') as f:
        if f.getsampwidth()!=2:raise RuntimeError('Expected 16-bit PCM export.')
        data=array('h',f.readframes(f.getnframes()))
        records[name]={'seconds':f.getnframes()/f.getframerate(),'channels':f.getnchannels(),
            'peak':max(abs(x) for x in data)/32768,'rms':math.sqrt(sum(x*x for x in data)/len(data))/32768,
            'clipped_samples':sum(abs(x)>=32760 for x in data)}
checks={'actual_signal_present':records['audible']['rms']>.0001,'no_master_clipping':records['audible']['clipped_samples']==0,
        'mute_silences_mix':records['muted']['rms']<max(.0001,records['audible']['rms']*.05),
        'pause_silences_mix':records['paused']['rms']<max(.0001,records['audible']['rms']*.05)}
report={'passed':all(checks.values()),'checks':checks,'recordings':records,
    'limits':'Rendered game master bus only; not a speaker, headphone or comfort measurement.'}
(folder/'audio_signal_test.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
print(json.dumps(report,indent=2))
if not report['passed']:raise SystemExit(1)
