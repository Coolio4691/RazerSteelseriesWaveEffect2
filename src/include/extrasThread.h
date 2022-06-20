#ifndef __EXTRASTHREAD_H__
#define __EXTRASTHREAD_H__
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <linux/input.h>
#include <linux/input-event-codes.h>
#include <fcntl.h>
#include <limits.h>
#include <pthread.h>

#include "globals.h"
#include "keys.h"
#include "keyboard.h"
#include "mouse.h"

static struct input_event inputEvent[64];
static struct input_event inputEventMouse[64];

int startsWith(const char *str, const char *pre) {
    size_t lenpre = strlen(pre);
    size_t lenstr = strlen(str);

    return lenstr < lenpre ? 0 : memcmp(pre, str, lenpre) == 0;
}

// device functions taken from https://www.linuxquestions.org/questions/programming-9/get-dev-input-eventx-from-vid-pid-c-c-906842/
static char* strmerge(const char* s1, const char* s2) {
	const size_t n1 = (s1 ? strlen(s1) : 0);
	const size_t n2 = (s2 ? strlen(s2) : 0);
	
	if(!n1 && !n2) {
		errno = EINVAL;
		return NULL;
	}

	char* s = (char*)malloc(n1 + n2 + 1);
	if(!s) {
		errno = ENOMEM;
		return NULL;
	}

	if(n1) memcpy(s, s1, n1);
	if(n2) memcpy(s + n1, s2, n2);

	s[n1 + n2] = 0;

	return s;
}

static int read_hex(const char* filename) {
	FILE* in = fopen(filename, "rb");
	unsigned int value;

	if(!in) return -1;

	if(fscanf(in, "%x", &value) == 1) {
		fclose(in);
		
        return (int)value;
	}

	fclose(in);
    return -1;
}

static int vendor_id(const char* event) {
	if(event && *event) {
		char filename[256];
		int	result;

		result = snprintf(filename, sizeof(filename), "/sys/class/input/%s/device/id/vendor", event);
		if(result < 1 || result >= sizeof(filename)) return -1;

		return read_hex(filename);
	}

	return -1;
}

static int product_id(const char* event) {
	if(event && *event) {
		char filename[256];
		int	result;

		result = snprintf(filename, sizeof(filename), "/sys/class/input/%s/device/id/product", event);
        if(result < 1 || result >= sizeof(filename)) return -1;

		return read_hex(filename);
	}

	return -1;
}

char* find_event(int vendor, int product, int keyboard) {
	struct dirent* entry;
    struct dirent **namelist;

    int numFiles = 0, file = 0;

    // get all files in alphabetical order
    numFiles = scandir("/sys/class/input", &namelist, NULL, alphasort);

    // loop through every file in folder in alphabetical order
	while(file++ != numFiles) {
        // get current entry
        entry = namelist[file];

		if(vendor_id(entry->d_name) == vendor && product_id(entry->d_name) == product) {
			char* name = strmerge("/dev/input/", entry->d_name);

            // ignore mouse and led event types if keyboard
            if(keyboard) {
                char* subStr = strstr(name, "mouse");

                if(subStr) continue;

                // buffer for realpath
                char buf[PATH_MAX];
                // evstr = /sys/class/input/curfile
                char* evStr = strmerge("/sys/class/input/", entry->d_name); 

                // get realpath from symlink at /sys/class/input/curfile
                char* res = realpath(evStr, buf);
                // free evstr memory
                free(evStr);

                // string for file location of capabilities/led
	            char* ledStr;
                if(startsWith(entry->d_name, "event")) ledStr = strmerge(res, "/../capabilities/led");
                else ledStr = strmerge(res, "/capabilities/led");

                // open file at location ledstr 
                int ledFile = open(ledStr, O_RDONLY);
                // free led string
                free(ledStr);
                
                // if file exists check if it isnt an led type
                if(ledFile > -1) {
                    // char at first position of file
                    char num;
                    read(ledFile, &num, sizeof(char));

                    // close file
                    close(ledFile);

                    // if num > 0 skip this event device
                    if(num != '0') continue;
                }
            }

            // return name if exists
			if(name) return name;

            // return error
			errno = ENOMEM;
			return NULL;
		}
	}

    errno = ENOENT;
    return NULL;

}
// end device functions

