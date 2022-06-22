#ifndef __KEYBOARD_H__
#define __KEYBOARD_H__
#include <razer/keyboard.h>


static struct keyboard keyboard;


void keyboard_init() {
    char** devices;
    int devicesLen;

    devicemanager_get_devices(&devices, &devicesLen);
    if(devicesLen <= 0) return;

    keyboard_from_id(devices[0], &keyboard);

    pressedKeys = (int*)malloc((keyboard.lighting.matrix.cols * keyboard.lighting.matrix.rows) * sizeof(int));
    for(int i = 0; i < keyboard.lighting.matrix.cols * keyboard.lighting.matrix.rows; i++) {
        pressedKeys[i] = 0;
    }
}

void keyboard_set_key(struct keyboard* keyboard, int col, int row, struct rgb rgb) {
    device_lighting_set_led(&keyboard->lighting, key_index_from_2D(&keyboard->lighting, col, row), rgb.red, rgb.green, rgb.blue);
}

#endif