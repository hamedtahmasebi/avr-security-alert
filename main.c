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

#include <delay.h>
#include <lcd.h>
#include <mega32.h>
#include <stdio.h>
#include <string.h>
#define KEYPAD_PIN PINB
#define LCD_PORT PORTC
#define LCD_TOTAL_LENGTH 32
#define KEYPAD_CLEAR_CHAR '*'
#define ALARM_OUTPUT PORTD .1
#define ENABLE_KEYPAD_PIN PINA .1
#define IS_ON_PORT PORTD .0
#asm
.equ __lcd_port = 0x15
#endasm

    // Declare your global variables here

    typedef struct {
  int is_on;
  int is_locked;
} tStatusObj;

typedef enum {
  kp_turn_on,
  kp_turn_off,
  kp_lock_remote,
  kp_unlock_remote,
  kp_discard_alarm
} eKeypadAction;

typedef enum { rmt_turn_on, rmt_turn_off, rmt_discard_alarm } eRemoteAction;

void handle_remote_action(eRemoteAction remote_action, tStatusObj *status_obj) {
  if (remote_action == rmt_turn_on) {
    (*status_obj).is_on = 1;
    return;
  }
  if (remote_action == rmt_turn_off) {
    (*status_obj).is_on = 0;
    return;
  }
  if (remote_action == rmt_discard_alarm) {
    ALARM_OUTPUT = 0;
  }
}

const char KEYPAD_MATRIX[4][3] = {
    {'1', '2', '3'}, {'4', '5', '6'}, {'7', '8', '9'}, {'*', '0', '#'}};

char read_keypad(unsigned char *enable_switch_status) {
  char key = 0;

  // Set columns as output and rows as input
  DDRB = 0x0F;

  while (1) {

    int i, j;
    if (*enable_switch_status == 0) {
      break;
    }
    for (i = 0; i < 3; i++) {
      // Set one column low at a time
      PORTB |= 0x0F;
      PORTB &= ~(1 << i);

      delay_ms(5);

      // Check rows
      for (j = 0; j < 4; j++) {
        if (!(KEYPAD_PIN & (1 << (j + 4)))) {
          // Key pressed
          key = KEYPAD_MATRIX[j][i];
          while (!(KEYPAD_PIN & (1 << (j + 4))))
            ; // Wait for key release
          return key;
        }
      }
    }
  }
}

void handle_keypad_action(eKeypadAction action, tStatusObj *status_obj) {

  if (action == kp_turn_on) {
    (*status_obj).is_on = 1;
  }

  if (action == kp_turn_off) {
    (*status_obj).is_on = 0;
  }

  if (action == kp_lock_remote) {
    (*status_obj).is_locked = 1;
  }

  if (action == kp_unlock_remote) {
    (*status_obj).is_locked = 0;
  }
}

interrupt[2] void trigger_alarm() { PORTD .1 = 1; }

void lcd_render_guide() {
  lcd_clear();
  _lcd_ready();
  lcd_gotoxy(0, 0);
  delay_ms(10);
  lcd_puts("1 on | 2 off");
  delay_ms(15);
  lcd_gotoxy(0, 1);
  delay_ms(15);
  lcd_puts("3 lock | 4 unlock");
}

void main(void) {
  // Declare your local variables here
  tStatusObj status_obj = {.is_on = 1, .is_locked = 0};
  DDRA = 0x00; // Input
  PORTA = 0x00;
  DDRB = 0x0f;
  PORTB = 0xf0;
  DDRC = 0xff;
  PORTC = 0x00;
  DDRD = 0b11001111; // Output
  PORTD = 0b00110000;
  GICR = 0b11000000;  // Enabling interrupt 0
  MCUCR = 0b00000010; // Rising Edge

#asm("sei")
  lcd_init(16);
  lcd_render_guide();
  while (1) {
    // Handle remote action: Start
    if (status_obj.is_locked != 1) {
      int turn_off_flag = PINA .5;
      int turn_on_flag = PINA .6;
      int discard_alarm_flag = PINA .7;
      eRemoteAction remote_action;
      if (turn_off_flag == 1) {
        remote_action = rmt_turn_off;
      }

      if (turn_on_flag == 1) {
        remote_action = rmt_turn_on;
      }

      if (discard_alarm_flag == 1) {
        remote_action = rmt_discard_alarm;
      }

      handle_remote_action(remote_action, &status_obj);
    }
    // Handle Remote Action: End

    // Handle Keypad Action: Start
    if (ENABLE_KEYPAD_PIN == 1) {
      char pressed_key = read_keypad(&ENABLE_KEYPAD_PIN);
      eKeypadAction keypad_action;
      if (pressed_key == '1') {
        keypad_action = kp_turn_on;
      }
      if (pressed_key == '2') {
        keypad_action = kp_turn_off;
      }
      if (pressed_key == '3') {
        keypad_action = kp_lock_remote;
      }
      if (pressed_key == '4') {
        keypad_action = kp_unlock_remote;
      }
      handle_keypad_action(keypad_action, &status_obj);
    }
    // Handle Keypad Action: End
    PORTD .0 = status_obj.is_on;
  }
}
