/*******************************************************************************
 BEGIN_JUCE_MODULE_DECLARATION
 ID:                 bv_Harmonizer
 vendor:             Ben Vining
 version:            0.0.1
 name:               Harmonizer
 description:        base class for a polyphonic real-time pitch shifting instrument
 dependencies:       bv_PitchDetector
 END_JUCE_MODULE_DECLARATION
 *******************************************************************************/


// class templates defined in this file: Harmonizer<SampleType>, HarmonizerVoice<SampleType>

#pragma once

#include "bv_PitchDetector/bv_PitchDetector.h"  // this file includes the bv_SharedCode header
#include "PanningManager/PanningManager.h"
#include "GrainExtractor/GrainExtractor.h"



namespace bav
{
    

template<typename SampleType>
class Harmonizer; // forward declaration...



/*
 HarmonizerVoice : represents a "voice" that the Harmonizer can use to generate one monophonic note. A voice plays a single note/sound at a time; the Harmonizer holds an array of voices so that it can play polyphonically.
*/
    
template<typename SampleType>
class HarmonizerVoice
{
    
    using AudioBuffer = juce::AudioBuffer<SampleType>;
    
public:
    
    // NB. I play a bit fast and loose with private vs public functions here, because really, you should never interface directly with any non-const methods of HarmonizerVoice from outside the Harmonizer class that owns it...
    
    HarmonizerVoice (Harmonizer<SampleType>* h);
    
    ~HarmonizerVoice();
    
    void renderNextBlock (const AudioBuffer& inputAudio, AudioBuffer& outputBuffer,
                          const int origPeriod, const juce::Array<int>& indicesOfGrainOnsets,
                          const AudioBuffer& windowToUse);
    
    void bypassedBlock (const int numSamples);
    
    void prepare (const int blocksize);
    
    void releaseResources();
    
    int getCurrentlyPlayingNote() const noexcept { return currentlyPlayingNote; }
    
    bool isVoiceActive() const noexcept { return (currentlyPlayingNote >= 0); }
    
    bool isPlayingButReleased()   const noexcept { return playingButReleased; } // returns true if a voice is sounding, but its key has been released
    
    // Returns true if this voice started playing its current note before the other voice did.
    bool wasStartedBefore (const HarmonizerVoice& other) const noexcept { return noteOnTime < other.noteOnTime; }
    
    // Returns true if the key that triggered this voice is still held down. Note that the voice may still be playing after the key was released (e.g because the sostenuto pedal is down).
    bool isKeyDown() const noexcept { return keyIsDown; }
    
    int getCurrentMidiPan() const noexcept { return panner.getLastMidiPan(); }
    
    // DANGER!!! FOR NON REALTIME USE ONLY!
    void increaseBufferSizes (const int newMaxBlocksize);
    
    float getLastRecievedVelocity() const noexcept { return lastRecievedVelocity; }
    
    void updateSampleRate (const double newSamplerate);
    
    bool isCurrentPedalVoice()   const noexcept { return isPedalPitchVoice; }
    bool isCurrentDescantVoice() const noexcept { return isDescantVoice; }
    
    
protected:
    
    //  These functions will be called by the Harmonizer object that owns this voice
    
    void startNote (const int midiPitch,  const float velocity,
                    const uint32 noteOnTimestamp,
                    const bool keyboardKeyIsDown = true,
                    const bool isPedal = false, const bool isDescant = false);
    
    void stopNote (const float velocity, const bool allowTailOff);
    
    void aftertouchChanged (const int newAftertouchValue);
    
    void setVelocityMultiplier (const float newMultiplier) { midiVelocityGain.setTargetValue (newMultiplier); }
    
    void setCurrentOutputFreq (const float newFreq) { currentOutputFreq = newFreq; }
    
    void setKeyDown (bool isNowDown);
    
    void setPan (int newPan);
    
    void setAdsrParameters (const ADSR::Parameters newParams) { adsr.setParameters(newParams); }
    void setQuickReleaseParameters (const ADSR::Parameters newParams) { quickRelease.setParameters(newParams); }
    void setQuickAttackParameters  (const ADSR::Parameters newParams) { quickAttack.setParameters(newParams); }
    
    void softPedalChanged (bool isDown);
    
    
private:
    friend class Harmonizer<SampleType>;
    
