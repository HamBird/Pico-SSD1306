// base code for SSD1306 https://github.com/raspberrypi/pico-examples/blob/master/i2c/ssd1306_i2c/ssd1306_i2c.c
// modified to not need a passed in buffer and struct, easier coordinates to print onto SSD1306

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include "pico/cyw43_arch.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "pico/binary_info.h"

// I2C defines
// This example will use I2C0 on GPIO8 (SDA) and GPIO9 (SCL) running at 400KHz.
// Pins can be changed, see the GPIO function select table in the datasheet for information on GPIO assignments
#define I2C_PORT i2c0


// Define the size of the display we have attached. This can vary, make sure you
// have the right size defined or the output will look rather odd!
// Code has been tested on 128x32 and 128x64 OLED displays
#define SSD1306_HEIGHT 64
#define SSD1306_WIDTH 128

#define SSD1306_I2C_ADDR _u(0x3C)

// 400 is usual, but often these can be overclocked to improve display response.
// Tested at 1000 on both 32 and 84 pixel height devices and it worked.
#define SSD1306_I2C_CLK 400
//#define SSD1306_I2C_CLK 1000

// commands (see datasheet)
#define SSD1306_SET_MEM_MODE _u(0x20)
#define SSD1306_SET_COL_ADDR _u(0x21)
#define SSD1306_SET_PAGE_ADDR _u(0x22)
#define SSD1306_SET_HORIZ_SCROLL _u(0x26)
#define SSD1306_SET_SCROLL _u(0x2E)

#define SSD1306_SET_DISP_START_LINE _u(0x40)

#define SSD1306_SET_CONTRAST _u(0x81)
#define SSD1306_SET_CHARGE_PUMP _u(0x8D)

#define SSD1306_SET_SEG_REMAP _u(0xA0)
#define SSD1306_SET_ENTIRE_ON _u(0xA4)
#define SSD1306_SET_ALL_ON _u(0xA5)
#define SSD1306_SET_NORM_DISP _u(0xA6)
#define SSD1306_SET_INV_DISP _u(0xA7)
#define SSD1306_SET_MUX_RATIO _u(0xA8)
#define SSD1306_SET_DISP _u(0xAE)
#define SSD1306_SET_COM_OUT_DIR _u(0xC0)
#define SSD1306_SET_COM_OUT_DIR_FLIP _u(0xC0)

#define SSD1306_SET_DISP_OFFSET _u(0xD3)
#define SSD1306_SET_DISP_CLK_DIV _u(0xD5)
#define SSD1306_SET_PRECHARGE _u(0xD9)
#define SSD1306_SET_COM_PIN_CFG _u(0xDA)
#define SSD1306_SET_VCOM_DESEL _u(0xDB)

#define SSD1306_PAGE_HEIGHT _u(8)
#define SSD1306_NUM_PAGES (SSD1306_HEIGHT / SSD1306_PAGE_HEIGHT)
#define SSD1306_BUF_LEN (SSD1306_NUM_PAGES * SSD1306_WIDTH)

#define SSD1306_WRITE_MODE _u(0xFE)
#define SSD1306_READ_MODE _u(0xFF)

static uint8_t dispBuff[SSD1306_BUF_LEN];
// Static global buffer for display data
// static uint8_t display_buffer[SSD1306_WIDTH * SSD1306_HEIGHT / 8];


struct render_area {
    uint8_t start_col;
    uint8_t end_col;
    uint8_t start_page;
    uint8_t end_page;

    int buflen;
};

#define SSD1306_START_COL 0
#define SSD1306_END_COL (SSD1306_WIDTH - 1)
#define SSD1306_START_PAGE 0
#define SSD1306_END_PAGE (SSD1306_NUM_PAGES -1)
#define SSD1306_BUFLEN ((SSD1306_END_COL - SSD1306_START_COL + 1) * (SSD1306_END_PAGE - SSD1306_START_PAGE + 1))


struct render_area dispArea = {
    start_col: 0,
    end_col: SSD1306_WIDTH - 1,
    start_page: 0,
    end_page: SSD1306_NUM_PAGES - 1,
};

