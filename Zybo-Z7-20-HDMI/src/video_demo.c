/************************************************************************/
/*																		*/
/*	video_demo.c	--	ZYBO Video demonstration 						*/
/*																		*/
/************************************************************************/
/*	Author: Sam Bobrowicz												*/
/*	Copyright 2015, Digilent Inc.										*/
/************************************************************************/
/*  Module Description: 												*/
/*																		*/
/*		This file contains code for running a demonstration of the		*/
/*		Video input and output capabilities on the ZYBO. It is a good	*/
/*		example of how to properly use the display_ctrl and				*/
/*		video_capture drivers.											*/
/*																		*/
/*																		*/
/************************************************************************/
/*  Revision History:													*/
/* 																		*/
/*		11/25/2015(SamB): Created										*/
/*																		*/
/************************************************************************/

/* ------------------------------------------------------------ */
/*				Include File Definitions						*/
/* ------------------------------------------------------------ */

#include "video_demo.h"
#include "video_capture/video_capture.h"
#include "display_ctrl/display_ctrl.h"
#include "intc/intc.h"
#include <stdio.h>
#include "xuartps.h"
#include "math.h"
#include <ctype.h>
#include <stdlib.h>
#include "xil_types.h"
#include "xil_cache.h"
#include "timer_ps/timer_ps.h"
#include "xparameters.h"
#include "xil_cache.h"
#include "xgpio.h"

#define BUTTONS_DEVICE_ID XPAR_BUTTON_GPIO_DEVICE_ID

/*
 * XPAR redefines
 */
#define DYNCLK_BASEADDR 		XPAR_AXI_DYNCLK_0_S_AXI_LITE_BASEADDR
#define VDMA_ID 				XPAR_AXIVDMA_0_DEVICE_ID
#define HDMI_OUT_VTC_ID 		XPAR_V_TC_OUT_DEVICE_ID
#define HDMI_IN_VTC_ID 			XPAR_V_TC_IN_DEVICE_ID
#define HDMI_IN_GPIO_ID 		XPAR_AXI_GPIO_VIDEO_DEVICE_ID
#define HDMI_IN_VTC_IRPT_ID 	XPAR_FABRIC_V_TC_IN_IRQ_INTR
#define HDMI_IN_GPIO_IRPT_ID 	XPAR_FABRIC_AXI_GPIO_VIDEO_IP2INTC_IRPT_INTR
#define SCU_TIMER_ID 			XPAR_SCUTIMER_DEVICE_ID
#define UART_BASEADDR 			XPAR_PS7_UART_1_BASEADDR
/*
 * Define Car and scale size
 */
#define CAR_W 7
#define CAR_H 13
#define CAR_SCALE 3

/*
 * Define Track and tile size
 */
#define TILE_SIZE 16
#define MAP_W (640/TILE_SIZE)
#define MAP_H (480/TILE_SIZE)

#define TILE_GRASS 0
#define TILE_TRACK 1
#define TILE_WALL 2
#define TILE_FINISH 3

/*
 * Define directional values
 */
#define DIR_UP 0
#define DIR_DOWN 1
#define DIR_LEFT 2
#define DIR_RIGHT 3

/*
 * Define display digits
 */
#define DIGIT_W 5
#define DIGIT_H 7
#define DIGIT_SCALE 3



/* ------------------------------------------------------------ */
/*				Global Variables								*/
/* ------------------------------------------------------------ */

/*
 * Define Sprite Design
 */
int carUp[CAR_H][CAR_W] = {
	{0, 0, 0, 4, 0, 0, 0},	//Front wing tip
	{0, 0, 4, 3, 4, 0, 0},	//Front wing
	{0, 4, 0, 3, 0, 4, 0},
	{0, 1, 3, 3, 3, 1, 0},  //Front Wheels
	{0, 1, 0, 3, 0, 1, 0},	//Front Wheels
	{0, 0, 0, 3, 0, 0, 0},
	{0, 0, 2, 3, 2, 0, 0},
	{0, 0, 2, 3, 2, 0, 0},  // cockpit
	{0, 0, 2, 3, 2, 0, 0},  // sidepods
	{0, 1, 2, 3, 2, 1, 0},  // sidepods
	{0, 1, 3, 0, 3, 1, 0},  // rear wheels
	{0, 3, 3, 3, 3, 3, 0},  // rear wing
	{0, 0, 3, 3, 3, 0, 0},  // rear wing base
};

// CAR_H=13, CAR_W=7
int carDown[CAR_H][CAR_W] = {
    {0, 0, 3, 3, 3, 0, 0},  // rear wing base
    {0, 3, 3, 3, 3, 3, 0},  // rear wing
    {0, 1, 3, 0, 3, 1, 0},  // rear wheels
    {0, 1, 2, 3, 2, 1, 0},  // sidepods
    {0, 0, 2, 3, 2, 0, 0},  // sidepods
    {0, 0, 2, 3, 2, 0, 0},  // cockpit
    {0, 0, 2, 3, 2, 0, 0},
    {0, 0, 0, 3, 0, 0, 0},
    {0, 1, 0, 3, 0, 1, 0},  // Front Wheels
    {0, 1, 3, 3, 3, 1, 0},  // Front Wheels
    {0, 4, 0, 3, 0, 4, 0},
    {0, 0, 4, 3, 4, 0, 0},  // Front wing
    {0, 0, 0, 4, 0, 0, 0},  // Front wing tip
};

// rows=CAR_W=7, cols=CAR_H=13
int carLeft[CAR_W][CAR_H] = {
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 3, 1, 1, 0, 0, 0, 0, 1, 1, 3, 3, 0},
    {0, 3, 0, 0, 3, 0, 2, 2, 2, 2, 3, 3, 0},
    {4, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3},  // spine + nose
    {0, 3, 0, 0, 3, 0, 2, 2, 2, 2, 3, 3, 0},
    {0, 3, 1, 1, 0, 0, 0, 0, 1, 1, 3, 3, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
};

