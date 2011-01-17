/*
 * taskPushButtonMenu.c
 *
 * \brief This task provides LCD Menu functions, triggered by the use of the
 * push button associated with the Rotary Encoder.  There are two levels
 * of functionality.
 *
 * The initial level is just to cycle through 9 frequency
 * memories, when doing short pushes of the push button.
 *
 * One long push triggers the Menu functions, which utilize the Rotary
 * Encoder for menu choice selection, and use the push button for confirmation
 * of choice.  The Menu functions may be several levels deep, depending on
 * choice.
 *
 *  Created on: 2010-06-26
 *      Author: Loftur Jonasson, TF3LJ
 */


#include "board.h"

#if LCD_DISPLAY			// Multi-line LCD display

#include <stdio.h>
//#include <string.h>
//#include <math.h>

#include "gpio.h"
#include "FreeRTOS.h"
#include "rtc.h"
#include "flashc.h"
#include "wdt.h"

#include "taskPushButtonMenu.h"
#include "Mobo_config.h"
#include "taskLCD.h"
#include "rotary_encoder.h"
#include "DG8SAQ_cmd.h"
//#include "taskAK5394A.h"
//#include "LCD_bargraphs.h"

// First Level Menu Items
const uint8_t level0_menu_size = 15;
const char *level0_menu_items[] =
				{  " 1-Save Frequency",		//done
				   " 2-VFO Resolution",		//done
				   " 3-PCF8574 Control",
				   " 4-Power and SWR",
				   " 5-Bias Settings",
				   " 6-PA Temperature",		//done
				   " 7-Manage Filters",		//part, scrambled filters order missing
				   " 8-PSDR-IQ RX offs",	//done
				   " 9-I2C Addresses",		//done
				   "10-Si570 Calibrate",	//done
				   "11-Frq Add/Sub/Mul",
				   "12-Encoder Steps",		//done
				   "13-UAC1/UAC2 Audio",	//done
				   "14-Factory Reset",		//done
				   "15-Exit" };				//done

// Flag for Frequency Menu Selection
#define FREQ_MENU		1
// Frequency Menu Items
const uint8_t freq_menu_size = 11;
const char *freq_menu_items[] =
				{  "CH1",
				   "CH2",
				   "CH3",
				   "CH4",
				   "CH5",
				   "CH6",
				   "CH7",
				   "CH8",
				   "CH9",
				   "Back",
				   "Exit" };

// Flag for VFO Resolution Control
#define VFORES_MENU		2

// Flag for PCF8584 Control
#define PCF8574_MENU	3

// Flag for Power and SWR
#define PSWR_MENU		4

// Flag for Bias Settings
#define BIAS_MENU		5

// Flag for Temperature Control Menu Selection
#define TMP_MENU		6
// Temperature Control Menu Items
const uint8_t temperature_menu_size = 5;
const char *temperature_menu_items[] =
				{  "1-Tmp Alarm",
				   "2-Fan ON",
				   "3-Fan Off",
				   "4-Go Back",
				   "5-Exit"};

// Flags for Temperature Control Submenu functions
#define TMP_CASE0_MENU	601
#define TMP_CASE1_MENU	602
#define TMP_CASE2_MENU	603

// Flag for Filters Setpoint Control Menu Selection
#define FILTERS_MENU	7
// Filters top level menu Items
const uint8_t filters_menu_size = 6;
const char *filters_menu_items[] =
				{  "1-BPF Crossover",
				   "2-BPF Order",
				   "3-LPF Crossover",
				   "4-LPF Order",
				   "5-Go Back",
				   "6-Exit"};

// Flags for Filter Control Submenu functions
#define FILTER_BPF_SETPOINT_MENU	701
#define FILTER_BPF_ORDER_MENU		702
#define FILTER_LPF_SETPOINT_MENU	703
#define FILTER_LPF_ORDER_MENU		704
#define FILTER_BPF_ADJUST_MENU		711
#define FILTER_LPF_ADJUST_MENU		712
// Filter Menu Items
const uint8_t swp_menu_size = 17;
const char *swp_menu_items[] =
				{  "CP-1",
				   "CP-2",
				   "CP-3",
				   "CP-4",
				   "CP-5",
				   "CP-6",
				   "CP-7",
				   "CP-8",
				   "CP-9",
				   "CP10",
				   "CP11",
				   "CP12",
				   "CP13",
				   "CP14",
				   "CP15",
				   "Back",
				   "Exit" };

// Flag for PSDR-IQ Offset
#define PSDR_IQ_MENU	8

// Flag for I2C Address Management
#define I2C_MENU		9
// I2C Control Menu Items
const uint8_t i2c_menu_size = 10;
const char *i2c_menu_items[] =
				{  "Si570 VCXO",
				   "PCF8574 Mobo",
				   "PCF8574 LPF1",
				   "PCF8574 LPF2",
				   "PCF8574 FAN",
				   "TMP100/101",
				   "AD5301 DAC",
				   "AD7991 ADC",
				   "Go Back",
				   "Exit"  };

// Flags for I2C Submenu functions
#define I2C_SI570_MENU			901
#define I2C_PCF8574_MOBO_MENU	902
#define I2C_PCF8574_LPF1_MENU	903
#define I2C_PCF8574_LPF2_MENU	904
#define I2C_PCF8574_FAN_MENU	905
#define I2C_TMP100_MENU			906
#define I2C_AD5301_MENU			907
#define I2C_AD7991_MENU			908

// Flag for Si570 Calibration
#define SI570_MENU		10

// Flag for Frequency Add/Subtract/Multiply
#define ADDSUBMUL		11

// Flag for Encoder Resolution Change
#define ENCODER_MENU	12

// Flag for UAC Menu Selection
#define UAC_MENU		13
// UAC1/UAC2 Audio Selection Menu items
const uint8_t uac_menu_size = 3;
const char *uac_menu_items[] =
				{  "Select UAC1 Audio",
				   "Select UAC2 Audio",
				   "Exit w/o change" };

// Flag for Factory Reset
#define FACTORY_MENU	14
// Factory Reset menu Items
const uint8_t factory_menu_size = 3;
const char *factory_menu_items[] =
				{  "1-Yes  Reset",
				   "2-No - Go back",
				   "3-No - Exit"	};


// LCD Queue Buffer for Menu Print
char menu_lcd0[21];
char menu_lcd1[21];
char menu_lcd2[21];
char menu_lcd3[21];
// A second LCD Queue Buffer for Menu Print
char menu2_lcd0[10];
char menu2_lcd1[10];
char menu2_lcd2[10];
char menu2_lcd3[10];

bool		BUTTON_pushed	= 	FALSE;				// Button is being pushed
bool		BUTTON_long		=	FALSE;				// Bool for Long push and not released yet

uint16_t		menu_level = 0;						// Keep track of which menu we are in
uint8_t			menu_data = 0;						// Pass data to lower menu


/*! \brief Determine if a Frequency/Menu button push was LONG or SHORT
 *
 * \retval NOT_PUSHED, LONG_PUSH or SHORT_PUSH
 */
uint8_t	scan_menu_button(void)
{
	static uint32_t	button_start_time;
	uint32_t 		button_current_time;
	uint8_t			return_value;

	return_value = NOT_PUSHED;					// Give an initial value

	// read pin and set regbit accordingly
	if (gpio_get_pin_value(MENU_BUTTON) == 0) 	//Do stuff when pushed
	{
		// If long push and not released yet, then do nothing
		if (BUTTON_long == TRUE)
		{
			// Do nothing
		}
		else
		{
			// Is this the first iteration sensing button push?
			if (BUTTON_pushed == FALSE)			// Button has just been pushed
			{
				BUTTON_pushed = TRUE;

				// Get start time of push
				button_start_time = rtc_get_value(&AVR32_RTC);
			}
			// This is not the first iteration, check for Long Push
			else
			{
				// Get current RTC time
				button_current_time = rtc_get_value(&AVR32_RTC);

				// Timer overrun, compensate
				if (button_current_time < button_start_time)
					button_current_time = RTC_COUNTER_MAX + button_current_time;

				// Is this a long push
				if ((button_current_time - button_start_time) > (PUSHB_LONG_MIN * 1150))
				{
					return_value = LONG_PUSH;
					BUTTON_long = TRUE;
				}
			}
		}
	}
	// Do stuff when not pushed
	else
	{
		// Has Button just been released?
		if (BUTTON_pushed == TRUE)
		{
			BUTTON_pushed = FALSE;

			// Return without doing nothing, we have already enacted a LONG Push
			if (BUTTON_long == TRUE)
			{
				BUTTON_long = FALSE;
				return_value = NOT_PUSHED;
			}
			// Check if time corresponds with a short push
			else
			{
				// Measure the time since last encoder activity in units of appr 1/(RTC = 115kHz) seconds
				// Current RTC time
				button_current_time = rtc_get_value(&AVR32_RTC);

				// Timer overrun, compensate
				if (button_current_time < button_start_time)
					button_current_time = RTC_COUNTER_MAX + button_current_time;

				// Is this a valid push
				if ((button_current_time - button_start_time) > (PUSHB_SHORT_MIN * 1150))
				{
					return_value = SHORT_PUSH;
				}
				// Too short to be a qualified push
				else
				{
					return_value = NOT_PUSHED;
				}
			}
		}
	}
	return return_value;
}


/*
 * \brief Display a 3 or 4 line Menu of choices
 *
 * **menu refers to a pointer array containing the Menu to be printed
 *
 * menu_size indicates how many pointers (menu items) there are in the array
 *
 * current_choice indicates which item is currently up for selection if pushbutton is pushed
 *
 * begin row, begin_col indicate the upper lefthand corner of the three or four lines to be printed
 *
 * lines can take the value of 3 or 4, indicating how many lines the scrolling menu contains
 *
 * *lcd0 - *lcd4 are pointers to string addresses to contain the printed data being passed to \
 * the LCD print queue.  This is necessary, as there may be more than one scrolling menu active at
 * the same time, side by side.
 *
 * \retval choice
 */
