
#define USING_MICROS_RESOLUTION true  //false

#include "STM32_PWM.h"
#define boolean bool

#include <SimpleTimer.h>  // https://github.com/jfturcot/SimpleTimer
#define HW_TIMER_INTERVAL_US 10L

#define PWM_PIN PB1

PinName pinNameToUse_PWM = digitalPinToPinName(PWM_PIN);
TIM_TypeDef *Instanc_PWM = (TIM_TypeDef *)pinmap_peripheral(pinNameToUse_PWM, PinMap_PWM);
uint8_t timerIndex = get_timer_index(Instanc_PWM);
uint32_t PWM_channel = STM_PIN_CHANNEL(pinmap_function(pinNameToUse_PWM, PinMap_PWM));
HardwareTimer *PWM1;

void setfrequency(uint32_t freq, int duty) {
  if (PWM1)
    delete PWM1;

  PWM1 = new HardwareTimer(Instanc_PWM);
  PWM1->setPWM(PWM_channel, PWM_PIN, freq, duty);
}

void setup() {

  pinMode(PWM_PIN, OUTPUT);
  digitalWrite(PWM_PIN, LOW);

  delay(100);
}

//////////////////////////////////////////////////////

void loop() {
  for (int i = 1; i < 100; i++) {
    setfrequency(100000 * i, 50);
    delay(100);
  }
}
