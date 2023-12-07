/*******************************************************
This program was created by the
CodeWizardAVR V3.14 Advanced
Automatic Program Generator
� Copyright 1998-2014 Pavel Haiduc, HP InfoTech s.r.l.
http://www.hpinfotech.com

Project :
Version :
Date    : 12/6/2023
Author  :
Company :
Comments:


Chip type               : ATmega32
Program type            : Application
AVR Core Clock frequency: 8.000000 MHz
Memory model            : Small
External RAM size       : 0
Data Stack size         : 512
*******************************************************/

#include <lcd.h>
#include <mega32.h>
#include <string.h>
#include <stdio.h>
#include <delay.h>
#define KEYPAD_PIN PINB
#define LCD_PORT PORTC
#define LCD_TOTAL_LENGTH 32
#define KEYPAD_CLEAR_CHAR '*'

#asm
    .equ __lcd_port=0x15
#endasm

// Declare your global variables here

enum REMOTE_ACTION { NO_ACTION, TURN_OFF, TURN_ON, DISCARD_ALARM };

typedef enum REMOTE_ACTION EREMOTE_ACTION;

void handle_remote_action(EREMOTE_ACTION remote_action, int *IS_ON,
                          int *IS_ALARMING) {
  if (remote_action == TURN_ON) {
    *IS_ON = 1;
    return;
  }
  if (remote_action == TURN_OFF) {
    *IS_ON = 0;
    return;
  }
  if (remote_action == DISCARD_ALARM) {
    *IS_ALARMING = 0;
  }
}

const char KEYPAD_MATRIX[4][3] = {
    {'1', '2', '3'},
    {'4', '5', '6'},
    {'7', '8', '9'},
    {'*', '0', '#'}
};

char read_keypad() {
    char key = 0;

    // Set columns as output and rows as input
    DDRB = 0x0F;

    while (1) {
        int i,j;
        for (i=0; i < 3; i++) {
            // Set one column low at a time
            PORTB |= 0x0F;
            PORTB &= ~(1 << i);

            delay_ms(5);

            // Check rows
            for (j=0; j < 4; j++) {
                if (!(PINB & (1 << (j + 4)))) {
                    // Key pressed
                    key = KEYPAD_MATRIX[j][i];
                    while (!(PINB & (1 << (j + 4))));  // Wait for key release
                    return key;
                }
            }
        }
    }
}

void handle_keypad_action(unsigned char input_char, char *lcd_buffer) {
  int is_lcd_full;
  // Clear the last character
  if (input_char == KEYPAD_CLEAR_CHAR) {
    lcd_buffer[strlen(*lcd_buffer)] = '';
    (*lcd_buffer_letters_count)--;
    return;
  }

  // LCD is full, do nothing
  is_lcd_full = *lcd_buffer_letters_count == LCD_TOTAL_LENGTH;
  if (is_lcd_full) {
    return;
  }

  // Add the character to lcd and increase the count of letters;
  lcd_buffer[(*lcd_buffer_letters_count)++] = input_char;
  lcd_buffer[*lcd_buffer_letters_count] = NULL;  // Set the last character to null
}

void main(void) {
  // Declare your local variables here
  int IS_ON = 1;       // The security alarm should be listening or not
  int IS_ALARMING = 0; // If the alarm is making noise
  char lcd_buffer[LCD_TOTAL_LENGTH] = "Empty LCD";
  int lcd_buffer_letters_count = 0;
  DDRA = 0x00;         // Input
  PORTA = 0x00;
  DDRB = 0x0f;
  PORTB = 0xf0;
  DDRC = 0xff;
  PORTC = 0x00;
  DDRD = 0xff; // Output
  PORTD = 0x00;
  lcd_init(16);
  lcd_clear();
  _lcd_ready();
  lcd_gotoxy(0,0);
  delay_ms(10);
  lcd_puts(lcd_buffer);
  while (1) {
    // Handle remote action: Start
    int turn_off_flag = PINA .5;
    int turn_on_flag = PINA .6;
    int discard_alarm_flag = PINA .7;
    int warning_flag = PINA .0; // Coming from the sensor
    unsigned char input_char;
    EREMOTE_ACTION remote_action = NO_ACTION;
    if (turn_off_flag == 1) {
      remote_action = TURN_OFF;
    }

    if (turn_on_flag == 1) {
      remote_action = TURN_ON;
    }

    if (discard_alarm_flag == 1) {
      remote_action = DISCARD_ALARM;
    }

    if (warning_flag == 1) {
      IS_ALARMING = 1;
    }

    handle_remote_action(remote_action, &IS_ON, &IS_ALARMING);

    PORTD .0 = IS_ON;
    PORTD .1 = IS_ON && IS_ALARMING;
    // Handle Remote Action: End

    // Handle Keypad Action: Start
    input_char = read_keypad();
    if (input_char == NULL) continue;
    handle_keypad_action(input_char, lcd_buffer, &lcd_buffer_letters_count);
    lcd_clear();
    delay_ms(10);
    lcd_gotoxy(0,0);
    delay_ms(10);
    lcd_puts(lcd_buffer);
    // Handle Keypad Action: End
  }
}