    void clearCurrentNote(); // this function resets the voice's internal state & marks it as avaiable to accept a new note
    
    void sola (const SampleType* input, const int totalNumInputSamples,
               const int origPeriod, const int newPeriod, const Array<int>& indicesOfGrainOnsets,
               const SampleType* window);
    
    void olaFrame (const SampleType* inputAudio, const int frameStartSample, const int frameEndSample, const int frameSize, 
                   const SampleType* window, const int newPeriod);
    
    void moveUpSamples (const int numSamplesUsed);
    
    ADSR adsr;         // the main/primary ADSR driven by MIDI input to shape the voice's amplitude envelope. May be turned off by the user.
    ADSR quickRelease; // used to quickly fade out signal when stopNote() is called with the allowTailOff argument set to false, instead of jumping signal to 0
    ADSR quickAttack;  // used for if normal ADSR user toggle is OFF, to prevent jumps/pops at starts of notes.
    
    Harmonizer<SampleType>* parent; // this is a pointer to the Harmonizer object that controls this HarmonizerVoice
    
    uint32 noteOnTime;
    
    bool isQuickFading, noteTurnedOff;
    
    bool keyIsDown;
    
    int currentAftertouch;
    
    bool sustainingFromSostenutoPedal;
    
    AudioBuffer synthesisBuffer; // mono buffer that this voice's synthesized samples are written to
    AudioBuffer copyingBuffer;
    AudioBuffer windowingBuffer; // used to apply the window to the analysis grains before OLA, so windowing only needs to be done once per analysis grain
    
    int currentlyPlayingNote;
    float currentOutputFreq;
    float lastRecievedVelocity;
    
    juce::SmoothedValue<SampleType, juce::ValueSmoothingTypes::Multiplicative> midiVelocityGain, softPedalGain, playingButReleasedGain, aftertouchGain, outputLeftGain, outputRightGain;
    
    void resetRampedValues (int blocksize);
    
    bav::dsp::Panner panner;
    
    bool isPedalPitchVoice, isDescantVoice;
    
    bool playingButReleased;
    
    float lastPBRmult;
    
    int synthesisIndex;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HarmonizerVoice)
};


/***********************************************************************************************************************************************
***********************************************************************************************************************************************/

/*
    Harmonizer: base class for the polyphonic instrument owning & managing a collection of HarmonizerVoices
*/

template<typename SampleType>
class Harmonizer
{
    using AudioBuffer = juce::AudioBuffer<SampleType>;
    using MidiBuffer  = juce::MidiBuffer;
    using MidiMessage = juce::MidiMessage;
    
    using Voice = HarmonizerVoice<SampleType>;
    
    using ADSRParams = juce::ADSR::Parameters;
    
    
public:
    Harmonizer();
    
    ~Harmonizer();
    
    void initialize (const int initNumVoices, const double initSamplerate, const int initBlocksize);
    
    void prepare (const int blocksize);
    
    void setCurrentPlaybackSampleRate (const double newRate);
    
    void releaseResources();
    
    int getLatencySamples() const noexcept { return pitchDetector.getLatencySamples(); }
    
    void renderVoices (const AudioBuffer& inputAudio, AudioBuffer& outputBuffer, MidiBuffer& midiMessages);
    
    void bypassedBlock (const int numSamples,  MidiBuffer& midiMessages);
    
    void processMidi (MidiBuffer& midiMessages);
    
    void processMidiEvent (const MidiMessage& m);
    
    void playChord (const juce::Array<int>& desiredPitches,
                    const float velocity = 1.0f,
                    const bool allowTailOffOfOld = false);
    
    void allNotesOff (const bool allowTailOff, const float velocity = 1.0f);
    
    void turnOffAllKeyupNotes (const bool allowTailOff,  
                               const bool includePedalPitchAndDescant,
                               const float velocity,
                               const bool overrideSostenutoPedal);
    
    bool isPitchActive (const int midiPitch, const bool countRingingButReleased = false, const bool countKeyUpNotes = false) const;
    
    void reportActiveNotes (juce::Array<int>& outputArray,
                            const bool includePlayingButReleased = false,
                            const bool includeKeyUpNotes = true) const;
    
    int getNumActiveVoices() const;
    
