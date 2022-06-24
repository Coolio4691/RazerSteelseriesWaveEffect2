#ifndef __EXTRASTHREAD_H__
#define __EXTRASTHREAD_H__

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <errno.h>
#include <linux/input.h>
#include <fcntl.h>
#include <pthread.h>
#include <linux/limits.h>
#include <libvirt/libvirt.h>

extern int alphasort(const struct dirent **__e1, const struct dirent **__e2); // vscode requires this

#include "globals.h"
#include "keys.h"
#include "keyboard.h"
#include "mouse.h"
#include "password.h"

static struct input_event inputEvent[64];
static struct input_event inputEventMouse[64];


size_t executable_name(char** exeName) {
    // allocate path max to name
    *exeName = (char*)malloc(PATH_MAX * sizeof(char));
    
    // openexename file
    int fd = open("/proc/self/comm", O_RDONLY);
    // read exename file
    size_t len = read(fd, *exeName, PATH_MAX * sizeof(char));
    // close exename file
    close(fd);

    return len;
}

char* get_pwd() {
    // allocate 4096 bytes of memory
    char* buff = (char*)malloc(4096 * sizeof(char));
    // set string length to output of readlink 
    ssize_t len = readlink("/proc/self/exe", buff, 4096 * sizeof(char));
    
    // if string length is not invalid
    if (len != -1) {
        // set buffer at end to null
        buff[len] = 0;

        // get executablename
        char* exeName;
        size_t exeNameLen = executable_name(&exeName);

        // free exename
        free(exeName);

        // allocate memory of path 
        char* pathStr = (char*)malloc(((len + 1) - exeNameLen) * sizeof(char));
        // set chars from buff[0] to buff[len - exenamelen] to pathstr
        for(int i = 0; i < len - exeNameLen; i++) pathStr[i] = buff[i];
        // set char at len + 1 - exenamelen to null
        pathStr[len - exeNameLen] = 0;

        // free buffer
        free(buff);
        
        // return string
        return pathStr;
    }

    // free buffer
    free(buff);
    // return null
    return NULL;
}


int startsWith(const char *str, const char *pre) {
    // get length of both strings
    size_t lenpre = strlen(pre);
    size_t lenstr = strlen(str);

    // if prefix of the string is the same return true
    return lenstr < lenpre ? 0 : memcmp(pre, str, lenpre) == 0;
}

// device functions taken from https://www.linuxquestions.org/questions/programming-9/get-dev-input-eventx-from-vid-pid-c-c-906842/
static char* strmerge(const char* s1, const char* s2) {
    // set string size to 0 if string is invalid or get size
	const size_t n1 = (s1 ? strlen(s1) : 0);
    // set string size to 0 if string is invalid or get size
	const size_t n2 = (s2 ? strlen(s2) : 0);
	
    // if string1 and string2 dont exist return error
	if(!n1 && !n2) {
		errno = EINVAL;
		return NULL;
	}

    // create string with size of string1 + string2 + nullterminator space
	char* s = (char*)malloc(n1 + n2 + 1);
    // if string is invalid return
	if(!s) {
		errno = ENOMEM;
		return NULL;
	}

    // copy first string to output string
	if(n1) memcpy(s, s1, n1);
    // copy second string to output string with offset of first string length
	if(n2) memcpy(s + n1, s2, n2);

    // set string end to null
	s[n1 + n2] = 0;

    // return string
	return s;
}

static int read_hex(const char* filename) {
    // open file with "rb" permissions
	FILE* in = fopen(filename, "rb");
    // create uint variable
	unsigned int value;

    // if file is invalid return
	if(!in) return -1;

    // if it sets value properly close file and return value
	if(fscanf(in, "%x", &value) == 1) {
		// close file
        fclose(in);

        // return value
        return (int)value;
	}

    // close fine
	fclose(in);
    // return error
    return -1;
}

