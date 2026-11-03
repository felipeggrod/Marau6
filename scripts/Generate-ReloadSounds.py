"""Synthesise the four rifle reload sounds (gritty, military, close to the ear).

Writes 44.1 kHz mono 16-bit WAVs into OUT_DIR:
  S_Reload_Cloth   sleeve and gear rustle as the arms move, soft leather creak
  S_Reload_MagOut  release button click, then the mag sliding out with a scrape
  S_Reload_MagIn   new mag sliding in, ending in a firm metallic seat clack
  S_Reload_Charge  ratchety charging handle pull, released forward with a snap
Each is soft-clipped by its own drive and normalised to -1 dBFS peak, which
lands them around -20 LUFS, near the footsteps rather than the gunfire.
Seeded, so the output is reproducible.
Usage: python -I scripts/Generate-ReloadSounds.py OUT_DIR
"""
import os
import sys
import wave

import numpy as np
from scipy.signal import butter, sosfilt

RATE = 44100
rng = np.random.default_rng(6)


def band(signal, low, high, order=4):
    if low is None:
        sos = butter(order, high, btype="lowpass", fs=RATE, output="sos")
    elif high is None:
        sos = butter(order, low, btype="highpass", fs=RATE, output="sos")
    else:
        sos = butter(order, [low, high], btype="bandpass", fs=RATE, output="sos")
    return sosfilt(sos, signal)


def norm(signal):
    return signal / np.abs(signal).max()


def timeline(seconds):
    return np.arange(int(RATE * seconds)) / RATE


def noise(size):
    return rng.standard_normal(size)


def hit(t, at, decay):
    """Exponential decay starting at `at`, silent before it."""
    local = t - at
    return np.where(local >= 0, np.exp(-np.maximum(local, 0) / decay), 0.0)


def swell(t, start, peak, end):
    """Smooth rise from start to peak, smooth fall to end, zero outside."""
    rise = np.clip((t - start) / (peak - start), 0, 1)
    fall = np.clip((end - t) / (end - peak), 0, 1)
    return np.sin(0.5 * np.pi * rise) ** 2 * np.sin(0.5 * np.pi * fall) ** 2


def ring(t, at, partials):
    """Damped metallic partials (freq, decay, gain) struck at `at`."""
    local = np.maximum(t - at, 0)
    out = sum(g * np.sin(2 * np.pi * f * local + rng.uniform(0, 2 * np.pi)) * np.exp(-local / d)
              for f, d, g in partials)
    return np.where(t >= at, out, 0.0)


def click(t, at, decay, low=2000, gain=1.0):
    return gain * norm(band(noise(t.size), low, None)) * hit(t, at, decay)


def grit(t, rate, depth):
    """Random amplitude jitter, like metal catching on grit or fabric folds."""
    raw = band(noise(t.size), None, rate, order=2)
    return 1 + depth * norm(raw)


def cloth():
    t = timeline(0.35)
    # Two overlapping sleeve swishes: a broad one and a shorter brighter fold.
    swish = norm(band(noise(t.size), 500, 3500)) * swell(t, 0.0, 0.11, 0.33) * grit(t, 60, 0.5)
    fold = norm(band(noise(t.size), 1800, 7000)) * swell(t, 0.07, 0.15, 0.25) * grit(t, 120, 0.6)
    # Leather creak: a stick-slip pulse train (~70 Hz, wandering) through a body band.
    creak = np.zeros(t.size)
    pos = 0.12
    while pos < 0.27:
        idx = int(pos * RATE)
        creak[idx] = rng.uniform(0.6, 1.0)
        pos += 1 / rng.uniform(55, 85)
    creak = band(creak, 300, 1400, order=2) * swell(t, 0.11, 0.18, 0.28)
    # Strap buckle tap as the gear settles.
    buckle = ring(t, 0.205, ((2400, 0.012, 1.0), (3900, 0.008, 0.6)))
    # Sparse crinkles of webbing and pouch fabric riding on the swish.
    crinkle = sum(click(t, at, 0.0015, 2500, rng.uniform(0.4, 0.9))
                  for at in np.sort(rng.uniform(0.04, 0.26, 7)))
    return [(swish, 0.15), (fold, 0.08), (norm(creak), 0.18), (buckle, 0.3), (crinkle, 1.0)], 0.4


