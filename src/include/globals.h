#ifndef __GLOBALS_H__
#define __GLOBALS_H__

static int exitWave = 0;
static char* keyboardInputPath;
static char* mouseInputPath;

// keyboard pressed keys
static int* pressedKeys;
// mouse pressed buttons
static int* pressedButtons;

static int lockShortcut[] = {
    42, //lshift
    29, // lctrl
    38, // l
    -1 // end
};

static char* pwd;
static int* lockShortcutPressed;


static char* vmName = "win10"; 
static int hasVM = 0;


#endif