    int getNumVoices() const noexcept { return voices.size(); }
    
    void changeNumVoices (const int newNumVoices);
    
    void setNoteStealingEnabled (const bool shouldSteal) noexcept { shouldStealNotes = shouldSteal; }
    void updateMidiVelocitySensitivity (int newSensitivity);
    void updatePitchbendSettings (const int rangeUp, const int rangeDown);
    
    void setAftertouchGainOnOff (const bool shouldBeOn) { aftertouchGainIsOn = shouldBeOn; }
    
    void setPedalPitch (const bool isOn);
    void setPedalPitchUpperThresh (int newThresh);
    void setPedalPitchInterval (const int newInterval);
    
    void setDescant (const bool isOn);
    void setDescantLowerThresh (int newThresh);
    void setDescantInterval (const int newInterval);
    
    void setConcertPitchHz (const int newConcertPitchhz);
    
    void updateStereoWidth (int newWidth);
    void updateLowestPannedNote (int newPitchThresh);
    
    void setMidiLatch (const bool shouldBeOn, const bool allowTailOff);
    bool isLatched()  const noexcept { return latchIsOn; }
    
    void updateADSRsettings (const float attack, const float decay, const float sustain, const float release);
    void setADSRonOff (const bool shouldBeOn) noexcept { adsrIsOn = shouldBeOn; }
    
    double getSamplerate() const noexcept { return sampleRate; }

    void updatePitchDetectionHzRange (const int minHz, const int maxHz)
    {
        pitchDetector.setHzRange (minHz, maxHz);
        if (sampleRate > 0) pitchDetector.setSamplerate (sampleRate);
    }
    
    void handlePitchWheel (int wheelValue);
    
    void resetRampedValues (int blocksize);
    
    
protected:
    
    // these functions will be called by the harmonizer's voices, to query for important harmonizer-wide info
    
    // returns a float velocity weighted according to the current midi velocity sensitivity settings
    float getWeightedVelocity (const float inputFloatVelocity) const
    {
        return velocityConverter.floatVelocity(inputFloatVelocity);
    }
    
    // returns the actual frequency in Hz a HarmonizerVoice needs to output for its latest recieved midiNote, as an integer -- weighted for pitchbend with the current settings & pitchwheel position, then converted to frequency with the current concert pitch settings.
    float getOutputFrequency (const int midipitch) const
    {
        return pitchConverter.mtof (bendTracker.newNoteRecieved(midipitch));
    }
    
    bool isSustainPedalDown()   const noexcept { return sustainPedalDown;   }
    bool isSostenutoPedalDown() const noexcept { return sostenutoPedalDown; }
    bool isSoftPedalDown()      const noexcept { return softPedalDown;      }
    float getSoftPedalMultiplier() const noexcept { return softPedalMultiplier; }
    float getPlayingButReleasedMultiplier() const noexcept { return playingButReleasedMultiplier; }
    bool isAftertouchGainOn() const noexcept { return aftertouchGainIsOn; }
    
    bool isADSRon() const noexcept { return adsrIsOn; }
    ADSRParams getCurrentAdsrParams() const noexcept { return adsrParams; }
    ADSRParams getCurrentQuickReleaseParams() const noexcept { return quickReleaseParams; }
    ADSRParams getCurrentQuickAttackParams()  const noexcept { return quickAttackParams; }
    
    
private:
    friend class HarmonizerVoice<SampleType>;
    
    // adds a specified # of voices
    void addNumVoices (const int voicesToAdd);
    
    // removes a specified # of voices, attempting to remove inactive voices first, and only removes active voices if necessary
    void removeNumVoices (const int voicesToRemove);
    
    // this function should be called any time the number of voices the harmonizer owns changes
    void numVoicesChanged();
    
    // MIDI
    void handleMidiEvent (const MidiMessage& m, const int samplePosition);
    void noteOn (const int midiPitch, const float velocity, const bool isKeyboard = true);
    void noteOff (const int midiNoteNumber, const float velocity, const bool allowTailOff, const bool isKeyboard = true);
    void handleAftertouch (int midiNoteNumber, int aftertouchValue);
    void handleChannelPressure (int channelPressureValue);
    void updateChannelPressure (int newIncomingAftertouch);
    void handleController (const int controllerNumber, int controllerValue);
    void handleSustainPedal (const int value);
    void handleSostenutoPedal (const int value);
    void handleSoftPedal (const int value);
    void handleModWheel (const int wheelValue);
    void handleBreathController (const int controlValue);
    void handleFootController (const int controlValue);
    void handlePortamentoTime (const int controlValue);
    void handleBalance (const int controlValue);
    void handleLegato (const bool isOn);
    