static int vendor_id(const char* event) {
    // check if event is valid
	if(event && *event) {
        // set filename size to 256 * sizeof(char)
        size_t filenameSize = 256 * sizeof(char);

        // create filename and result variable
		char* filename = (char*)malloc(filenameSize);
		int	result;

        // set string to /sys/class/input/event/device/id/vendor
		result = snprintf(filename, filenameSize, "/sys/class/input/%s/device/id/vendor", event);
        // if result is invalid return with error
		if(result < 1 || result >= filenameSize) return -1;

        // get hex value
        int hex = read_hex(filename);
        // free file name string
        free(filename);
        // return hex value
		return hex;
	}

    // return error
	return -1;
}

static int product_id(const char* event) {
    // check if event is valid
	if(event && *event) {
        // set filename size to 256 * sizeof(char)
        size_t filenameSize = 256 * sizeof(char);

        // create filename and result variable
		char* filename = (char*)malloc(filenameSize);
		int	result;

        // set string to /sys/class/input/event/device/id/product
		result = snprintf(filename, filenameSize, "/sys/class/input/%s/device/id/product", event);
        // if result is invalid return with error
        if(result < 1 || result >= filenameSize) return -1;

        // get hex value
        int hex = read_hex(filename);
        // free file name string
        free(filename);
        // return hex value
		return hex;
	}

    // return error
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

            // free file names
            for (int i = 0; i < numFiles; i++) {
                // free name
                free(namelist[i]);
            }
            // free list
            free(namelist);
            
            // return name if exists
			if(name) return name;

            // return error
			errno = ENOMEM;
			return NULL;
		}
	}

    // free file names
	for (int i = 0; i < numFiles; i++) {
		// free name
        free(namelist[i]);
	}
    // free list
	free(namelist);

    errno = ENOENT;
    return NULL;

}
// end device functions

// lock mouse and keyboard states
static int devicesLocked = 0;
static int keyQueue = 0;
static int lockReq = 0;

static int rAltHeld = 0;
static int rAltHeldTime = 0;

static int keyboardFD;
static int mouseFD;

static int keyboardConnected = 0;
static int mouseConnected = 0;

static virConnectPtr conn;

void doLock() {
    // invert deviceslocked
    devicesLocked = !devicesLocked;

    // if keyboard is connected lock it
    if(keyboardConnected) ioctl(keyboardFD, EVIOCGRAB, devicesLocked);
    // if mouse is connected lock it
    if(mouseConnected) ioctl(mouseFD, EVIOCGRAB, devicesLocked);
}

void checkLock() {
    // loop through lockshortcut
    for(int i = 0; lockShortcut[i] != -1; i++) {
        // if key is not pressed return
        if(!lockShortcutPressed[i]) {
            return; 
        }
    }

    // if fully looped without returning set lock to 1
    lockReq = 1;
}


void keyDown(int key) {
    // if devices are not locked check if key is ralt
    if(!devicesLocked) { 
        if(key == 100) { // if key == ralt 
            // set raltheld to true
            rAltHeld = 1;
        }
    }
    
    // if key is in lock shortcut set to pressed
    for(int i = 0; lockShortcut[i] != -1; i++) {
        if(lockShortcut[i] == key) {
            lockShortcutPressed[i] = 1;
            
            break;
        }
    }

    // increment key queue
    keyQueue++;

    // check if key at col, row exists
    struct colrow colrow = col_row_from_key(key);
    // if col and row are invalid return
    if(colrow.col <= -1 && colrow.row <= -1) return;

    // set key at col, row to 11
    pressedKeys[key_index_from_2D(&keyboard.lighting, colrow.col, colrow.row)] = 11; 
}