// rows=CAR_W=7, cols=CAR_H=13
int carRight[CAR_W][CAR_H] = {
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 3, 3, 1, 1, 0, 0, 0, 0, 1, 1, 3, 0},
    {0, 3, 3, 2, 2, 2, 2, 0, 3, 0, 0, 3, 0},
    {3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4},  // spine + nose
    {0, 3, 3, 2, 2, 2, 2, 0, 3, 0, 0, 3, 0},
    {0, 3, 3, 1, 1, 0, 0, 0, 0, 1, 1, 3, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
};


/*
 * Define Track design
 */
u8 trackMap[MAP_H][MAP_W] = {
//   0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32 33 34 35 36 37 38 39
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // 0
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0}, // 1
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 0, 0, 0, 0, 0, 0}, // 2
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 0, 0, 0, 0, 0, 0}, // 3
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 2, 0, 0, 0, 0, 0, 0}, // 4  -- wait this goes out of bounds
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 1, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 1, 1, 1, 2, 0, 0, 0, 0, 0, 0}, // 5
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 1, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 1, 1, 1, 2, 0, 0, 0, 0, 0, 0}, // 6
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 1, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 1, 1, 1, 2, 0, 0, 0, 0, 0, 0}, // 7
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 1, 1, 2, 0, 0, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 0, 0, 2, 1, 1, 1, 2, 0, 0, 0, 0, 0, 0}, // 8
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 1, 1, 2, 0, 0, 0, 2, 1, 1, 1, 1, 1, 1, 1, 2, 0, 0, 0, 2, 1, 1, 1, 2, 0, 0, 0, 0, 0, 0}, // 9
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 1, 1, 2, 0, 0, 0, 2, 1, 1, 1, 1, 1, 1, 1, 2, 0, 0, 0, 2, 1, 1, 1, 2, 0, 0, 0, 0, 0, 0}, // 10
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 1, 1, 2, 0, 0, 0, 2, 1, 1, 1, 1, 1, 1, 1, 2, 0, 0, 0, 2, 1, 1, 1, 2, 0, 0, 0, 0, 0, 0}, // 11
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 1, 1, 2, 0, 0, 0, 2, 1, 1, 1, 1, 1, 1, 1, 2, 0, 0, 0, 2, 1, 1, 1, 2, 0, 0, 0, 0, 0, 0}, // 12
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 1, 1, 2, 0, 0, 0, 0, 2, 2, 2, 2, 2, 2, 2, 0, 0, 0, 0, 2, 1, 1, 1, 2, 0, 0, 0, 0, 0, 0}, // 13
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 1, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 1, 1, 1, 2, 0, 0, 0, 0, 0, 0}, // 14
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 1, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 1, 1, 1, 2, 0, 0, 0, 0, 0, 0}, // 15
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 0, 0, 0, 0, 0, 0}, // 16
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 0, 0, 0, 0, 0, 0}, // 17
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 1, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 1, 1, 1, 2, 0, 0, 0, 0, 0, 0}, // 18
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 1, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 1, 1, 1, 2, 0, 0, 0, 0, 0, 0}, // 19
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 1, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 1, 1, 1, 2, 0, 0, 0, 0, 0, 0}, // 20
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 1, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 1, 1, 1, 2, 0, 0, 0, 0, 0, 0}, // 21
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 1, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 1, 1, 1, 2, 0, 0, 0, 0, 0, 0}, // 22
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 1, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 1, 1, 1, 2, 0, 0, 0, 0, 0, 0}, // 23
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 2, 0, 0, 0, 0, 0, 0}, // 24
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 1, 1, 1, 1, 1, 1, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 0, 0, 0, 0, 0, 0}, // 25
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 1, 1, 1, 1, 1, 1, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 0, 0, 0, 0, 0, 0}, // 26
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 1, 1, 1, 1, 1, 1, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 0, 0, 0, 0, 0, 0}, // 27
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0}, // 28
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // 29
};


int digit0[DIGIT_H][DIGIT_W] = {
    {0, 1, 1, 1, 0},
    {1, 0, 0, 0, 1},
    {1, 0, 0, 0, 1},
    {1, 0, 0, 0, 1},
    {1, 0, 0, 0, 1},
    {1, 0, 0, 0, 1},
    {0, 1, 1, 1, 0},
};

int digit1[DIGIT_H][DIGIT_W] = {
    {0, 0, 1, 0, 0},
    {0, 1, 1, 0, 0},
    {0, 0, 1, 0, 0},
    {0, 0, 1, 0, 0},
    {0, 0, 1, 0, 0},
    {0, 0, 1, 0, 0},
    {0, 1, 1, 1, 0},
};

int digit2[DIGIT_H][DIGIT_W] = {
    {0, 1, 1, 1, 0},
    {1, 0, 0, 0, 1},
    {0, 0, 0, 0, 1},
    {0, 0, 0, 1, 0},
    {0, 0, 1, 0, 0},
    {0, 1, 0, 0, 0},
    {1, 1, 1, 1, 1},
};

int digit3[DIGIT_H][DIGIT_W] = {
    {0, 1, 1, 1, 0},
    {1, 0, 0, 0, 1},
    {0, 0, 0, 0, 1},
    {0, 0, 1, 1, 0},
    {0, 0, 0, 0, 1},
    {1, 0, 0, 0, 1},
    {0, 1, 1, 1, 0},
};

