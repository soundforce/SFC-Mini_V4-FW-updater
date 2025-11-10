//
//  usb_functions.h
//  SFC-Mini_V4-FW-updater - App
//
//  Created by Nicolas on 09/11/2025.
//

#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/IOMessage.h>
#include <IOKit/IOCFPlugIn.h>
#include <IOKit/usb/IOUSBLib.h>

int usb_enum(void);
bool get_string_from_descriptor_idx(IOUSBDeviceInterface **dev, UInt8 idx, char **str);
void str_trim(char *str);
