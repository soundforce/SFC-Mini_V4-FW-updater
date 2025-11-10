# SFC-Mini_V4-FW-updater

This is a simple JUCE-based Mac OS app to update my SoundForce stm32-based MIDI controllers. I needed a quick way to make a GUI based automatic updater running the STM32 cube programmer CLI as a system command.
Utlimately I am only using JUCE to make a GUI app very quickly. I wanted to process to be automatic and self guiding, without intervention from the user: plug the device, start the application and watch it do its thing.

- How it works:
The app runs on 1s timer callback, progressing through a switch/case state machine. The state machine goes through the different states: 
enum states {scanning_USB, scanning_Mini_port, bootloader_request, scanning_stm32_bootloader, start_cube_prog, wait_for_reboot, open_MIDI_IN, request_FW_version, received_message, endpoint};

At every state, a juce::Label is updated to show users the progress.

I am using the Mac IO kit functions to list USB devices and detect my device. After which I am checking that the MIDI port is available for my device.
For MIDI I used RTmidi as I didn't have experience with the JUCE MIDI functions.
The app will send a magic sysex to the controller which causes it to reboot in bootloader mode (that's implemented on the firmware side).
The user can also hold 2 switch to pull up the BOOT pin, this makes the device unbrickable in case of error during the firmware update process.

Next step is to rescan the USB device to check for the STM32 BOOTLOADER device. I check that both manufacturer and device names are matching.

After that stm32cube programmer cli will run.
The stm32 cube programmer cli unfortunately doesn't return the right value in cause of errors. So in order to validate the update I am comparing the target FW version with the loaded FW version, at the end of the process.
The controllers will return the firmware version when received a magic MIDI message.
So the last steps is to wait a bit to make sure that the controller reboots, that its MIDI port is available again, send the magic message to get the FW version back.
And finally compare that the FW updater version.

-Important to note:
    * It was built on Mac OS 15.1, using JUCE 8 and Xcode 16.2. The built app won't run on Mac OS < Big sur, unless I build the project on my Mac running Mac OS 10.15. Even though the deployment targer is 10.13 everywhere.
    
    * When you clean the build folder in Xcode the qt framework folders (as found in the To manually add folder) will be deleted, and they need to copy copied again to the built app Content/Ressources folder. Normally you do that automatically by adding the files in the projucer, and that worked for all the other stm32cube programmer files. But those wouldn't be added to the app package. The qt frameworks are needed by the stm32cube programmer cli.
    
    * The Data_Base folder is unused.
    
    * Some stm32 cube programmer cli versions won't work. This is using 2.10. I only tried 2.20 and that wouldn't run. It's already included in the repo so no need to add it.
    
    * An alternative would be to use DFU util. Something that reports the progress status would nice.