int digit4[DIGIT_H][DIGIT_W] = {
    {0, 0, 0, 1, 0},
    {0, 0, 1, 1, 0},
    {0, 1, 0, 1, 0},
    {1, 0, 0, 1, 0},
    {1, 1, 1, 1, 1},
    {0, 0, 0, 1, 0},
    {0, 0, 0, 1, 0},
};

int digit5[DIGIT_H][DIGIT_W] = {
    {1, 1, 1, 1, 1},
    {1, 0, 0, 0, 0},
    {1, 1, 1, 1, 0},
    {0, 0, 0, 0, 1},
    {0, 0, 0, 0, 1},
    {1, 0, 0, 0, 1},
    {0, 1, 1, 1, 0},
};

int digit6[DIGIT_H][DIGIT_W] = {
    {0, 1, 1, 1, 0},
    {1, 0, 0, 0, 0},
    {1, 0, 0, 0, 0},
    {1, 1, 1, 1, 0},
    {1, 0, 0, 0, 1},
    {1, 0, 0, 0, 1},
    {0, 1, 1, 1, 0},
};

int digit7[DIGIT_H][DIGIT_W] = {
    {1, 1, 1, 1, 1},
    {0, 0, 0, 0, 1},
    {0, 0, 0, 1, 0},
    {0, 0, 1, 0, 0},
    {0, 1, 0, 0, 0},
    {0, 1, 0, 0, 0},
    {0, 1, 0, 0, 0},
};

int digit8[DIGIT_H][DIGIT_W] = {
    {0, 1, 1, 1, 0},
    {1, 0, 0, 0, 1},
    {1, 0, 0, 0, 1},
    {0, 1, 1, 1, 0},
    {1, 0, 0, 0, 1},
    {1, 0, 0, 0, 1},
    {0, 1, 1, 1, 0},
};

int digit9[DIGIT_H][DIGIT_W] = {
    {0, 1, 1, 1, 0},
    {1, 0, 0, 0, 1},
    {1, 0, 0, 0, 1},
    {0, 1, 1, 1, 1},
    {0, 0, 0, 0, 1},
    {0, 0, 0, 0, 1},
    {0, 1, 1, 1, 0},
};

/*
 * Lap time table variables
 */
int lapTimes[10];
int lapCount = 0;

/*
 * Defining button variables
 */
XGpio Gpiobuttons;
int psb_check;

/*
 * Display and Video Driver structs
 */
DisplayCtrl dispCtrl;
XAxiVdma vdma;
VideoCapture videoCapt;//no need for volatile because the videoCapt's state is either checked just one time
//in functions VideoStart and VideoStop, or (in the DemoGetInactiveFrame function) the worst case scenario is
//loosing the frame with the inverted colors of the frame gotten from the disconnected input
INTC intc;
char fRefresh; //flag used to trigger a refresh of the Menu on video detect

/*
 * Framebuffers for video data
 */
u8 frameBuf[DISPLAY_NUM_FRAMES][DEMO_MAX_FRAME] __attribute__((aligned(0x20)));
u8 *pFrames[DISPLAY_NUM_FRAMES]; //array of pointers to the frame buffers

/*
 * Interrupt vector table
 */
const ivt_t ivt[] = {
	videoGpioIvt(HDMI_IN_GPIO_IRPT_ID, &videoCapt),
	videoVtcIvt(HDMI_IN_VTC_IRPT_ID, &(videoCapt.vtc))
};

/* ------------------------------------------------------------ */
/*				Procedure Definitions							*/
/* ------------------------------------------------------------ */



int main(void)
{
	DemoInitialize();

	DemoRun();

	return 0;
}

void drawCarUp(u8 *frame, int carX, int carY)
{
	int row, col, py, px;

	for(row = 0; row < CAR_H; row++)
	{
		for(col = 0; col < CAR_W; col++)
		{
			if(carUp[row][col] == 0) continue; //simply continue for those that are black

			for(py = 0; py < CAR_SCALE; py++)
			{
				for(px = 0; px < CAR_SCALE; px++)
				{
					int screenX = carX + (col * CAR_SCALE) + px;
					int screenY = carY + (row * CAR_SCALE) + py;

					u8 r = 0, g = 0, b = 0;

					if(carUp[row][col] == 1) { r=0;   g=0;   b=0;   } // wheels - black
					if(carUp[row][col] == 2) { r=225; g=50;  b=50;  } // body - red
					if(carUp[row][col] == 3) { r=50;  g=50;  b=50;  } // rear wing - grey
					if(carUp[row][col] == 4) { r=50;  g=50;  b=50;  } // front wing - grey

					// remember byte order is B, G, R
					frame[(screenY * DEMO_STRIDE) + (screenX * 3) + 0] = b;
					frame[(screenY * DEMO_STRIDE) + (screenX * 3) + 1] = g;
					frame[(screenY * DEMO_STRIDE) + (screenX * 3) + 2] = r;
				}
			}
		}
	}
}

void drawCarDown(u8 *frame, int carX, int carY)
{
	int row, col, py, px;

	for(row = 0; row < CAR_H; row++)
	{
		for(col = 0; col < CAR_W; col++)
		{
			if(carDown[row][col] == 0) continue; //simply continue for those that are black

			for(py = 0; py < CAR_SCALE; py++)
			{
				for(px = 0; px < CAR_SCALE; px++)
				{
					int screenX = carX + (col * CAR_SCALE) + px;
					int screenY = carY + (row * CAR_SCALE) + py;

					u8 r = 0, g = 0, b = 0;

					if(carDown[row][col] == 1) { r=0;   g=0;   b=0;   } // wheels - black
					if(carDown[row][col] == 2) { r=225; g=50;  b=50;  } // body - red
					if(carDown[row][col] == 3) { r=50;  g=50;  b=50;  } // rear wing - grey
					if(carDown[row][col] == 4) { r=50;  g=50;  b=50;  } // front wing - grey

					// remember byte order is B, G, R
					frame[(screenY * DEMO_STRIDE) + (screenX * 3) + 0] = b;
					frame[(screenY * DEMO_STRIDE) + (screenX * 3) + 1] = g;
					frame[(screenY * DEMO_STRIDE) + (screenX * 3) + 2] = r;
				}
			}
		}
	}
}

