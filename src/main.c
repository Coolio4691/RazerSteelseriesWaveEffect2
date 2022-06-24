#include <stdio.h>
#include <razer/devicemanager.h>
#include <razer/devicelighting.h>
#include <stdlib.h>
#include <libusb-1.0/libusb.h>
#include <hidapi/hidapi.h>
#include <unistd.h>
#include <pthread.h>

#include <string.h>
#include <signal.h>
#include "mouse.h"
#include "colour.h"
#include "extrasThread.h"
#include "wave.h"
#include "globals.h"
#include "keyboard.h"
#include "extrasThread.h"

void exit_signal(int signum) {
    // set exit to 1
    exitWave = 1;
}

int main() {
    signal(SIGINT, exit_signal); // set signal handler for exiting
    pwd = get_pwd(); // get pwd in pwd variable

    pthread_t waveThread_id;
    pthread_t waveValuesThread_id;
    pthread_t extrasThread_id;

    // init hid then exit with code -1 if error
    if (hid_init() == -1)
        return -1;
    
    // get mouse
    mouse_init();
    
    // allocate pressed buttons
    pressedButtons = (int*)malloc(RIVAL600_LEDS * sizeof(int));
    for(int i = 0; i < RIVAL600_LEDS; i++) {
        // set button pressed to 0
        pressedButtons[i] = 0;
    }
    
    keyboard_init();
    wave_init();   
    
    // init lock shortcut
    int lockShortcutSize = 0;
    for(lockShortcutSize = 0; lockShortcut[lockShortcutSize] >= 0; lockShortcutSize++);

    lockShortcutPressed = (int*)malloc(lockShortcutSize * sizeof(int));
    for(int i = 0; i < lockShortcutSize; i++) lockShortcutPressed[i] = 0;

    // create threads
    pthread_create(&waveThread_id, NULL, wave, NULL);
    pthread_create(&waveValuesThread_id, NULL, wave_values_loop, NULL);
    //pthread_create(&extrasThread_id, NULL, initExtras, NULL);
    
    // wait for threads to exit
    pthread_join(waveThread_id, NULL);
    pthread_join(waveValuesThread_id, NULL);
    //pthread_join(extrasThread_id, NULL);

    // free keyboard input path
    free(keyboardInputPath);
    // free mouse input path
    free(mouseInputPath);
    // free lockshortcut pressed
    free(lockShortcutPressed);
    // free pressedbuttons
    free(pressedButtons);
    // free pressedkeys
    free(pressedKeys);
    // free pwd string
    free(pwd);

    // close connection to mouse and hid
    mouse_exit();
    hid_exit();
    return 0;
}