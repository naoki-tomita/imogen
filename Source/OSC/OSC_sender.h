
#include "GUI/GUI_Framework.h"

#include <juce_osc/juce_osc.h>


class ImogenOSCSender    :   public ImogenEventSender,
                             public juce::OSCSender
{
public:
    ImogenOSCSender() = default;
    
    void sendParameterChange (ParameterID paramID, float newValue) override final
    {
        send (getParameterOSCaddress (paramID), newValue);
    }
    
    void sendParameterChangeGestureStart (ParameterID paramID) override final
    {
        juce::ignoreUnused (paramID);
    }
    
    void sendParameterChangeGestureEnd (ParameterID paramID) override final
    {
        juce::ignoreUnused (paramID);
    }
    
    void sendEditorPitchbend (int wheelValue) override final
    {
        juce::ignoreUnused (wheelValue);
    }
    
    void sendMidiLatch (bool shouldBeLatched) override final
    {
        juce::ignoreUnused (shouldBeLatched);
    }
    
    void sendKillAllMidiEvent() override final
    {
        
    }
    
    void sendLoadPreset   (const juce::String& presetName) override final
    {
        juce::ignoreUnused (presetName);
    }
    
    void sendSavePreset   (const juce::String& presetName) override final
    {
        juce::ignoreUnused (presetName);
    }
    
    void sendDeletePreset (const juce::String& presetName) override final
    {
        juce::ignoreUnused (presetName);
    }
    
    void sendEnableAbletonLink (bool shouldBeEnabled) override final
    {
        juce::ignoreUnused (shouldBeEnabled);
    }
    
    void sendErrorCode (ErrorCode code) override final
    {
        juce::ignoreUnused (code);
    }
    
private:
};