void keyUp(int key) {
    if(!devicesLocked) {
        if(key == 100) { // if key == ralt 
            rAltHeld = 0;
            rAltHeldTime = 0;
        }
    }

    // check if lock is fully pressed
    checkLock();

    // if key is in lockshortcut set to not pressed
    for(int i = 0; lockShortcut[i] != -1; i++) {
        if(lockShortcut[i] == key) {
            lockShortcutPressed[i] = 0;
            
            break;
        }
    }

    // decrement keyqueue
    keyQueue--;

    // if keyqueue is none and lock request
    if(keyQueue <= 0 && lockReq) {
        // lock devices
        doLock();

        // set lock request to 0
        lockReq = 0;
    }

    // check if col, row exists
    struct colrow colrow = col_row_from_key(key);
    // return if col and rol are invalid
    if(colrow.col <= -1 && colrow.row <= -1) return;

    // set key at col, row to 10 
    pressedKeys[key_index_from_2D(&keyboard.lighting, colrow.col, colrow.row)] = 10;
}


void mouseDown(int button) {
    // increment keyqueue
    keyQueue++;

    // check if led exists
    int led = led_from_button(button);
    // if led is invalid return
    if(led <= -1) return;

    // set button at led to 11
    pressedButtons[led] = 11;
}

void mouseUp(int button) {
    // decrement keyqueue
    keyQueue--;

    // if keyqueue is none and there is a request to lock dolock
    if(keyQueue <= 0 && lockReq) {
        // lock devices
        doLock();

        // set lock request to 0
        lockReq = 0;
    }

    // check if led exists
    int led = led_from_button(button);
    // if led does not exist return
    if(led <= -1) return;

    // set button at led to 10
    pressedButtons[led] = 10;
}


void* mouseButtonListener(void* threadArgs) {
    // get input path
    mouseInputPath = find_event(RIVAL600_VID, RIVAL600_PID, 0);
    // open mouseinputpath
    mouseFD = open(mouseInputPath, O_RDONLY);
    
    // loop till exit signal is given
    while(!exitWave) {
        // set mouse connected to 1
        mouseConnected = 1;

        // read for input
        int readDevice = read(mouseFD, inputEventMouse, sizeof(struct input_event) * 64);

        // if output is wrong reconnect
        if(readDevice < (int) sizeof(struct input_event)) {
            mouseConnected = 0;

            printf("Mouse error reading - mouse lost?\n");
            close(mouseFD);

            usleep(500 * 1000);
            // try reconnecting after 500ms
            // free mouse input path
            free(mouseInputPath);
            // get mouse input path
            mouseInputPath = find_event(RIVAL600_VID, RIVAL600_PID, 0);
            mouseFD = open(mouseInputPath, O_RDONLY);  
            
            continue;
        }
        else {
            // loop through output
            for(long unsigned int i = 0; i < readDevice / sizeof(struct input_event); i++) {
                // check if event type is key
                if(inputEventMouse[i].type == EV_KEY) {
                    switch(inputEventMouse[i].value) {
                    case 0:
                        // if value is keyup do mouseup function
                        mouseUp(inputEventMouse[i].code);

                        break;
                    case 1:
                        // if value is keydown do mousedown function
                        mouseDown(inputEventMouse[i].code);

                        break;
                    default:
                        break;
                    }
                }
            }
        }
    }

    // exit
    return NULL;
}