def mag_out():
    t = timeline(0.4)
    # Release button: a tight plastic-metal click with a small spring ring.
    button = click(t, 0.015, 0.0025, 2500) + ring(t, 0.015, ((3300, 0.015, 0.5), (5200, 0.008, 0.3)))
    # The catch lets go: a dull knock as the mag drops a few millimetres.
    knock = ring(t, 0.05, ((210, 0.025, 0.6), (780, 0.018, 0.4), (1900, 0.01, 0.3)))
    knock += click(t, 0.05, 0.004, 600, 0.5)
    # Slide out: gritty metal-on-metal scrape, brightening as it speeds up.
    scrape = norm(band(noise(t.size), 1500, 6500)) * grit(t, 90, 0.7)
    scrape += 0.5 * norm(band(noise(t.size), 4000, 10000)) * grit(t, 200, 0.8)
    scrape *= swell(t, 0.07, 0.22, 0.33)
    # The mag clears the well with a last small tick.
    tick = click(t, 0.31, 0.003, 3000, 0.45) + ring(t, 0.31, ((2700, 0.02, 0.3),))
    return [(button, 0.9), (knock, 0.7), (scrape, 0.35), (tick, 0.8)], 0.8


def mag_in():
    t = timeline(0.4)
    # Insert: scrape rising in level and pitch as the mag goes up the well.
    scrape = norm(band(noise(t.size), 1200, 5500)) * grit(t, 80, 0.7)
    scrape *= swell(t, 0.0, 0.2, 0.235)
    rattle = ring(t, 0.12, ((1700, 0.01, 0.4),)) + ring(t, 0.17, ((2100, 0.008, 0.3),))
    # Seat: firm clack, a broadband slap, a body thunk and steel partials.
    seat = 0.24
    slap = click(t, seat, 0.004, 900)
    thunk = ring(t, seat, ((160, 0.035, 1.0), (330, 0.03, 0.6)))
    steel = ring(t, seat, ((1250, 0.05, 0.7), (2730, 0.035, 0.5), (4310, 0.02, 0.4), (6900, 0.012, 0.25)))
    # The catch springs home a few ms after the seat.
    catch = click(t, seat + 0.012, 0.002, 3500, 0.5) + ring(t, seat + 0.012, ((3800, 0.012, 0.3),))
    return [(scrape, 0.35), (rattle, 0.6), (slap, 0.9), (thunk, 0.8), (steel, 0.7), (catch, 0.7)], 1.2


def charge():
    t = timeline(0.45)
    # Pull back: a light slide under a ratchet of detent clicks that spread out
    # as the handle slows near the stop.
    slide = norm(band(noise(t.size), 1000, 5000)) * grit(t, 70, 0.6) * swell(t, 0.02, 0.1, 0.19)
    ratchet = np.zeros(t.size)
    pos, step = 0.03, 0.009
    while pos < 0.17:
        ratchet += click(t, pos, 0.0012, 3000, rng.uniform(0.5, 0.9))
        ratchet += ring(t, pos, ((rng.uniform(2800, 3400), 0.004, 0.35),))
        pos += step
        step *= 1.12
    # Hits the rear stop.
    stop = ring(t, 0.18, ((520, 0.02, 0.6), (1600, 0.015, 0.4))) + click(t, 0.18, 0.003, 1200, 0.6)
    # Release: the bolt carrier slams home, sharp snap with a ringing body.
    snap_at = 0.255
    snap = click(t, snap_at, 0.0025, 1500)
    body = ring(t, snap_at, ((120, 0.04, 0.9), (870, 0.045, 0.6), (2120, 0.03, 0.55),
                             (3560, 0.022, 0.4), (6100, 0.012, 0.25)))
    rush = norm(band(noise(t.size), 1500, 6000)) * swell(t, 0.235, 0.252, 0.258)
    return [(slide, 0.3), (ratchet, 0.55), (stop, 0.7), (snap, 1.0), (body, 0.9), (rush, 0.25)], 1.2


def render(layers, drive):
    mix = sum(gain * signal for signal, gain in layers)
    mix = band(mix, 40, None, order=2)  # no rumble, no DC
    mix = np.tanh(drive * mix / np.abs(mix).max())
    fade_in, fade_out = int(0.001 * RATE), int(0.02 * RATE)
    mix[:fade_in] *= np.linspace(0, 1, fade_in)
    mix[-fade_out:] *= np.linspace(1, 0, fade_out) ** 2
    mix -= mix.mean()
    return mix * 10 ** (-1 / 20) / np.abs(mix).max()


def write(path, signal):
    with wave.open(path, "wb") as out:
        out.setnchannels(1)
        out.setsampwidth(2)
        out.setframerate(RATE)
        out.writeframes(np.round(signal * 32767).astype("<i2").tobytes())


out_dir = sys.argv[1]
os.makedirs(out_dir, exist_ok=True)
for seed, (name, build) in enumerate((("S_Reload_Cloth", cloth), ("S_Reload_MagOut", mag_out),
                                      ("S_Reload_MagIn", mag_in), ("S_Reload_Charge", charge))):
    rng = np.random.default_rng(6 + seed)  # one stream per sound, so tuning one leaves the rest
    layers, drive = build()
    write(os.path.join(out_dir, name + ".wav"), render(layers, drive))