// lock mouse and keyboard states
static int mouseLocked = 0;
static int keyboardLocked = 0;

void keyDown(int key) {
    // check if key at col, row exists
    struct colrow colrow = col_row_from_key(key);
    if(colrow.col <= -1 && colrow.row <= -1) return;

    // set key at col, row to 11
    pressedKeys[key_index_from_2D(&keyboard.lighting, colrow.col, colrow.row)] = 11; 
}

void keyUp(int key) {
    // check if col, row exists
    struct colrow colrow = col_row_from_key(key);
    if(colrow.col <= -1 && colrow.row <= -1) return;

    // set key at col, row to 10 
    pressedKeys[key_index_from_2D(&keyboard.lighting, colrow.col, colrow.row)] = 10;
}


void mouseDown(int button) {
    // check if led exists
    int led = led_from_button(button);
    if(led <= -1) return;

    // set button at led to 11
    pressedButtons[led] = 11;
}

void mouseUp(int button) {
    // check if led exists
    int led = led_from_button(button);
    if(led <= -1) return;

    // set button at led to 10
    pressedButtons[led] = 10;
}


void* mouseButtonListener(void* threadArgs) {
    mouseInputPath = find_event(RIVAL600_VID, RIVAL600_PID, 0);

    int mouseFD = open(mouseInputPath, O_RDONLY);    
    int connected;

    while(!exitWave) {
        int readDevice = read(mouseFD, inputEventMouse, sizeof(struct input_event) * 64);

        if (readDevice < (int) sizeof(struct input_event)) {
            printf("Mouse error reading - mouse lost?\n");
            close(mouseFD);

            usleep(500 * 1000);
            // try reconnecting after 500ms
            mouseInputPath = find_event(RIVAL600_VID, RIVAL600_PID, 0);
            mouseFD = open(mouseInputPath, O_RDONLY);  
            
            continue;
        }
        else {
            for (long unsigned int i = 0; i < readDevice / sizeof(struct input_event); i++) {

                if (inputEventMouse[i].type == EV_KEY) {
                    switch (inputEventMouse[i].value) {
                        case 0:
                            mouseUp(inputEventMouse[i].code);

                            break;
                        case 1:
                            mouseDown(inputEventMouse[i].code);

                            break;
                        default:
                            break;
                    }
                }
            }
        }
    }

    return NULL;
}

void* keyboardKeyListener(void* threadArgs) {
    keyboardInputPath = find_event(keyboard.VIDPID.vidInt, keyboard.VIDPID.pidInt, 1);
    int keyboardFD = open(keyboardInputPath, O_RDONLY);

    while(!exitWave) {
        int readDevice = read(keyboardFD, inputEvent, sizeof(struct input_event) * 64);

        if (readDevice < (int) sizeof(struct input_event)) {
            printf("Keyboard error reading - keyboard lost?\n");
            close(keyboardFD);

            usleep(500 * 1000);
            // try reconnecting after 500ms
            keyboardInputPath = find_event(keyboard.VIDPID.vidInt, keyboard.VIDPID.pidInt, 1);
            keyboardFD = open(keyboardInputPath, O_RDONLY);  
            
            continue;
        }
        else {
            for (long unsigned int i = 0; i < readDevice / sizeof(struct input_event); i++) {

                if (inputEvent[i].type == EV_KEY) {
                    switch (inputEvent[i].value) {
                        case 0:
                            keyUp(inputEvent[i].code);

                            break;
                        case 1:
                            keyDown(inputEvent[i].code);

                            break;
                        default:
                            break;
                    }
                }
            }
        }
    }

    return NULL;
}


void* initExtras(void* threadArgs) {
    // create thread id variables
    pthread_t keyboardCheckerThread_id;
    pthread_t mouseCheckerThread_id;

    // create threads
    pthread_create(&keyboardCheckerThread_id, NULL, keyboardKeyListener, NULL);
    pthread_create(&mouseCheckerThread_id, NULL, mouseButtonListener, NULL);

    // wait until threads are gone
    pthread_join(keyboardCheckerThread_id, NULL);
    pthread_join(mouseCheckerThread_id, NULL);
    
    return NULL;
}

#endif