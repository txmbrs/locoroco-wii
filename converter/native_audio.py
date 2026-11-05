"""Render original PSP music from the imported user's SGXD bank."""
from pathlib import Path
import collections,hashlib,json,mmap,struct,subprocess,wave
import numpy as np
RATE=32000

def read_wave(path):
    with wave.open(str(path)) as w:
        assert w.getsampwidth() == 2
        return np.frombuffer(w.readframes(w.getnframes()), dtype='<i2').astype(np.float32).reshape(-1, w.getnchannels()) / 32768, w.getframerate()


def midi_events(bank, offset):
    _, _, fmt, division, _, _, size = struct.unpack_from('<IIHHHHI', bank, offset)
    assert fmt == 1 and 0 < division < 32768
    data = bank[offset + 20:offset + 20 + size]
    pos, ticks, seconds, tempo, status = 0, 0, 0., 500000, 0
    result = []

    def vlq():
        nonlocal pos
        value = 0
        for _ in range(5):
            byte = data[pos]; pos += 1
            value = (value << 7) | (byte & 127)
            if byte < 128:
                return value
        raise ValueError('Invalid MIDI delta')

    while pos < len(data):
        delta = vlq(); ticks += delta; seconds += delta * tempo / division / 1e6
        if data[pos] >= 128:
            status = data[pos]; pos += 1
        if status == 255:
            meta = data[pos]; pos += 1
            size = vlq(); values = list(data[pos:pos + size]); pos += size
            result.append((seconds, 'meta', meta, values))
            if meta == 81: tempo = int.from_bytes(bytes(values), 'big')
            if meta == 47: break
        elif status in (240, 247):
            size = vlq(); pos += size
        else:
            size = 1 if status >> 4 in (12, 13) else 2
            values = list(data[pos:pos + size]); pos += size
            assert all(x < 128 for x in values)
            result.append((seconds, status >> 4, status & 15, values))
    return result



