#include <delay.h>
#include <lcd.h>
#include <mega32.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define KEYPAD_PIN PINB
#define LCD_PORT PORTC
#define ALARM_OUTPUT PORTD.5
#define ENABLE_KEYPAD_PIN PINA.3
#define IS_ON_PORT PORTD.6
#define REMOTE_PIN PINA
#asm
.equ __lcd_port = 0x15
#endasm



    // Declare your global variables here

bit alarm_triggered = 0;
bit is_on = 1;
bit is_locked = 0;
typedef enum
{
  kp_no_action,
  kp_turn_on,
  kp_turn_off,
  kp_lock_remote,
  kp_unlock_remote,
  kp_discard_alarm
} eKeypadAction;

typedef enum
{
  rmt_no_action,
  rmt_turn_on,
  rmt_turn_off,
  rmt_discard_alarm
} eRemoteAction;

void handle_remote_action(eRemoteAction remote_action)
{
  if (remote_action == rmt_no_action) return;
  if (remote_action == rmt_turn_on)
  {
    is_on = 1;
    return;
  }
  if (remote_action == rmt_turn_off)
  {
    is_on = 0;
    return;
  }
  if (remote_action == rmt_discard_alarm)
  {
    alarm_triggered = 0;
  }
}

const char KEYPAD_MATRIX[4][3] = {
    {'1', '2', '3'}, {'4', '5', '6'}, {'7', '8', '9'}, {'*', '0', '#'}};

char read_keypad()
{
  char key = 0;

  // Set columns as output and rows as input
  DDRB = 0x0F;
  
  while (1)
  {
    int i, j;
    if (ENABLE_KEYPAD_PIN == 0) {
        break;
    }
    

    for (i = 0; i < 3; i++)
    {
      // Set one column low at a time
      PORTB |= 0x0F;
      PORTB &= ~(1 << i);

      delay_ms(5);

      // Check rows
      for (j = 0; j < 4; j++)
      {
        if (!(KEYPAD_PIN & (1 << (j + 4))))
        {
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

void handle_keypad_action(eKeypadAction action)
{
  if (action == kp_no_action) return;
  if (action == kp_turn_on)
  {
    is_on = 1;
  }

  if (action == kp_turn_off)
  {
    is_on = 0;
  }

  if (action == kp_lock_remote)
  {
    is_locked = 1;
  }

  if (action == kp_unlock_remote)
  {
    is_locked = 0;
  }
  if (action == kp_discard_alarm)
  {
    alarm_triggered = 0;
  }
}

void lcd_render_guide()
{
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

interrupt[2] void trigger_alarm()
{
    if (is_on == 1) {
        alarm_triggered = 1;
    }
    
}



void main(void)
{
  // Declare your local variables here
  char pressed_key;
  eKeypadAction keypad_action;
  #asm("sei")
  DDRA = 0x00;              // Input
  DDRB = 0x0f;
  PORTB = 0xf0;
  DDRC = 0xff;
  PORTC = 0x00;
  DDRD = 0b01100000; // Output
  PORTD = 0b01000000;
  GICR = 0b01000000;  // Enabling interrupt 0
  MCUCR = 0b00000011; // Rising Edge
  GIFR = 0b1110000;
  

  lcd_init(16);
  lcd_render_guide();
  while (1)
  {
    // New remote
    // Handle remote action: Start
    eRemoteAction remote_action = rmt_no_action;
    if (REMOTE_PIN.0 == 1) {
        remote_action = rmt_turn_on;
    }
    if (REMOTE_PIN.1 == 1) {
        remote_action = rmt_turn_off;
    }
    if (REMOTE_PIN.2 == 1) {
        remote_action = rmt_discard_alarm;
    }
    if (is_locked != 1) {
        handle_remote_action(remote_action);
    }
    
    // Handle Remote Action: End

    // Handle Keypad Action: Start
      if (ENABLE_KEYPAD_PIN == 1) {
      pressed_key = read_keypad();
      keypad_action = kp_no_action;

      if (pressed_key == '1')
      {
        keypad_action = kp_turn_on;
      }
      if (pressed_key == '2')
      {
        keypad_action = kp_turn_off;
      }
      if (pressed_key == '3')
      {
        keypad_action = kp_lock_remote;
      }
      if (pressed_key == '4')
      {
        keypad_action = kp_unlock_remote;
      }
      if (pressed_key == '5')
      {
        keypad_action = kp_discard_alarm;
      }
      handle_keypad_action(keypad_action);
      }
    // Handle Keypad Action: End
    IS_ON_PORT = is_on == 1 ? 1 : 0;
    ALARM_OUTPUT = alarm_triggered == 1 ? 1 : 0;
    }
  }