void lcd_scroll_Menu(char **menu, uint8_t menu_size,
		uint8_t current_choice, uint8_t begin_row, uint8_t begin_col, uint8_t lines,
		char *lcd0, char *lcd1, char *lcd2, char *lcd3)
{
	uint8_t a, x;

	xSemaphoreTake( mutexQueLCD, portMAX_DELAY );

	// Clear LCD from begin_col to end of line.
	lcd_q_goto(begin_row,begin_col);
	for (a = begin_col; a < 20; a++)
		lcd_q_putc(' ');
	lcd_q_goto(begin_row+1,begin_col);
	for (a = begin_col; a < 20; a++)
		lcd_q_putc(' ');
	lcd_q_goto(begin_row+2,begin_col);
	for (a = begin_col; a < 20; a++)
		lcd_q_putc(' ');

	// Using Menu list pointed to by **menu, preformat for print:
	// First line contains previous choice, secon line contains
	// current choice preceded with a '->', and third line contains
	// next choice
	if (current_choice == 0) x = menu_size - 1;
	else x = current_choice - 1;
	sprintf(lcd0,"%s", *(menu + x));
	sprintf(lcd1,"->%s", *(menu + current_choice));
	if (current_choice == menu_size - 1) x = 0;
	else x = current_choice + 1;
	sprintf(lcd2,"%s", *(menu + x));

	// LCD print lines 1 to 3
	lcd_q_goto(begin_row,begin_col + 2);
	lcd_q_print(lcd0);
	lcd_q_goto(begin_row + 1,begin_col);
	lcd_q_print(lcd1);
	lcd_q_goto(begin_row + 2,begin_col + 2);
	lcd_q_print(lcd2);

	// 4 line display.  Preformat and print the fourth line as well
	if (lines == 4)
	{
		if (current_choice == menu_size-1) x = 1;
		else if (current_choice == menu_size - 2 ) x = 0;
		else x = current_choice + 2;
		sprintf(lcd3,"  %s", *(menu + x));
		lcd_q_goto(begin_row + 3,begin_col);
		for (a = begin_col; a < 20; a++)
			lcd_q_putc(' ');

		lcd_q_goto(begin_row + 3,begin_col + 2);
		lcd_q_print(lcd3);
	}
	xSemaphoreGive( mutexQueLCD );
}


//----------------------------------------------------------------------
// Menu functions begin:
//----------------------------------------------------------------------


/*! \brief Manage the Frequency Save Menu
 * Save Frequency in Long Term (nvram) Memory
 *
 * \retval none
 */
void freq_menu(void)
{
	static int8_t	current_selection;	// Keep track of current LCD menu selection
	static bool LCD_upd = FALSE;		// Keep track of LCD update requirements
	static bool FIRST_run = FALSE;		// Indicates if a consecutive run
	double freq_display;

	// Grab the currently selected frequency channel
	if (FIRST_run == FALSE)
	{
		current_selection = cdata.SwitchFreq - 1;
		FIRST_run = TRUE;
	}

	// Selection modified by encoder
	if (MENU_fromenc == TRUE)
	{
		current_selection += menu_steps_from_enc;
	    // Reset data from Encoder
		MENU_fromenc = FALSE;
		menu_steps_from_enc = 0;

		// Indicate that an LCD update is needed
		LCD_upd = FALSE;
	}

	// If LCD update is needed
	if (LCD_upd == FALSE)
	{
		LCD_upd = TRUE;					// We have serviced LCD

		// Keep Encoder Selection Within Bounds of the Menu Size
		uint8_t menu_size = freq_menu_size;
		while(current_selection >= menu_size)
			current_selection -= menu_size;
		while(current_selection < 0)
			current_selection += menu_size;

		// Indicate Current Frequency and the Frequency stored under the currently selected Channel number
		// The "stored" Frequency always changes according to which channel is currently selected.
		xSemaphoreTake( mutexQueLCD, portMAX_DELAY );
		lcd_q_clear();
		lcd_q_goto(0,0);
		lcd_q_print("Frequency to Save:");
		// frequency in floating point = freq / 2^23
		freq_display = (double)cdata.Freq[0]/(1<<23);
   		sprintf(menu_lcd0,"%2.06fMHz", freq_display);
   		lcd_q_goto(1,0);
   		if(freq_display<=10) lcd_q_print(" ");		// workaround...%2 print format doesn't work
		lcd_q_print(menu_lcd0);
		lcd_q_goto(2,0);
		lcd_q_print("Current Memory");
		lcd_q_goto(3,0);
		if (current_selection < 9)
		{
			freq_display = (double)nvram_cdata.Freq[current_selection + 1]/(1<<23);
			sprintf(menu_lcd1,"%2.06fMHz ", freq_display);
	  		if(freq_display<=10) lcd_q_print(" ");	// workaround...%2 print format doesn't work
		}
		else
		{
			sprintf(menu_lcd1,"Exit w/o save");
		}
		lcd_q_print(menu_lcd1);
		xSemaphoreGive( mutexQueLCD );

		// Print the Rotary Encoder scroll Menu
		lcd_scroll_Menu((char**)freq_menu_items, menu_size, current_selection,
				1, 14, 3, menu2_lcd0, menu2_lcd1, menu2_lcd2, menu2_lcd3);
	}

	// Enact selection
	if (scan_menu_button() == SHORT_PUSH)
	{

		switch (current_selection)
		{
			case 0:
			case 1:
			case 2:
			case 3:
			case 4:
			case 5:
			case 6:
			case 7:
			case 8:
				flashc_memset32((void *)&nvram_cdata.Freq[current_selection + 1], cdata.Freq[0], sizeof(uint32_t), TRUE);
				flashc_memset8((void *)&nvram_cdata.SwitchFreq, current_selection + 1, sizeof(uint8_t), TRUE);

				xSemaphoreTake( mutexQueLCD, portMAX_DELAY );
				lcd_q_clear();
				lcd_q_goto(1,1);
		   		lcd_q_print("New Frequency Saved");
		   		xSemaphoreGive( mutexQueLCD );
				vTaskDelay(20000 );
				MENU_mode = FALSE;	// We're done
				menu_level = 0;		// We are done with this menu level
				LCD_upd = FALSE;
				TX_state = TRUE;	// Force housekeeping
				FIRST_run = FALSE;	// Force read of cdata.SwitcFreq when run next time
				break;
			case 9:
				xSemaphoreTake( mutexQueLCD, portMAX_DELAY );
				lcd_q_clear();
				lcd_q_goto(1,1);
		   		lcd_q_print("Nothing Saved");
		   		xSemaphoreGive( mutexQueLCD );
				vTaskDelay(5000 );
				MENU_mode = TRUE;	// We're not done yet
				menu_level = 0;		// We are done with this menu level
				LCD_upd = FALSE;
				TX_state = TRUE;	// Force housekeeping
				FIRST_run = FALSE;	// Force read of cdata.SwitcFreq when run next time
				break;
			default:
				xSemaphoreTake( mutexQueLCD, portMAX_DELAY );
				lcd_q_clear();
				lcd_q_goto(1,1);
		   		lcd_q_print("Nothing Saved");
		   		xSemaphoreGive( mutexQueLCD );
				vTaskDelay(20000 );
				MENU_mode = FALSE;	// We're done
				menu_level = 0;		// We are done with this menu level
				LCD_upd = FALSE;
				TX_state = TRUE;	// Force housekeeping
				FIRST_run = FALSE;	// Force read of cdata.SwitcFreq when run next time
				break;
		}
	}
}


/*! \brief VFO Resolution 1/2/5/10/50/100kHz per revolution
 *
 * \retval none
 */
void vfores_menu(void)
{
	int8_t	current_selection = 0;		// Keep track of current LCD menu selection
	static bool LCD_upd = FALSE;		// Keep track of LCD update requirements

	#define VFO_VALUES 6
	const uint16_t vfo_values[] =
		{ 1, 2, 5, 10, 50, 100 };

	// Get Current value
	uint8_t i;
	for(i = 0; i<VFO_VALUES; i++)
	{
		if (cdata.VFO_resolution == vfo_values[i])
		{
			current_selection = i;
		}
	}

	// Selection modified by encoder
	if (MENU_fromenc == TRUE)
	{
		current_selection += menu_steps_from_enc;
	    // Reset data from Encoder
		MENU_fromenc = FALSE;
		menu_steps_from_enc = 0;

		// Indicate that an LCD update is needed
		LCD_upd = FALSE;
	}

	// If LCD update is needed
	if (LCD_upd == FALSE)
	{
		LCD_upd = TRUE;					// We have serviced LCD

		// Keep Encoder Selection Within Bounds
		if (current_selection >= VFO_VALUES) current_selection = 0;
		if (current_selection < 0) current_selection = VFO_VALUES-1;

		// Store Current value in running storage
		cdata.VFO_resolution = vfo_values[current_selection];

		xSemaphoreTake( mutexQueLCD, portMAX_DELAY );
		lcd_q_clear();
		lcd_q_goto(0,0);
		lcd_q_print("VFO Resolution:");
		lcd_q_goto(2,0);
		lcd_q_print("Rotate to Adjust");
		lcd_q_goto(3,0);
		lcd_q_print("Push to Save->");
		// Format current value for LCD print
		sprintf(menu_lcd0,"%3d", vfo_values[current_selection]);
		lcd_q_goto(3,17);
		lcd_q_print(menu_lcd0);
		xSemaphoreGive( mutexQueLCD );
	}

	// Enact selection by saving in permanent (nvram / EEPROM) storage
	if (scan_menu_button() == SHORT_PUSH)
	{
		// Save modified value
		if (cdata.VFO_resolution != nvram_cdata.VFO_resolution)
		{
			flashc_memset8((void *)&nvram_cdata.VFO_resolution,
					cdata.VFO_resolution, sizeof(cdata.VFO_resolution), TRUE);
			xSemaphoreTake( mutexQueLCD, portMAX_DELAY );
			lcd_q_clear();
			lcd_q_goto(1,1);
			lcd_q_print("New Value Saved");
			xSemaphoreGive( mutexQueLCD );
		}

		vTaskDelay(5000 );		// Show on screen for 0.5 seconds
		MENU_fromenc = FALSE;	// Clear Encoder output
		LCD_upd = FALSE;		// force LCD reprint
		menu_level = 0;
		MENU_mode = TRUE;		// We're NOT done, just backing off
	}
}


/***************************************************************************************************************************/
// Todo
void pcf8574_menu(void)
{
	//PCF8574_MENU
	static bool LCD_upd = FALSE;	// Keep track of LCD update requirements

	xSemaphoreTake( mutexQueLCD, portMAX_DELAY );
	lcd_q_clear();
	lcd_q_goto(1,1);
	lcd_q_print("Not Implemented yet");
	xSemaphoreGive( mutexQueLCD );
	vTaskDelay(20000 );
	MENU_mode = TRUE;				// We're NOT done, just backing off
	menu_level = 0;					// We are done with this menu level
	LCD_upd = FALSE;
}
/*****************************************************************************************************************************/