void drawCarLeft(u8 *frame, int carX, int carY)
{
	int row, col, py, px;

	for(row = 0; row < CAR_W; row++)
	{
		for(col = 0; col < CAR_H; col++)
		{
			if(carLeft[row][col] == 0) continue; //simply continue for those that are black

			for(py = 0; py < CAR_SCALE; py++)
			{
				for(px = 0; px < CAR_SCALE; px++)
				{
					int screenX = carX + (col * CAR_SCALE) + px;
					int screenY = carY + (row * CAR_SCALE) + py;

					u8 r = 0, g = 0, b = 0;

					if(carLeft[row][col] == 1) { r=0;   g=0;   b=0;   } // wheels - black
					if(carLeft[row][col] == 2) { r=225; g=50;  b=50;  } // body - red
					if(carLeft[row][col] == 3) { r=50;  g=50;  b=50;  } // rear wing - grey
					if(carLeft[row][col] == 4) { r=50;  g=50;  b=50;  } // front wing - grey

					// remember byte order is B, G, R
					frame[(screenY * DEMO_STRIDE) + (screenX * 3) + 0] = b;
					frame[(screenY * DEMO_STRIDE) + (screenX * 3) + 1] = g;
					frame[(screenY * DEMO_STRIDE) + (screenX * 3) + 2] = r;
				}
			}
		}
	}
}

void drawCarRight(u8 *frame, int carX, int carY)
{
	int row, col, py, px;

	for(row = 0; row < CAR_W; row++)
	{
		for(col = 0; col < CAR_H; col++)
		{
			if(carRight[row][col] == 0) continue; //simply continue for those that are black

			for(py = 0; py < CAR_SCALE; py++)
			{
				for(px = 0; px < CAR_SCALE; px++)
				{
					int screenX = carX + (col * CAR_SCALE) + px;
					int screenY = carY + (row * CAR_SCALE) + py;

					u8 r = 0, g = 0, b = 0;

					if(carRight[row][col] == 1) { r=0;   g=0;   b=0;   } // wheels - black
					if(carRight[row][col] == 2) { r=225; g=50;  b=50;  } // body - red
					if(carRight[row][col] == 3) { r=50;  g=50;  b=50;  } // rear wing - grey
					if(carRight[row][col] == 4) { r=50;  g=50;  b=50;  } // front wing - grey

					// remember byte order is B, G, R
					frame[(screenY * DEMO_STRIDE) + (screenX * 3) + 0] = b;
					frame[(screenY * DEMO_STRIDE) + (screenX * 3) + 1] = g;
					frame[(screenY * DEMO_STRIDE) + (screenX * 3) + 2] = r;
				}
			}
		}
	}
}

void drawMap(u8 *frame)
{
	int row, col, py, px;

	for (row = 0; row < MAP_H; row++)
	{
		for(col = 0; col < MAP_W; col++)
		{
			u8 r, g, b;
			switch (trackMap[row][col])
			{
				case TILE_TRACK: r = 80; g = 80; b = 80;  break;//track tarmac
				case TILE_WALL: r = 255; g = 255; b = 255; break;//wall colour
				case TILE_FINISH: if ((px / 4 + py /4) % 2 == 0)
				{
					r  =255, g = 255, b = 255;
				}
				else
				{
					r = 0; g = 0; b = 0;
				}
				break;
				default: r = 34; g = 139; b = 34; break;//grass/runnoff areas
			}
			for (py = 0; py < TILE_SIZE; py++)
			{
				for (px = 0; px < TILE_SIZE; px++)
				{
					int screenX = col * TILE_SIZE + px;
					int screenY = row * TILE_SIZE + py;

					frame[(screenY * DEMO_STRIDE) + (screenX * 3) + 0] = b;
					frame[(screenY * DEMO_STRIDE) + (screenX * 3) + 1] = g;
					frame[(screenY * DEMO_STRIDE) + (screenX * 3) + 2] = r;

				}
			}
		}
	}
}

void drawDigit(u8 *frame, int digit[DIGIT_H][DIGIT_W], int x, int y)
{
	int row, col, py, px;
	for (row = 0; row < DIGIT_H; row++)
	{
		for (col = 0; col < DIGIT_W; col++)
		{
			if (digit[row][col] == 0) continue;
			for (py = 0; py < DIGIT_SCALE; py++)
			{
				for (px = 0; px < DIGIT_SCALE; px++)
				{
					int screenX = x + (col * DIGIT_SCALE) + px;
					int screenY = y + (row * DIGIT_SCALE) + py;
					frame[(screenY * DEMO_STRIDE) + (screenX * 3) + 0] = 255;
					frame[(screenY * DEMO_STRIDE) + (screenX * 3) + 1] = 255;
					frame[(screenY * DEMO_STRIDE) + (screenX * 3) + 2] = 255;
				}
			}
		}
	}
}

int *digits[10] = {
    (int*)digit0, (int*)digit1, (int*)digit2, (int*)digit3,
    (int*)digit4, (int*)digit5, (int*)digit6, (int*)digit7,
    (int*)digit8, (int*)digit9
};

