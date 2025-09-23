int pwm_pin = 7;
int period_us = 100;  
int duty_percent = 0;  

void set_period(int period) {
  period_us = period;
}

void set_duty(int duty) {
  if (duty < 0) duty = 0;
  if (duty > 100) duty = 100;
  duty_percent = duty;
}

void pwm_output() {
  int high_time = (period_us * duty_percent) / 100;
  int low_time = period_us - high_time;

  if (high_time > 0) {
    digitalWrite(pwm_pin, HIGH);
    delayMicroseconds(high_time);
  }

  if (low_time > 0) {
    digitalWrite(pwm_pin, LOW);
    delayMicroseconds(low_time);
  }
}

void setup() {
  pinMode(pwm_pin, OUTPUT);
}

void loop() {
  for (int duty = 0; duty <= 100; duty++) {
    set_duty(duty);
    unsigned long start = millis();
    while (millis() - start < 5) {
      pwm_output();
    }
  }
  for (int duty = 100; duty >= 0; duty--) {
    set_duty(duty);
    unsigned long start = millis();
    while (millis() - start < 5) {
      pwm_output();
    }
  }
}