/***************************************************************************************************************************/
// Todo
void pswr_menu(void)
{
	//PSWR_MENU
	//FACTORY_MENU
	static bool LCD_upd = FALSE;	// Keep track of LCD update requirements

	xSemaphoreTake( mutexQueLCD, portMAX_DELAY );
	lcd_q_clear();
	lcd_q_goto(1,1);
	lcd_q_print("Not Implemented yet");
	xSemaphoreGive( mutexQueLCD );
	vTaskDelay(20000 );
	MENU_mode = TRUE;				// We're NOT done, just backing off
	menu_level = 0;					// We are done with this menu level
	LCD_upd = FALSE;
}
/*****************************************************************************************************************************/

/***************************************************************************************************************************/
// Todo
void bias_menu(void)
{
	//BIAS_MENU
	static bool LCD_upd = FALSE;	// Keep track of LCD update requirements

	xSemaphoreTake( mutexQueLCD, portMAX_DELAY );
	lcd_q_clear();
	lcd_q_goto(1,1);
	lcd_q_print("Not Implemented yet");
	xSemaphoreGive( mutexQueLCD );
	vTaskDelay(20000 );
	MENU_mode = TRUE;				// We're NOT done, just backing off
	menu_level = 0;					// We are done with this menu level
	LCD_upd = FALSE;
}
/*****************************************************************************************************************************/


/*! \brief Manage the Second Level Temperature Control Menu
 * Adjust Alarm, Fan ON and Fan OFF temperatures.
 *
 * \retval none
 */
void temperature_menu_level2(void)
{
	static int16_t	current_selection;	// Keep track of current LCD menu selection
	static bool LCD_upd = FALSE;		// Keep track of LCD update requirements

	// Get Current value
	if      (menu_level == TMP_CASE0_MENU)current_selection = cdata.hi_tmp_trigger;
	else if (menu_level == TMP_CASE1_MENU)current_selection = cdata.Fan_On;
	else if (menu_level == TMP_CASE2_MENU)current_selection = cdata.Fan_Off;

	// Selection modified by encoder
	if (VAL_fromenc == TRUE)
	{
		current_selection += val_steps_from_enc;
	    // Reset data from Encoder
		VAL_fromenc = FALSE;
		val_steps_from_enc = 0;

		// Indicate that an LCD update is needed
		LCD_upd = FALSE;
	}

	// If LCD update is needed
	if (LCD_upd == FALSE)
	{
		LCD_upd = TRUE;					// We have serviced LCD

		// Keep Encoder Selection Within Bounds of the Menu Size
		uint8_t max_value = 127;		// Highest temperature value in deg C
		uint8_t min_value = 0;			// Lowest temperature value in deg C
		if(current_selection > max_value) current_selection = max_value;
		if(current_selection < min_value) current_selection = min_value;

		// Store Current value in running storage
		if      (menu_level == TMP_CASE0_MENU)cdata.hi_tmp_trigger = current_selection;
		else if (menu_level == TMP_CASE1_MENU)cdata.Fan_On = current_selection;
		else if (menu_level == TMP_CASE2_MENU)cdata.Fan_Off = current_selection;

		// Format current value for LCD print
		sprintf(menu_lcd0,"%4d%cC",current_selection,0xdf);

		// Print Menu for Temperature Alarm Function
		if (menu_level == TMP_CASE0_MENU)	// Temperature Alarm function
		{
			// LCD Print Currently selected Value
			xSemaphoreTake( mutexQueLCD, portMAX_DELAY );

			lcd_q_clear();
			lcd_q_goto(0,0);
			lcd_q_print("Shutdown temperature");
			lcd_q_goto(1,0);
			lcd_q_print("of Power Amplifier:");
			lcd_q_goto(2,0);
			lcd_q_print("Rotate to Adjust");
			lcd_q_goto(3,0);
			lcd_q_print("Push to Save->");
			lcd_q_print(menu_lcd0);
			xSemaphoreGive( mutexQueLCD );
		}
		// Print Menu for Fan ON Temperature
		else if (menu_level == TMP_CASE1_MENU)
		{
			xSemaphoreTake( mutexQueLCD, portMAX_DELAY );
			lcd_q_clear();
			lcd_q_goto(0,0);
			lcd_q_print("FAN ON temperature");
			lcd_q_goto(1,0);
			lcd_q_print("for Power Amplifier:");
			lcd_q_goto(2,0);
			lcd_q_print("Rotate to Adjust");
			lcd_q_goto(3,0);
			lcd_q_print("Push to Save->");
			lcd_q_print(menu_lcd0);
			xSemaphoreGive( mutexQueLCD );
		}
		// Print Menu for Fan OFF Temperature
		else if (menu_level == TMP_CASE2_MENU)
		{
			xSemaphoreTake( mutexQueLCD, portMAX_DELAY );
			lcd_q_clear();
			lcd_q_goto(0,0);
			lcd_q_print("FAN OFF temperature");
			lcd_q_goto(1,0);
			lcd_q_print("for Power Amplifier:");
			lcd_q_goto(2,0);
			lcd_q_print("Rotate to Adjust");
			lcd_q_goto(3,0);
			lcd_q_print("Push to Save->");
			lcd_q_print(menu_lcd0);
			xSemaphoreGive( mutexQueLCD );
		}
	}

	// Enact selection by saving in permanent (nvram / EEPROM) storage
	if (scan_menu_button() == SHORT_PUSH)
	{
		// Save modified value
		if (menu_level == TMP_CASE0_MENU)
		{
			if (nvram_cdata.hi_tmp_trigger != current_selection)
			{
				flashc_memset8((void *)&nvram_cdata.hi_tmp_trigger, current_selection, sizeof(uint8_t), TRUE);
				cdata.hi_tmp_trigger = current_selection;
			}
		}
		else if (menu_level == TMP_CASE1_MENU)
		{
			if (nvram_cdata.Fan_On != current_selection)
			{
				flashc_memset8((void *)&nvram_cdata.Fan_On, current_selection, sizeof(uint8_t), TRUE);
				cdata.Fan_On = current_selection;
			}
		}
		else if (menu_level == TMP_CASE2_MENU)
		{
			if (nvram_cdata.Fan_Off != current_selection)
			{
				flashc_memset8((void *)&nvram_cdata.Fan_Off, current_selection, sizeof(uint8_t), TRUE);
				cdata.Fan_Off = current_selection;
			}
		}

		MENU_fromenc = FALSE;	// Clear Encoder output
		LCD_upd = FALSE;		// force LCD reprint
		xSemaphoreTake( mutexQueLCD, portMAX_DELAY );
		lcd_q_clear();
		lcd_q_goto(1,1);
		lcd_q_print("Value Stored");
		xSemaphoreGive( mutexQueLCD );
		vTaskDelay(5000 );		// Show on screen for 0.5 seconds
		MENU_mode = TRUE;		// We're NOT done, just backing off
		menu_level = TMP_MENU;
		LCD_upd = FALSE;
	}
}


/*! \brief Manage the Temperature Control Menu
 * Select Alarm, Fan ON and Fan OFF temperatures.
 *
 * \retval none
 */
void temperature_menu(void)
{
	static int8_t	current_selection;	// Keep track of current LCD menu selection
	static bool LCD_upd = FALSE;		// Keep track of LCD update requirements

	// Selection modified by encoder
	if (MENU_fromenc == TRUE)
	{
		current_selection += menu_steps_from_enc;
	    // Reset data from Encoder
		MENU_fromenc = FALSE;
		menu_steps_from_enc = 0;

		// Indicate that an LCD update is needed
		LCD_upd = FALSE;
	}

	// If LCD update is needed
	if (LCD_upd == FALSE)
	{
		LCD_upd = TRUE;					// We have serviced LCD

		// Keep Encoder Selection Within Bounds of the Menu Size
		uint8_t menu_size = temperature_menu_size;
		while(current_selection >= menu_size)
			current_selection -= menu_size;
		while(current_selection < 0)
			current_selection += menu_size;

		// Indicate Current Frequency and the Frequency stored under the currently selected Channel number
		// The "stored" Frequency always changes according to which channel is currently selected.
		xSemaphoreTake( mutexQueLCD, portMAX_DELAY );
		lcd_q_clear();
		lcd_q_goto(0,0);
		lcd_q_print("Temperature Mngmnt:");
		lcd_q_goto(2,0);
		lcd_q_print("Cur Val");
		lcd_q_goto(3,0);
		if (current_selection < 3)
		{
			int8_t value=0;

			switch (current_selection)
			{
				case 0:
					value = cdata.hi_tmp_trigger;
					break;
				case 1:
					value = cdata.Fan_On;
					break;
				case 2:
					value = cdata.Fan_Off;
					break;
			}
			sprintf(menu_lcd0," %4d%cC ", value,0xdf);
		}
		else
		{
			sprintf(menu_lcd0,"   --  ");
		}
		lcd_q_print(menu_lcd0);
		xSemaphoreGive( mutexQueLCD );

		// Print the Rotary Encoder scroll Menu
		lcd_scroll_Menu((char**)temperature_menu_items, menu_size, current_selection,
				1, 7, 3, menu2_lcd0, menu2_lcd1, menu2_lcd2, menu2_lcd3);
	}

	// Enact selection
	if (scan_menu_button() == SHORT_PUSH)
	{

		switch (current_selection)
		{
			case 0:
				menu_level = TMP_CASE0_MENU;
				LCD_upd = FALSE;	// force LCD reprint
				break;
			case 1:
				menu_level = TMP_CASE1_MENU;
				LCD_upd = FALSE;	// force LCD reprint
				break;
			case 2:
				menu_level = TMP_CASE2_MENU;
				LCD_upd = FALSE;	// force LCD reprint
				break;
			case 3:
				xSemaphoreTake( mutexQueLCD, portMAX_DELAY );
				lcd_q_clear();
				lcd_q_goto(1,1);
		   		lcd_q_print("Done w. Temperature");
		   		xSemaphoreGive( mutexQueLCD );
				vTaskDelay(5000 );
				MENU_mode = TRUE;	// We're NOT done, just backing off
				menu_level = 0;		// We are done with this menu level
				LCD_upd = FALSE;
				break;
			default:
				xSemaphoreTake( mutexQueLCD, portMAX_DELAY );
				lcd_q_clear();
				lcd_q_goto(1,1);
		   		lcd_q_print("Done w. Temperature");
		   		xSemaphoreGive( mutexQueLCD );
				vTaskDelay(20000 );
				MENU_mode = FALSE;	// We're done
				menu_level = 0;		// We are done with this menu level
				LCD_upd = FALSE;
				TX_state = TRUE;	// Force housekeeping
				break;
		}
	}
}


