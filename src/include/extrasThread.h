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

size_t split(char* str, char delim, char ***array) {
    // set elements to 1
    size_t elements = 1;
    // get string length
    size_t stringLen = strlen(str);

    // loop through string
    for(size_t i = 0; i < stringLen; i++) {
        // if string at index is delim up elements count
        if(str[i] == delim) {
            elements++;
        }
    }

    // allocate elements 
    *array = (char**)calloc(sizeof(char*), elements);
    
    // allocate string length + null terminator to buffer for max string 
    char* buf = (char*)calloc(sizeof(char), stringLen + 1);
    // set buf len to 0
    size_t bufLen = 0;
    
    // current element index
    size_t curElement = 0;
    // loop through string
    for(size_t i = 0; i < stringLen; i++) {
        // if string at index is delim
        if(str[i] == delim) {
            // set buf at buflen to null terminator
            buf[bufLen] = 0;
 
            // allocate string to array[curelement] 
            (*array)[curElement] = (char*)calloc(sizeof(char), bufLen + 1);
            // copy buf toarray[curelement] 
            memcpy((*array)[curElement++], buf, bufLen + 1);
            
            // set buflen to 0
            bufLen = 0;

            continue;
        }

        // append char to buf and up buflen
        buf[bufLen++] = str[i];
    }           

    // set buf at buflen to 0
    buf[bufLen] = 0;

    // allocate buflen + nullterminator to array[curelement]
    (*array)[curElement] = (char*)calloc(sizeof(char), bufLen + 1);
    // copy contents of buf to array[curelement]
    memcpy((*array)[curElement], buf, bufLen + 1);

    // free buf
    free(buf);
    
    // return amount of elements
    return elements;
}

size_t getLine(FILE* file, char** out) {
    // allocate a max of 4096 chars per line and set outlen to 0
    *out = (char*)calloc(sizeof(char), 4096);
    size_t outLen = 0;

    // get buffer char
    char c;
    while((c = getc(file))) {
        // check if end of file or new line
        if(feof(file) || c == '\n') {
            // set output at outputlen to null terminator
            (*out)[outLen] = 0;
            
            break;
        }

        // append char to output
        (*out)[outLen++] = c;
    }

    // return outputlen
    return outLen;
}

char* find_event(char* vendorId, char* productId, int keyboard) {       
    FILE* devicesFile;
 
    char* eventStr; 
    char* deviceListPath = "/proc/bus/input/devices";
    int deviceFound = 0;
    
    // open file into variable
    devicesFile = fopen(deviceListPath, "r");

    // allocate "Vendor=vendorid Product=productid"
    char* searchStr = (char*)calloc(sizeof(char), 25);
    // set to vendor=vendorid product=productid
    sprintf(searchStr, "Vendor=%s Product=%s", vendorId, productId);
    
    // file buffer for current string
    char* fileBuffer;
    
    while(1) {
        // get line from file
        getLine(devicesFile, &fileBuffer);

        // if device is not found look for event with vid, pid
        if(!deviceFound) {
            // if search str is in string set device found to true
            char* substr = strstr(fileBuffer, searchStr);

            if(substr != NULL) {
                // set device found to true
                deviceFound = 1;

                // free searchstr and set to event
                free(searchStr);
                
                // allocate "event" + nullterminator
                searchStr = (char*)calloc(sizeof(char), 6);
                strcat(searchStr, "event\0");
            }
        }
        // if device is found look if its the right device and get its event
        else {
            // check if filebuffer contains leds
            char* substr = strstr(fileBuffer, "leds");
            if(substr != NULL) {
                // if contains mouse set found to false and search again
                deviceFound = 0;
            }

            // check if filebuffer contains kbd if keyboard is false
            if(!keyboard) {
                substr = strstr(fileBuffer, "kbd");
                if(substr != NULL) {
                    // if contains mouse set found to false and search again
                    deviceFound = 0;
                }
            }
            // check if filebuffer contains mouse if keyboard is true
            else {
                substr = strstr(fileBuffer, "mouse");
                if(substr != NULL) {
                    // if contains mouse set found to false and search again
                    deviceFound = 0;
                }
            }

            if(!deviceFound) {
                // free search str
                free(searchStr);
                // allocate memory for "Vendor=vendorid Product=productid"
                searchStr = (char*)calloc(sizeof(char), 25);

                // set searchstr to Vendor=vendorid Product=productid
                sprintf(searchStr, "Vendor=%s Product=%s", vendorId, productId);
            }

            // get substr from line
            substr = strstr(fileBuffer, searchStr);

            // if substr exists
            if(substr != NULL) {
                // split substr into splitstr
                char** splitStr;
                size_t elements = split(substr, ' ', &splitStr);

                // get length of string at index 0
                size_t stringLen = strlen(splitStr[0]);
                // allocate 11("/dev/input/") + length of string + 1 for null terminator
                eventStr = (char*)calloc(sizeof(char), stringLen + 12);
                strcpy(eventStr, "/dev/input/");

                // copy contents of substr.split(' ')[0] to eventstr
                memcpy(eventStr + 11, splitStr[0], stringLen);
                // set eventstr at the end to null terminator
                eventStr[stringLen + 11] = 0;

                // free memory of substrings in splitstr
                for(int i = 0; i < elements; i++) {
                    free(splitStr[i]);
                }
                // free splitstr
                free(splitStr);
                // free filebuffer
                free(fileBuffer);

                // exit out of loop
                break;
            }
        }

        // free file buffer
        free(fileBuffer);

        // if end of file break;
        if(feof(devicesFile)) break;
        continue;
    }

    // close file
    fclose(devicesFile);
    // free search str
    free(searchStr);

    return eventStr;
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
    mouseInputPath = find_event(RIVAL600_VIDSTR, RIVAL600_PIDSTR, 0);
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
            mouseInputPath = find_event(RIVAL600_VIDSTR, RIVAL600_PIDSTR, 0);
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
    keyboardInputPath = find_event(keyboard.VIDPID.vid, keyboard.VIDPID.pid, 1);
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
            keyboardInputPath = find_event(keyboard.VIDPID.vid, keyboard.VIDPID.pid, 1);
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