static uint8_t font[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Nothing
    0x78, 0x14, 0x12, 0x11, 0x12, 0x14, 0x78, 0x00, //A
    0x7f, 0x49, 0x49, 0x49, 0x49, 0x49, 0x7f, 0x00, //B
    0x7e, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x00, //C
    0x7f, 0x41, 0x41, 0x41, 0x41, 0x41, 0x7e, 0x00, //D
    0x7f, 0x49, 0x49, 0x49, 0x49, 0x49, 0x49, 0x00, //E
    0x7f, 0x09, 0x09, 0x09, 0x09, 0x01, 0x01, 0x00, //F
    0x7f, 0x41, 0x41, 0x41, 0x51, 0x51, 0x73, 0x00, //G
    0x7f, 0x08, 0x08, 0x08, 0x08, 0x08, 0x7f, 0x00, //H
    0x00, 0x00, 0x00, 0x7f, 0x00, 0x00, 0x00, 0x00, //I
    0x21, 0x41, 0x41, 0x3f, 0x01, 0x01, 0x01, 0x00, //J
    0x00, 0x7f, 0x08, 0x08, 0x14, 0x22, 0x41, 0x00, //K
    0x7f, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x00, //L
    0x7f, 0x02, 0x04, 0x08, 0x04, 0x02, 0x7f, 0x00, //M
    0x7f, 0x02, 0x04, 0x08, 0x10, 0x20, 0x7f, 0x00, //N
    0x3e, 0x41, 0x41, 0x41, 0x41, 0x41, 0x3e, 0x00, //O
    0x7f, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0e, 0x00, //P
    0x3e, 0x41, 0x41, 0x49, 0x51, 0x61, 0x7e, 0x00, //Q
    0x7f, 0x11, 0x11, 0x11, 0x31, 0x51, 0x0e, 0x00, //R
    0x46, 0x49, 0x49, 0x49, 0x49, 0x30, 0x00, 0x00, //S
    0x01, 0x01, 0x01, 0x7f, 0x01, 0x01, 0x01, 0x00, //T
    0x3f, 0x40, 0x40, 0x40, 0x40, 0x40, 0x3f, 0x00, //U
    0x0f, 0x10, 0x20, 0x40, 0x20, 0x10, 0x0f, 0x00, //V
    0x7f, 0x20, 0x10, 0x08, 0x10, 0x20, 0x7f, 0x00, //W
    0x00, 0x41, 0x22, 0x14, 0x14, 0x22, 0x41, 0x00, //X
    0x01, 0x02, 0x04, 0x78, 0x04, 0x02, 0x01, 0x00, //Y
    0x41, 0x61, 0x59, 0x45, 0x43, 0x41, 0x00, 0x00, //Z
    0x3e, 0x41, 0x41, 0x49, 0x41, 0x41, 0x3e, 0x00, //0
    0x00, 0x00, 0x42, 0x7f, 0x40, 0x00, 0x00, 0x00, //1
    0x30, 0x49, 0x49, 0x49, 0x49, 0x46, 0x00, 0x00, //2
    0x49, 0x49, 0x49, 0x49, 0x49, 0x49, 0x36, 0x00, //3
    0x3f, 0x20, 0x20, 0x78, 0x20, 0x20, 0x00, 0x00, //4
    0x4f, 0x49, 0x49, 0x49, 0x49, 0x30, 0x00, 0x00, //5
    0x3f, 0x48, 0x48, 0x48, 0x48, 0x48, 0x30, 0x00, //6
    0x01, 0x01, 0x01, 0x61, 0x31, 0x0d, 0x03, 0x00, //7
    0x36, 0x49, 0x49, 0x49, 0x49, 0x49, 0x36, 0x00, //8
    0x06, 0x09, 0x09, 0x09, 0x09, 0x09, 0x7f, 0x00, //9
};
// Make correct bits for: ! " # $ % & ' ( ) * + , - . /
static uint8_t testFont[] = {
    //0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Nothing
    0x00, 0x00, 0x00, 0x5F, 0x00, 0x00, 0x00, 0x00, //!
    0x00, 0x00, 0x03, 0x00, 0x03, 0x00, 0x00, 0x00, // "

    //0x00, 0x28, 0x28, 0xFE, 0x28, 0xFE, 0x28, 0x28, // #
    0x28, 0xF8, 0x2E, 0xE9, 0x3C, 0x2B, 0x80, 0x00, // # Slanted //

    0x00, 0x44, 0x4A, 0xFF, 0x4A, 0x52, 0x20, 0x00, // $ //

    0x00, 0x60, 0xC6, 0x30, 0xC0, 0x63, 0x60, 0x00, // % //

    0x00, 0x80, 0x65, 0x96, 0x9A, 0x6C, 0x00, 0x00, // & //

    0x00, 0x00, 0x70, 0x60, 0x00, 0x00, 0x00, 0x00, // ' //

    0x00, 0x00, 0x00, 0xC3, 0x3E, 0x00, 0x00, 0x00, // ( //

    0x00, 0x00, 0x00, 0x3C, 0xC3, 0x00, 0x00, 0x00, // ) //

    0x00, 0x00, 0x20, 0x70, 0x20, 0x00, 0x00, 0x00, // * //

    0x00, 0x10, 0x10, 0xFE, 0x10, 0x10, 0x00, 0x00, // + //

    0x00, 0x00, 0x00, 0x30, 0x10, 0x00, 0x00, 0x00, // , //

    0x00, 0x00, 0x80, 0x80, 0x80, 0x00, 0x00, 0x00, // - //

    0x00, 0x00, 0x00, 0x30, 0x30, 0x00, 0x00, 0x00, // . //

    0x00, 0x00, 0xC0, 0x30, 0xC0, 0x30, 0x00, 0x00, // / //

    0x3e, 0x41, 0x41, 0x49, 0x41, 0x41, 0x3e, 0x00, //0
    0x00, 0x00, 0x42, 0x7f, 0x40, 0x00, 0x00, 0x00, //1
    0x30, 0x49, 0x49, 0x49, 0x49, 0x46, 0x00, 0x00, //2
    0x49, 0x49, 0x49, 0x49, 0x49, 0x49, 0x36, 0x00, //3
    0x3f, 0x20, 0x20, 0x78, 0x20, 0x20, 0x00, 0x00, //4
    0x4f, 0x49, 0x49, 0x49, 0x49, 0x30, 0x00, 0x00, //5
    0x3f, 0x48, 0x48, 0x48, 0x48, 0x48, 0x30, 0x00, //6
    0x01, 0x01, 0x01, 0x61, 0x31, 0x0d, 0x03, 0x00, //7
    0x36, 0x49, 0x49, 0x49, 0x49, 0x49, 0x36, 0x00, //8
    0x06, 0x09, 0x09, 0x09, 0x09, 0x09, 0x7f, 0x00, //9

    0x00, 0x00, 0x00, 0xE7, 0x00, 0x00, 0x00, 0x00, // : //

    0x00, 0x00, 0x00, 0xE6, 0x30, 0x00, 0x00, 0x00, // ; //

    0x00, 0x41, 0x63, 0x36, 0x1C, 0x80, 0x00, 0x00, // < //

    0x00, 0x00, 0x24, 0x24, 0x24, 0x24, 0x00, 0x00, // = //

    0x00, 0x80, 0x1C, 0x36, 0x63, 0x41, 0x00, 0x00, // > //

    0x00, 0x00, 0x70, 0x88, 0x88, 0x4D, 0x00, 0x00, // ? //

    0x7C, 0x82, 0xBE, 0xA8, 0xB8, 0x82, 0x7E, 0x00, // @ //

    0x78, 0x14, 0x12, 0x11, 0x12, 0x14, 0x78, 0x00, //A
    0x7f, 0x49, 0x49, 0x49, 0x49, 0x49, 0x7f, 0x00, //B
    0x7e, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x00, //C
    0x7f, 0x41, 0x41, 0x41, 0x41, 0x41, 0x7e, 0x00, //D
    0x7f, 0x49, 0x49, 0x49, 0x49, 0x49, 0x49, 0x00, //E
    0x7f, 0x09, 0x09, 0x09, 0x09, 0x01, 0x01, 0x00, //F
    0x7f, 0x41, 0x41, 0x41, 0x51, 0x51, 0x73, 0x00, //G
    0x7f, 0x08, 0x08, 0x08, 0x08, 0x08, 0x7f, 0x00, //H
    0x00, 0x00, 0x00, 0x7f, 0x00, 0x00, 0x00, 0x00, //I
    0x21, 0x41, 0x41, 0x3f, 0x01, 0x01, 0x01, 0x00, //J
    0x00, 0x7f, 0x08, 0x08, 0x14, 0x22, 0x41, 0x00, //K
    0x7f, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x00, //L
    0x7f, 0x02, 0x04, 0x08, 0x04, 0x02, 0x7f, 0x00, //M
    0x7f, 0x02, 0x04, 0x08, 0x10, 0x20, 0x7f, 0x00, //N
    0x3e, 0x41, 0x41, 0x41, 0x41, 0x41, 0x3e, 0x00, //O
    0x7f, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0e, 0x00, //P
    0x3e, 0x41, 0x41, 0x49, 0x51, 0x61, 0x7e, 0x00, //Q
    0x7f, 0x11, 0x11, 0x11, 0x31, 0x51, 0x0e, 0x00, //R
    0x46, 0x49, 0x49, 0x49, 0x49, 0x30, 0x00, 0x00, //S
    0x01, 0x01, 0x01, 0x7f, 0x01, 0x01, 0x01, 0x00, //T
    0x3f, 0x40, 0x40, 0x40, 0x40, 0x40, 0x3f, 0x00, //U
    0x0f, 0x10, 0x20, 0x40, 0x20, 0x10, 0x0f, 0x00, //V
    0x7f, 0x20, 0x10, 0x08, 0x10, 0x20, 0x7f, 0x00, //W
    0x00, 0x41, 0x22, 0x14, 0x14, 0x22, 0x41, 0x00, //X
    0x01, 0x02, 0x04, 0x78, 0x04, 0x02, 0x01, 0x00, //Y
    0x41, 0x61, 0x59, 0x45, 0x43, 0x41, 0x00, 0x00, //Z

    0x00, 0x00, 0x00, 0x81, 0x81, 0xFF, 0x00, 0x00, // [ //

    0x00, 0x00, 0x30, 0xC0, 0x30, 0xC0, 0x00, 0x00, // backslash //

    0x00, 0x00, 0x00, 0xFF, 0x81, 0x81, 0x00, 0x00, // ] //

    0x00, 0x00, 0x00, 0x30, 0xC0, 0x30, 0x00, 0x00, // ^ //

    0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, // _ //

    0x00, 0x00, 0x00, 0x40, 0x80, 0x00, 0x00, 0x00, // ` //

    0x00, 0x64, 0x92, 0x92, 0x92, 0x92, 0xFC, 0x00, // a
    0xFE, 0x88, 0x88, 0x88, 0x88, 0x88, 0x70, 0x00, // b
    0x70, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x00, // c
    0x70, 0x88, 0x88, 0x88, 0x88, 0x88, 0xFE, 0x00, // d
    0x78, 0x94, 0x94, 0x94, 0x94, 0x94, 0x58, 0x00, // e
    0x20, 0x20, 0xFC, 0x22, 0x22, 0x02, 0x04, 0x00, // f
    0x4C, 0x92, 0x92, 0x92, 0x92, 0x92, 0x7E, 0x00, // g
    0xFE, 0x08, 0x08, 0x08, 0x08, 0x08, 0xF0 ,0x00, // h
    0x00, 0x00, 0x00, 0xF6, 0x00, 0x00, 0x00, 0x00, // i
    0x40, 0x80, 0x80, 0x80, 0x80, 0x80, 0x7A, 0x00, // j
    0x00, 0xFE, 0x10, 0x28, 0x44, 0x82, 0x00, 0x00, // k
    0x00, 0x00, 0x00, 0xFE, 0x00, 0x00, 0x00, 0x00, // l
    0xF0, 0x08, 0x08, 0xF0, 0x08, 0x08, 0xF0, 0x00, // m
    0xF0, 0x08, 0x08, 0x08, 0x08, 0x08, 0xF0, 0x00, // n
    0x70, 0x88, 0x88, 0x88, 0x88, 0x88, 0x70, 0x00, // o
    0x00, 0xFC, 0x22, 0x22, 0x22, 0x1C, 0x00, 0x00, // p
    0x00, 0x1C, 0x22, 0x22, 0x22, 0xFC, 0x00, 0x00, // q
    0xF8, 0x04, 0x04, 0x04, 0x04, 0x04, 0x08, 0x00, // r
    0x00, 0x90, 0xA8, 0xA8, 0xA8, 0xA8, 0x48, 0x00, // s
    0x00, 0x08, 0x08, 0x7E, 0x88, 0x88, 0x40, 0x00, // t
    0x00, 0x7C, 0x80, 0x80, 0x80, 0x80, 0xFC, 0x00, // u
    0x00, 0x1C, 0x60, 0x80, 0x60, 0x1C, 0x00, 0x00, // v
    0x78, 0x80, 0x80, 0x78, 0x80, 0x80, 0x78, 0x00, // w
    0x00, 0x84, 0x48, 0x30, 0x48, 0x84, 0x00, 0x00, // x
    0x00, 0x04, 0x88, 0x50, 0x20, 0x10, 0x0C, 0x00, // y
    0x00, 0x88, 0xC8, 0xA8, 0xA8, 0x98, 0x88, 0x00, // z

//   0x00, 0x08, 0x77, 0x41, 0x00,	// 123 { +
//   0x00, 0x00, 0x7F, 0x00, 0x00,	// 124 | +
//   0x00, 0x41, 0x77, 0x08, 0x00,	// 125 } +
//   0x08, 0x04, 0x08, 0x10, 0x08	// 126 ~ +
};