void drawTimer(u8 *frame, int frameCount)
{
	int totalSeconds = frameCount / 18;
	int minutes = totalSeconds / 60;
	int seconds = totalSeconds % 60;

	int x = 10;
	int y = 10;
	int spacing = (DIGIT_W * DIGIT_SCALE) + 2;

	drawDigit(frame, (int(*)[DIGIT_W])digits[minutes % 10], x, y);
	x += spacing;

	x += spacing;

	drawDigit(frame, (int(*)[DIGIT_W])digits[seconds / 10], x ,y);
	x += spacing;

	drawDigit(frame, (int(*)[DIGIT_W])digits[seconds % 10], x ,y);
}

void drawLapTable(u8 *frame)
{
	int x = 10;
	int y = 50;
	int rowSpacing = (DIGIT_H * DIGIT_SCALE) + 4;
	int spacing = (DIGIT_W * DIGIT_SCALE) + 2;

	for (int i = 0; i < lapCount; i++)
	{
		int totalSeconds = lapTimes[i] / 18;
		int minutes = totalSeconds / 60;
		int seconds = totalSeconds % 60;

		drawDigit(frame, (int(*)[DIGIT_W])digits[i + 1], x , y);

		drawDigit(frame, (int(*)[DIGIT_W])digits[minutes % 10], x + spacing*2, y);

		drawDigit(frame, (int(*)[DIGIT_W])digits[seconds / 10], x + spacing*4, y);

		drawDigit(frame, (int(*)[DIGIT_W])digits[seconds % 10], x + spacing*5, y);

		y += rowSpacing;

	}
}

void DemoInitialize()
{
	int Status;
	XAxiVdma_Config *vdmaConfig;
	int i;

	/*
	 * Initialize an array of pointers to the 3 frame buffers
	 */
	for (i = 0; i < DISPLAY_NUM_FRAMES; i++)
	{
		pFrames[i] = frameBuf[i];
	}

	/*
	 * Initialize a timer used for a simple delay
	 */
	TimerInitialize(SCU_TIMER_ID);

	/*
	 * Initialize VDMA driver
	 */
	vdmaConfig = XAxiVdma_LookupConfig(VDMA_ID);
	if (!vdmaConfig)
	{
		xil_printf("No video DMA found for ID %d\r\n", VDMA_ID);
		return;
	}
	Status = XAxiVdma_CfgInitialize(&vdma, vdmaConfig, vdmaConfig->BaseAddress);
	if (Status != XST_SUCCESS)
	{
		xil_printf("VDMA Configuration Initialization failed %d\r\n", Status);
		return;
	}

	/*
	 * Initialize the Display controller and start it
	 */
	Status = DisplayInitialize(&dispCtrl, &vdma, HDMI_OUT_VTC_ID, DYNCLK_BASEADDR, pFrames, DEMO_STRIDE);
	if (Status != XST_SUCCESS)
	{
		xil_printf("Display Ctrl initialization failed during demo initialization%d\r\n", Status);
		return;
	}
	Status = DisplayStart(&dispCtrl);
	if (Status != XST_SUCCESS)
	{
		xil_printf("Couldn't start display during demo initialization%d\r\n", Status);
		return;
	}

	/*
	 * Initialize the Interrupt controller and start it.
	 */
	Status = fnInitInterruptController(&intc);
	if(Status != XST_SUCCESS) {
		xil_printf("Error initializing interrupts");
		return;
	}
	fnEnableInterrupts(&intc, &ivt[0], sizeof(ivt)/sizeof(ivt[0]));

	/*
	 * Initialize the Video Capture device
	 */
	Status = VideoInitialize(&videoCapt, &intc, &vdma, HDMI_IN_GPIO_ID, HDMI_IN_VTC_ID, HDMI_IN_VTC_IRPT_ID, pFrames, DEMO_STRIDE, DEMO_START_ON_DET);
	if (Status != XST_SUCCESS)
	{
		xil_printf("Video Ctrl initialization failed during demo initialization%d\r\n", Status);
		return;
	}

	/*
	 * Set the Video Detect callback to trigger the menu to reset, displaying the new detected resolution
	 */
	VideoSetCallback(&videoCapt, DemoISR, &fRefresh);

	DemoPrintTest(dispCtrl.framePtr[dispCtrl.curFrame], dispCtrl.vMode.width, dispCtrl.vMode.height, dispCtrl.stride, DEMO_PATTERN_1);

	/*
	 * Setup and Initialize Button GPIO
	 */
	xil_printf("Initializing Buttons GPIO...\r\n");

	Status = XGpio_Initialize(&Gpiobuttons, BUTTONS_DEVICE_ID);
	if (Status != XST_SUCCESS)
	{
		xil_printf("Button GPIO Initialization Failed...\r\n");
	}
	xil_printf("Setting up buttons as input...\r\n");
	XGpio_SetDataDirection(&Gpiobuttons, 1, 0xFF);

	return;
}


