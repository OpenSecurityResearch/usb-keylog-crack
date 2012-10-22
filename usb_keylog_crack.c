/* usb_keylog_crack.c - Unlock code brute forcers for USB 
 *			keyloggers
 *
 * Originally written by Michael G. Spohn
 * Updated by brad.antoniewicz@foundstone.com
 * see blog.opensecurityresearch.com for more info
 *
 *
 * Based on example code from:
 * Keyboard example with debug channel, for Teensy USB Development Board
 * http://www.pjrc.com/teensy/usb_keyboard.html
 * Copyright (c) 2008 PJRC.COM, LLC
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "usb_keyboard_debug.h"
#include "print.h"
#include "usb_keylog_crack.h"
#include <string.h>
#include <stdio.h>
//#include "usb_debug_only.h"


#define LED_CONFIG	(DDRD |= (1<<6))
// Teensy 2.0: LED is active high
#if defined(__AVR_ATmega32U4__) || defined(__AVR_AT90USB1286__)
#define LED_ON		(PORTD |= (1<<6))
#define LED_OFF		(PORTD &= ~(1<<6))
#endif

#define CPU_PRESCALE(n)	(CLKPR = 0x80, CLKPR = (n))

void led_flash();
void unlock_it();
void reset_key_codes();
void send_valid_string();

uint8_t number_keys[10]=
	{KEY_0,KEY_1,KEY_2,KEY_3,KEY_4,KEY_5,KEY_6,KEY_7,KEY_8,KEY_9};

uint16_t idle_count=0;
const int DELAY_MS = 75;	// Do not set below 75 or you will overrun buffers!

// Session ranges for full keyboard (48 keys)
/*
const int SESSION_ONE_START = 0;
const int SESSION_ONE_STOP  = 12;

const int SESSION_TWO_START = 12;
const int SESSION_TWO_STOP  = 23;

const int SESSION_THREE_START = 23;
const int SESSION_THREE_STOP  = 35;

const int SESSION_FOUR_START = 35;
const int SESSION_FOUR_STOP  = 48;
*/

// Session ranges for Alpha's only (26 keys)
const int SESSION_ONE_START = 0;
const int SESSION_ONE_STOP  = 12;

const int SESSION_TWO_START = 12;
const int SESSION_TWO_STOP  = 26;


