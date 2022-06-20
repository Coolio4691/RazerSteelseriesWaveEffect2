#ifndef __WAVE_H__
#define __WAVE_H__
#include "globals.h"
#include "keyboard.h"
#include "mouse.h"

static int minHue = 240;
static int maxHue = 284; 
static int colorStep = 2;
static int colorBackStep = 6;

static int framerate = 25; // draws keyboard and mouse every 25ms
static int speed = 50; // updates values every 50ms

struct columnValue {
    int* rows;
    int hue;
    int reverse;
};

static struct columnValue* colValues;

void wave_init() {
    // allocate memory for columns
    colValues = (struct columnValue*)malloc((keyboard.lighting.matrix.cols + RIVAL600_COLUMNS) * sizeof(struct columnValue));
    int colValuesSize = 0;

    // setup colours
    
    struct columnValue colValue;
    colValue.rows = NULL; // set to null to get automatically
    colValue.hue = minHue;
    colValue.reverse = 0;

    // initialize column colours
    for(int col = 0; col < keyboard.lighting.matrix.cols + RIVAL600_COLUMNS; col++) {
        // set column value to current hue + reversed state
        colValues[colValuesSize++] = colValue;
    
        //increment or decrement hue based on reverse
        colValue.hue = (colValue.reverse ? colValue.hue - colorBackStep : colValue.hue + colorStep);

        // if hue > maxhue set hue to maxhue then set to reverse
        if(colValue.hue > maxHue) {
            colValue.hue = maxHue;
            colValue.reverse = 1;
        }
        // if hue < minhue set hue to minhue then set to not reverse
        else if(colValue.hue < minHue) {
            colValue.hue = minHue;
            colValue.reverse = 0;
        }
    }
    // end setup colours
}

void* wave_values_loop(void* threadArgs) {
    while(!exitWave) {
        // go through all columns
        for(int col = 0; col < keyboard.lighting.matrix.cols + RIVAL600_COLUMNS; col++) {
            // get column value pointer
            struct columnValue* val = &colValues[col];
        
            // decrement or increment hue based on reversed 
            val->hue = (val->reverse ? val->hue - colorBackStep : val->hue + colorStep);

            // if hue > maxhue set columnvalue to reverse and set hue to maxhue
            if(val->hue > maxHue) {
                val->hue = maxHue;
                val->reverse = 1;
            }
            // if hue < minhue set columnvalue to not reverse and set hue to minhue
            else if(val->hue < minHue) {
                val->hue = minHue;
                val->reverse = 0;
            }
        }

        for(int i = 0; i < keyboard.lighting.matrix.cols * keyboard.lighting.matrix.rows; i++) {
            if(pressedKeys[i] > 0 && pressedKeys[i] <= 10) {
                pressedKeys[i] -= 1;
            }
        }

        for(int i = 0; i < RIVAL600_LEDS; i++) {
            if(pressedButtons[i] > 0 && pressedButtons[i] <= 10) {
                pressedButtons[i] -= 1;
            }
        }

        // wait x amount of microseconds then loop
        usleep(speed * 1000);
    }
    
    return NULL;
}

void* wave(void* threadArgs) {
    // init hsv saturation and value to 1
    struct hsv hsv;
    hsv.saturation = 1.0;
    hsv.value = 1.0;
    // create rgb value
    struct rgb rgb;

    while(!exitWave) {
        int colOffset = keyboard.lighting.matrix.cols;

        for(int col = 0; col < colOffset; col++) {
            // set hue then get rgb from hsv
            hsv.hue = colValues[col].hue;
            struct rgb rgb = hsv_to_rgb(hsv); 

            // set key lights at column to hsv 
            for(int row = 0; row < keyboard.lighting.matrix.rows; row++) {
                int idx = key_index_from_2D(&keyboard.lighting, col, row);
                struct rgb keyRGB = rgb;
                
                if(pressedKeys[idx] > 0) {
                    struct hsv keyHSV = hsv;

                    keyHSV.value -= (double)pressedKeys[idx] / 12;
                    keyHSV.value = (keyHSV.value > 1.0 ? 1.0 : keyHSV.value);
                    keyHSV.value = (keyHSV.value < 0.0 ? 0.0 : keyHSV.value);
                    keyRGB = hsv_to_rgb(keyHSV); 
                }

                keyboard_set_key_light(&keyboard, idx, keyRGB.red, keyRGB.green, keyRGB.blue);
            }
        }

        // go through mouse rows
        for(int mouseCol = 0; mouseCol < RIVAL600_COLUMNS; mouseCol++) {
            // set hue then get rgb from hsv
            hsv.hue = colValues[colOffset + mouseCol].hue;
            struct rgb rgb = hsv_to_rgb(hsv);

            // set led lights at column to hsv 
            for(int row = 0; mouseCols[mouseCol][row] != -1; row++) {
                struct rgb buttonRGB = rgb;
                int led = mouseCols[mouseCol][row];

                if(pressedButtons[led] > 0) {
                    struct hsv buttonHSV = hsv;

                    buttonHSV.value -= (double)pressedButtons[led] / 12;
                    buttonHSV.value = (buttonHSV.value > 1.0 ? 1.0 : buttonHSV.value);
                    buttonHSV.value = (buttonHSV.value < 0.0 ? 0.0 : buttonHSV.value);
                    buttonRGB = hsv_to_rgb(buttonHSV); 

                }

                set_mouse_led(led, buttonRGB);
            }
        }

        // draw keyboard
        keyboard_draw(&keyboard);
        // wait x amount of microseconds
        usleep(framerate * 1000);
    }

    return NULL;
}

#endif