// Functions

void calc_render_area_buflen(struct render_area *area) {
    // calculate how long the flattened buffer will be for a render area
    area->buflen = (area->end_col - area->start_col + 1) * (area->end_page - area->start_page + 1);
}

void SSD1306_send_cmd(uint8_t cmd) {
    // I2C write process expects a control byte followed by data
    // this "data" can be a command or data to follow up a command
    // Co = 1, D/C = 0 => the driver expects a command
    uint8_t buf[2] = {0x80, cmd};
    i2c_write_blocking(i2c_default, SSD1306_I2C_ADDR, buf, 2, false);
}

void SSD1306_send_cmd_list(uint8_t *buf, int num) {
    for (int i=0;i<num;i++)
        SSD1306_send_cmd(buf[i]);
}

void SSD1306_init() {
    // Some of these commands are not strictly necessary as the reset

    // process defaults to some of these but they are shown here
    // to demonstrate what the initialization sequence looks like
    // Some configuration values are recommended by the board manufacturer

    uint8_t cmds[] = {
        SSD1306_SET_DISP, // set display off
        /* memory mapping */
        SSD1306_SET_MEM_MODE, // set memory address mode 0 = horizontal, 1 =
        //vertical, 2 = page
        0x00, // horizontal addressing mode
        /* resolution and layout */
        SSD1306_SET_DISP_START_LINE, // set display start line to 0
        SSD1306_SET_SEG_REMAP | 0x01, // set segment re-map, column address 127 is mapped
        //to SEG0
        SSD1306_SET_MUX_RATIO, // set multiplex ratio
        SSD1306_HEIGHT - 1, // Display height - 1
        SSD1306_SET_COM_OUT_DIR | 0x08, // set COM (common) output scan direction. Scan from
        //bottom up, COM[N-1] to COM0
        SSD1306_SET_DISP_OFFSET, // set display offset
        0x00, // no offset
        SSD1306_SET_COM_PIN_CFG, // set COM (common) pins hardware configuration.
        //Board specific magic number.
        // 0x02 Works for 128x32, 0x12 Possibly works for
        //128x64. Other options 0x22, 0x32
        #if ((SSD1306_WIDTH == 128) && (SSD1306_HEIGHT == 32))
            0x02,
        #elif ((SSD1306_WIDTH == 128) && (SSD1306_HEIGHT == 64))
            0x12,
        #else
            0x02,
        #endif
        /* timing and driving scheme */
        SSD1306_SET_DISP_CLK_DIV, // set display clock divide ratio
        0x80, // div ratio of 1, standard freq
        SSD1306_SET_PRECHARGE, // set pre-charge period
        0xF1, // Vcc internally generated on our board
        SSD1306_SET_VCOM_DESEL, // set VCOMH deselect level
        0x30, // 0.83xVcc
        /* display */
        SSD1306_SET_CONTRAST, // set contrast control
        0xFF,
        SSD1306_SET_ENTIRE_ON, // set entire display on to follow RAM content
        SSD1306_SET_NORM_DISP, // set normal (not inverted) display
        SSD1306_SET_CHARGE_PUMP, // set charge pump
        0x14, // Vcc internally generated on our board
        SSD1306_SET_SCROLL | 0x00, // deactivate horizontal scrolling if set. This is
        //necessary as memory writes will corrupt if scrolling was enabled
        SSD1306_SET_DISP | 0x01, // turn display on
    };

    calc_render_area_buflen(&dispArea);

    SSD1306_send_cmd_list(cmds, count_of(cmds));
}

