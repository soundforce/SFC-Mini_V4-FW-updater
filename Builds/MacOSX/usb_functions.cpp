#include "usb_functions.h"

#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/IOMessage.h>
#include <IOKit/IOCFPlugIn.h>
#include <IOKit/usb/IOUSBLib.h>


int usb_enum(void)
{
    io_registry_entry_t entry   = 0;
    io_iterator_t       iter    = 0;
    io_service_t        service = 0;
    kern_return_t       kret;
    bool manuc_found = 0;
    bool device_found = 0;

    /* 1. Get the IO registry which has the system information for connected hardware. */
    entry = IORegistryGetRootEntry(kIOMasterPortDefault);
    if (entry == 0)
        return 0;

    /* 2. Get an iterator for the USB plane. */
    kret = IORegistryEntryCreateIterator(entry, kIOUSBPlane, kIORegistryIterateRecursively, &iter);
    if (kret != KERN_SUCCESS || iter == 0)
        return 0;

    /* 3. Walk the iterator. */
    while ((service = IOIteratorNext(iter))) {
        IOCFPlugInInterface  **plug  = NULL;
        IOUSBDeviceInterface **dev   = NULL;
        io_string_t            path;
        SInt32                 score = 0;
        IOReturn               ioret;

        /* 4. Pull out IO Plugins for each iterator. */
        kret = IOCreatePlugInInterfaceForService(service, kIOUSBDeviceUserClientTypeID, kIOCFPlugInInterfaceID, &plug, &score);
        IOObjectRelease(service);
        if (kret != KERN_SUCCESS || plug == NULL) {
            continue;
        }

        /* 5. Get an IOUSBDeviceInterface for each USB device from the IO Plugin object. */
        ioret = (*plug)->QueryInterface(plug, CFUUIDGetUUIDBytes(kIOUSBDeviceInterfaceID), (LPVOID *)&dev);
        (*plug)->Release(plug);
        if (ioret != kIOReturnSuccess || dev == NULL) {
            continue;
        }

        /* Print out the path in the IO Plane the device is at. */
        if (IORegistryEntryGetPath(service, kIOServicePlane, path) != KERN_SUCCESS) {
            (*dev)->Release(dev);
            continue;
        }

      UInt8 si;

        //printf("Found device at '%s'\n", path);
        /* Print device metadata. */
        //print_dev_info(dev);

      char *str;

      if ((*dev)->USBGetManufacturerStringIndex(dev, &si) == kIOReturnSuccess)
      {
            get_string_from_descriptor_idx(dev, si, &str);
            if (str != NULL)
            {
              if(!strcmp(str, "STMicroelectronics")){
                //cout << "st found" << endl;
                manuc_found = 1;
              }
            }
      }

    if ((*dev)->USBGetProductStringIndex(dev, &si) == kIOReturnSuccess)
      {
            get_string_from_descriptor_idx(dev, si, &str);
            if (str != NULL)
            {
              if (!strcmp(str, "STM32  BOOTLOADER"))
              {
                //cout << "device found" << endl;
                device_found = 1;
              }
            }
      }

      /* All done with this device. */
      (*dev)->Release(dev);
    }

    IOObjectRelease(iter);

    if(manuc_found && device_found){
      return 2;
    }

    else{
      return 0;
    }
}

bool get_string_from_descriptor_idx(IOUSBDeviceInterface **dev, UInt8 idx, char **str)
{
    IOUSBDevRequest request;
    IOReturn        ioret;
    char            buffer[4086] = { 0 };
    CFStringRef     cfstr;
    CFIndex         len;

    if (str != NULL)
        *str = NULL;

    request.bmRequestType = USBmakebmRequestType(kUSBIn, kUSBStandard, kUSBDevice);
    request.bRequest      = kUSBRqGetDescriptor;
    request.wValue        = (kUSBStringDesc << 8) | idx;
    request.wIndex        = 0x409;
    request.wLength       = sizeof(buffer);
    request.pData         = buffer;

    ioret = (*dev)->DeviceRequest(dev, &request);
    if (ioret != kIOReturnSuccess)
        return false;

    if (str == NULL || request.wLenDone <= 2)
        return true;

    /* Now we need to parse out the actual data.
     * Byte 1 - Length of packet (same as request.wLenDone)
     * Byte 2 - Type
     * Byte 3+ - Data
     *
     * Data is a little endian UTF16 string which we need to convert to a utf-8 string. */
    cfstr   = CFStringCreateWithBytes(NULL, (const UInt8 *)buffer+2, request.wLenDone-2, kCFStringEncodingUTF16LE, 0);
    len     = CFStringGetMaximumSizeForEncoding(CFStringGetLength(cfstr), kCFStringEncodingUTF8) + 1;
    if (len < 0) {
        CFRelease(cfstr);
        return true;
    }
    *str    = (char*)(calloc(1, (size_t)len));
    CFStringGetCString(cfstr, *str, len, kCFStringEncodingUTF8);
    str_trim(*str);

    CFRelease(cfstr);
    return true;
}

void str_trim(char *str)
{
    size_t len;
    size_t i;
    size_t offset;

    if (str == NULL)
        return;

    len = strlen(str);
    if (len == 0)
        return;

    /* Trim white space from the end. */
    offset = 0;
    for (i = len; i-- > 0; ) {
        if (!isspace(str[i])) {
            break;
        }
        offset++;
    }
    if (offset > 0)
        str[len-offset] = '\0';

    /* Trim white space from the beginning. */
    len    = strlen(str);
    offset = 0;
    for (i = 0; i < len; i++) {
        if (!isspace(str[i])) {
            break;
        }
        offset++;
    }

    if (offset > 0)
        memmove(str, str+offset, len-offset+1);
}
