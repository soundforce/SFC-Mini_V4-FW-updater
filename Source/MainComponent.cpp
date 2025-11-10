#include "MainComponent.h"
#include "RtMidi.h"
#include "rtmidi_c.h"
#include <unistd.h>
#include <stdlib.h>
#include "usb_functions.h"
#include "string.h"

std::vector<unsigned char> sysex_bootloader = {0xF0, 0x02, 0x04, 0x17, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x04, 0xF7};
std::vector<unsigned char> message_firmware_version = {0x90, 88,88};

using namespace std;

string firmware_version = "1.0";

int state = 0;
enum states {scanning_USB, scanning_Mini_port, bootloader_request, scanning_stm32_bootloader, start_cube_prog, wait_for_reboot, open_MIDI_IN, request_FW_version, received_message, endpoint};

RtMidiOut *midiout;
RtMidiIn *midiin;

juce::String infoText;
string firmware_version_received;
vector<unsigned char> message_in;
bool message_came_in = 0;

void MIDI_IN_CALLBACK( double deltatime, std::vector< unsigned char > *message, void */*userData*/ )
{
    //cout << "message in of size " << message->size() << endl;
    //getting firmware version
    if((int)message->at(0) == 210){
        
        /*
         int byte = (int)message->at(1);
         //cout << "other byte " << byte << endl;
         stringstream ss;
         ss << byte/10;
         ss << ".";
         ss << byte-10;
         string firmware_version_received = ss.str();
         cout << firmware_version_received << endl;
         */
        message_came_in = 1;
        memcpy(&message_in, message, sizeof(*message));
    }
}

//==============================================================================
MainComponent::MainComponent()
{
    setSize (600, 200);
    
    cout << "////////////////////////////////" << endl;
    cout << "SFC-Mini V4 firmware updater " << firmware_version << endl;
    cout << "////////////////////////////////" << endl
    << endl;
    
    startTimer(1000);
    
    addAndMakeVisible(titleLabel);
    titleLabel.setFont (juce::Font (24.0f, juce::Font::bold));
    titleLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    titleLabel.setJustificationType (juce::Justification::centred);
    titleLabel.setBounds (10,  10, getWidth() - 20,  24);
    titleLabel.setText ("SoundForce SFC-Mini V4 Updater FW " + firmware_version, juce::dontSendNotification);
    
    addAndMakeVisible (progressLabel);
    progressLabel.setFont (juce::Font (16.0f, juce::Font::bold));
    progressLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    progressLabel.setJustificationType (juce::Justification::topLeft);
    progressLabel.setBounds (50,  40, getWidth() - 20,  200);
    
    cout << "constructor" << endl;
    cout << "" << endl;
}

MainComponent::~MainComponent()
{
    cout << "destructor" << endl;
}

//==============================================================================
void MainComponent::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
    
    g.setFont (juce::FontOptions (16.0f));
    g.setColour (juce::Colours::white);
    //g.drawText ("SoundForce SFC-Mini V4 FW updater", 0, 0, 600, 20, juce::Justification::horizontallyCentred);
    //cout << "paint" << endl;
}

void MainComponent::resized()
{
    // This is called when the MainComponent is resized.
    // If you add any child components, this is where you should
    // update their positions.
    cout << "resized" << endl;
}

