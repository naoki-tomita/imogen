/*
 ==============================================================================
 
 StaffDisplay.h
 Created: 26 Nov 2020 11:27:15pm
 Author:  Ben Vining
 
 ==============================================================================
 */

#pragma once

#include <JuceHeader.h>
#include "../../Source/DSP/PluginProcessor.h"
#include "../../Source/DSP/Utils/GeneralUtils.h"
#include "../../Source/GUI/LookAndFeel.h"


//==============================================================================
/*
 */
class StaffDisplay  : public juce::Component
{
public:
    ImogenAudioProcessor& audioProcessor;
    ImogenLookAndFeel& lookAndFeel;
    StaffDisplay(ImogenAudioProcessor& p, ImogenLookAndFeel& l);
    ~StaffDisplay() override;
    
    void paint (juce::Graphics&) override;
    void resized() override;
    
    ComboBox displayFlats;
    
private:
    
    Image grandStaff;
    ImageComponent staffImage;
    
    Array<int> yCoordsOfActiveNotes;
    int yCoordLookupTable[127];
    bool useFlats;
    
    void drawPitches(const Array<int>& activePitches, Graphics& g);
    void drawNotehead(const int x, const int y, Graphics& g);
    void drawAccidental(const int x, const int y, Graphics& g);
    
    const float halfTheStafflineHeight;
    
    String noteheadSvg;
    Path noteheadPath;
    
    //String flatSvg;
    //Path flatPath;
    //String sharpSvg;
    //Path sharpPath;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StaffDisplay)
};
