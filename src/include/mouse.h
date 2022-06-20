#ifndef __MOUSE_H__
#define __MOUSE_H__
#include <hidapi/hidapi.h>
#include <razer/devicematrix.h>
#include "bytes.h"
#include "colour.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

static hid_device* mouse;


#define RIVAL600_VID 0x1038
#define RIVAL600_PID 0x1724
#define RIVAL600_LEDS 8
#define RIVAL600_COLUMNS 3


#define RIVAL600_WHEEL 0x00
#define RIVAL600_LOGO 0x01
#define RIVAL600_TOPLEFT 0x02
#define RIVAL600_TOPRIGHT 0x03
#define RIVAL600_LEFT 0x04
#define RIVAL600_RIGHT 0x05
#define RIVAL600_BOTTOMLEFT 0x06
#define RIVAL600_BOTTOMRIGHT 0x07

#define RIVAL600_NONE -0x01

static const uint8_t LED_LOGO = 0x06;
static const uint8_t LED_WHEEL = 0x01;

static const uint8_t CMD_LED_COLOR = 0x05;
static const uint8_t CMD_SAVE = 0x09;

// list of led indexes for the column
static int* mouseCols[RIVAL600_COLUMNS];

#define HEADER_LENGTH 28
#define BODY_LENGTH 7
#define REPEAT_INDEX 22
#define TRIGGERS_INDEX 23
#define COLOURS_COUNT_INDEX 27
#define DURATION_INDEX 6
#define DURATION_LENGTH 2
#define DEFAULT_DURATION 1000

// contains the rows for the column

void mouse_init() {
    mouse = hid_open(RIVAL600_VID, RIVAL600_PID, NULL);
    
    // left section
    mouseCols[0] = malloc(4 * sizeof(int));
    mouseCols[0][0] = RIVAL600_TOPLEFT;
    mouseCols[0][1] = RIVAL600_LEFT;
    mouseCols[0][2] = RIVAL600_BOTTOMLEFT;
    mouseCols[0][3] = RIVAL600_NONE;

    // middle section
    mouseCols[1] = malloc(3 * sizeof(int));
    mouseCols[1][0] = RIVAL600_WHEEL;
    mouseCols[1][1] = RIVAL600_LOGO;
    mouseCols[1][2] = RIVAL600_NONE;

    // right section
    mouseCols[2] = malloc(4 * sizeof(int));
    mouseCols[2][0] = RIVAL600_TOPRIGHT;
    mouseCols[2][1] = RIVAL600_RIGHT;
    mouseCols[2][2] = RIVAL600_BOTTOMRIGHT;
    mouseCols[2][3] = RIVAL600_NONE;
}


void set_mouse_led(uint8_t led, struct rgb rgb) {
    if(led >= RIVAL600_LEDS || led < 0) return; 

    uint8_t* header = malloc(HEADER_LENGTH * sizeof(uint8_t));
    for(int i = 0; i < HEADER_LENGTH; i++) {
        header[i] = 0.0f;
    } 
    
    header[REPEAT_INDEX] = 0x01; 
    header[COLOURS_COUNT_INDEX] = 0x01; // gradient count(1)

    // led id indices
    header[0] = led; 
    header[5] = led;

    uint8_t* bytearray = uint_to_little_endian_bytearray(DEFAULT_DURATION, DURATION_LENGTH);
    
    for(int i = 0; i < DURATION_LENGTH; i++) {
        header[DURATION_INDEX + i] = bytearray[i];
    } 

    free(bytearray);

    uint8_t* body = malloc(BODY_LENGTH * sizeof(uint8_t)); // rgb rgb offset

    // rgb
    body[0] = rgb.red;
    body[1] = rgb.green;
    body[2] = rgb.blue;
    // rgb
    body[3] = rgb.red;
    body[4] = rgb.green;
    body[5] = rgb.blue;
    // offset
    body[6] = 0;

    uint8_t* data;
    uint8_t* payload;
    int dataLen;
    int payloadLen;

    merge_bytes(header, HEADER_LENGTH, body, BODY_LENGTH, &data, &dataLen);
    free(header);
    free(body);

    int reportLen = 3;
    uint8_t* report = malloc(reportLen * sizeof(uint8_t));
    report[0] = 0x00; // reportid
    report[1] = CMD_LED_COLOR; // command
    report[2] = 0x00; // command

    merge_bytes(report, reportLen, data, dataLen, &payload, &payloadLen);
    free(data);
    free(report);

    hid_send_feature_report(mouse, payload, payloadLen * sizeof(uint8_t));
    free(payload);
}


void mouse_exit() {
    hid_close(mouse);
}

#endif