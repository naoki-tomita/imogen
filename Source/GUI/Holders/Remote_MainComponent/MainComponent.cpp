
#include "MainComponent.h"


MainComponent::MainComponent()
{
    this->setBufferedToImage (true);

    addAndMakeVisible (gui);
    
    state.addAllAsInternal();

    setSize (800, 2990);
}


MainComponent::~MainComponent()
{
}


/*=========================================================================================================
    juce::Component functions
 =========================================================================================================*/

void MainComponent::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void MainComponent::resized()
{
    gui.setBounds (getLocalBounds());
}