    void startVoice (Voice* voice, const int midiPitch, const float velocity, const bool isKeyboard);
    void stopVoice  (Voice* voice, const float velocity, const bool allowTailOff);
    
    void turnOnList  (const Array<int>& toTurnOn,  const float velocity, const bool partOfChord = false);
    void turnOffList (const Array<int>& toTurnOff, const float velocity, const bool allowTailOff, const bool partOfChord = false);
    
    void updateQuickReleaseMs (const int newMs);
    void updateQuickAttackMs  (const int newMs);
    
    // this function is called any time the collection of pitches is changed (ie, with regular keyboard input, on each note on/off, or for chord input, once after each chord is triggered). Used for things like pedal pitch, descant, etc
    void pitchCollectionChanged();
    
    void applyPedalPitch();
    void applyDescant();
    
    // voice allocation
    Voice* findFreeVoice (const bool stealIfNoneAvailable);
    Voice* findVoiceToSteal();
    Voice* getVoicePlayingNote (const int midiPitch) const;
    Voice* getCurrentDescantVoice() const;
    Voice* getCurrentPedalPitchVoice() const;
    
    void fillWindowBuffer (const int numSamples);
    
    juce::OwnedArray< HarmonizerVoice<SampleType> > voices;
    
    PitchDetector<SampleType> pitchDetector;
    
    GrainExtractor<SampleType> grains;
    juce::Array<int> indicesOfGrainOnsets;
    
    // the arbitrary "period" imposed on the signal for analysis for unpitched frames of audio will be randomized within this range
    // NB max value should be 1 greater than the largest possible generated number 
    const juce::Range<int> unpitchedArbitraryPeriodRange { 50, 201 };
    
    bool latchIsOn;
    
    juce::Array<int> currentNotes;
    juce::Array<int> desiredNotes;
    
    ADSRParams adsrParams;
    ADSRParams quickReleaseParams;
    ADSRParams quickAttackParams;
    
    float currentInputFreq;
    
    double sampleRate;
    uint32 lastNoteOnCounter;
    int lastPitchWheelValue;
    
    bool shouldStealNotes;
    
    // *********************************
    
    // for clarity & cleanliness, the individual descant & pedal preferences are each encapsulated into their own struct:
    
    struct pedalPitchPrefs
    {
        bool isOn;
        int lastPitch;
        int upperThresh; // pedal pitch has an UPPER thresh - the auto harmony voice is only activated if the lowest keyboard note is BELOW a certain thresh
        int interval;
    };
    
    struct descantPrefs
    {
        bool isOn;
        int lastPitch;
        int lowerThresh; // descant has a LOWER thresh - the auto harmony voice is only activated if the highest keyboard note is ABOVE a certain thresh
        int interval;
    };
    
    pedalPitchPrefs pedal;
    descantPrefs descant;
    
    // *********************************
    
    PanningManager  panner;
    int lowestPannedNote;
    
    bav::midi::VelocityHelper  velocityConverter;
    bav::midi::PitchConverter  pitchConverter;
    bav::midi::PitchBendHelper bendTracker;
    
    bool adsrIsOn;
    
    MidiBuffer aggregateMidiBuffer; // this midi buffer will be used to collect the harmonizer's aggregate MIDI output
    int lastMidiTimeStamp;
    int lastMidiChannel;
    
    bool aftertouchGainIsOn;
    
    float playingButReleasedMultiplier;
    
    bool sustainPedalDown, sostenutoPedalDown, softPedalDown;
    
    float softPedalMultiplier; // the multiplier by which each voice's output will be multiplied when the soft pedal is down
    
    AudioBuffer windowBuffer;
    int windowSize;
    
    AudioBuffer polarityReversalBuffer;
    
    juce::Array< Voice* > usableVoices; // this array is used to sort the voices when a 'steal' is requested
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Harmonizer)
};


} // namespace