/*! \brief Manage the Third Level Filter Setpoint Menu
 * Adjust Filter Changeover Settings.
 *
 * \retval none
 */
void filters_adjust_menu_level3(void)
{
	static int16_t	current_selection;	// Keep track of current LCD menu selection
	static bool LCD_upd = FALSE;		// Keep track of LCD update requirements
	double freq_display;

	// Get Current value
	if (menu_level == FILTER_BPF_ADJUST_MENU)current_selection = cdata.FilterCrossOver[menu_data];
	else current_selection = cdata.TXFilterCrossOver[menu_data];

	// Selection modified by encoder
	if (VAL_fromenc == TRUE)
	{
		current_selection += val_steps_from_enc;
	    // Reset data from Encoder
		VAL_fromenc = FALSE;
		val_steps_from_enc = 0;

		// Indicate that an LCD update is needed
		LCD_upd = FALSE;
	}

	// If LCD update is needed
	if (LCD_upd == FALSE)
	{
		LCD_upd = TRUE;					// We have serviced LCD

		// Keep Encoder Selection Within Bounds of the Menu Size
		uint16_t max_value = 54*(1<<7);	// Highest possible Switchover frequency
		uint8_t min_value = 0;			// Lowest Switchover frequency
		if(current_selection > max_value) current_selection = max_value;
		if(current_selection < min_value) current_selection = min_value;

		// Store Current value in running storage
		if (menu_level == FILTER_BPF_ADJUST_MENU)cdata.FilterCrossOver[menu_data] = current_selection;
		else  cdata.TXFilterCrossOver[menu_data] = current_selection;

		xSemaphoreTake( mutexQueLCD, portMAX_DELAY );
		lcd_q_clear();
		lcd_q_goto(0,0);
		lcd_q_print("Set Filter");
		lcd_q_goto(1,4);
		lcd_q_print("Crossover Point:");
		lcd_q_goto(2,0);
		lcd_q_print("Rotate to Adjust");
		lcd_q_goto(3,0);
		lcd_q_print("Push to Save->");
		// Format current value for LCD print
		freq_display = (double)current_selection/(1<<7);
		sprintf(menu_lcd0,"%2.03f", freq_display);
		lcd_q_goto(3,14);
		if(freq_display<=10) lcd_q_print(" ");		// workaround...%2 print format doesn't work
		lcd_q_print(menu_lcd0);
		xSemaphoreGive( mutexQueLCD );
	}

	// Enact selection by saving in permanent (nvram / EEPROM) storage
	if (scan_menu_button() == SHORT_PUSH)
	{
		// Save modified value
		if (menu_level == FILTER_BPF_ADJUST_MENU)
		{
			if (current_selection != nvram_cdata.FilterCrossOver[menu_data])
			{
				flashc_memset16((void *)&nvram_cdata.FilterCrossOver[menu_data],
						current_selection, sizeof(nvram_cdata.FilterCrossOver[menu_data]), TRUE);
				xSemaphoreTake( mutexQueLCD, portMAX_DELAY );
				lcd_q_clear();
				lcd_q_goto(1,1);
				lcd_q_print("New Value Saved");
				xSemaphoreGive( mutexQueLCD );
			}
		}
		else
		{
			if (current_selection != nvram_cdata.TXFilterCrossOver[menu_data])
			{
				flashc_memset16((void *)&nvram_cdata.TXFilterCrossOver[menu_data],
						current_selection, sizeof(nvram_cdata.TXFilterCrossOver[menu_data]), TRUE);
				xSemaphoreTake( mutexQueLCD, portMAX_DELAY );
				lcd_q_clear();
				lcd_q_goto(1,1);
				lcd_q_print("New Value Saved");
				xSemaphoreGive( mutexQueLCD );
			}
		}

		MENU_fromenc = FALSE;	// Clear Encoder output
		LCD_upd = FALSE;		// force LCD reprint
		menu_level = FILTERS_MENU;
		MENU_mode = TRUE;		// We're NOT done, just backing off
		vTaskDelay(5000 );		// Show on screen for 0.5 seconds
	}
}


/*! \brief Manage the Second Level Filter Setpoint Menu
 * Select Filter Changeover Settings.
 *
 * \retval none
 */
void filters_setpoint_menu_level2(void)
{
	static int8_t	current_selection;	// Keep track of current LCD menu selection
	uint8_t 		menu_size;
	static bool LCD_upd = FALSE;		// Keep track of LCD update requirements
	double freq_display = 0;

	// Selection modified by encoder
	if (MENU_fromenc == TRUE)
	{
		current_selection += menu_steps_from_enc;
	    // Reset data from Encoder
		MENU_fromenc = FALSE;
		menu_steps_from_enc = 0;

		// Indicate that an LCD update is needed
		LCD_upd = FALSE;
	}

	// If LCD update is needed
	if (LCD_upd == FALSE)
	{
		LCD_upd = TRUE;					// We have serviced LCD

		// Keep Encoder Selection Within Bounds of the Max Menu Size
		while(current_selection >= swp_menu_size)
			current_selection -= swp_menu_size;
		while(current_selection < 0)
			current_selection += swp_menu_size;

		// Indicate Current Filter Switchover frequency
		// The "stored" Frequency always changes according to which channel is currently selected.
		xSemaphoreTake( mutexQueLCD, portMAX_DELAY );
		lcd_q_clear();
		lcd_q_goto(0,0);
		if (menu_level == FILTER_BPF_SETPOINT_MENU)
		{
			menu_size = 8-1;	// Number of setpoints is one entry smaller than number of filters
			lcd_q_print("BandPass Filter");
			if (current_selection < menu_size)
				// frequency in floating point = freq / 2^7
				freq_display = (double)cdata.FilterCrossOver[current_selection]/(1<<7);
		}
		else
		{
			menu_size = TXF-1;	// Number of setpoints is one entry smaller than number of filters
			lcd_q_print("LowPass Filter");
			if (current_selection < menu_size)
				// frequency in floating point = freq / 2^7
				freq_display = (double)cdata.TXFilterCrossOver[current_selection]/(1<<7);
		}
		lcd_q_goto(1,0);
		lcd_q_print("Crossover:");

		if (current_selection < menu_size)
			sprintf(menu_lcd0,"%2.03fMHz", freq_display);
		else
			sprintf(menu_lcd0,"-.---");
   		lcd_q_goto(3,0);
   		if(freq_display<=10) lcd_q_print(" ");		// workaround...%2 print format doesn't work
   		lcd_q_print(menu_lcd0);
		xSemaphoreGive( mutexQueLCD );

		lcd_scroll_Menu((char**)swp_menu_items, swp_menu_size, current_selection,
				1, 14, 3, menu2_lcd0, menu2_lcd1, menu2_lcd2, menu2_lcd3);
	}

	// Enact selection
	if (scan_menu_button() == SHORT_PUSH)
	{
		if (current_selection == swp_menu_size-1)	// Return
		{
			xSemaphoreTake( mutexQueLCD, portMAX_DELAY );
			lcd_q_clear();
			lcd_q_goto(1,1);
			lcd_q_print("Nothing Changed");
			xSemaphoreGive( mutexQueLCD );
			vTaskDelay(20000 );
			MENU_mode = FALSE;				// We're done
			menu_level = 0;					// We are done with this menu level
		}
		else if (current_selection >= menu_size)	// Go back
		{
			xSemaphoreTake( mutexQueLCD, portMAX_DELAY );
			lcd_q_clear();
			lcd_q_goto(1,1);
			lcd_q_print("Nothing Changed");
			xSemaphoreGive( mutexQueLCD );
			vTaskDelay(5000 );
			MENU_mode = TRUE;				// We're NOT done, just backing off
			menu_level = FILTERS_MENU;		// We are done with this menu level
		}
		else
		{
			if (menu_level == FILTER_BPF_SETPOINT_MENU)
				menu_level = FILTER_BPF_ADJUST_MENU;
			else
				menu_level = FILTER_LPF_ADJUST_MENU;
			menu_data = current_selection;
			VAL_fromenc = FALSE;
		}
		LCD_upd = FALSE;	// force LCD reprint
	}
}


/***************************************************************************************************************************/
// Todo
/*! \brief Manage the Second Level Filter Order Menu
 * Change order of filters
 *
 * \retval none
 */
void filters_order_menu_level2(void)
{
	static bool LCD_upd = FALSE;	// Keep track of LCD update requirements

	xSemaphoreTake( mutexQueLCD, portMAX_DELAY );
	lcd_q_clear();
	lcd_q_goto(1,1);
	lcd_q_print("Not Implemented yet");
	xSemaphoreGive( mutexQueLCD );
	vTaskDelay(20000 );
	MENU_mode = TRUE;				// We're NOT done, just backing off
	menu_level = FILTERS_MENU;		// We are done with this menu level
	LCD_upd = FALSE;
}
/***************************************************************************************************************************/


/*! \brief Manage Changeover settings and order of
 * Bandpass and Lowpass filters
 *
 * \retval none
 */