void DemoRun()
{


	int carX = 300;
	int carY = 300;
	int prevcarX;
	int prevcarY;
	int drawFrame;
	int frameCount = 0;
	u8 *frame;
	int carDir = DIR_UP;
	int timerRunning = 0;

	xil_printf("Resolution: %dx%d\r\n", (int)dispCtrl.vMode.width, (int)dispCtrl.vMode.height);

	while(1)
	{
		// pick back buffer
		drawFrame = (dispCtrl.curFrame == 1) ? 2 : 1;
		frame = pFrames[drawFrame];

		// 1. clear screen to light grey
		memset(frame, 30, DEMO_MAX_FRAME);

		//button test
		psb_check = XGpio_DiscreteRead(&Gpiobuttons, 1);
		//xil_printf("Push Buttons Status: %x\r\n", psb_check);

		//Draw timer and map on screen
		drawMap(frame);
		drawTimer(frame, frameCount);
		drawLapTable(frame);

		prevcarX = carX;
		prevcarY = carY;
		//Movement rules
		if (psb_check & 0x01)
		{
			carX = carX +5;
			carDir = DIR_RIGHT;
		}
		if (psb_check & 0x02)
		{
			carX = carX -5;
			carDir = DIR_LEFT;
		}
		if (psb_check & 0x04)
		{
			carY = carY + 5;
			carDir = DIR_DOWN;
		}
		if (psb_check & 0x08)
		{
			carY = carY -5;
			carDir = DIR_UP;
		}

		//Directional Rules
		if (carDir == DIR_UP)
		{
			drawCarUp(frame, carX, carY);
		}
		if (carDir == DIR_DOWN)
		{
			drawCarDown(frame, carX, carY);
		}
		if (carDir == DIR_LEFT)
		{
			drawCarLeft(frame, carX, carY);
		}
		if (carDir == DIR_RIGHT)
		{
			drawCarRight(frame, carX, carY);
		}
		//Vertical Screen wrap around rules
		if (carY < 0)
		{
			carY = (int)dispCtrl.vMode.height;
		}
		if (carY > (int)dispCtrl.vMode.height)
		{
			carY = 0;
		}

		//Horizontal Screen wrap around rules
		if (carX < 0)
		{
			carX = (int)dispCtrl.vMode.width;
		}
		if (carX > (int)dispCtrl.vMode.width)
		{
			carX = 0;
		}

		/*
		 * Set up collision rules here
		 */
		if (trackMap[carY/TILE_SIZE][carX/TILE_SIZE] == TILE_WALL)
		{
			carX = prevcarX;
			carY = prevcarY;
			frameCount+= 90;
		}

		int currentTileX = carX / TILE_SIZE;
		int currentTileY = carY / TILE_SIZE;

		if (trackMap[currentTileY][currentTileX] == TILE_FINISH)
		{
			if (!timerRunning)
			{
				timerRunning = 1;
				frameCount = 0;
			}
			else
			{
				lapTimes[lapCount] = frameCount;
				lapCount++;
				frameCount = 0;
			}
		}

		if (timerRunning) frameCount++;



		// 3. flush cache
		Xil_DCacheFlushRange((unsigned int)frame, DEMO_MAX_FRAME);

		// 4. swap framebuffer
		DisplayChangeFrame(&dispCtrl, drawFrame);

		// 5. small delay
		TimerDelay(16000);
	}
}

void DemoPrintMenu()
{
	xil_printf("\x1B[H"); //Set cursor to top left of terminal
	xil_printf("\x1B[2J"); //Clear terminal
	xil_printf("**************************************************\n\r");
	xil_printf("*                ZYBO Video Demo                 *\n\r");
	xil_printf("**************************************************\n\r");
	xil_printf("*Display Resolution: %28s*\n\r", dispCtrl.vMode.label);
	printf("*Display Pixel Clock Freq. (MHz): %15.3f*\n\r", dispCtrl.pxlFreq);
	xil_printf("*Display Frame Index: %27d*\n\r", dispCtrl.curFrame);
	if (videoCapt.state == VIDEO_DISCONNECTED) xil_printf("*Video Capture Resolution: %22s*\n\r", "!HDMI UNPLUGGED!");
	else xil_printf("*Video Capture Resolution: %17dx%-4d*\n\r", videoCapt.timing.HActiveVideo, videoCapt.timing.VActiveVideo);
	xil_printf("*Video Frame Index: %29d*\n\r", videoCapt.curFrame);
	xil_printf("**************************************************\n\r");
	xil_printf("\n\r");
	xil_printf("1 - Change Display Resolution\n\r");
	xil_printf("2 - Change Display Framebuffer Index\n\r");
	xil_printf("3 - Print Blended Test Pattern to Display Framebuffer\n\r");
	xil_printf("4 - Print Color Bar Test Pattern to Display Framebuffer\n\r");
	xil_printf("5 - Start/Stop Video stream into Video Framebuffer\n\r");
	xil_printf("6 - Change Video Framebuffer Index\n\r");
	xil_printf("7 - Grab Video Frame and invert colors\n\r");
	xil_printf("8 - Grab Video Frame and scale to Display resolution\n\r");
	xil_printf("q - Quit\n\r");
	xil_printf("\n\r");
	xil_printf("\n\r");
	xil_printf("Enter a selection:");
}

void DemoChangeRes()
{
	int fResSet = 0;
	int status;
	char userInput = 0;

	/* Flush UART FIFO */
	while (XUartPs_IsReceiveData(UART_BASEADDR))
	{
		XUartPs_ReadReg(UART_BASEADDR, XUARTPS_FIFO_OFFSET);
	}

	while (!fResSet)
	{
		DemoCRMenu();

		/* Wait for data on UART */
		while (!XUartPs_IsReceiveData(UART_BASEADDR))
		{}

		/* Store the first character in the UART recieve FIFO and echo it */
		userInput = XUartPs_ReadReg(UART_BASEADDR, XUARTPS_FIFO_OFFSET);
		xil_printf("%c", userInput);
		status = XST_SUCCESS;
		switch (userInput)
		{
		case '1':
			status = DisplayStop(&dispCtrl);
			DisplaySetMode(&dispCtrl, &VMODE_640x480);
			DisplayStart(&dispCtrl);
			fResSet = 1;
			break;
		case '2':
			status = DisplayStop(&dispCtrl);
			DisplaySetMode(&dispCtrl, &VMODE_800x600);
			DisplayStart(&dispCtrl);
			fResSet = 1;
			break;
		case '3':
			status = DisplayStop(&dispCtrl);
			DisplaySetMode(&dispCtrl, &VMODE_1280x720);
			DisplayStart(&dispCtrl);
			fResSet = 1;
			break;
		case '4':
			status = DisplayStop(&dispCtrl);
			DisplaySetMode(&dispCtrl, &VMODE_1280x1024);
			DisplayStart(&dispCtrl);
			fResSet = 1;
			break;
		case '5':
			status = DisplayStop(&dispCtrl);
			DisplaySetMode(&dispCtrl, &VMODE_1600x900);
			DisplayStart(&dispCtrl);
			fResSet = 1;
			break;
		case '6':
			status = DisplayStop(&dispCtrl);
			DisplaySetMode(&dispCtrl, &VMODE_1920x1080);
			DisplayStart(&dispCtrl);
			fResSet = 1;
			break;
		case 'q':
			fResSet = 1;
			break;
		default :
			xil_printf("\n\rInvalid Selection");
			TimerDelay(500000);
		}
		if (status == XST_DMA_ERROR)
		{
			xil_printf("\n\rWARNING: AXI VDMA Error detected and cleared\n\r");
		}
	}
}

