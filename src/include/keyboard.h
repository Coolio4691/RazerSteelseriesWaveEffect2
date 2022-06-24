#ifndef __KEYBOARD_H__
#define __KEYBOARD_H__
#include <razer/keyboard.h>


static struct keyboard keyboard;


void keyboard_init() {
    // create variable for devices
    char** devices;
    // create variable for elements in devices
    int devicesLen;

    // get all devices 
    devicemanager_get_devices(&devices, &devicesLen);
    if(devicesLen <= 0) return;

    // get keyboard from id
    keyboard_from_id(devices[0], &keyboard);

    // allocate int array of all keys to pressed keys
    pressedKeys = (int*)malloc((keyboard.lighting.matrix.cols * keyboard.lighting.matrix.rows) * sizeof(int));
    for(int i = 0; i < keyboard.lighting.matrix.cols * keyboard.lighting.matrix.rows; i++) {
        // set pressedkeys at key to 0
        pressedKeys[i] = 0;
    }
}

#endif