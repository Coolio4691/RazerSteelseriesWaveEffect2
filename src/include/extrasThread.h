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


char* executable_name() {
    char* name = malloc(PATH_MAX * sizeof(char));
    
    int fd = open("/proc/self/comm", O_RDONLY);
    read(fd, name, sizeof(name));

    return name;
}

char* get_pwd() {
    char buff[4096];
    ssize_t len = readlink("/proc/self/exe", buff, sizeof(buff) - 1);
    if (len != -1) {
        buff[len] = 0;
        
        char* exeName = executable_name();
        int exeNameLen = strlen(exeName);
        

        char* pathStr = malloc((len + 1 - exeNameLen) * sizeof(char));

        for(int i = 0; i <= len - exeNameLen; i++) {
            pathStr[i] = buff[i];
        }

        pathStr[len + 1 - exeNameLen] = 0;

        free(exeName);
        
        return pathStr;
    }
    
    return "";
}


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
    devicesLocked = !devicesLocked;

    if(keyboardConnected) {
        ioctl(keyboardFD, EVIOCGRAB, devicesLocked);
    }
    if(mouseConnected) {
        ioctl(mouseFD, EVIOCGRAB, devicesLocked);
    }
}

void checkLock() {
    int toLock = 1;
    for(int i = 0; lockShortcut[i] != -1; i++) {
        if(!lockShortcutPressed[i]) {
            toLock = 0;
            
            break;
        }
    }

    if(!toLock) return;

    lockReq = 1;
}


void keyDown(int key) {
    if(!devicesLocked) { 
        if(key == 100) { // if key == ralt 
            rAltHeld = 1;
        }
    }
    
    for(int i = 0; lockShortcut[i] != -1; i++) {
        if(lockShortcut[i] == key) {
            lockShortcutPressed[i] = 1;
            
            break;
        }
    }

    keyQueue++;

    // check if key at col, row exists
    struct colrow colrow = col_row_from_key(key);
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

    checkLock();

    for(int i = 0; lockShortcut[i] != -1; i++) {
        if(lockShortcut[i] == key) {
            lockShortcutPressed[i] = 0;
            
            break;
        }
    }

    keyQueue--;

    if(keyQueue <= 0 && lockReq) {
        doLock();

        lockReq = 0;
    }

    // check if col, row exists
    struct colrow colrow = col_row_from_key(key);
    if(colrow.col <= -1 && colrow.row <= -1) return;

    // set key at col, row to 10 
    pressedKeys[key_index_from_2D(&keyboard.lighting, colrow.col, colrow.row)] = 10;
}


void mouseDown(int button) {
    keyQueue++;

    // check if led exists
    int led = led_from_button(button);
    if(led <= -1) return;

    // set button at led to 11
    pressedButtons[led] = 11;
}

void mouseUp(int button) {
    keyQueue--;

    if(keyQueue <= 0 && lockReq) {
        doLock();

        lockReq = 0;
    }

    // check if led exists
    int led = led_from_button(button);
    if(led <= -1) return;

    // set button at led to 10
    pressedButtons[led] = 10;
}


void* mouseButtonListener(void* threadArgs) {
    mouseInputPath = find_event(RIVAL600_VID, RIVAL600_PID, 0);

    mouseFD = open(mouseInputPath, O_RDONLY);

    while(!exitWave) {
        mouseConnected = 1;

        int readDevice = read(mouseFD, inputEventMouse, sizeof(struct input_event) * 64);

        if (readDevice < (int) sizeof(struct input_event)) {
            mouseConnected = 0;

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
    keyboardFD = open(keyboardInputPath, O_RDONLY);

    while(!exitWave) {
        keyboardConnected = 1;
        int readDevice = read(keyboardFD, inputEvent, sizeof(struct input_event) * 64);

        if (readDevice < (int) sizeof(struct input_event)) {
            keyboardConnected = 0;

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


int checkVM(char* VM) {
    virDomainPtr *domains;

    int VMOn = 0;
    unsigned int flags = VIR_CONNECT_LIST_DOMAINS_ACTIVE;
    
    int ret = virConnectListAllDomains(conn, &domains, flags);
    if(ret < 0) return -1;

    for(size_t i = 0; i < ret; i++) {
        // only check if windows is on if not found
        if(!VMOn) {
            // set haswindows to 1 then loop and free the rest of the domains
            if(strcmp(virDomainGetName(domains[i]), VM) == 0) {
                VMOn = 1;
            }
        }
        
        // free domain memory
        virDomainFree(domains[i]);
    }

    // free memory of domains variable
    free(domains);

    return VMOn;
}

void toggleVM(char* VM) {
    int vmOn = checkVM(VM);

    printf("%s: %s\n", VM, vmOn ? "on" : "off");
    char* pwd = get_pwd();
    

    char* scriptPath = strmerge(pwd, "/VMToggle.sh");
    free(pwd);


    char* scriptPathWithOnOff = strmerge(scriptPath, vmOn ? " off " : " on ");
    free(scriptPath);


    char* scriptPathWithArgs = strmerge(scriptPathWithOnOff, VM);
    free(scriptPathWithOnOff);


    char* scriptPathWithSudo = strmerge("\" | sudo -S ", scriptPathWithArgs);
    free(scriptPathWithArgs);


    char* scriptPathWithPass = strmerge(sysPass, scriptPathWithSudo);
    free(scriptPathWithSudo);


    char* scriptPathWithEcho = strmerge("echo \"", scriptPathWithPass);
    free(scriptPathWithPass);


    system(scriptPathWithEcho);

    free(scriptPathWithEcho);
}

void* vmThread(void* threadArgs) {
    conn = virConnectOpen("qemu:///system");
    unsigned int flags = VIR_CONNECT_LIST_DOMAINS_ACTIVE;

    virDomainPtr *domains;

    while(!exitWave) {
        int ret = checkVM(vmName);

        if(ret >= 0) {
            hasVM = ret;

            if(rAltHeld && keyQueue <= 1) {
                rAltHeldTime++;
            
                if(rAltHeldTime == 25) { // 2.5s
                    toggleVM(vmName);
                }
            }
            else {
                rAltHeldTime = 0;
            }
        }
        else {
            // attempt to reconnect
            virConnectClose(conn);
            // sleep for 100ms
            usleep(100 * 1000);
            // reconnect
            virConnectPtr conn = virConnectOpen("qemu:///system");
        }
        
        // sleep for 100ms
        usleep(100 * 1000);
    }

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