void DemoCRMenu()
{
	xil_printf("\x1B[H"); //Set cursor to top left of terminal
	xil_printf("\x1B[2J"); //Clear terminal
	xil_printf("**************************************************\n\r");
	xil_printf("*                ZYBO Video Demo                 *\n\r");
	xil_printf("**************************************************\n\r");
	xil_printf("*Current Resolution: %28s*\n\r", dispCtrl.vMode.label);
	printf("*Pixel Clock Freq. (MHz): %23.3f*\n\r", dispCtrl.pxlFreq);
	xil_printf("**************************************************\n\r");
	xil_printf("\n\r");
	xil_printf("1 - %s\n\r", VMODE_640x480.label);
	xil_printf("2 - %s\n\r", VMODE_800x600.label);
	xil_printf("3 - %s\n\r", VMODE_1280x720.label);
	xil_printf("4 - %s\n\r", VMODE_1280x1024.label);
	xil_printf("5 - %s\n\r", VMODE_1600x900.label);
	xil_printf("6 - %s\n\r", VMODE_1920x1080.label);
	xil_printf("q - Quit (don't change resolution)\n\r");
	xil_printf("\n\r");
	xil_printf("Select a new resolution:");
}

int DemoGetInactiveFrame(DisplayCtrl *DispCtrlPtr, VideoCapture *VideoCaptPtr)
{
	int i;
	for (i=1; i<DISPLAY_NUM_FRAMES; i++)
	{
		if (DispCtrlPtr->curFrame == i && DispCtrlPtr->state == DISPLAY_RUNNING)
		{
			continue;
		}
		else if (VideoCaptPtr->curFrame == i && VideoCaptPtr->state == VIDEO_STREAMING)
		{
			continue;
		}
		else
		{
			return i;
		}
	}
	xil_printf("Unreachable error state reached. All buffers are in use.\r\n");
}

void DemoInvertFrame(u8 *srcFrame, u8 *destFrame, u32 width, u32 height, u32 stride)
{
	u32 xcoi, ycoi;
	u32 lineStart = 0;
	Xil_DCacheInvalidateRange((unsigned int) srcFrame, DEMO_MAX_FRAME);
	for(ycoi = 0; ycoi < height; ycoi++)
	{
		for(xcoi = 0; xcoi < (width * 3); xcoi+=3)
		{
			destFrame[xcoi + lineStart] = ~srcFrame[xcoi + lineStart];         //Red
			destFrame[xcoi + lineStart + 1] = ~srcFrame[xcoi + lineStart + 1]; //Blue
			destFrame[xcoi + lineStart + 2] = ~srcFrame[xcoi + lineStart + 2]; //Green
		}
		lineStart += stride;
	}
	/*
	 * Flush the framebuffer memory range to ensure changes are written to the
	 * actual memory, and therefore accessible by the VDMA.
	 */
	Xil_DCacheFlushRange((unsigned int) destFrame, DEMO_MAX_FRAME);
}


/*
 * Bilinear interpolation algorithm. Assumes both frames have the same stride.
 */
void DemoScaleFrame(u8 *srcFrame, u8 *destFrame, u32 srcWidth, u32 srcHeight, u32 destWidth, u32 destHeight, u32 stride)
{
	float xInc, yInc; // Width/height of a destination frame pixel in the source frame coordinate system
	float xcoSrc, ycoSrc; // Location of the destination pixel being operated on in the source frame coordinate system
	float x1y1, x2y1, x1y2, x2y2; //Used to store the color data of the four nearest source pixels to the destination pixel
	int ix1y1, ix2y1, ix1y2, ix2y2; //indexes into the source frame for the four nearest source pixels to the destination pixel
	float xDist, yDist; //distances between destination pixel and x1y1 source pixels in source frame coordinate system

	int xcoDest, ycoDest; // Location of the destination pixel being operated on in the destination coordinate system
	int iy1; //Used to store the index of the first source pixel in the line with y1
	int iDest; //index of the pixel data in the destination frame being operated on

	int i;

	xInc = ((float) srcWidth - 1.0) / ((float) destWidth);
	yInc = ((float) srcHeight - 1.0) / ((float) destHeight);

	ycoSrc = 0.0;
	for (ycoDest = 0; ycoDest < destHeight; ycoDest++)
	{
		iy1 = ((int) ycoSrc) * stride;
		yDist = ycoSrc - ((float) ((int) ycoSrc));

		/*
		 * Save some cycles in the loop below by presetting the destination
		 * index to the first pixel in the current line
		 */
		iDest = ycoDest * stride;

		xcoSrc = 0.0;
		for (xcoDest = 0; xcoDest < destWidth; xcoDest++)
		{
			ix1y1 = iy1 + ((int) xcoSrc) * 3;
			ix2y1 = ix1y1 + 3;
			ix1y2 = ix1y1 + stride;
			ix2y2 = ix1y1 + stride + 3;

			xDist = xcoSrc - ((float) ((int) xcoSrc));

			/*
			 * For loop handles all three colors
			 */
			for (i = 0; i < 3; i++)
			{
				x1y1 = (float) srcFrame[ix1y1 + i];
				x2y1 = (float) srcFrame[ix2y1 + i];
				x1y2 = (float) srcFrame[ix1y2 + i];
				x2y2 = (float) srcFrame[ix2y2 + i];

				/*
				 * Bilinear interpolation function
				 */
				destFrame[iDest] = (u8) ((1.0-yDist)*((1.0-xDist)*x1y1+xDist*x2y1) + yDist*((1.0-xDist)*x1y2+xDist*x2y2));
				iDest++;
			}
			xcoSrc += xInc;
		}
		ycoSrc += yInc;
	}

	/*
	 * Flush the framebuffer memory range to ensure changes are written to the
	 * actual memory, and therefore accessible by the VDMA.
	 */
	Xil_DCacheFlushRange((unsigned int) destFrame, DEMO_MAX_FRAME);

	return;
}

