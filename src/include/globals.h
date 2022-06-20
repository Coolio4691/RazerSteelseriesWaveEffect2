#ifndef __GLOBALS_H__
#define __GLOBALS_H__

static int exitWave = 0;
static char* keyboardInputPath;
static char* mouseInputPath;

// keyboard pressed keys
int* pressedKeys;
// mouse pressed buttons
int* pressedButtons;

int lockShortcut[] = {
    42, //lshift
    29, // lctrl
    38, // l
    -1 // end
};

int* lockShortcutPressed;


static char* vmName = "win10"; 
static int hasVM = 0;


#endif