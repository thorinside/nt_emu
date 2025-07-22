# VCV Rack Plugin Audio Processing Guide

This document provides essential information for implementing audio/CV processing in VCV Rack plugins, specifically for the Disting NT emulator.

## Core Concepts

### 1. Sample Rate and Block Processing

VCV Rack processes audio sample-by-sample, but DSP algorithms often work more efficiently with blocks:
- Default sample rate: 48kHz (can vary based on audio interface)
- Block sizes: Typically powers of 2 (64, 128, 256 samples)
- Disting NT uses 4-sample blocks for SIMD optimization

### 2. Voltage Standards

**Audio Signals**
- Range: ±5V (10Vpp)
- Headroom: Can handle ±12V (Eurorack power rails)
- Unity gain: 5V = 0dBFS

**CV Signals**
- Unipolar: 0V to 10V (modulation, envelopes)
- Bipolar: ±5V (LFOs, audio-rate modulation)
- V/Oct: 1V per octave pitch standard

**Gates/Triggers**
- High: 10V (some modules accept >2V as high)
- Low: 0V
- Trigger duration: ~1ms pulse

### 3. Real-time Constraints

- Audio callback must complete within ~1ms at 48kHz
- Avoid memory allocation in process()
- No blocking operations (file I/O, mutex locks)
- Use atomic operations for thread safety

## Implementation Patterns

### Input Processing
```cpp
void process(const ProcessArgs& args) {
    // Check connection before reading
    if (inputs[INPUT].isConnected()) {
        float v = inputs[INPUT].getVoltage();
        // Scale from ±5V to ±1.0 for DSP
        float normalized = v / 5.0f;
        // Soft clip for safety
        normalized = clamp(normalized, -2.0f, 2.0f);
    }
}
```

### Output Generation
```cpp
// Single channel
outputs[OUTPUT].setVoltage(value * 5.0f);

// Polyphonic (up to 16 channels)
outputs[OUTPUT].setChannels(numChannels);
for (int c = 0; c < numChannels; c++) {
    outputs[OUTPUT].setVoltage(values[c] * 5.0f, c);
}
```

### Parameter Smoothing
```cpp
// One-pole filter for parameter smoothing
struct SmoothValue {
    float target = 0.f;
    float current = 0.f;
    
    void process(float sampleTime) {
        float lambda = 1.f - std::exp(-2.f * M_PI * 20.f * sampleTime);
        current += lambda * (target - current);
    }
};
```

## DSP Best Practices

### 1. Anti-aliasing
- Use oversampling for nonlinear processing
- Apply lowpass filtering before downsampling
- Consider using polyphase filters

### 2. Denormal Prevention
```cpp
// Flush denormals to zero
if (!std::isnormal(x)) x = 0.f;

// Or add DC offset
x += 1e-10f;
// ... process ...
x -= 1e-10f;
```

### 3. NaN/Inf Handling
```cpp
// Sanitize output
float out = processAudio(in);
if (!std::isfinite(out)) out = 0.f;
outputs[OUTPUT].setVoltage(out);
```

### 4. Efficient Processing
- Process multiple samples in tight loops
- Use SIMD instructions when possible
- Minimize branching in audio loops
- Cache frequently used values

## Debugging Tips

1. **Audio Debugging**
   - Use scope modules to visualize signals
   - Add debug outputs for internal signals
   - Log sparingly (not in audio thread)

2. **Performance Profiling**
   - Use `args.sampleTime` to measure DSP load
   - VCV's CPU meter shows module performance
   - Profile with native tools (Instruments, perf)

3. **Common Issues**
   - Clicks/pops: Check for discontinuities
   - Aliasing: Need better filtering
   - Dropouts: Reduce CPU usage
   - DC offset: Use highpass filtering

## Reference Implementation

See the VCV Fundamental modules for best practices:
https://github.com/VCVRack/Fundamental

Key modules to study:
- VCO: Oscillator with anti-aliasing
- VCF: Filter implementation
- LFO: Low-frequency modulation
- ADSR: Envelope generation