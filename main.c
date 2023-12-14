#include <delay.h>
#include <eeprom.h>
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
#define REMOTE_PIN PINA .0
#define REMOTE_LEARN_PIN PINA .2
#define LEARN_ENABLED_LED PORTD .7
#define REMOTE_CODE_EEPROM_ADDRESS 0x00
#asm
.equ __lcd_port = 0x15
#endasm

    // Declare your global variables here

    typedef struct
{
  int is_on;     // alarm status
  int is_locked; // remote status
} tStatusObj;

typedef enum
{
  kp_turn_on,
  kp_turn_off,
  kp_lock_remote,
  kp_unlock_remote,
  kp_discard_alarm
} eKeypadAction;

typedef enum
{
  rmt_turn_on,
  rmt_turn_off,
  rmt_discard_alarm
} eRemoteAction;

void handle_remote_action(eRemoteAction remote_action, tStatusObj *status_obj)
{
  if (remote_action == rmt_turn_on)
  {
    (*status_obj).is_on = 1;
    return;
  }
  if (remote_action == rmt_turn_off)
  {
    (*status_obj).is_on = 0;
    return;
  }
  if (remote_action == rmt_discard_alarm)
  {
    ALARM_OUTPUT = 0;
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
    if (ENABLE_KEYPAD_PIN == 0)
    {
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

void handle_keypad_action(eKeypadAction action, tStatusObj *status_obj)
{

  if (action == kp_turn_on)
  {
    (*status_obj).is_on = 1;
  }

  if (action == kp_turn_off)
  {
    (*status_obj).is_on = 0;
  }

  if (action == kp_lock_remote)
  {
    (*status_obj).is_locked = 1;
  }

  if (action == kp_unlock_remote)
  {
    (*status_obj).is_locked = 0;
  }
  if (action == kp_discard_alarm) {
     ALARM_OUTPUT=0;
  }
}

interrupt[2] void trigger_alarm() { PORTD .1 = 1; }

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

// TODO: The name bin_to_bcd is confusing, switch to hex
void bin_to_bcd(char *arr, int length, char *dest)
{
  // This is not ACTUAL BCD, we're just separating them 4 by 4 so we can work
  // more easily
  int idx;
  int bcd_eq;
  for (idx = 0; idx < length; idx += 4)
  {
    bcd_eq = ((arr[idx] * 8) + (arr[idx + 1] * 4) + (arr[idx + 2] * 2) +
              arr[idx + 3]);
    dest[idx / 4] = bcd_eq;
  }
}

char authorize_remote(char *known_remote, char *remote_code)
{
  int i;
  for (i = 0; i < 5; ++i)
  {
    if (known_remote[i] != remote_code[i])
      return -1;
  }
  return 0;
}

int read_remote(char *dest)
{
  // We're gonna use code learn remotes
  // The msg is gonna be modulated in pulse widths
  // 300us < pulse width  < 750us means zero
  // 900us < pulse width < 2500us means one
  unsigned int pulse_widths[24];
  char normalized_pulses[24];
  unsigned int i = 0;
  char bcd_code[6];
  bit error;
  if (REMOTE_PIN == 1)
  {
    while (REMOTE_PIN == 1) // wait until signal is zero
      ;
    // Count the time signal is zero
    TCNT1 = 0;  // Reset timer value
    TCCR1B = 2; // TIMER1: ON, Prescale is set to 8
    while (REMOTE_PIN == 0)
      ;         // let the timer count
    TCCR1B = 0; // Timer 1 off
    // Detect preamble signal
    if (!(TCNT1 > 10000 && TCNT1 < 22000))
      return; // if preamble signal wasn't recognized, just ignore the rest and return
    // Read all the 24 bits from pulse widths
    while (i < 24)
    {
      if (REMOTE_PIN == 1)
      {
        TCNT1 = 0;
        TCCR1B = 2;
        while (REMOTE_PIN == 1)
          ;
        TCCR1B = 0;
        pulse_widths[i] = TCNT1;
        i++;
      }
    }
    // Normalize pulse widths to zeros and ones so we can use them
    for (i = 0; i < 24; i++)
    {
      if (pulse_widths[i] >= 300 && pulse_widths[i] <= 750)
      {
        normalized_pulses[i] = 0;
      }
      else if (pulse_widths[i] >= 900 && pulse_widths[i] <= 2500)
      {
        normalized_pulses[i] = 1;
      }
      else
      {
        error = 1;
      }
    }

    if (error == 1)
      return -1;

    bin_to_bcd(normalized_pulses, 24, dest);

  }
}

void main(void)
{
  // Declare your local variables here
  tStatusObj status_obj = {.is_on = 1, .is_locked = 0};
  int save_remote_flag = 0; // for code learn remote to be remembered
  DDRA = 0x00;              // Input
  PORTA = 0b00000110;
  DDRB = 0x0f;
  PORTB = 0xf0;
  DDRC = 0xff;
  PORTC = 0x00;
  DDRD = 0b10000011; // Output
  PORTD = 0b00110000;
  GICR = 0b11000000;  // Enabling interrupt 0
  MCUCR = 0b00000010; // Rising Edge
  TCCR1B = 0;         // Timer 1 off
  
  PORTD = 0b00110000;
  
#asm("sei")
  lcd_init(16);
  lcd_render_guide();
  while (1)
  {
    // New remote
    // Handle remote action: Start
    if (status_obj.is_locked != 1)
    { 
      char received_code[6];
      char remote_code[5];
      eRemoteAction remote_action; 
      if (REMOTE_LEARN_PIN == 1)
      {
          LEARN_ENABLED_LED = 1;
          //read_remote(received_code);
          strcpy(remote_code, received_code);
          lcd_clear();
          lcd_gotoxy(0,0);
          delay_ms(15);
          lcd_putchar(received_code[0]);
          //eeprom_write_block(remote_code, REMOTE_CODE_EEPROM_ADDRESS, sizeof(remote_code));
      } else {
        LEARN_ENABLED_LED = 0;
      }
      
      if (received_code[5] == '3')
      {
        remote_action = rmt_turn_off;
      }

      if (received_code[5] == '2')
      {
        remote_action = rmt_turn_on;
      }

      if (received_code[5] == '1')
      {
        remote_action = rmt_discard_alarm;
      }

      //handle_remote_action(remote_action, &status_obj);
    }
    // Handle Remote Action: End

    // Handle Keypad Action: Start
    if (ENABLE_KEYPAD_PIN == 1)
    {
      char pressed_key = read_keypad();
      eKeypadAction keypad_action;

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
      if (pressed_key == '5') {
        keypad_action = kp_discard_alarm;
      }
      handle_keypad_action(keypad_action, &status_obj);
    }
    // Handle Keypad Action: End
    PORTD .0 = status_obj.is_on;
  }
}