void DemoPrintTest(u8 *frame, u32 width, u32 height, u32 stride, int pattern)
{
	u32 xcoi, ycoi;
	u32 iPixelAddr;
	u8 wRed, wBlue, wGreen;
	u32 wCurrentInt;
	double fRed, fBlue, fGreen, fColor;
	u32 xLeft, xMid, xRight, xInt;
	u32 yMid, yInt;
	double xInc, yInc;


	switch (pattern)
	{
	case DEMO_PATTERN_0:

		xInt = width / 4; //Four intervals, each with width/4 pixels
		xLeft = xInt * 3;
		xMid = xInt * 2 * 3;
		xRight = xInt * 3 * 3;
		xInc = 256.0 / ((double) xInt); //256 color intensities are cycled through per interval (overflow must be caught when color=256.0)

		yInt = height / 2; //Two intervals, each with width/2 lines
		yMid = yInt;
		yInc = 256.0 / ((double) yInt); //256 color intensities are cycled through per interval (overflow must be caught when color=256.0)

		fBlue = 0.0;
		fRed = 256.0;
		for(xcoi = 0; xcoi < (width*3); xcoi+=3)
		{
			/*
			 * Convert color intensities to integers < 256, and trim values >=256
			 */
			wRed = (fRed >= 256.0) ? 255 : ((u8) fRed);
			wBlue = (fBlue >= 256.0) ? 255 : ((u8) fBlue);
			iPixelAddr = xcoi;
			fGreen = 0.0;
			for(ycoi = 0; ycoi < height; ycoi++)
			{

				wGreen = (fGreen >= 256.0) ? 255 : ((u8) fGreen);
				frame[iPixelAddr] = wRed;
				frame[iPixelAddr + 1] = wBlue;
				frame[iPixelAddr + 2] = wGreen;
				if (ycoi < yMid)
				{
					fGreen += yInc;
				}
				else
				{
					fGreen -= yInc;
				}

				/*
				 * This pattern is printed one vertical line at a time, so the address must be incremented
				 * by the stride instead of just 1.
				 */
				iPixelAddr += stride;
			}

			if (xcoi < xLeft)
			{
				fBlue = 0.0;
				fRed -= xInc;
			}
			else if (xcoi < xMid)
			{
				fBlue += xInc;
				fRed += xInc;
			}
			else if (xcoi < xRight)
			{
				fBlue -= xInc;
				fRed -= xInc;
			}
			else
			{
				fBlue += xInc;
				fRed = 0;
			}
		}
		/*
		 * Flush the framebuffer memory range to ensure changes are written to the
		 * actual memory, and therefore accessible by the VDMA.
		 */
		Xil_DCacheFlushRange((unsigned int) frame, DEMO_MAX_FRAME);
		break;
	case DEMO_PATTERN_1:

		xInt = width / 7; //Seven intervals, each with width/7 pixels
		xInc = 256.0 / ((double) xInt); //256 color intensities per interval. Notice that overflow is handled for this pattern.

		fColor = 0.0;
		wCurrentInt = 1;
		for(xcoi = 0; xcoi < (width*3); xcoi+=3)
		{

			/*
			 * Just draw white in the last partial interval (when width is not divisible by 7)
			 */
			if (wCurrentInt > 7)
			{
				wRed = 255;
				wBlue = 255;
				wGreen = 255;
			}
			else
			{
				if (wCurrentInt & 0b001)
					wRed = (u8) fColor;
				else
					wRed = 0;

				if (wCurrentInt & 0b010)
					wBlue = (u8) fColor;
				else
					wBlue = 0;

				if (wCurrentInt & 0b100)
					wGreen = (u8) fColor;
				else
					wGreen = 0;
			}

			iPixelAddr = xcoi;

			for(ycoi = 0; ycoi < height; ycoi++)
			{
				frame[iPixelAddr] = wRed;
				frame[iPixelAddr + 1] = wBlue;
				frame[iPixelAddr + 2] = wGreen;
				/*
				 * This pattern is printed one vertical line at a time, so the address must be incremented
				 * by the stride instead of just 1.
				 */
				iPixelAddr += stride;
			}

			fColor += xInc;
			if (fColor >= 256.0)
			{
				fColor = 0.0;
				wCurrentInt++;
			}
		}
		/*
		 * Flush the framebuffer memory range to ensure changes are written to the
		 * actual memory, and therefore accessible by the VDMA.
		 */
		Xil_DCacheFlushRange((unsigned int) frame, DEMO_MAX_FRAME);
		break;
	default :
		xil_printf("Error: invalid pattern passed to DemoPrintTest");
	}
}

void DemoISR(void *callBackRef, void *pVideo)
{
	char *data = (char *) callBackRef;
	*data = 1; //set fRefresh to 1
}


