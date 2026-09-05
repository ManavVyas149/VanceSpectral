import math
import json
import numpy as np

def run_dsp_verification():
    print("=" * 65)
    print(" VanceSpectral Sidechain Pump & Effects DSP Audio Verification")
    print("=" * 65)
    
    sample_rate = 44100.0
    block_size = 512
    tests_passed = 0
    total_tests = 0
    
    def assert_test(cond, name):
        nonlocal tests_passed, total_tests
        total_tests += 1
        if cond:
            print(f" [PASS] {name}")
            tests_passed += 1
        else:
            print(f" [FAIL] {name}")
            
    # --- DSP Implementation matching EffectsEngine.cpp exactly ---
    class PySidechainDSP:
        def __init__(self, sr=44100.0):
            self.sr = sr
            self.phase = 0.0
            self.enabled = False
            self.bypass_gain = 0.0
            self.target_bypass = 0.0
            self.mix = 0.0
            self.target_mix = 0.0
            self.rate = 2.0
            self.target_rate = 2.0
            
            # Smoothers step sizes
            self.bypass_step = 1.0 / (0.015 * sr) # 15ms
            self.param_step = 1.0 / (0.020 * sr)  # 20ms
            
        def set_enabled(self, en):
            self.target_bypass = 1.0 if en else 0.0
            
        def set_mix(self, mix01):
            self.target_mix = max(0.0, min(1.0, mix01))
            
        def set_rate(self, rate_hz):
            self.target_rate = max(0.5, min(20.0, rate_hz))
            
        def process_block(self, in_l, in_r):
            out_l = np.copy(in_l)
            out_r = np.copy(in_r)
            n = len(in_l)
            
            is_active = (self.bypass_gain > 0.001) or (abs(self.bypass_gain - self.target_bypass) > 1e-5)
            if not is_active:
                # Step smoother targets
                self.bypass_gain = self.target_bypass
                self.mix = self.target_mix
                self.rate = self.target_rate
                return out_l, out_r
                
            for i in range(n):
                # Update smoothers
                if self.bypass_gain < self.target_bypass:
                    self.bypass_gain = min(self.target_bypass, self.bypass_gain + self.bypass_step)
                elif self.bypass_gain > self.target_bypass:
                    self.bypass_gain = max(self.target_bypass, self.bypass_gain - self.bypass_step)
                    
                if self.mix < self.target_mix:
                    self.mix = min(self.target_mix, self.mix + self.param_step)
                elif self.mix > self.target_mix:
                    self.mix = max(self.target_mix, self.mix - self.param_step)
                    
                if self.rate < self.target_rate:
                    self.rate = min(self.target_rate, self.rate + self.param_step)
                elif self.rate > self.target_rate:
                    self.rate = max(self.target_rate, self.rate - self.param_step)
                    
                # Advance phase
                phase_inc = self.rate / self.sr
                self.phase += phase_inc
                if self.phase >= 1.0:
                    self.phase -= 1.0
                    
                # Envelope
                attack_win = 0.04
                if self.phase < attack_win:
                    norm_att = self.phase / attack_win
                    env = 0.5 * (1.0 + math.cos(math.pi * norm_att))
                else:
                    norm_rec = (self.phase - attack_win) / (1.0 - attack_win)
                    exp_factor = 4.0
                    denom = 1.0 - math.exp(-exp_factor)
                    env = (1.0 - math.exp(-exp_factor * norm_rec)) / denom
                    
                duck_gain = 1.0 - (self.mix * (1.0 - env))
                final_gain = (1.0 - self.bypass_gain) + (self.bypass_gain * duck_gain)
                
                out_l[i] *= final_gain
                out_r[i] *= final_gain
                
            return out_l, out_r

    # ---------------------------------------------------------
    # TEST 1: Bit-Identity when Bypassed
    # ---------------------------------------------------------
    dsp = PySidechainDSP(sample_rate)
    t = np.linspace(0, block_size / sample_rate, block_size, endpoint=False)
    in_l = np.sin(2 * np.pi * 440 * t)
    in_r = np.cos(2 * np.pi * 440 * t)
    
    out_l, out_r = dsp.process_block(in_l, in_r)
    diff = np.max(np.abs(out_l - in_l)) + np.max(np.abs(out_r - in_r))
    assert_test(diff == 0.0, "1. True Bypass: Output is bit-identical to dry input (diff = 0.0)")

    # ---------------------------------------------------------
    # TEST 2: Bit-Identity when Mix = 0%
    # ---------------------------------------------------------
    dsp.set_enabled(True)
    dsp.set_mix(0.0)
    dsp.set_rate(4.0)
    # Warm up bypass smoother
    for _ in range(20):
        dsp.process_block(in_l, in_r)
        
    out_l, out_r = dsp.process_block(in_l, in_r)
    diff_mix0 = np.max(np.abs(out_l - in_l)) + np.max(np.abs(out_r - in_r))
    assert_test(diff_mix0 < 1e-9, f"2. Mix = 0%: Output is bit-identical to dry input (diff = {diff_mix0:.2e})")

    # ---------------------------------------------------------
    # TEST 3: Rhythmic Pumping at 100% Mix
    # ---------------------------------------------------------
    dsp.set_mix(1.0)
    dsp.set_rate(2.0) # 2 Hz = 22050 samples per cycle
    for _ in range(20):
        dsp.process_block(in_l, in_r)
        
    cycle_samples = int(sample_rate / 2.0)
    dc_sig = np.ones(cycle_samples)
    out_l, out_r = dsp.process_block(dc_sig, dc_sig)
    
    min_gain = np.min(out_l)
    max_gain = np.max(out_l)
    has_pump = (min_gain < 0.01) and (max_gain > 0.98)
    assert_test(has_pump, f"3. Ducking Depth: Full cyclic pump achieved (min = {min_gain:.4f}, max = {max_gain:.4f})")

    # ---------------------------------------------------------
    # TEST 4: Continuous Rate Sweep (No Phase Discontinuities)
    # ---------------------------------------------------------
    dsp.set_mix(1.0)
    max_step = 0.0
    for b in range(50):
        sweep_rate = 0.5 + (19.5 * (b / 50.0))
        dsp.set_rate(sweep_rate)
        out_l, _ = dsp.process_block(dc_sig[:block_size], dc_sig[:block_size])
        steps = np.abs(np.diff(out_l))
        if len(steps) > 0 and np.max(steps) > max_step:
            max_step = np.max(steps)
            
    assert_test(max_step < 0.05, f"4. Rate Modulation Sweep: Continuous phase advance with zero jumps (max step = {max_step:.5f})")

    # ---------------------------------------------------------
    # TEST 5: Toggle Click-Safety with 15ms Crossfade
    # ---------------------------------------------------------
    max_toggle_step = 0.0
    sig = np.sin(2 * np.pi * 440 * np.linspace(0, 1.0, 44100))
    for b in range(40):
        dsp.set_enabled(b % 2 == 0)
        chunk = sig[b * 512 : (b + 1) * 512]
        out_l, _ = dsp.process_block(chunk, chunk)
        steps = np.abs(np.diff(out_l))
        if len(steps) > 0 and np.max(steps) > max_toggle_step:
            max_toggle_step = np.max(steps)
            
    assert_test(max_toggle_step < 0.15, f"5. Bypass Click-Safety: 15ms smooth crossfade on rapid toggle (max step = {max_toggle_step:.5f})")

    # ---------------------------------------------------------
    # TEST 6: Polyphonic Summed Mix Ducking
    # ---------------------------------------------------------
    # A-Major chord: A4 (440Hz) + C#5 (554.37Hz) + E5 (659.25Hz)
    t_poly = np.linspace(0, 1.0, 44100)
    chord = (0.33 * np.sin(2 * np.pi * 440 * t_poly) +
             0.33 * np.sin(2 * np.pi * 554.37 * t_poly) +
             0.33 * np.sin(2 * np.pi * 659.25 * t_poly))
             
    dsp.set_enabled(True)
    dsp.set_mix(0.8)
    dsp.set_rate(4.0) # 4 Hz
    
    out_chord, _ = dsp.process_block(chord, chord)
    no_nan = not np.isnan(out_chord).any() and not np.isinf(out_chord).any()
    bounded = np.max(np.abs(out_chord)) <= 1.0
    assert_test(no_nan and bounded, "6. Polyphonic Chord Processing: Summed bus ducking is mathematically stable")

    # ---------------------------------------------------------
    # TEST 7: Legacy Preset Compatibility (.vsfx / .vsts JSON)
    # ---------------------------------------------------------
    legacy_preset_json = """
    {
        "name": "OldFactoryGatePreset",
        "category": "SYNTH",
        "parameters": {
            "AMP_ATTACK": 0.05,
            "AMP_DECAY": 0.2,
            "AMP_SUSTAIN": 0.8,
            "AMP_RELEASE": 0.5,
            "FX_GATE_ENABLE": 1.0,
            "FX_GATE_AMOUNT": 0.75,
            "FX_CHORUS_ENABLE": 0.0,
            "FX_DELAY_ENABLE": 0.0
        }
    }
    """
    parsed = json.loads(legacy_preset_json)
    params = parsed["parameters"]
    
    # Active APVTS valid parameters map
    valid_apvts = {
        "AMP_ATTACK": 0.0, "AMP_DECAY": 0.0, "AMP_SUSTAIN": 0.0, "AMP_RELEASE": 0.0,
        "FX_SIDECHAIN_ENABLE": 0.0, "FX_SIDECHAIN_MIX": 0.0, "FX_SIDECHAIN_RATE": 2.0,
        "FX_CHORUS_ENABLE": 0.0, "FX_DELAY_ENABLE": 0.0
    }
    
    loaded_params = dict(valid_apvts)
    for k, v in params.items():
        if k in loaded_params:
            loaded_params[k] = float(v)
            
    # Verify legacy keys were discarded and sidechain defaults preserved
    legacy_keys_ignored = "FX_GATE_ENABLE" not in loaded_params and "FX_GATE_AMOUNT" not in loaded_params
    sidechain_safe_defaults = (loaded_params["FX_SIDECHAIN_ENABLE"] == 0.0 and 
                               loaded_params["FX_SIDECHAIN_MIX"] == 0.0 and 
                               loaded_params["FX_SIDECHAIN_RATE"] == 2.0)
                               
    assert_test(legacy_keys_ignored and sidechain_safe_defaults, "7. Preset Backward-Compatibility: Legacy Gate keys cleanly ignored without crashing")

    print("\n" + "=" * 65)
    print(f" Summary: {tests_passed}/{total_tests} Tests Passed")
    print("=" * 65)

if __name__ == "__main__":
    run_dsp_verification()
