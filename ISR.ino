/*#include <Arduino.h>*/
#define pir_pin 2
#define microwave_pin 3
#define fan_pin 7
#define callibration_time 10
#define operation_time 150


bool armed = false, fan_on = false;
unsigned int timer = 0, low_sec_num = 0;


void setup() {

  cli(); /*Close all interupptions*/
  TCCR1A = 0;
  TCCR1B = 0;
  /*To enable timer 1 we need to set WG1x(x=1,2,3,0) bits to zero, which are located in the TCCR1A and TCCR1B registers*/
  TCCR1B |= B00000100;
  /*To enable 256 prescaler we need to set CS12 bit (second bit of TCCR1B register) to 1 */
  TCNT1 = 0;
  /*Resets the timer1's value to 0*/
  TIMSK1 |= B00000010;
  /*To enable compare match A mode we need to set 0CIE1A bit to 1*/
  OCR1A = 62500;/*Number of pulses*/
  /*OCRxA and OCRxB registers store the value that will be compared to the TCNT1 registers value*/
  sei();/*Enable all interupptions*/
  
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(pir_pin, INPUT);
  pinMode(microwave_pin, INPUT);
  pinMode(fan_pin, OUTPUT);

  sensor_callibration();
  Serial.begin(1200);
}

void loop() {
  if (digitalRead(pir_pin) && digitalRead(microwave_pin)) {/*The sensors are active*/
    if (fan_on) {/*If the fan is on, and motion is detected the timer will reset to 5 minutes (continue to work for 5 minutes)*/
      timer = operation_time;
    }
    else {/*If the fan wasn't operating, then we arm the timer to tick for 2 minutes (if it wasn't armed previously)*/
      if (!armed) {
        armed = true;
        timer = 120;
      }
    }
  }
  digitalWrite(LED_BUILTIN, digitalRead(pir_pin) && digitalRead(microwave_pin));
  digitalWrite(fan_pin, fan_on);
}

void sensor_callibration() {
  for (int i = 0; i < callibration_time; i++)
  {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(500);
    digitalWrite(LED_BUILTIN, LOW);
    delay(500);
  }
}

void check_motion() {/*This functions checks if there was any 10 seconds when the both of the sensors were inactive. The low_sec_num variable counts the seconds when the signals of the sensors were LOW*/
  if (armed) {
    if (!(digitalRead(pir_pin) && digitalRead(microwave_pin))) {
      low_sec_num++;/*This variable counts the seconds when there's no motion.*/
    }
    else{
      low_sec_num = 0;/*The value of this variable will grow, because the sensors give high signal for 2-3 seconds then go off for 0.5-1 second low. If we detect motion during the timer's delay time, it wall start to count another 30 seconds(value will be reset).*/
      return;
    }
  }
  if (low_sec_num > 30) { /*If there was no motion for 30 seconds straight then the timer deactivates and goes off. */
    armed = false;
    timer = 0;
    low_sec_num = 0;
  }
}

void timer_stops() {/*This functions gets called when the timer's value is 0*/
  if (fan_on) {/*If the fan was working, then we turn it off*/
    fan_on = false;
  }
  else {
    if (armed) {/*If the timer was armed and the went off, we turn on the fan and set the timer for 3 minutes*/
      fan_on = true;
      timer = operation_time;
    }
    else {
      timer = 0;/*We do nothing, if the timer wasn't armed.*/
    }
  }
  low_sec_num = 0;
  armed = 0;/*At the end of the function we deactivate the timer. No matter what was the reason the timer went off.*/
}



ISR(TIMER1_COMPA_vect) {/*The program enters the interrupt service routine every 1 second*/
  TCNT1 = 0;/*Set timer_1's value to 0*/

  check_motion();

  if (timer > 0) {/*If the timer hasn't reached 0, then decrement it by one second.*/
    Serial.println(timer);
    timer--;
  }
  else {
    timer_stops();
  }

  
}