void MainComponent::timerCallback(){
    
    switch(state){
            
            //Scanning USB Devices
        case scanning_USB:{
            cout << "////////////////////////////////" << endl;
            cout << "Scanning USB devices" << endl;
            cout << "////////////////////////////////" << endl
            << endl;
            
            infoText << "- Scanning USB devices\n";
            progressLabel.setText (infoText, juce::dontSendNotification);
            
            int return_usb = usb_enum();
            
            //Mini V4 is found
            if(return_usb == 1){
                state = scanning_Mini_port;
            }
            
            //Device is found as STM32 BOOTLOADER this is an important failsafe for hardware jump to bootloader
            else if(return_usb == 2){
                state = start_cube_prog;
            }
            
            else{
                infoText << "- SFC-Mini not found, plug in your controller and restart the app \n";
                infoText << "- If the controller is plugged but not responsive (= leds not on): \n1) Unplug the controller\n2) Hold down the 2 switches under the MOD. amount and MOD. mix knobs\n";
                infoText << "3) Keeping the switches pressed down, replug the controller \n4) Restart this app\n";

                progressLabel.setText (infoText, juce::dontSendNotification);
                state = endpoint;
            }
            
            break;
        }
            
            //Checking for the MIDI device name
        case scanning_Mini_port:{
            infoText << "- SFC-Mini V4 is found\n";
            progressLabel.setText (infoText, juce::dontSendNotification);
            
            cout << endl << "////////////////////////////////" << endl;
            cout << "Scanning MIDI devices" << endl;
            cout << "////////////////////////////////" << endl
            << endl;
            
            midiout = new RtMidiOut();
            
            unsigned int nPorts = midiout->getPortCount();
            cout << "Found " << nPorts << " Midi port(s): " << endl;
            for (unsigned int i = 0; i < nPorts; i++)
            {
                string port_name = midiout->getPortName(i);
                cout << "Port number: " << i << " - Port name: " << port_name << endl;
                if (port_name == "SFC-Mini V4")
                {
                    midiout->openPort(i);
                    
                    cout << endl << "////////////////////////////////////////" << endl;
                    cout << "SFC-Mini V4 MIDI PORT IS CONFIRMED FOUND" << endl;
                    cout << "////////////////////////////////////////" << endl;
                    state = bootloader_request;
                }
            }
                        
            break;
        }
            
            //Restarting the device in bootloader by sending message
        case bootloader_request:{
            
            midiout->sendMessage(&sysex_bootloader);
            
            infoText << "- Restarting the controller in bootloader mode\n";
            progressLabel.setText (infoText, juce::dontSendNotification);
            
            cout << endl << "////////////////////////////////" << endl;
            cout << "Sending bootloader sysex message" << endl;
            cout << "////////////////////////////////" << endl << endl;
            
            state = scanning_stm32_bootloader;
            break;
        }
            
            
            //Check the stm32 bootloader is preset
        case scanning_stm32_bootloader:{
            
            midiout->closePort();

            int return_usb = usb_enum();
            
            if(return_usb == 2){
                infoText << "- SFC-MINI V4 in bootloader mode is found\n";
                progressLabel.setText (infoText, juce::dontSendNotification);
                cout << endl << "///////////////////////////////////////" << endl;
                cout << "SFC-MINI V4 in bootloader mode is found" << endl;
                cout << "///////////////////////////////////////" << endl
                << endl;
                
                state = start_cube_prog;
            }
            break;
        }
            
            //Programming the firmware
        case start_cube_prog:{
            
            infoText << "- Starting the firmware update\n";
            progressLabel.setText (infoText, juce::dontSendNotification);
            
            std::string dir = juce::File::getSpecialLocation(juce::File::currentApplicationFile).getFullPathName().toStdString();
            
            //Need to edit spaces in the path and replace them by 2 slashes
            string bla = "\\";
            for (int i = 0; i < dir.length(); i++)
            {
                if (dir[i] == ' ')
                {
                    dir.insert(i, bla);
                    i++;
                }
            }
            
            
            //string command = dir + "/Contents/Resources/STM32_Programmer_CLI -c port=usb1 -w " + dir + "/Contents/Resources/SFC-Mini_V4_V1.0.elf -v --start 0x08000000";
            string command = dir + "/Contents/Resources/STM32_Programmer_CLI -c port=usb1 -w " + dir + "/Contents/Resources/SFC-Mini_V4_V" + firmware_version + ".elf" + " -v --start 0x08000000";
            //cout << command << endl;
            
            int returnCode = system(command.c_str());
            
            cout << "return code: " << returnCode << endl;
            
            if(returnCode == 0){
                cout << endl << "///////////////////////////////////" << endl;
                cout << "///Firmware updated sucessfully////" << endl;
                cout << "///////////////////////////////////" << endl << endl;
                state = wait_for_reboot;
            }
            
            //Unfortunately this version of cubeprogrammer returns 0 if in case of errors
            //So to verify the update, we need to compare firmware version in the next steps
            else{
                cout << endl << "///////////////////////////////////" << endl;
                cout << "///Firmware update has failed, please unplug and replug your controller and start this application again////" << endl;
                cout << "///////////////////////////////////" << endl << endl;
                
                infoText << "- Firmware update has failed, please unplug and replug your controller and start this application again\n";
                progressLabel.setText (infoText, juce::dontSendNotification);
            }
            
            cout << "wait 1sec" << endl;
            break;
        }
            
            
            //Wait for reboot and open virtual port
        case wait_for_reboot:{
            infoText << "- Firmware update finished, verifying version\n";
            progressLabel.setText (infoText, juce::dontSendNotification);
            
            midiin = new RtMidiIn();
            midiin->openVirtualPort();
            
            state = open_MIDI_IN;
            break;
        }
            
            //WAit for Mini V4 midi in to come up
        case open_MIDI_IN:{
            int nPorts = midiin->getPortCount();
            
            if (nPorts == 0 ) {
                std::cout << "No input ports available!" << std::endl;
            }
            
            else{
                for (int i=0; i<nPorts; i++ ) {
                    
                    string portName = midiin->getPortName(i);
                    
                    std::cout << "  Input port #" << i << ": " << portName << '\n';
                    if(portName == "SFC-Mini V4"){
                        midiin->openPort(i);
                        midiin->setCallback(&MIDI_IN_CALLBACK);
                        state = request_FW_version;
                    }
                }
            }
            
    
            break;
        }
            
            //Request FW version
        case request_FW_version:{
            infoText << "- Version info requested\n";
            progressLabel.setText (infoText, juce::dontSendNotification);
            
            midiout = new RtMidiOut();
            
            unsigned int nPorts = midiout->getPortCount();
            cout << "Found " << nPorts << " Midi port(s): " << endl;
            
            for (unsigned int i = 0; i < nPorts; i++)
            {
                string port_name = midiout->getPortName(i);
                cout << "Port number: " << i << " - Port name: " << port_name << endl;
                if (port_name == "SFC-Mini V4")
                {
                    midiout->openPort(i);
                    cout << endl << "////////////////////////////////////////" << endl;
                    cout << "SFC-Mini V4 MIDI PORT IS CONFIRMED FOUND" << endl;
                    cout << "////////////////////////////////////////" << endl;
                }
            }
            
            midiout->sendMessage(&message_firmware_version);
            state = received_message;
            break;
        }
            
            
        case received_message:{
            midiout->closePort();

            if(message_came_in){
                message_came_in = 0;
                
                midiin->closePort();
                
                int byte = message_in[1];
                stringstream ss;
                ss << byte/10;
                ss << ".";
                ss << byte-10;
                string firmware_version_received = ss.str();
                cout << firmware_version_received << endl;
                
                cout << "Firmware version " << firmware_version_received << endl;
                infoText << "- Current firmware version " << firmware_version_received;
                progressLabel.setText (infoText, juce::dontSendNotification);
                
                if(!firmware_version_received.compare(firmware_version)){
                    infoText << " - update succesfull\n";
                }
                else{
                    infoText << "- Update failed \n - Close this app, unplug/replug the controller and try again.\n";
                }
                state = endpoint;
                
                progressLabel.setText (infoText, juce::dontSendNotification);
                
            }
            break;
        }
            
        case endpoint:{
            break;
        }
    }
}
