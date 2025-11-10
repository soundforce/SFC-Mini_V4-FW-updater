/*
  ==============================================================================

    usb_functions.h
    Created: 9 Nov 2025 4:53:40pm
    Author:  Nicolas

  ==============================================================================
*/

#pragma once

#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/IOMessage.h>
#include <IOKit/IOCFPlugIn.h>
#include <IOKit/usb/IOUSBLib.h>

int usb_enum(void);
bool get_string_from_descriptor_idx(IOUSBDeviceInterface **dev, UInt8 idx, char **str);
void str_trim(char *str);