int main(void)
{
	// set for 16 MHz clock
	CPU_PRESCALE(0);
	LED_CONFIG;

	// Initialize the USB, and then wait for the host to set configuration.
	// If the Teensy is powered without a PC connected to the USB port,
	// this will wait forever.
	usb_init();
	while (!usb_configured()) /* wait */ ;

	// Wait an extra second for the PC's operating system to load drivers
	// and do whatever it does to actually be ready for input
	_delay_ms(1000);
	
	// Turn on LED so we know app is running
	LED_ON;
	_delay_ms(5000);
	led_flash();
	
	// Fill thefghjkl arrays with keycode information
	memcpy(crack_code_0, key_codes_alpha, sizeof(key_codes_alpha));
	memcpy(crack_code_1, key_codes_alpha, sizeof(key_codes_alpha));
	memcpy(crack_code_2, key_codes_alpha, sizeof(key_codes_alpha));

	int x=0,y=0,z=0;
    int lCrackCount = 0;

	print("Keylog crack application\n");
	print("Written by Michael G. Spohn\n");
	print("June 3, 2012\n");

	send_valid_string();	// "Foundstone investigative services."


	// Main loop		  // Reset code is "PSN"
	for( x=0;x<26; x++)   
	{
		reset_key_codes();

		// Set and send 1st keycode
		keyboard_keys[0] = crack_code_0[x];
		usb_keyboard_send();
		_delay_ms(DELAY_MS);

		for(y=0; y<KEY_CODE_ARRAY_ALPHA_LEN; y++)
		{
			if(crack_code_1[y] == crack_code_0[x])
				continue;
		 
			// Make sure 2nd keycode is empty
			keyboard_keys[1] = 0;
			usb_keyboard_send();
			_delay_ms(DELAY_MS);
			
			// Make sure 3rd keycode is empty
			keyboard_keys[2] = 0;
			usb_keyboard_send();
			_delay_ms(DELAY_MS);
		
			// Press and send 2nd keycode	
    		keyboard_keys[1] = crack_code_1[y];
			usb_keyboard_send();
			_delay_ms(DELAY_MS);
			
			for(z=0; z<KEY_CODE_ARRAY_ALPHA_LEN; z++)
			{
				if((crack_code_2[z] == crack_code_1[y]) )
					continue;
					
		     	// Do not send the Reset Code! (PSN)
				if((crack_code_0[x] == KEY_P) && (crack_code_1[y] == KEY_S) && (crack_code_2[z] == KEY_N))
				{	
					print("Excluding reset code!\n");
					phex16(lCrackCount); print(") "); phex16(crack_code_0[x]); print(" "); phex16(crack_code_1[y]); print(" "); phex16(crack_code_2[z]); print("\n");
					continue;
				}
				if((crack_code_0[x] == KEY_S) && (crack_code_1[y] == KEY_N) && (crack_code_2[z] == KEY_P))
				{	
					print("Excluding reset code!\n");
                    phex16(lCrackCount); print(") "); phex16(crack_code_0[x]); print(" "); phex16(crack_code_1[y]); print(" "); phex16(crack_code_2[z]); print("\n");				
					continue;
				}
				if((crack_code_0[x] == KEY_N) && (crack_code_1[y] == KEY_P) && (crack_code_2[z] == KEY_S))
				{	
					print("Excluding reset code!\n");
				    phex16(lCrackCount); print(") "); phex16(crack_code_0[x]); print(" "); phex16(crack_code_1[y]); print(" "); phex16(crack_code_2[z]); print("\n");
					continue;
				}
				if((crack_code_0[x] == KEY_P) && (crack_code_1[y] == KEY_N) && (crack_code_2[z] == KEY_S))
				{	
					print("Excluding reset code!\n");
					phex16(lCrackCount); print(") "); phex16(crack_code_0[x]); print(" "); phex16(crack_code_1[y]); print(" "); phex16(crack_code_2[z]); print("\n");
					continue;
				}
				if((crack_code_0[x] == KEY_S) && (crack_code_1[y] == KEY_P) && (crack_code_2[z] == KEY_N))
				{	
					print("Excluding reset code!\n");
					phex16(lCrackCount); print(") "); phex16(crack_code_0[x]); print(" "); phex16(crack_code_1[y]); print(" "); phex16(crack_code_2[z]); print("\n");
				
					keyboard_keys[2] = 0;
					usb_keyboard_send();
					_delay_ms(DELAY_MS);			
		     	
					continue;
				}
				if((crack_code_0[x] == KEY_N) && (crack_code_1[y] == KEY_S) && (crack_code_2[z] == KEY_P))
				{	
					print("Excluding reset code!\n");
					phex16(lCrackCount); print(") "); phex16(crack_code_0[x]); print(" "); phex16(crack_code_1[y]); print(" "); phex16(crack_code_2[z]); print("\n");
					
					keyboard_keys[2] = 0;
					usb_keyboard_send();
					_delay_ms(DELAY_MS);			
		     	
					continue;
				}
				
				// Make sure 3rd keycode is empty
				keyboard_keys[2] = 0;
				usb_keyboard_send();
				_delay_ms(DELAY_MS);			
		
				// Set and send 3rd keycode
				keyboard_keys[2] = crack_code_2[z];
				usb_keyboard_send();
				_delay_ms(DELAY_MS);

				lCrackCount++;
				phex16(lCrackCount); print(") "); print("Trying: "); pchar(*key_char_alpha[x]); pchar(*key_char_alpha[y]); pchar(*key_char_alpha[z]); print(" (");  phex16(crack_code_0[x]); print(" "); phex16(crack_code_1[y]); print(" "); phex16(crack_code_2[z]); print(")\n");
				if(lCrackCount % 100 == 0)
				{	
					led_flash();
				}
			}
		}
		
	}

	reset_key_codes();
	print("Reset Key Codes..\n");

	print("Sleeping...\n");
	while(1)
	{
     _delay_ms(50000);
	}
}

void led_flash()
{
	LED_OFF;
	_delay_ms(100);
	LED_ON;
	_delay_ms(100);
	LED_OFF;
}


void reset_key_codes()
{
			keyboard_keys[0] = 0;
			usb_keyboard_send();
			_delay_ms(DELAY_MS);

			keyboard_keys[1] = 0;
			usb_keyboard_send();
			_delay_ms(DELAY_MS);

			keyboard_keys[2] = 0;
			usb_keyboard_send();
			_delay_ms(DELAY_MS);
}

