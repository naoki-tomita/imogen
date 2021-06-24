
/*******************************************************************************
 BEGIN_JUCE_MODULE_DECLARATION
 ID:                 ImogenEngine
 vendor:             Ben Vining
 version:            0.0.1
 name:               ImogenEngine
 description:        base class that wraps the Harmonizer class into a self-sufficient audio engine
 dependencies:       bv_synth bv_psola ImogenCommon
 END_JUCE_MODULE_DECLARATION
 *******************************************************************************/


#pragma once

#include <bv_synth/bv_synth.h>
#include <bv_psola/bv_psola.h>
#include <ImogenCommon/ImogenCommon.h>

#include "Harmonizer/HarmonizerVoice/HarmonizerVoice.h"
#include "Harmonizer/Harmonizer.h"

namespace Imogen
{
template < typename SampleType >
class Engine : public dsp::LatencyEngine< SampleType >
{
public:
    using AudioBuffer = juce::AudioBuffer< SampleType >;
    using MidiBuffer  = juce::MidiBuffer;

    Engine (State& stateToUse);

private:
    void renderChunk (const AudioBuffer& input, AudioBuffer& output, MidiBuffer& midiMessages, bool isBypassed) final;

    void onPrepare (int blocksize, double samplerate) final;
    void onRelease() final;

    void updateHarmonizerParameters();
    void updateStereoWidth (int width);
    void updateCompressorAmount (int amount);
    void updateReverbDecay (int decay);
    void updateStereoReductionMode (int mode);

    void processNoiseGate (AudioBuffer& audio);
    void processDeEsser (AudioBuffer& audio);
    void processCompressor (AudioBuffer& audio);
    void processDelay (AudioBuffer& audio);
    void processReverb (AudioBuffer& audio);
    void processEQ (AudioBuffer& audio);
    void processLimiter (AudioBuffer& audio);

    void updateInternals();

    State&      state;
    Parameters& parameters {state.parameters};
    Meters&     meters {state.meters};
    Internals&  internals {state.internals};
    
    dsp::FX::Filter<SampleType> initialLoCut {dsp::FX::FilterType::HighPass, 65.f};

    Harmonizer< SampleType > harmonizer;

    AudioBuffer monoBuffer, wetBuffer, dryBuffer;

    dsp::FX::MonoStereoConverter< SampleType > stereoReducer;
    dsp::FX::SmoothedGain< SampleType, 1 >     inputGain;
    dsp::FX::SmoothedGain< SampleType, 2 >     outputGain;
    dsp::FX::NoiseGate< SampleType >           gate;
    dsp::FX::DeEsser< SampleType >             deEsser;
    dsp::FX::Compressor< SampleType >          compressor;
    dsp::FX::Delay< SampleType >               delay;
    dsp::FX::Reverb                            reverb;
    dsp::FX::Limiter< SampleType >             limiter;
    dsp::FX::EQ< SampleType >                  EQ;

    dsp::FX::MonoToStereoPanner< SampleType > dryPanner;
    dsp::FX::DryWetMixer< SampleType >        dryWetMixer;
};

}  // namespace Imogen
