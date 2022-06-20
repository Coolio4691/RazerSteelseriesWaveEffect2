#ifndef __KEYS_H__
#define __KEYS_H__

#include "mouse.h"

struct colrow {
    int col;
    int row;
};

struct keyMap {
    int key;
    struct colrow value;  
};

struct buttonMap {
    int button;
    int value;  
};

static struct keyMap keyPositions[] = {
    // first row
    { 1, { 1, 0} }, // esc

    { 59, { 3, 0} }, // f1
    { 60, { 4, 0} }, // f2
    { 61, { 5, 0} }, // f3
    { 62, { 6, 0} }, // f4

    { 63, { 7, 0} }, // f5
    { 64, { 8, 0} }, // f6
    { 65, { 9, 0} }, // f7
    { 66, { 10, 0} }, // f8

    { 67, { 11, 0} }, // f9
    { 68, { 12, 0} }, // f10
    { 87, { 13, 0} }, // f11
    { 88, { 14, 0} }, // f12

    { 99, { 15, 0} }, // prt sc
    { 70, { 16, 0} }, // scrl lk
    { 119, { 17, 0} }, // pause

    // second row
    { 41, { 1, 1 } }, // `
    { 2, { 2, 1 } }, // 1
    { 3, { 3, 1 } }, // 2
    { 4, { 4, 1 } }, // 3
    { 5, { 5, 1 } }, // 4
    { 6, { 6, 1 } }, // 5
    { 7, { 7, 1 } }, // 6
    { 8, { 8, 1 } }, // 7
    { 9, { 9, 1 } }, // 8
    { 10, { 10, 1 } }, // 9
    { 11, { 11, 1 } }, // 0
    { 12, { 12, 1 } }, // -
    { 13, { 13, 1 } }, // =
    { 14, { 14, 1 } }, // backspace

    { 110, { 15, 1 } }, // ins
    { 102, { 16, 1 } }, // home
    { 104, { 17, 1 } }, // page up

    // third row
    { 15, { 1, 2 } }, // tab
    { 16, { 2, 2 } }, // q
    { 17, { 3, 2 } }, // w
    { 18, { 4, 2 } }, // e
    { 19, { 5, 2 } }, // r
    { 20, { 6, 2 } }, // t
    { 21, { 7, 2 } }, // y
    { 22, { 8, 2 } }, // u
    { 23, { 9, 2 } }, // i
    { 24, { 10, 2 } }, // o
    { 25, { 11, 2 } }, // p
    { 26, { 12, 2 } }, // [
    { 27, { 13, 2 } }, // ]
    { 43, { 14, 2 } }, // |
    { 111, { 15, 2 } }, // del
    { 107, { 16, 2 } }, // end
    { 109, { 17, 2 } }, // page down

    // fourth row
    { 58, { 1, 3 } }, // caps lock
    { 30, { 2, 3 } }, // a
    { 31, { 3, 3 } }, // s
    { 32, { 4, 3 } }, // d
    { 33, { 5, 3 } }, // f
    { 34, { 6, 3 } }, // g
    { 35, { 7, 3 } }, // h
    { 36, { 8, 3 } }, // j
    { 37, { 9, 3 } }, // k
    { 38, { 10, 3 } }, // l
    { 39, { 11, 3 } }, // ;
    { 40, { 12, 3 } }, // '
    { 28, { 14, 3 } }, // enter

    // fifth row
    { 42, { 1, 4 } }, // lshift
    { 44, { 3, 4 } }, // z
    { 45, { 4, 4 } }, // x
    { 46, { 5, 4 } }, // c
    { 47, { 6, 4 } }, // v
    { 48, { 7, 4 } }, // b
    { 49, { 8, 4 } }, // n
    { 50, { 9, 4 } }, // m
    { 51, { 10, 4 } }, // ,
    { 52, { 11, 4 } }, // .
    { 53, { 12, 4 } }, // /
    { 54, { 14, 4 } }, // rshift

    { 103, { 16, 4 } }, // up arrow

    // sixth row
    { 29, { 1, 5 } }, // lctrl
    { 125, { 2, 5 } }, // super
    { 56, { 3, 5 } }, // lalt
    // no key for space
    { 100, { 11, 5 } }, // ralt
    // fn doesnt have an led
    { 127, { 13, 5 } }, // menu
    { 97, { 14, 5 } }, // rctrl

    { 105, { 15, 5 } }, // left arrow
    { 108, { 16, 5 } }, // left arrow
    { 106, { 17, 5 } }, // left arrow

    // none
    { -1, { -1, -1 } } // none key to show the end of the positions
}; 

static struct buttonMap buttonPositions[] = {

    // mouse
    { 272, RIVAL600_TOPLEFT },
    { 273, RIVAL600_TOPRIGHT },
    { 274, RIVAL600_WHEEL },
    
    // none
    { -1, -1 }
};

struct colrow col_row_from_key(int key) {
    struct colrow toReturn = { -1, -1 };

    for(int i = 0; keyPositions[i].key != -1; i++) {
        if(keyPositions[i].key != key) continue;

        toReturn = keyPositions[i].value;        
    }

    return toReturn;
}

int led_from_button(int button) {
    int toReturn = -1;

    for(int i = 0; buttonPositions[i].button != -1; i++) {
        if(buttonPositions[i].button != button) continue;

        toReturn = buttonPositions[i].value;        
    }

    return toReturn;
}

#endif