void SSD1306_send_buf(uint8_t buf[], int buflen) {
    // in horizontal addressing mode, the column address pointer auto-increments
    // and then wraps around to the next page, so we can send the entire frame
    // buffer in one gooooooo!

    // copy our frame buffer into a new buffer because we need to add the control byte
    // to the beginning

    uint8_t *temp_buf = malloc(buflen + 1);

    temp_buf[0] = 0x40;
    memcpy(temp_buf+1, buf, buflen);

    i2c_write_blocking(i2c_default, SSD1306_I2C_ADDR, temp_buf, buflen + 1, false);

    free(temp_buf);
}

static inline int GetFontIndex(uint8_t ch) {
    if(ch < ' ' || ch > '~')
        return 0;
    return ch - ' ';
}

/// @brief Writes desired character to OLED
/// @param iX Desired column for character (0 - 15)
/// @param iY Desired row for character (0 - 7)
/// @param ch character to be written to OLED
void SSD1306_CharXY(unsigned char iX, unsigned char iY, uint8_t ch) {
    if(iX > 15 || iY > 7)
        return;    

    int idx = GetFontIndex(ch);
    int fb_idx = iY * 128 + (iX*8);

    for (int i = 0; i < 8; i++) {
        dispBuff[fb_idx++] = testFont[idx * 8 + i];
    }
}