void send_valid_string()
{
	reset_key_codes();
	usb_keyboard_press(KEY_ENTER, 0);
	_delay_ms(DELAY_MS);
	usb_keyboard_press(KEY_F, 0x02);
	_delay_ms(DELAY_MS);
	usb_keyboard_press(KEY_O, 0);
	_delay_ms(DELAY_MS);	
	usb_keyboard_press(KEY_U, 0);
	_delay_ms(DELAY_MS);	
	usb_keyboard_press(KEY_N, 0);
	_delay_ms(DELAY_MS);	
	usb_keyboard_press(KEY_D, 0);
	_delay_ms(DELAY_MS);	
	usb_keyboard_press(KEY_S, 0);
	_delay_ms(DELAY_MS);	
	usb_keyboard_press(KEY_T, 0);
	_delay_ms(DELAY_MS);	
	usb_keyboard_press(KEY_O, 0);
	_delay_ms(DELAY_MS);	
	usb_keyboard_press(KEY_N, 0);
	_delay_ms(DELAY_MS);	
	usb_keyboard_press(KEY_E, 0);
	_delay_ms(DELAY_MS);	
	usb_keyboard_press(KEY_SPACE, 0);
	_delay_ms(DELAY_MS);	
	usb_keyboard_press(KEY_I, 0);
	_delay_ms(DELAY_MS);	
	usb_keyboard_press(KEY_N, 0);
	_delay_ms(DELAY_MS);	
	usb_keyboard_press(KEY_V, 0);
	_delay_ms(DELAY_MS);	
	usb_keyboard_press(KEY_E, 0);
	_delay_ms(DELAY_MS);	
	usb_keyboard_press(KEY_S, 0);
	_delay_ms(DELAY_MS);	
	usb_keyboard_press(KEY_T, 0);
	_delay_ms(DELAY_MS);	
	usb_keyboard_press(KEY_I, 0);
	_delay_ms(DELAY_MS);	
	usb_keyboard_press(KEY_G, 0);
	_delay_ms(DELAY_MS);	
	usb_keyboard_press(KEY_A, 0);
	_delay_ms(DELAY_MS);	
	usb_keyboard_press(KEY_T, 0);
	_delay_ms(DELAY_MS);	
	usb_keyboard_press(KEY_I, 0);
	_delay_ms(DELAY_MS);	
	usb_keyboard_press(KEY_V, 0);
	_delay_ms(DELAY_MS);	
	usb_keyboard_press(KEY_E, 0);
	_delay_ms(DELAY_MS);	
	usb_keyboard_press(KEY_SPACE, 0);
	_delay_ms(DELAY_MS);	
	usb_keyboard_press(KEY_S, 0);
	_delay_ms(DELAY_MS);	
	usb_keyboard_press(KEY_E, 0);
	_delay_ms(DELAY_MS);	
	usb_keyboard_press(KEY_R, 0);
	_delay_ms(DELAY_MS);	
	usb_keyboard_press(KEY_V, 0);
	_delay_ms(DELAY_MS);	
	usb_keyboard_press(KEY_I, 0);
	_delay_ms(DELAY_MS);	
	usb_keyboard_press(KEY_C, 0);
	_delay_ms(DELAY_MS);	
	usb_keyboard_press(KEY_E, 0);
	_delay_ms(DELAY_MS);	
	usb_keyboard_press(KEY_S, 0);
	_delay_ms(DELAY_MS);	
	usb_keyboard_press(KEY_PERIOD, 0);
	_delay_ms(DELAY_MS);	
	usb_keyboard_press(KEY_ENTER, 0);
}


void unlock_it()
{
	// MGS key-code test
	keyboard_keys[0] = 0;
	usb_keyboard_send();
	_delay_ms(DELAY_MS);
	keyboard_keys[1] = 0;
	usb_keyboard_send();
	_delay_ms(DELAY_MS);
	keyboard_keys[2] = 0;
	usb_keyboard_send();
	_delay_ms(DELAY_MS);
	
	keyboard_keys[0] = KEY_M;
	usb_keyboard_send();
	_delay_ms(DELAY_MS);
	keyboard_keys[1] = KEY_G;
	usb_keyboard_send();
	_delay_ms(DELAY_MS);
	keyboard_keys[2] = KEY_S;
	usb_keyboard_send();
	_delay_ms(DELAY_MS);
	keyboard_keys[0] = 0;
	usb_keyboard_send();
	_delay_ms(DELAY_MS);
	keyboard_keys[1] = 0;
	usb_keyboard_send();
	_delay_ms(DELAY_MS);
	keyboard_keys[2] = 0;	
	usb_keyboard_send();	
}