void filters_menu(void)
{
	static int8_t	current_selection;	// Keep track of current LCD menu selection
	static bool LCD_upd = FALSE;		// Keep track of LCD update requirements

	// Selection modified by encoder
	if (MENU_fromenc == TRUE)
	{
		current_selection += menu_steps_from_enc;
	    // Reset data from Encoder
		MENU_fromenc = FALSE;
		menu_steps_from_enc = 0;

		// Indicate that an LCD update is needed
		LCD_upd = FALSE;
	}

	// If LCD update is needed
	if (LCD_upd == FALSE)
	{
		LCD_upd = TRUE;					// We have serviced LCD

		// Keep Encoder Selection Within Bounds of the Menu Size
		uint8_t menu_size = filters_menu_size;
		while(current_selection >= menu_size)
			current_selection -= menu_size;
		while(current_selection < 0)
			current_selection += menu_size;

		xSemaphoreTake( mutexQueLCD, portMAX_DELAY );
		lcd_q_clear();
		lcd_q_goto(0,0);
		lcd_q_print("Filters Menu:");
   		xSemaphoreGive( mutexQueLCD );

		// Print the Rotary Encoder scroll Menu
		lcd_scroll_Menu((char**)filters_menu_items, menu_size, current_selection,
				1, /*column*/ 3, 3, menu2_lcd0, menu2_lcd1, menu2_lcd2, menu2_lcd3);
	}

	// Enact selection
	if (scan_menu_button() == SHORT_PUSH)
	{

		switch (current_selection)
		{
			case 0:
				menu_level = FILTER_BPF_SETPOINT_MENU;
				LCD_upd = FALSE;	// force LCD reprint
				break;
			case 1:
				menu_level = FILTER_BPF_ORDER_MENU;
				LCD_upd = FALSE;	// force LCD reprint
				break;
			case 2:
				menu_level = FILTER_LPF_SETPOINT_MENU;
				LCD_upd = FALSE;	// force LCD reprint
				break;
			case 3:
				menu_level = FILTER_LPF_ORDER_MENU;
				LCD_upd = FALSE;	// force LCD reprint
				break;
			case 4:
				xSemaphoreTake( mutexQueLCD, portMAX_DELAY );
				lcd_q_clear();
				lcd_q_goto(1,1);
		   		lcd_q_print("Done with Filters");
		   		xSemaphoreGive( mutexQueLCD );
				vTaskDelay(5000 );
				MENU_mode = TRUE;	// We're NOT done, just backing off
				menu_level = 0;		// We are done with this menu level
				LCD_upd = FALSE;
				break;
			default:
				xSemaphoreTake( mutexQueLCD, portMAX_DELAY );
				lcd_q_clear();
				lcd_q_goto(1,1);
		   		lcd_q_print("Done with Filters");
		   		xSemaphoreGive( mutexQueLCD );
				vTaskDelay(20000 );
				MENU_mode = FALSE;	// We're done
				menu_level = 0;		// We are done with this menu level
				LCD_upd = FALSE;
				TX_state = TRUE;	// Force housekeeping
				break;
		}
	}
}


/*! \brief PowerSDR-IQ LCD Frequency Offset during Receive
 * change settings
 *
 * \retval none
 */
void psdr_iq_menu(void)
{
	static int16_t	current_selection;	// Keep track of current LCD menu selection
	static bool LCD_upd = FALSE;		// Keep track of LCD update requirements

	// Get Current value
	current_selection = cdata.LCD_RX_Offset;


	// Selection modified by encoder
	if (MENU_fromenc == TRUE)
	{
		current_selection += menu_steps_from_enc;
	    // Reset data from Encoder
		MENU_fromenc = FALSE;
		menu_steps_from_enc = 0;

		// Indicate that an LCD update is needed
		LCD_upd = FALSE;
	}

	// If LCD update is needed
	if (LCD_upd == FALSE)
	{
		LCD_upd = TRUE;					// We have serviced LCD

		// Keep Encoder Selection Within Bounds
		int8_t max_value = 31;		// Highest possible Offset frequency
		int8_t min_value = -32;		// Lowest possible Offset frequency
		if(current_selection > max_value) current_selection = max_value;
		if(current_selection < min_value) current_selection = min_value;

		// Store Current value in running storage
		cdata.LCD_RX_Offset = current_selection;

		xSemaphoreTake( mutexQueLCD, portMAX_DELAY );
		lcd_q_clear();
		lcd_q_goto(0,0);
		lcd_q_print("PowerSDR-IQ Receive");
		lcd_q_goto(1,3);
		lcd_q_print("Frequency Offset:");
		lcd_q_goto(2,0);
		lcd_q_print("Rotate to Adjust");
		lcd_q_goto(3,0);
		lcd_q_print("Push to Save->");
		// Format current value for LCD print
		sprintf(menu_lcd0,"%3dkHz", current_selection);
		lcd_q_goto(3,14);
		lcd_q_print(menu_lcd0);
		xSemaphoreGive( mutexQueLCD );
	}

	// Enact selection by saving in permanent (nvram / EEPROM) storage
	if (scan_menu_button() == SHORT_PUSH)
	{
		// Save modified value
		if (current_selection != nvram_cdata.LCD_RX_Offset)
		{
			flashc_memset8((void *)&nvram_cdata.LCD_RX_Offset,
					current_selection, sizeof(cdata.LCD_RX_Offset), TRUE);
			xSemaphoreTake( mutexQueLCD, portMAX_DELAY );
			lcd_q_clear();
			lcd_q_goto(1,1);
			lcd_q_print("New Value Saved");
			xSemaphoreGive( mutexQueLCD );
		}

		vTaskDelay(5000 );		// Show on screen for 0.5 seconds
		MENU_fromenc = FALSE;	// Clear Encoder output
		LCD_upd = FALSE;		// force LCD reprint
		menu_level = 0;
		MENU_mode = TRUE;		// We're NOT done, just backing off
	}
}


/*! \brief Manage the Second Level I2C Settings,
 * change settings
 *
 * \retval none
 */
void i2c_menu_level2(void)
{
	static int16_t	current_selection;	// Keep track of current LCD menu selection
	static bool LCD_upd = FALSE;		// Keep track of LCD update requirements
	static bool FIRST_run = FALSE;		// Indicates if a consecutive run
	bool REBOOT_now	= FALSE;

	if (FIRST_run == FALSE)		// Fetch values if first time
	{
		if      (menu_level == I2C_SI570_MENU)current_selection = cdata.Si570_I2C_addr;
		else if (menu_level == I2C_PCF8574_MOBO_MENU)current_selection = cdata.PCF_I2C_Mobo_addr;
		else if (menu_level == I2C_PCF8574_LPF1_MENU)current_selection = cdata.PCF_I2C_lpf1_addr;
		else if (menu_level == I2C_PCF8574_LPF2_MENU)current_selection = cdata.PCF_I2C_lpf2_addr;
		else if (menu_level == I2C_PCF8574_FAN_MENU)current_selection = cdata.PCF_I2C_Ext_addr;
		else if (menu_level == I2C_TMP100_MENU)current_selection = cdata.TMP100_I2C_addr;
		else if (menu_level == I2C_AD5301_MENU)current_selection = cdata.AD5301_I2C_addr;
		else if (menu_level == I2C_AD7991_MENU)current_selection = cdata.AD7991_I2C_addr;

		FIRST_run = TRUE;		// We have run one time
	}

	// Here we use menu_steps_from_enc rather than val_steps_from_enc, because of
	// higher granularity (10 steps per rev, rather than 100 steps per rev)
	// Selection modified by encoder
	if (MENU_fromenc == TRUE)
	{
		current_selection += menu_steps_from_enc;
	    // Reset data from Encoder
		MENU_fromenc = FALSE;
		menu_steps_from_enc = 0;

		// Indicate that an LCD update is needed
		LCD_upd = FALSE;
	}

	// If LCD update is needed
	if (LCD_upd == FALSE)
	{
		LCD_upd = TRUE;					// We have serviced LCD

		// Keep Encoder Selection Within Bounds of the Menu Size
		uint8_t max_value = 127;		// Highest I2C address value
		uint8_t min_value = 0;			// Lowest I2C address value
		if(current_selection > max_value) current_selection = max_value;
		if(current_selection < min_value) current_selection = min_value;

		// Format current value for LCD print
		sprintf(menu_lcd0,"%3d",current_selection);

		// LCD Print Currently selected Value
		xSemaphoreTake( mutexQueLCD, portMAX_DELAY );
		lcd_q_clear();
		lcd_q_goto(0,0);
		lcd_q_print("I2C Address for");

		lcd_q_goto(1,0);
		switch (menu_level)
		{
			case I2C_SI570_MENU:
				lcd_q_print("Si570 VCXO:");
				break;
			case I2C_PCF8574_MOBO_MENU:
				lcd_q_print("PCF8574 Mobo:");
				break;
			case I2C_PCF8574_LPF1_MENU:
				lcd_q_print("PCF8574 LPF1:");
				break;
			case I2C_PCF8574_LPF2_MENU:
				lcd_q_print("PCF8574 LPF2:");
				break;
			case I2C_PCF8574_FAN_MENU:
				lcd_q_print("PCF8574 Fan:");
				break;
			case I2C_TMP100_MENU:
				lcd_q_print("TMP100/101:");
				break;
			case I2C_AD5301_MENU:
				lcd_q_print("AD5301:");
				break;
			case I2C_AD7991_MENU:
				lcd_q_print("AD7991:");
				break;
		}
		lcd_q_goto(2,0);
		lcd_q_print("Rotate to Adjust");
		lcd_q_goto(3,0);
		lcd_q_print("Push to Save->");
		lcd_q_goto(3,17);
		lcd_q_print(menu_lcd0);
		xSemaphoreGive( mutexQueLCD );
	}

	// Enact selection by saving in permanent (nvram / EEPROM) storage
	if (scan_menu_button() == SHORT_PUSH)
	{
		// Save modified value
		if (menu_level == I2C_SI570_MENU)
		{
			if (nvram_cdata.Si570_I2C_addr != current_selection)
			{
				flashc_memset8((void *)&nvram_cdata.Si570_I2C_addr, current_selection, sizeof(uint8_t), TRUE);
				REBOOT_now = TRUE;
			}
		}
		else if (menu_level == I2C_PCF8574_MOBO_MENU)
		{
			if (nvram_cdata.PCF_I2C_Mobo_addr != current_selection)
			{
				flashc_memset8((void *)&nvram_cdata.PCF_I2C_Mobo_addr, current_selection, sizeof(uint8_t), TRUE);
				REBOOT_now = TRUE;
			}
		}
		else if (menu_level == I2C_PCF8574_LPF1_MENU)
		{
			if (nvram_cdata.PCF_I2C_lpf1_addr != current_selection)
			{
				flashc_memset8((void *)&nvram_cdata.PCF_I2C_lpf1_addr, current_selection, sizeof(uint8_t), TRUE);
				REBOOT_now = TRUE;
			}
		}
		else if (menu_level == I2C_PCF8574_LPF2_MENU)
		{
			if (nvram_cdata.PCF_I2C_lpf2_addr != current_selection)
			{
				flashc_memset8((void *)&nvram_cdata.PCF_I2C_lpf2_addr, current_selection, sizeof(uint8_t), TRUE);
				REBOOT_now = TRUE;
			}
		}
		else if (menu_level == I2C_PCF8574_FAN_MENU)
		{
			if (nvram_cdata.PCF_I2C_Ext_addr != current_selection)
			{
				flashc_memset8((void *)&nvram_cdata.PCF_I2C_Ext_addr, current_selection, sizeof(uint8_t), TRUE);
				REBOOT_now = TRUE;
			}
		}
		else if (menu_level == I2C_TMP100_MENU)
		{
			if (nvram_cdata.TMP100_I2C_addr != current_selection)
			{
				flashc_memset8((void *)&nvram_cdata.TMP100_I2C_addr, current_selection, sizeof(uint8_t), TRUE);
				REBOOT_now = TRUE;
			}
		}
		else if (menu_level == I2C_AD5301_MENU)
		{
			if (nvram_cdata.AD5301_I2C_addr != current_selection)
			{
				flashc_memset8((void *)&nvram_cdata.AD5301_I2C_addr, current_selection, sizeof(uint8_t), TRUE);
				REBOOT_now = TRUE;
			}
		}
		else if (menu_level == I2C_AD7991_MENU)
		{
			if (nvram_cdata.AD7991_I2C_addr != current_selection)
			{
				flashc_memset8((void *)&nvram_cdata.AD7991_I2C_addr, current_selection, sizeof(uint8_t), TRUE);
				REBOOT_now = TRUE;
			}
		}

		if (REBOOT_now == TRUE)
		{
			xSemaphoreTake( mutexQueLCD, portMAX_DELAY );
			vTaskDelay(20000 );
			lcd_q_clear();
			lcd_q_goto(0,0);
	   		lcd_q_print("I2C Address Modified");
			lcd_q_goto(2,0);
	   		lcd_q_print("SDR Widget will be");
			lcd_q_goto(3,1);
	   		lcd_q_print("RESET into new mode");
	   		xSemaphoreGive( mutexQueLCD );
			vTaskDelay(20000 );
			wdt_enable(100000);					// Enable Watchdog with 100ms patience
			while (1);
		}
		else
		{
			MENU_fromenc = FALSE;	// Clear Encoder output
			LCD_upd = FALSE;		// force LCD reprint
			xSemaphoreTake( mutexQueLCD, portMAX_DELAY );
			lcd_q_clear();
			lcd_q_goto(1,1);
			lcd_q_print("Nothing Changed");
			xSemaphoreGive( mutexQueLCD );
			vTaskDelay(10000 );		// Show on screen for 1 second
			MENU_mode = TRUE;		// We're NOT done, just backing off
			menu_level = I2C_MENU;
		}
	}
}


