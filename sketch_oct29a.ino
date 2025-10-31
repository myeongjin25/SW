#include <Servo.h>

// ===== User params =====
#define PIN_SERVO 10
#define _DUTY_MIN 500    // 0°
#define _DUTY_NEU 1500   // 90°
#define _DUTY_MAX 2500   // 180°

#define _SERVO_SPEED 0.3f  // °/s  
#define _START_DEG   0.0f  // 시작 각도

#define INTERVAL 20        // ms (제어 틱 간격)
#define PRINT_PLOT 0       // 1=시리얼 출력, 0=끄기

// ===== Internals =====
unsigned long last_tick = 0;
Servo myservo;

float duty_curr = 0.0f, duty_target = 0.0f;
float duty_step_per_tick = 0.0f;
bool  reached_target = false;

static inline float clampf(float x, float lo, float hi) {
  if (x < lo) return lo;
  if (x > hi) return hi;
  return x;
}
static inline float deg_to_us(float deg) {
  deg = clampf(deg, 0.0f, 180.0f);
  return _DUTY_MIN + (deg / 180.0f) * (_DUTY_MAX - _DUTY_MIN);
}

// speed만으로 각도/시간 자동 결정:
// move_deg = min(180, speed * 300)
// move_time = move_deg / speed (= min(300, 180/speed))
void plan_move_from_speed() {
  float speed    = _SERVO_SPEED;          // °/s
  float move_deg = speed * 300.0f;
  if (move_deg > 180.0f) move_deg = 180.0f;

  float start_deg = _START_DEG;
  float end_deg   = clampf(start_deg + move_deg, 0.0f, 180.0f);

  duty_curr   = deg_to_us(start_deg);
  duty_target = deg_to_us(end_deg);

  float move_time_s = move_deg / speed;
  float ticks = (move_time_s * 1000.0f) / INTERVAL;
  duty_step_per_tick = (duty_target - duty_curr) / ticks;
}

void setup() {
  myservo.attach(PIN_SERVO, _DUTY_MIN, _DUTY_MAX);
  plan_move_from_speed();
  myservo.writeMicroseconds((int)(duty_curr + 0.5f));

#if PRINT_PLOT
  Serial.begin(57600);
  Serial.print("step(us/tick): ");
  Serial.println(duty_step_per_tick);
#endif

  last_tick = 0;
}

void loop() {
  if (millis() < last_tick + INTERVAL) return;

  if (!reached_target) {
    duty_curr += duty_step_per_tick;

    // 도착/넘침 처리
    if ((duty_step_per_tick > 0 && duty_curr >= duty_target) ||
        (duty_step_per_tick < 0 && duty_curr <= duty_target)) {
      duty_curr = duty_target;
      reached_target = true;
    }

    duty_curr = clampf(duty_curr, (float)_DUTY_MIN, (float)_DUTY_MAX);
    myservo.writeMicroseconds((int)(duty_curr + 0.5f));
  }

#if PRINT_PLOT
  Serial.print("duty_curr:");   Serial.print((int)duty_curr);
  Serial.print(",duty_target:");Serial.print((int)duty_target);
  Serial.print(",speed_dps:");  Serial.println(_SERVO_SPEED);
#endif

  last_tick += INTERVAL;
}