void* keyboardKeyListener(void* threadArgs) {
    // get input path
    keyboardInputPath = find_event(keyboard.VIDPID.vidInt, keyboard.VIDPID.pidInt, 1);
    // open input path file
    keyboardFD = open(keyboardInputPath, O_RDONLY);

    // loop till exit signal
    while(!exitWave) {
        // set connected to 1
        keyboardConnected = 1;
        // read for input from input path file
        int readDevice = read(keyboardFD, inputEvent, sizeof(struct input_event) * 64);

        // if output is wrong reconnect
        if(readDevice < (int) sizeof(struct input_event)) {
            // set connected to 0
            keyboardConnected = 0;

            // print exit then close file connection
            printf("Keyboard error reading - keyboard lost?\n");
            close(keyboardFD);

            // try reconnecting after 500ms
            usleep(500 * 1000);

            // free keyboard input path
            free(keyboardInputPath);
            // get keyboard input path
            keyboardInputPath = find_event(keyboard.VIDPID.vidInt, keyboard.VIDPID.pidInt, 1);
            // open path as file
            keyboardFD = open(keyboardInputPath, O_RDONLY);  
            
            continue;
        }
        else {
            // loop through output
            for(long unsigned int i = 0; i < readDevice / sizeof(struct input_event); i++) {
                // if input is type key check values
                if(inputEvent[i].type == EV_KEY) {
                    switch(inputEvent[i].value) {
                    case 0:
                        // if output is keyup do keyup function
                        keyUp(inputEvent[i].code);

                        break;
                    case 1:
                        // if output is keydown do keydown function
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


int checkVM(char* VM) {
    // set return value to 0
    int VMOn = 0;
    // get domain by name
    virDomainPtr domain = virDomainLookupByName(conn, VM);
    // if domain doesnt exist return with exit code
    if(!domain) return -1;
    
    // create info variable
    virDomainInfo info;
    // get domain info into variable
    virDomainGetInfo(domain, &info);
    // set vmon to running state
    VMOn = info.state == VIR_DOMAIN_RUNNING; 
    
    // free domain memory
    virDomainFree(domain);

    // return if vm is on or off
    return VMOn;
}

void toggleVM(char* VM) {
    // merge pwd with /VMToggle.sh
    char* scriptPath = strmerge(pwd, "/VMToggle.sh");

    // merge string with either " off " or " on "
    char* scriptPathWithOnOff = strmerge(scriptPath, hasVM ? " off " : " on ");
    // free previous string
    free(scriptPath);

    // merge string with vm name
    char* scriptPathWithArgs = strmerge(scriptPathWithOnOff, VM);
    // free previous string
    free(scriptPathWithOnOff);

    // merge " | sudo -S " with string
    char* scriptPathWithSudo = strmerge("\" | sudo -S ", scriptPathWithArgs);
    // free previous string
    free(scriptPathWithArgs);

    // merge pass with string
    char* scriptPathWithPass = strmerge(sysPass, scriptPathWithSudo);
    // free previous string
    free(scriptPathWithSudo);

    // merge echo with string
    char* scriptPathWithEcho = strmerge("echo \"", scriptPathWithPass);
    // free previous string
    free(scriptPathWithPass);

    // execute command
    system(scriptPathWithEcho);
    // free previous string
    free(scriptPathWithEcho);
}

void* vmThread(void* threadArgs) {
    // connect to qemu
    conn = virConnectOpen("qemu:///system");

    // loop till exit signal is given
    while(!exitWave) {
        // check if vm is on
        int ret = checkVM(vmName);

        // if return value is valid do
        if(ret >= 0) {
            // set has vm to vm on off state
            hasVM = ret;

            if(rAltHeld && keyQueue <= 1) {
                rAltHeldTime++; // increment raltheldtime by one
            
                if(rAltHeldTime == 25) { // ralt held for 2.5s
                    // toggle vm on/off
                    toggleVM(vmName);
                }
            }
            else {
                rAltHeldTime = 0;
            }
        } // else if invalid reconnect and try again
        else {
            // attempt to reconnect
            virConnectClose(conn);
            // sleep for 100ms
            usleep(100 * 1000);
            // reconnect
            conn = virConnectOpen("qemu:///system");
        }
        
        // sleep for 100ms
        usleep(100 * 1000);
    }

    // close connection then exit
    virConnectClose(conn);

    return NULL;
}

void* initExtras(void* threadArgs) {
    // create thread id variables
    pthread_t keyboardCheckerThread_id;
    pthread_t mouseCheckerThread_id;
    pthread_t vmCheckerThread_id;

    // create threads
    pthread_create(&keyboardCheckerThread_id, NULL, keyboardKeyListener, NULL);
    pthread_create(&mouseCheckerThread_id, NULL, mouseButtonListener, NULL);
    pthread_create(&vmCheckerThread_id, NULL, vmThread, NULL);

    // wait until threads are gone
    pthread_join(keyboardCheckerThread_id, NULL);
    pthread_join(mouseCheckerThread_id, NULL);
    pthread_join(vmCheckerThread_id, NULL);

    return NULL;
}

#endif