/// @brief Writes Strings to OLED
/// @param iX X parameter for columns (between 0 to 15)
/// @param iY Y parameter for rows (between 0 to  7)
/// @param pStr Desired string to write onto OLED
void SSD1306_StringXY(unsigned char iX, unsigned char iY, char * pStr) {
    // Cull out any string off the screen
    if(iX > 15 || iY > 7)
        return; 

    while (*pStr) {
        SSD1306_CharXY(iX, iY, *pStr++);
        iX+=1;
    }
}

/// @brief Renders written bits (chars) on OLED
void SSD1306_Render() {
    // update a portion of the display with a render area
    uint8_t cmds[] = {
        SSD1306_SET_COL_ADDR,
        dispArea.start_col,
        dispArea.end_col,
        SSD1306_SET_PAGE_ADDR,
        dispArea.start_page,
        dispArea.end_page
    };

    SSD1306_send_cmd_list(cmds, count_of(cmds));
    SSD1306_send_buf(dispBuff, dispArea.buflen);
}

/// @brief Clears all written bit (chars) on OLED
static void SSD1306_Clear() {
    // zero the entire display
    memset(dispBuff, 0, SSD1306_BUFLEN);
    SSD1306_Render();
}

int main()
{
    stdio_init_all();
    // Initialise the Wi-Fi chip
    if (cyw43_arch_init()) {
        printf("Wi-Fi init failed\n");
        return -1;
    }

    // I2C Initialisation. Using it at 400Khz.
    i2c_init(I2C_PORT, 400*1000);
    
    gpio_set_function(PICO_DEFAULT_I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(PICO_DEFAULT_I2C_SDA_PIN);
    gpio_pull_up(PICO_DEFAULT_I2C_SCL_PIN);
    // For more examples of I2C use see https://github.com/raspberrypi/pico-examples/tree/master/i2c
    SSD1306_init();

    SSD1306_Clear();

    // SSD1306_StringXY(0, 0, "Test");
    // SSD1306_StringXY(0, 1, "Updated");
    // SSD1306_StringXY(0, 2, "OLED");
    // SSD1306_StringXY(0, 3, "Code");

    //SSD1306_StringXY(0, 0, "! \" # $ % &");
    //SSD1306_StringXY(0, 1, "' ( ) * + , - .");
    //SSD1306_StringXY(0, 2,"/ : ; < = > ? @");
    SSD1306_StringXY(0, 3,"a b c d e f");
    SSD1306_StringXY(0, 4,"stuvwxyz");
    SSD1306_StringXY(0, 5,"why hello");
    SSD1306_Render();
    // Example to turn on the Pico W LED
    //cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);

    while (true) {

    }
}