/*! \brief Manage I2C address settings
 * Select which I2C address to change
 *
 * \retval none
 */
void i2c_menu(void)
{
	static int8_t	current_selection;	// Keep track of current LCD menu selection
	static bool LCD_upd = FALSE;		// Keep track of LCD update requirements

	// Selection modified by encoder
	if (MENU_fromenc == TRUE)
	{
		current_selection += menu_steps_from_enc;
	    // Reset data from Encoder
		MENU_fromenc = FALSE;
		menu_steps_from_enc = 0;

		// Indicate that an LCD update is needed
		LCD_upd = FALSE;
	}

	// If LCD update is needed
	if (LCD_upd == FALSE)
	{
		LCD_upd = TRUE;					// We have serviced LCD

		// Keep Encoder Selection Within Bounds of the Menu Size
		uint8_t menu_size = i2c_menu_size;
		while(current_selection >= menu_size)
			current_selection -= menu_size;
		while(current_selection < 0)
			current_selection += menu_size;

		// Indicate Current Frequency and the Frequency stored under the currently selected Channel number
		// The "stored" Frequency always changes according to which channel is currently selected.
		xSemaphoreTake( mutexQueLCD, portMAX_DELAY );
		lcd_q_clear();
		lcd_q_goto(0,0);
		lcd_q_print("I2C Addresses:");
		lcd_q_goto(2,0);
		lcd_q_print("Addr:");
		xSemaphoreGive( mutexQueLCD );

		if (current_selection < 8)
		{
			int8_t value=0;

			switch (current_selection)
			{
				case 0:
					value = cdata.Si570_I2C_addr;
					break;
				case 1:
					value = cdata.PCF_I2C_Mobo_addr;
					break;
				case 2:
					value = cdata.PCF_I2C_lpf1_addr;
					break;
				case 3:
					value = cdata.PCF_I2C_lpf2_addr;
					break;
				case 4:
					value = cdata.PCF_I2C_Ext_addr;
					break;
				case 5:
					value = cdata.TMP100_I2C_addr;
					break;
				case 6:
					value = cdata.AD5301_I2C_addr;
					break;
				case 7:
					value = cdata.AD7991_I2C_addr;
					break;
			}
			sprintf(menu_lcd0," %3d ", value);
		}
		else
		{
			sprintf(menu_lcd0,"  --  ");
		}
		xSemaphoreTake( mutexQueLCD, portMAX_DELAY );
		lcd_q_goto(3,0);
		lcd_q_print(menu_lcd0);
		xSemaphoreGive( mutexQueLCD );

		// Print the Rotary Encoder scroll Menu
		lcd_scroll_Menu((char**)i2c_menu_items, menu_size, current_selection,
				1, 6, 3, menu2_lcd0, menu2_lcd1, menu2_lcd2, menu2_lcd3);
	}

	// Enact selection
	if (scan_menu_button() == SHORT_PUSH)
	{

		switch (current_selection)
		{
			case 0:
				menu_level = I2C_SI570_MENU;
				LCD_upd = FALSE;	// force LCD reprint
				break;
			case 1:
				menu_level = I2C_PCF8574_MOBO_MENU;
				LCD_upd = FALSE;	// force LCD reprint
				break;
			case 2:
				menu_level = I2C_PCF8574_LPF1_MENU;
				LCD_upd = FALSE;	// force LCD reprint
				break;
			case 3:
				menu_level = I2C_PCF8574_LPF2_MENU;
				LCD_upd = FALSE;	// force LCD reprint
				break;
			case 4:
				menu_level = I2C_PCF8574_FAN_MENU;
				LCD_upd = FALSE;	// force LCD reprint
				break;
			case 5:
				menu_level = I2C_TMP100_MENU;
				LCD_upd = FALSE;	// force LCD reprint
				break;
			case 6:
				menu_level = I2C_AD5301_MENU;
				LCD_upd = FALSE;	// force LCD reprint
				break;
			case 7:
				menu_level = I2C_AD7991_MENU;
				LCD_upd = FALSE;	// force LCD reprint
				break;
			case 8:
				xSemaphoreTake( mutexQueLCD, portMAX_DELAY );
				lcd_q_clear();
				lcd_q_goto(1,1);
		   		lcd_q_print("Nothing Changed");
		   		xSemaphoreGive( mutexQueLCD );
				vTaskDelay(5000 );
				MENU_mode = TRUE;	// We're NOT done, just backing off
				menu_level = 0;		// We are done with this menu level
				LCD_upd = FALSE;
				break;
			default:
				xSemaphoreTake( mutexQueLCD, portMAX_DELAY );
				lcd_q_clear();
				lcd_q_goto(1,1);
		   		lcd_q_print("Nothing Saved");
		   		xSemaphoreGive( mutexQueLCD );
				vTaskDelay(20000 );
				MENU_mode = FALSE;	// We're done
				menu_level = 0;		// We are done with this menu level
				LCD_upd = FALSE;
				TX_state = TRUE;	// Force housekeeping
				break;
		}
	}
}


/*! \brief Calibrate the Si570 output frequency against a known source
 *
 * \retval none
 */
void si570_menu(void)
{
	static int32_t	current_selection;	// Keep track of current LCD menu selection
	static bool LCD_upd = FALSE;		// Keep track of LCD update requirements
	double freq_display;

	// Get Current value
	current_selection = cdata.FreqXtal;


	// Selection modified by encoder
	if (FRQ_fromenc == TRUE)
	{
		current_selection += freq_delta_from_enc;
	    // Reset data from Encoder
		FRQ_fromenc = FALSE;
		freq_delta_from_enc = 0;

		// Indicate that an LCD update is needed
		LCD_upd = FALSE;
	}

	// If LCD update is needed
	if (LCD_upd == FALSE)
	{
		LCD_upd = TRUE;					// We have serviced LCD

		// Store Updated value in running storage
		cdata.FreqXtal = current_selection;
		// Recalibrate Si570 running frequency using new value
		freq_from_usb = cdata.Freq[0];  // Refresh frequency
		FRQ_fromusb = TRUE;

		xSemaphoreTake( mutexQueLCD, portMAX_DELAY );
		lcd_q_clear();
		lcd_q_goto(0,0);
		lcd_q_print("Si570 Calibration:");
		lcd_q_goto(1,0);
		lcd_q_print("Set Frq:");
		freq_display = (double)cdata.Freq[0]/(1<<23);
   		sprintf(menu_lcd0,"%2.06fMHz", freq_display);
   		if(freq_display<=10) lcd_q_print(" ");		// workaround...%2 print format doesn't work
		lcd_q_print(menu_lcd0);

		lcd_q_goto(2,0);
		lcd_q_print("Rotate to Adjust and");
		lcd_q_goto(3,0);
		lcd_q_print("Push-> ");
		// Format current value for LCD print
		freq_display = (double)cdata.FreqXtal/(1<<24);
   		sprintf(menu_lcd1,"%3.06fMHz", freq_display);
		lcd_q_print(menu_lcd1);
		xSemaphoreGive( mutexQueLCD );
	}

	// Enact selection by saving in permanent (nvram / EEPROM) storage
	if (scan_menu_button() == SHORT_PUSH)
	{
		// Save modified value
		if (current_selection != nvram_cdata.FreqXtal)
		{
			flashc_memset32((void *)&nvram_cdata.FreqXtal,
					current_selection, sizeof(cdata.FreqXtal), TRUE);
			xSemaphoreTake( mutexQueLCD, portMAX_DELAY );
			lcd_q_clear();
			lcd_q_goto(1,1);
			lcd_q_print("New Value Saved");
			xSemaphoreGive( mutexQueLCD );
		}

		vTaskDelay(5000 );		// Show on screen for 0.5 seconds
		MENU_fromenc = FALSE;	// Clear Encoder output
		LCD_upd = FALSE;		// force LCD reprint
		menu_level = 0;
		MENU_mode = TRUE;		// We're NOT done, just backing off
	}
}


/***************************************************************************************************************************/
// Todo
/*! \brief Adjust Frequency Add/Subtract and Multiply parameters
 *
 * \retval none
 */
