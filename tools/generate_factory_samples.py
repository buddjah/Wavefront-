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


if __name__ == "__main__":
    print("Génération des samples factory :")
    write_wav("01_Sine_Sweep.wav", sine_sweep())
    write_wav("02_Pad_Chord.wav", pad_chord())
    write_wav("03_Noise_Sweep.wav", noise_sweep())
    print("Terminé.")