def prepare(import_root,app,work,decoder):
    import_root=Path(import_root);app=Path(app);WORK=Path(work);OUT=app/'assets/music'
    WORK.mkdir(parents=True,exist_ok=True);OUT.mkdir(parents=True,exist_ok=True)
    bank_name='yel_flower'
    bank_path=find_bank(import_root,bank_name,WORK)
    bank=bank_path.read_bytes()
    path = WORK / f'{bank_name}.sgd'; path.write_bytes(bank)
    samples_dir = WORK / bank_name
    samples_dir.mkdir(exist_ok=True)
    chunks = {}; pos = 16
    while pos < struct.unpack_from('<I', bank, 8)[0]:
        size = struct.unpack_from('<I', bank, pos + 4)[0]
        chunks[bank[pos:pos + 4]] = pos
        pos += 8 + size

    seq_group = struct.unpack_from('<I', bank, chunks[b'SEQD'] + 16)[0]
    seq_offset = struct.unpack_from('<I', bank, seq_group + 8)[0]
    events = midi_events(bank, seq_offset)
    loop_start = next(t for t, typ, ch, v in events if typ == 11 and v == [99, 20])
    loop_end = next(t for t, typ, ch, v in events if typ == 11 and v == [99, 30])
    wave_offset = chunks[b'WAVE']
    count = struct.unpack_from('<I', bank, wave_offset + 12)[0]
    decoder = Path(decoder)
    subprocess.run([str(decoder), '-i', '-s', '1', '-S', str(count), '-o', str(samples_dir / 'sample-?s.wav'), str(path)],
                   check=True, stdout=subprocess.DEVNULL)
    backing, input_rate = read_wave(samples_dir / 'sample-1.wav')
    frames = ((round(len(backing) * RATE / input_rate) + 15) // 16) * 16
    target = np.arange(frames) * input_rate / RATE
    backing = np.column_stack([np.interp(target, np.arange(len(backing)), backing[:, c]) for c in range(2)])

    regions = []
    program_count = struct.unpack_from('<I', bank, chunks[b'RGND'] + 12)[0]
    for program in range(program_count):
        count, offset = struct.unpack_from('<II', bank, chunks[b'RGND'] + 16 + program * 8)
        rows = []
        for i in range(count):
            p = offset + i * 56
            rows.append(dict(low=bank[p + 24], high=bank[p + 25], root=bank[p + 28],
                             fine=struct.unpack_from('b', bank, p + 29)[0],
                             sample=struct.unpack_from('<I', bank, p + 52)[0] + 1,
                             pan=struct.unpack_from('<h', bank, p + 34)[0] / 1024,
                             gain=struct.unpack_from('<H', bank, p + 32)[0] / 4096))
        regions.append(rows)

    curves = collections.defaultdict(list)
    programs = [0] * 16; notes = []; active = {}
    for time, typ, channel, values in events:
        if typ == 12: programs[channel] = values[0]
        if typ == 11 and values[0] in (7, 11): curves[channel, values[0]].append((time, values[1] / 127))
        if typ in (8,9):
            key, velocity = values
            if typ == 9 and velocity:
                active[channel, key] = (time, velocity, programs[channel])
            elif (channel, key) in active:
                start, velocity, program = active.pop((channel, key))
                notes.append((start, time, channel, key, velocity, program))
    assert not active
    stems = [np.zeros((frames,2), dtype=np.float32) for _ in range(16)]
    sample_cache = {}
    ignored = collections.Counter()
    for start, end, channel, key, velocity, program in notes:
        matching = [r for r in regions[program] if r['low'] <= key <= r['high']]
        if not matching: ignored[f'program {program}, key {key}'] += 1
        # Paired matching regions are the original left/right sample layers.
        for region in matching:
            sample_id = region['sample']
            if sample_id not in sample_cache: sample_cache[sample_id] = read_wave(samples_dir / f'sample-{sample_id}.wav')
            sample, sample_rate = sample_cache[sample_id]
            assert sample.shape[1] == 1
            ratio = 2 ** ((key - region['root'] + region['fine'] / 100) / 12)
            first = round(start * RATE); last = min(frames, round(end * RATE))
            positions = np.arange(last - first) * sample_rate / RATE * ratio
            voice = np.interp(positions, np.arange(len(sample)), sample[:, 0], right=0)
            times = np.arange(first, last) / RATE
            for cc in (7, 11):
                points = [(0., 1.)] + curves[channel, cc]
                xp, fp = map(np.array, zip(*points))
                voice *= fp[np.searchsorted(xp,times,side='right')-1]
            voice *= region['gain'] * velocity / 127
            ramp = min(96, len(voice) // 2)
            if ramp:
                voice[:ramp] *= np.linspace(0, 1, ramp)
                voice[-ramp:] *= np.linspace(1, 0, ramp)
            pan = np.clip(region['pan'],-1,1)
            stems[channel][first:last] += voice[:,None] * np.array([min(1,1-pan),min(1,1+pan)])

    # Flower's channels 10..13 are instruments/percussion, not extra singers.
    # They belong in the accompaniment at every population size.
    for channel in range(4,16):
        backing += stems[channel]
    peak = np.max(np.abs(backing + sum(stems[:4])))
    gain = min(0.82, 0.95 / max(peak, 0.01))
    arrays = [('backing', backing)] + [(f'voice{i}', stem) for i, stem in enumerate(stems[:4])]
    for name, data in arrays:
        (OUT / f'{name}.pcm').write_bytes(np.rint(np.clip(data * gain, -1, 1) * 32767).astype('<i2').tobytes())
    # A common aligned sample boundary for stereo and mono ASND buffers.
    loop_frame = round(loop_start * RATE / 16) * 16
    manifest = dict(bank=bank_name, bank_sha256=hashlib.sha256(bank).hexdigest(), rate=RATE,
                    frames=frames, loop_start_frame=loop_frame, loop_start_seconds=loop_start,
                    loop_end_seconds=loop_end, notes=len(notes), gain=gain, channels=2,
                    unassigned_notes=dict(ignored))
    (OUT / 'manifest.json').write_text(json.dumps(manifest, indent=2))

    (app/'assets/music/info.bin').write_bytes(b'LRCMUS01'+struct.pack('>III',RATE,frames,loop_frame))
    return manifest

def find_bank(import_root,name,work):
    """Some SGXD banks are uncompressed islands between GPRS members."""
    source=Path(import_root)/'work/disc-data/DATA.BIN'
    if not source.exists():
        report=json.loads((Path(import_root)/'import-report.json').read_text(encoding='utf8'))
        source=Path(report.get('data_archive',''))
    if not source.is_file():raise ValueError('Missing original DATA.BIN for SGXD extraction')
    with source.open('rb') as f,mmap.mmap(f.fileno(),0,access=mmap.ACCESS_READ) as data:
        offset=0
        while True:
            offset=data.find(b'SGXD',offset)
            if offset<0:break
            if offset+16<=len(data):
                title,header,size=struct.unpack_from('<III',data,offset+4);size &= 0x7fffffff
                if 16<title<header<1000000 and offset+header+size<=len(data) and data[offset+title:offset+title+len(name)+1]==name.encode()+b'\0':
                    raw=bytes(data[offset:offset+header+size]);out=Path(work)/(name+'.sgd');out.parent.mkdir(parents=True,exist_ok=True);out.write_bytes(raw)
                    out.with_suffix('.source.json').write_text(json.dumps({'archive':str(source),'offset':offset,'size':len(raw),'sha256':hashlib.sha256(raw).hexdigest()},indent=2))
                    return out
            offset+=4
    raise ValueError('SGXD music bank not found: '+name)