void frqaddsubmul_menu(void)
{
	static bool LCD_upd = FALSE;	// Keep track of LCD update requirements

	xSemaphoreTake( mutexQueLCD, portMAX_DELAY );
	lcd_q_clear();
	lcd_q_goto(1,1);
	lcd_q_print("Not Implemented yet");
	xSemaphoreGive( mutexQueLCD );
	vTaskDelay(20000 );
	MENU_mode = TRUE;				// We're NOT done, just backing off
	menu_level = 0;					// We are done with this menu level
	LCD_upd = FALSE;
}
/***************************************************************************************************************************/


/*! \brief Rotary Encoder Resolution
 * change settings
 *
 * \retval none
 */
void encoder_menu(void)
{
	int8_t	current_selection = 0;		// Keep track of current LCD menu selection
	static bool LCD_upd = FALSE;		// Keep track of LCD update requirements

	#define ENCODER 16
	const uint16_t encoder_values[] =
		{ 16, 20, 24, 32, 36, 48, 64,
		  72, 96, 128, 256, 512, 1024,
		  2048, 4096, 8192 };

	// Get Current value
	uint8_t i;
	for(i = 0; i<ENCODER; i++)
	{
		if (cdata.Resolvable_States == encoder_values[i])
		{
			current_selection = i;
		}
	}

	// Selection modified by encoder
	if (VAL_fromenc == TRUE)
	{
		if (val_steps_from_enc > 0) current_selection++;
		if (val_steps_from_enc < 0)	current_selection--;
	    // Reset data from Encoder
		VAL_fromenc = FALSE;
		val_steps_from_enc = 0;

		// Indicate that an LCD update is needed
		LCD_upd = FALSE;
	}

	// If LCD update is needed
	if (LCD_upd == FALSE)
	{
		LCD_upd = TRUE;					// We have serviced LCD

		// Keep Encoder Selection Within Bounds
		if (current_selection >= ENCODER) current_selection = 0;
		if (current_selection < 0) current_selection = ENCODER-1;

		// Store Current value in running storage
		cdata.Resolvable_States = encoder_values[current_selection];

		xSemaphoreTake( mutexQueLCD, portMAX_DELAY );
		lcd_q_clear();
		lcd_q_goto(0,0);
		lcd_q_print("Rotary Encoder");
		lcd_q_goto(1,9);
		lcd_q_print("Resolution:");
		lcd_q_goto(2,0);
		lcd_q_print("Rotate to Adjust");
		lcd_q_goto(3,0);
		lcd_q_print("Push to Save->");
		// Format current value for LCD print
		sprintf(menu_lcd0,"%4d", encoder_values[current_selection]);
		lcd_q_goto(3,16);
		lcd_q_print(menu_lcd0);
		xSemaphoreGive( mutexQueLCD );
	}

	// Enact selection by saving in permanent (nvram / EEPROM) storage
	if (scan_menu_button() == SHORT_PUSH)
	{
		// Save modified value
		if (cdata.Resolvable_States != nvram_cdata.Resolvable_States)
		{
			flashc_memset16((void *)&nvram_cdata.Resolvable_States,
					cdata.Resolvable_States, sizeof(cdata.Resolvable_States), TRUE);
			xSemaphoreTake( mutexQueLCD, portMAX_DELAY );
			lcd_q_clear();
			lcd_q_goto(1,1);
			lcd_q_print("New Value Saved");
			xSemaphoreGive( mutexQueLCD );
		}

		vTaskDelay(5000 );		// Show on screen for 0.5 seconds
		MENU_fromenc = FALSE;	// Clear Encoder output
		LCD_upd = FALSE;		// force LCD reprint
		menu_level = 0;
		MENU_mode = TRUE;		// We're NOT done, just backing off
	}
}


/*! \brief Manage the UAC1/UAC2 Menu
 *
 * \retval none
 */
void uac_menu(void)
{
	static int8_t	current_selection;	// Keep track of current LCD menu selection
	static bool LCD_upd = FALSE;		// Keep track of LCD update requirements

	// Selection modified by encoder
	if (MENU_fromenc == TRUE)
	{
		current_selection += menu_steps_from_enc;
	    // Reset data from Encoder
		MENU_fromenc = FALSE;
		menu_steps_from_enc = 0;

		// Indicate that an LCD update is needed
		LCD_upd = FALSE;
	}

	// If LCD update is needed
	if (LCD_upd == FALSE)
	{
		LCD_upd = TRUE;					// We have serviced LCD

		// Keep Encoder Selection Within Bounds of the Menu Size
		uint8_t menu_size = uac_menu_size;
		while(current_selection >= menu_size)
			current_selection -= menu_size;
		while(current_selection < 0)
			current_selection += menu_size;

		xSemaphoreTake( mutexQueLCD, portMAX_DELAY );
		lcd_q_clear();
		lcd_q_goto(0,0);
		if (cdata.UAC2_Audio == TRUE)
			lcd_q_print("Current sel: UAC2");
		else
			lcd_q_print("Current sel: UAC1");
   		xSemaphoreGive( mutexQueLCD );

		// Print the Menu
		lcd_scroll_Menu((char**)uac_menu_items, menu_size, current_selection,
				1, 1, 3, menu_lcd0, menu_lcd1, menu_lcd2, menu_lcd3);
	}

	// Enact selection
	if (scan_menu_button() == SHORT_PUSH)
	{

		switch (current_selection)
		{
			case 0: // UAC1
				//cdata.UAC2_Audio = FALSE;
				flashc_memset8((void *)&nvram_cdata.UAC2_Audio, FALSE, sizeof(uint8_t), TRUE);
				// Display result for 2 seconds
				xSemaphoreTake( mutexQueLCD, portMAX_DELAY );
				lcd_q_clear();
				lcd_q_goto(0,1);
		   		lcd_q_print("UAC1 Mode Selected");
				lcd_q_goto(2,0);
		   		lcd_q_print("SDR Widget will be");
				lcd_q_goto(3,1);
		   		lcd_q_print("RESET into new mode");
		   		xSemaphoreGive( mutexQueLCD );
				vTaskDelay(20000 );
				wdt_enable(100000);					// Enable Watchdog with 100ms patience
				while (1);							// Bye bye, Reset to Enact change
			case 1: // UAC2
				//cdata.UAC2_Audio = FALSE;
				flashc_memset8((void *)&nvram_cdata.UAC2_Audio, TRUE, sizeof(uint8_t), TRUE);
				// Display result for 2 seconds
				xSemaphoreTake( mutexQueLCD, portMAX_DELAY );
				lcd_q_clear();
				lcd_q_goto(0,1);
		   		lcd_q_print("UAC2 Mode Selected");
				lcd_q_goto(2,0);
		   		lcd_q_print("SDR Widget will be");
				lcd_q_goto(3,1);
		   		lcd_q_print("RESET into new mode");
		   		xSemaphoreGive( mutexQueLCD );
				vTaskDelay(20000 );
				wdt_enable(100000);					// Enable Watchdog with 100ms patience
				while (1);							// Bye bye, Reset to Enact change
			default:
				xSemaphoreTake( mutexQueLCD, portMAX_DELAY );
				lcd_q_clear();
				lcd_q_goto(1,1);
				if (cdata.UAC2_Audio == TRUE)
					lcd_q_print("UAC2 mode unchanged");
				else
					lcd_q_print("UAC1 mode unchanged");
		   		xSemaphoreGive( mutexQueLCD );
				vTaskDelay(20000 );
				MENU_mode = FALSE;	// Drop out of Menu
				LCD_upd = FALSE;	// force LCD reprint
				TX_state = TRUE;	// Force housekeeping
				menu_level = 0;		// We are done with this menu level
				break;
		}
	}
}


/*! \brief Factory Reset with all default values
 *
 * \retval none
 */
void factory_menu(void)
{
	static int8_t	current_selection;	// Keep track of current LCD menu selection
	static bool LCD_upd = FALSE;		// Keep track of LCD update requirements

	// Selection modified by encoder
	if (MENU_fromenc == TRUE)
	{
		current_selection += menu_steps_from_enc;
	    // Reset data from Encoder
		MENU_fromenc = FALSE;
		menu_steps_from_enc = 0;

		// Indicate that an LCD update is needed
		LCD_upd = FALSE;
	}

	// If LCD update is needed
	if (LCD_upd == FALSE)
	{
		LCD_upd = TRUE;					// We have serviced LCD

		// Keep Encoder Selection Within Bounds of the Menu Size
		uint8_t menu_size = factory_menu_size;
		while(current_selection >= menu_size)
			current_selection -= menu_size;
		while(current_selection < 0)
			current_selection += menu_size;

		xSemaphoreTake( mutexQueLCD, portMAX_DELAY );
		lcd_q_clear();
		lcd_q_goto(0,0);
		lcd_q_print("Set all to default:");
   		xSemaphoreGive( mutexQueLCD );

		// Print the Rotary Encoder scroll Menu
		lcd_scroll_Menu((char**)factory_menu_items, menu_size, current_selection,
				1, /*column*/ 3, 6, menu2_lcd0, menu2_lcd1, menu2_lcd2, menu2_lcd3);
	}

	// Enact selection
	if (scan_menu_button() == SHORT_PUSH)
	{
		switch (current_selection)
		{
			case 0: // Factory Reset
				// Force an EEPROM update:
				flashc_memset8((void *)&nvram_cdata.EEPROM_init_check, 0xff, sizeof(uint8_t), TRUE);
				xSemaphoreTake( mutexQueLCD, portMAX_DELAY );
				lcd_q_clear();
				lcd_q_goto(0,0);
				lcd_q_print("Factory Reset was");
				lcd_q_goto(1,0);
				lcd_q_print("selected.");
				lcd_q_goto(2,0);
				lcd_q_print("All settings reset");
				lcd_q_goto(3,0);
				lcd_q_print("to default values.");
				xSemaphoreGive( mutexQueLCD );
				vTaskDelay(20000 );
				// Reboot by Watchdog timer
				wdt_enable(100000);	// Enable Watchdog with 100ms patience
				while (1);			// Bye bye, Reset to Enact change
			case 1:
				xSemaphoreTake( mutexQueLCD, portMAX_DELAY );
				lcd_q_clear();
				lcd_q_goto(1,1);
		   		lcd_q_print("Nothing Changed");
		   		xSemaphoreGive( mutexQueLCD );
				vTaskDelay(5000 );
				MENU_mode = TRUE;	// We're NOT done, just backing off
				menu_level = 0;		// We are done with this menu level
				LCD_upd = FALSE;
				break;
			default:
				xSemaphoreTake( mutexQueLCD, portMAX_DELAY );
				lcd_q_clear();
				lcd_q_goto(1,1);
		   		lcd_q_print("Nothing Changed");
		   		xSemaphoreGive( mutexQueLCD );
				vTaskDelay(20000 );
				MENU_mode = FALSE;	// We're done
				menu_level = 0;		// We are done with this menu level
				LCD_upd = FALSE;
				TX_state = TRUE;	// Force housekeeping
				break;
		}
	}
}


