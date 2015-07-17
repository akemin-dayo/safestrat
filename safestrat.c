/* link: -framework CoreFoundation -framework IOKit */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <CoreFoundation/CoreFoundation.h>
#include <mach/mach_time.h>

/* test_volume_down decls */

void *IOHIDEventCreateKeyboardEvent(CFAllocatorRef, uint64_t, uint32_t, uint32_t,
                                    bool, uint32_t);
void *IOHIDEventSystemCreate(CFAllocatorRef);
bool IOHIDEventSystemOpen(void *, void *, void*, void*, void*);
void IOHIDEventSystemClose(void *, void*);
void *IOHIDEventSystemCopyEvent(void *, uint32_t, void *, uint32_t);

CFIndex IOHIDEventGetIntegerValue(void *, uint32_t);
enum {
    kIOHIDEventTypeKeyboard = 3,
    kIOHIDEventFieldKeyboardDown = 3 << 16 | 2,
};

/* enable_usb decls */

typedef int IOReturn;

/* most of these are in old IOKitUser, but CreateWithType is new */
typedef struct __IOUSBDeviceController* IOUSBDeviceControllerRef;
typedef struct __IOUSBDeviceDescription* IOUSBDeviceDescriptionRef;
IOReturn IOUSBDeviceControllerCreate(CFAllocatorRef allocator,
                                     IOUSBDeviceControllerRef* deviceRef);
IOUSBDeviceDescriptionRef IOUSBDeviceDescriptionCreateWithType(CFAllocatorRef al,
                                                               CFStringRef type);
void IOUSBDeviceDescriptionSetSerialString(IOUSBDeviceDescriptionRef ref,
                                           CFStringRef serial);
IOReturn IOUSBDeviceControllerSetDescription(IOUSBDeviceControllerRef device,
                                             IOUSBDeviceDescriptionRef description);

static void enable_usb() {
    IOUSBDeviceDescriptionRef descr =
    IOUSBDeviceDescriptionCreateWithType(kCFAllocatorDefault,
                                         CFSTR("standardMuxOnly"));
    if (!descr) {
        printf("IOUSBDeviceDescriptionCreateWithType failed :(\n");
        exit(1);
    }
    IOUSBDeviceDescriptionSetSerialString(descr, CFSTR("i am a serial number"));
    IOUSBDeviceControllerRef controller;
    IOReturn ir;
    if ((ir = IOUSBDeviceControllerCreate(kCFAllocatorDefault, &controller))) {
        printf("couldn't create controller (%x), trying again...\n", ir);
        sleep(1);
    }
    if ((ir = IOUSBDeviceControllerSetDescription(controller, descr))) {
        printf("couldn't set description (%x) :(\n", ir);
        exit(1);
    }
    printf("USB enabled\n");
}

static bool test_volume_down() {
    bool pressed = false;
    void *event_system = IOHIDEventSystemCreate(NULL);
    if (!event_system) {
        printf("couldn't create HID event system\n");
        goto tvd_return;
    }
    if (!IOHIDEventSystemOpen(event_system, NULL, NULL, NULL, NULL)) {
        printf("couldn't open HID event system\n");
        goto tvd_return;
    }
    /* volume *decrement* */
    void *dummy = IOHIDEventCreateKeyboardEvent(NULL, mach_absolute_time(),
                                                0x0c, 0xea,
                                                0, 0);
    if (!dummy) {
        printf("couldn't create dummy HID event\n");
        goto tvd_return;
    }
    void *event = IOHIDEventSystemCopyEvent(event_system,
                                            kIOHIDEventTypeKeyboard,
                                            dummy, 0);
    if (!event)
        goto tvd_return;
    pressed = !!IOHIDEventGetIntegerValue(event, kIOHIDEventFieldKeyboardDown);
tvd_return:
    if (event_system)
        IOHIDEventSystemClose(event_system, NULL);
    return pressed;
}

int main(int argc, char **argv) {
    bool force = false;
    if (argc != 1 && argc != 2)
        goto usage;
    if (argc == 2) {
        if (strcmp(argv[1], "force"))
            goto usage;
        force = true;
    }
    if (!(force || test_volume_down()))
        return 0;
    enable_usb();
    execl("/usr/sbin/sshd", "/usr/sbin/sshd", "-D", NULL);
    printf("exec fail :(\n");
    return 1;
usage:
    printf("usage: safestrat [force]\n");
    return 1;
}
