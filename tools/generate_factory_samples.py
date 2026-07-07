#!/usr/bin/env python3
"""Génère des samples factory procéduraux (libres de droits) pour Wavefront.

Écrit des WAV mono 48 kHz / 16 bits dans Resources/Samples/. Aucune dépendance
externe (module `wave` de la stdlib). Reproductible (pas d'aléatoire non germé).
"""
import math
import os
import struct
import wave

SR = 48000
DURATION = 3.0
OUT_DIR = os.path.join(os.path.dirname(__file__), "..", "Resources", "Samples")


def write_wav(name, samples):
    os.makedirs(OUT_DIR, exist_ok=True)
    path = os.path.join(OUT_DIR, name)
    # Normalisation à -1 dBFS.
    peak = max(1e-9, max(abs(s) for s in samples))
    scale = 0.89 / peak
    with wave.open(path, "w") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(SR)
        frames = bytearray()
        for s in samples:
            v = int(max(-1.0, min(1.0, s * scale)) * 32767.0)
            frames += struct.pack("<h", v)
        w.writeframes(bytes(frames))
    print(f"  {name}: {len(samples)} samples ({len(samples)/SR:.1f}s)")


def env(i, n, atk=0.02, rel=0.2):
    """Enveloppe attaque/release simple (0..1)."""
    a = int(atk * n)
    r = int(rel * n)
    if i < a:
        return i / a
    if i > n - r:
        return max(0.0, (n - i) / r)
    return 1.0


def sine_sweep():
    n = int(SR * DURATION)
    f0, f1 = 80.0, 4000.0
    out = []
    phase = 0.0
    for i in range(n):
        t = i / n
        f = f0 * (f1 / f0) ** t  # sweep exponentiel
        phase += 2 * math.pi * f / SR
        out.append(math.sin(phase) * env(i, n))
    return out


def pad_chord():
    n = int(SR * DURATION)
    # Do majeur (C4, E4, G4) avec léger désaccord pour l'épaisseur.
    freqs = [261.63, 329.63, 392.00]
    detunes = [1.0, 1.003, 0.997]
    out = []
    for i in range(n):
        s = 0.0
        for f in freqs:
            for d in detunes:
                s += math.sin(2 * math.pi * f * d * i / SR)
        # léger vibrato d'amplitude
        trem = 0.85 + 0.15 * math.sin(2 * math.pi * 0.7 * i / SR)
        out.append(s / (len(freqs) * len(detunes)) * trem * env(i, n, 0.15, 0.3))
    return out


def noise_sweep():
    n = int(SR * DURATION)
    # LCG déterministe pour un bruit reproductible.
    state = 0x2545F491
    out = []
    lp = 0.0
    for i in range(n):
        state = (1103515245 * state + 12345) & 0x7FFFFFFF
        white = (state / 0x3FFFFFFF) - 1.0
        # coupure du passe-bas qui monte puis descend
        t = i / n
        cutoff = 0.02 + 0.5 * math.sin(math.pi * t)
        lp += cutoff * (white - lp)
        out.append(lp * env(i, n, 0.05, 0.3))
    return out


def pluck():
    """Plucks harmoniques répétés (boucle-friendly)."""
    n = int(SR * DURATION)
    hits = 4
    hit_len = n // hits
    freq = 196.0  # G3
    out = [0.0] * n
    for h in range(hits):
        start = h * hit_len
        for i in range(hit_len):
            t = i / SR
            s = 0.0
            for k in range(1, 9):
                s += (1.0 / k) * math.sin(2 * math.pi * freq * k * (start + i) / SR)
            decay = math.exp(-4.0 * t)  # extinction rapide
            out[start + i] = s / 3.0 * decay
    return out


def bell():
    """Cloche : partiels inharmoniques amortis, répétés."""
    n = int(SR * DURATION)
    hits = 3
    hit_len = n // hits
    base = 440.0
    ratios = [1.0, 2.76, 5.40, 8.93, 13.34]
    decays = [3.0, 3.5, 4.5, 6.0, 8.0]
    out = [0.0] * n
    for h in range(hits):
        start = h * hit_len
        for i in range(hit_len):
            t = i / SR
            s = 0.0
            for r, d in zip(ratios, decays):
                s += math.exp(-d * t) * math.sin(2 * math.pi * base * r * (start + i) / SR)
            out[start + i] = s / len(ratios)
    return out


def sub_pulse():
    """Basse sinus 55 Hz gatée en croches (~120 bpm) — rythmique, boucle."""
    n = int(SR * DURATION)
    freq = 55.0
    rate = 4.0  # gates par seconde
    out = []
    for i in range(n):
        gate_phase = (i / SR * rate) % 1.0
        gate = math.exp(-6.0 * gate_phase)  # enveloppe percussive par croche
        out.append(math.sin(2 * math.pi * freq * i / SR) * gate)
    return out


def vowel_aah():
    """Voyelle /a/ synthétique : saw filtré par 3 formants (additif pondéré)."""
    n = int(SR * DURATION)
    f0 = 130.81  # C3
    formants = [(700.0, 80.0), (1220.0, 90.0), (2600.0, 120.0)]

    def formant_gain(f):
        g = 0.02
        for fc, bw in formants:
            g += math.exp(-((f - fc) ** 2) / (2 * bw * bw))
        return g

    # Pré-calcul des amplitudes d'harmoniques.
    harmonics = []
    k = 1
    while f0 * k < SR / 2:
        harmonics.append((f0 * k, (1.0 / k) * formant_gain(f0 * k)))
        k += 1

    out = []
    for i in range(n):
        s = 0.0
        for f, a in harmonics:
            s += a * math.sin(2 * math.pi * f * i / SR)
        vib = 1.0 + 0.01 * math.sin(2 * math.pi * 5.0 * i / SR)  # léger vibrato
        out.append(s * vib * env(i, n, 0.2, 0.3))
    return out


def metal_drone():
    """Drone métallique : partiels désaccordés soutenus, battements lents."""
    n = int(SR * DURATION)
    base = 110.0
    partials = [1.0, 1.5, 2.01, 2.67, 3.99, 5.13]
    detune = [1.0, 1.002, 0.998, 1.004, 0.997, 1.001]
    out = []
    for i in range(n):
        s = 0.0
        for p, d in zip(partials, detune):
            s += math.sin(2 * math.pi * base * p * d * i / SR)
        out.append(s / len(partials) * env(i, n, 0.3, 0.3))
    return out


if __name__ == "__main__":
    print("Génération des samples factory :")
    write_wav("01_Sine_Sweep.wav", sine_sweep())
    write_wav("02_Pad_Chord.wav", pad_chord())
    write_wav("03_Noise_Sweep.wav", noise_sweep())
    write_wav("04_Pluck.wav", pluck())
    write_wav("05_Bell.wav", bell())
    write_wav("06_Sub_Pulse.wav", sub_pulse())
    write_wav("07_Vowel_Aah.wav", vowel_aah())
    write_wav("08_Metal_Drone.wav", metal_drone())
    print("Terminé.")
