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
    // allocate keyboardcols + mousecols to colvalues
    colValues = (struct columnValue*)malloc((keyboard.lighting.matrix.cols + RIVAL600_COLUMNS) * sizeof(struct columnValue));
    // set colvaluessize to 0
    int colValuesSize = 0;

    // setup colours
    struct columnValue colValue;
    // set to null to get automatically
    colValue.rows = NULL;
    // set hue to minhue
    colValue.hue = minHue;
    // set reverse to false
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
    // loop till exit signal
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

        // loop through cols and rows
        for(int i = 0; i < keyboard.lighting.matrix.cols * keyboard.lighting.matrix.rows; i++) {
            // if key is pressed and less than 11 decrement
            if(pressedKeys[i] > 0 && pressedKeys[i] <= 10) {
                pressedKeys[i]--;
            }
        }

        // loop through mouse leds
        for(int i = 0; i < RIVAL600_LEDS; i++) {
            // if button is pressed and less than 11 decrement
            if(pressedButtons[i] > 0 && pressedButtons[i] <= 10) {
                pressedButtons[i]--;
            }
        }

        // wait x amount of microseconds then loop
        usleep(speed * 1000);
    }
    
    // return null
    return NULL;
}

void* wave(void* threadArgs) {
    // init hsv saturation and value to 1
    struct hsv hsv;
    // set saturation to 1.0
    hsv.saturation = 1.0;
    // set value to 1.0
    hsv.value = 1.0;

    // create rgb value
    struct rgb rgb;

    // while no exit signal
    while(!exitWave) {
        // set column offset to keyboardcols
        int colOffset = keyboard.lighting.matrix.cols;

        // loop through keyboard columns
        for(int col = 0; col < colOffset; col++) {
            // set hue then get rgb from hsv
            hsv.hue = colValues[col].hue;
            
            // get rgb from hsv
            struct rgb rgb = hsv_to_rgb(hsv); 

            // set key lights at column to hsv 
            for(int row = 0; row < keyboard.lighting.matrix.rows; row++) {
                // get key index from col, row
                int idx = key_index_from_2D(&keyboard.lighting, col, row);
                // get keyrgb and keyhsv
                struct rgb keyRGB = rgb;
                struct hsv keyHSV = hsv;
                int keyChanged = 0;

                // if key == scrl lk and vm is on set to red
                if(row == 0 && col == 16 && !hasVM) {
                    // set hue to red
                    keyHSV.hue = 1;
                    // make value dark
                    keyHSV.value = 0.4;
                    
                    // set key changed to 1
                    keyChanged = 1;
                }
                
                // if key is pressed
                if(pressedKeys[idx] > 0) {
                    // set value to value - pressedkeys[key] / 12
                    keyHSV.value -= (double)pressedKeys[idx] / 12;
                    // if value < 0 set to 0
                    keyHSV.value = (keyHSV.value < 0.0 ? 0.0 : keyHSV.value);

                    // set key changed to 1
                    keyChanged = 1;
                }

                // set keyrgb to rgb from keyhsv if key is changed
                if(keyChanged) keyRGB = hsv_to_rgb(keyHSV); 

                // set key light at index to rgb
                device_lighting_set_led(&keyboard.lighting, idx, keyRGB.red, keyRGB.green, keyRGB.blue);
            }
        }

        // go through mouse rows
        for(int mouseCol = 0; mouseCol < RIVAL600_COLUMNS; mouseCol++) {
            // set hue then get rgb from hsv
            hsv.hue = colValues[colOffset + mouseCol].hue;
            
            // get rgb from hsv
            struct rgb rgb = hsv_to_rgb(hsv);

            // set led lights at column to hsv 
            for(int row = 0; mouseCols[mouseCol][row] != -1; row++) {
                // set buttonrgb to col rgb
                struct rgb buttonRGB = rgb;
                // get led from col and row
                int led = mouseCols[mouseCol][row];

                // if button is pressed set hsv
                if(pressedButtons[led] > 0) {
                    // set buttonhsv to col hsv
                    struct hsv buttonHSV = hsv;

                    // set buttonhsv value to buttonhsv value - pressedbuttons[led] / 12
                    buttonHSV.value -= (double)pressedButtons[led] / 12;
                    // if buttonhsv value < 0 set buttonhsv value to 0
                    buttonHSV.value = (buttonHSV.value < 0.0 ? 0.0 : buttonHSV.value);
                    
                    // set buttonrgb to rgb from hsv
                    buttonRGB = hsv_to_rgb(buttonHSV); 
                }

                // set mouse led at index to rgb
                set_mouse_led(led, buttonRGB);
            }
        }

        // draw keyboard
        keyboard_draw(&keyboard);
        // wait x amount of microseconds
        usleep(framerate * 1000);
    }

    // return null
    return NULL;
}

#endif