/*! \brief Manage the first level of Menus
 *
 * \retval none
 */
void menu_level0(void)
{
	static int8_t	current_selection;	// Keep track of current LCD menu selection
	static bool LCD_upd = FALSE;		// Keep track of LCD update requirements

	// Selection modified by encoder.  We remember last selection, even if exit and re-entry
	if (MENU_fromenc == TRUE)
	{
		current_selection += menu_steps_from_enc;
	    // Reset data from Encoder
		MENU_fromenc = FALSE;
		menu_steps_from_enc = 0;

		// Indicate that an LCD update is needed
		LCD_upd = FALSE;
	}

	if (LCD_upd == FALSE)				// Need to update LCD
	{
		LCD_upd = TRUE;					// We have serviced LCD

		// Keep Encoder Selection Within Bounds of the Menu Size
		uint8_t menu_size = level0_menu_size;
		while(current_selection >= menu_size)
			current_selection -= menu_size;
		while(current_selection < 0)
			current_selection += menu_size;

		xSemaphoreTake( mutexQueLCD, portMAX_DELAY );
		lcd_q_clear();
		lcd_q_goto(0,0);
   		lcd_q_print("Configuration Menu:");
   		xSemaphoreGive( mutexQueLCD );

		// Print the Menu
		lcd_scroll_Menu((char**)level0_menu_items, menu_size, current_selection,
				1, 0, 3, menu_lcd0, menu_lcd1, menu_lcd2, menu_lcd3);
	}

	if (scan_menu_button() == SHORT_PUSH)
	{
	    // Reset higher resolution data from Encoder, used by some lower level menus
		VAL_fromenc = FALSE;
		FRQ_fromenc = FALSE;

		switch (current_selection)
		{
			case 0: // Save Frequency in Long Term (nvram) Memory
				menu_level = FREQ_MENU;
				LCD_upd = FALSE;	// force LCD reprint
				break;
			case 1: // VFO Resolution 1/2/5/10/50/100kHz per revolution
				menu_level = VFORES_MENU;
				LCD_upd = FALSE;	// force LCD reprint
				break;
			case 2: // PCF8574 Control
				menu_level = PCF8574_MENU;
				LCD_upd = FALSE;	// force LCD reprint
				break;
			case 3: // Power and SWR Management
				menu_level = PSWR_MENU;
				LCD_upd = FALSE;	// force LCD reprint
				break;
			case 4: // Bias Management
				menu_level = BIAS_MENU;
				LCD_upd = FALSE;	// force LCD reprint
				break;
			case 5: // PA Temperature Management
				menu_level = TMP_MENU;
				LCD_upd = FALSE;	// force LCD reprint
				break;
			case 6: // Filters Management
				menu_level = FILTERS_MENU;
				LCD_upd = FALSE;	// force LCD reprint
				break;
			case 7: // PSDR-IQ RX Offset Frequency
				menu_level = PSDR_IQ_MENU;
				LCD_upd = FALSE;	// force LCD reprint
				break;
			case 8: // I2C Address Management
				menu_level = I2C_MENU;
				LCD_upd = FALSE;	// force LCD reprint
				break;
			case 9: // Si570 Calibrate
				menu_level = SI570_MENU;
				LCD_upd = FALSE;	// force LCD reprint
				break;
			case 10:// Frequency Add/Subtract/Multiply
				menu_level = ADDSUBMUL;
				LCD_upd = FALSE;	// force LCD reprint
				break;
			case 11:// Encoder Resolution
				menu_level = ENCODER_MENU;
				LCD_upd = FALSE;	// force LCD reprint
				break;
			case 12: // UAC1/UAC2 Audio feature selection
				menu_level = UAC_MENU;
				LCD_upd = FALSE;	// force LCD reprint
				break;
			case 13: // Factory Reset
				menu_level = FACTORY_MENU;
				LCD_upd = FALSE;	// force LCD reprint
				break;
			default:
				// Exit
				xSemaphoreTake( mutexQueLCD, portMAX_DELAY );
				lcd_q_clear();
				lcd_q_goto(1,1);
		   		lcd_q_print("Return from Menu");
		   		xSemaphoreGive( mutexQueLCD );
				vTaskDelay(20000 );
				MENU_mode = FALSE;	// We're done
				LCD_upd = FALSE;
				TX_state = TRUE;	// Force housekeeping
		}
	}
}


/*! \brief Scan the Frequency/Menu Push Button and delegate tasks accordingly
 *
 * \retval none
 */
static void vtaskPushButtonMenu( void * pcParameters )
{
	gpio_enable_pin_pull_up(MENU_BUTTON);		// Enable pullup for the Frequency/Menu button

	// Wait for 9 seconds while the Mobo Stuff catches up
	vTaskDelay( 90000 );

    while( 1 )
    {
    	//
    	// Actions outside of Menu Mode
    	//
    	if (!MENU_mode)
    	{
    	   	uint8_t push;

    	   	push = scan_menu_button();

    		// Flip Frequency memories
    	    if (push == SHORT_PUSH)
    	    {
    	    	cdata.Freq[cdata.SwitchFreq] = cdata.Freq[0];	// Store current frequency in short term memory
    	    	cdata.SwitchFreq++;								// Rotate through short term Frequency Memories
    	    	if (cdata.SwitchFreq > 9) cdata.SwitchFreq = 1;
    	    	cdata.Freq[0] = cdata.Freq[cdata.SwitchFreq];	// Fetch next stored frequency channel
    	    	// Trigger a frequency update
    	    	FRQ_fromenc = TRUE;
    	    }
    	    // We have been asked to enter Menu mode
    	    else if (push == LONG_PUSH)
    	    {
       	    	MENU_mode = TRUE;	// Enter Menu mode (also grabs total ownership of LCD from other tasks,
									// provided that they are wrapped with a "if(!MENU_mode){...}"

       	    	// We're done with data from Encoder, zero/initialze
       	    	MENU_fromenc = FALSE;
       	    	menu_steps_from_enc = 0;
    	   	}
    	}
    	//
    	// Menu Mode
    	//
    	else
    	{
    		//---------------------------------------
    		// Manage Menu once every 105ms (21*5ms)
    		//---------------------------------------
    		static uint8_t menu_manage = 0;
   	 		menu_manage++;
    		if (menu_manage == 21) menu_manage = 0;
    		if (menu_manage == 0)
    		{
        		// Select which menu level to manage
    			if (menu_level == 0) menu_level0();						//done

    			else if (menu_level == FREQ_MENU) freq_menu();			//done

    			else if (menu_level == VFORES_MENU) vfores_menu();		//done

    			else if (menu_level == PCF8574_MENU) pcf8574_menu();

    			else if (menu_level == PSWR_MENU) pswr_menu();

        		else if (menu_level == BIAS_MENU) bias_menu();

        		else if (menu_level == TMP_MENU) temperature_menu();	//done
    			else if (menu_level == TMP_CASE0_MENU) temperature_menu_level2();//done
    			else if (menu_level == TMP_CASE1_MENU) temperature_menu_level2();//done
    			else if (menu_level == TMP_CASE2_MENU) temperature_menu_level2();//done

    			else if (menu_level == FILTERS_MENU) filters_menu();	//part
    			else if (menu_level == FILTER_BPF_SETPOINT_MENU) filters_setpoint_menu_level2();//done
    			else if (menu_level == FILTER_LPF_SETPOINT_MENU) filters_setpoint_menu_level2();//done
    			else if (menu_level == FILTER_BPF_ORDER_MENU) filters_order_menu_level2();
    			else if (menu_level == FILTER_LPF_ORDER_MENU) filters_order_menu_level2();
    			else if (menu_level == FILTER_BPF_ADJUST_MENU)filters_adjust_menu_level3();//done
    			else if (menu_level == FILTER_LPF_ADJUST_MENU)filters_adjust_menu_level3();//done

    			else if (menu_level == PSDR_IQ_MENU) psdr_iq_menu();	//done

    			else if (menu_level == I2C_MENU) i2c_menu();			//done
    			else if (menu_level == I2C_SI570_MENU) i2c_menu_level2();//done
    			else if (menu_level == I2C_PCF8574_MOBO_MENU) i2c_menu_level2();//done
    			else if (menu_level == I2C_PCF8574_LPF1_MENU) i2c_menu_level2();//done
    			else if (menu_level == I2C_PCF8574_LPF2_MENU) i2c_menu_level2();//done
    			else if (menu_level == I2C_PCF8574_FAN_MENU) i2c_menu_level2();//done
    			else if (menu_level == I2C_TMP100_MENU) i2c_menu_level2();//done
    			else if (menu_level == I2C_AD5301_MENU) i2c_menu_level2();//done
    			else if (menu_level == I2C_AD7991_MENU) i2c_menu_level2();//done

    			else if (menu_level == SI570_MENU) si570_menu();		//done

    			else if (menu_level == ADDSUBMUL) frqaddsubmul_menu();

    			else if (menu_level == ENCODER_MENU) encoder_menu();	//done

    			else if (menu_level == UAC_MENU) uac_menu();			//done

    			else if (menu_level == FACTORY_MENU) factory_menu();	//done
    		}
    	}

    	// Delay as defined in FreeRTOSConfig.h, initially 10ms
        vTaskDelay(configTSK_PBTNMENU_PERIOD );
    }
}


/*! \brief RTOS initialisation of the PowerDisplay task
 *
 * \retval none
 */
void vStartTaskPushButtonMenu(void)
{
	xStatus = xTaskCreate( vtaskPushButtonMenu,
						   configTSK_PBTNMENU_NAME,
                           configTSK_PBTNMENU_STACK_SIZE,
                           NULL, 
       					   configTSK_PBTNMENU_PRIORITY,
                         ( xTaskHandle * ) NULL );
}

#endif