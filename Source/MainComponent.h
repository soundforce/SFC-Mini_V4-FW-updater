#pragma once

#include <JuceHeader.h>
using namespace std;

//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/

class MainComponent  : public juce::Component, private juce::Timer, private juce::URL
{
public:
    //==============================================================================
    MainComponent();
    ~MainComponent() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
    
private:
    //==============================================================================
    // Your private member variables go here...
    juce::Label titleLabel;
    juce::Label progressLabel;
    juce::TextButton link_button{"Open source code"};
        
    void timerCallback() final;


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
