#include <Servo.h>

// ===== Pins =====
#define PIN_LED   9    // LED (active-low)
#define PIN_TRIG  12
#define PIN_ECHO  13
#define PIN_SERVO 10

// ===== Sonar / timing =====
#define SND_VEL 346.0
#define INTERVAL 25            // ms
#define PULSE_DURATION 10      // us
#define _DIST_MIN 180.0        // mm (18 cm)
#define _DIST_MAX 360.0        // mm (36 cm)
#define TIMEOUT ((unsigned long)((INTERVAL / 2.0) * 1000.0))
#define SCALE   (0.001 * 0.5 * SND_VEL) // us -> mm

// ===== EMA =====
#define _EMA_ALPHA 0.3         // 0~1

// ===== Angle mapping & safety =====
#define ANGLE_MIN 0
#define ANGLE_MAX 180
#define ANGLE_SFT_MIN 5        // soft limit (기구물 보호)
#define ANGLE_SFT_MAX 175

// ===== Motion smoothing =====
#define MAX_DEG_PER_SEC 120.0  // 각속도 한계(°/s) — 필요시 60~180 사이로 조정
#define ANGLE_DEADBAND 2.0     // ±2° 이내면 유지

// ===== Globals =====
float dist_prev = _DIST_MIN;     // range-filter된 직전 유효값
float dist_ema  = _DIST_MIN;     // EMA 출력
unsigned long last_sampling_time = 0;

Servo myservo;
float current_angle = 90.0f;

static inline float clampf(float x, float lo, float hi) {
  if (x < lo) return lo;
  if (x > hi) return hi;
  return x;
}

// 18 cm(180 mm) -> 0°, 36 cm(360 mm) -> 180° 선형 매핑
int mm_to_angle(float mm) {
  float ang = (mm - _DIST_MIN) * (ANGLE_MAX - ANGLE_MIN) / (_DIST_MAX - _DIST_MIN) + ANGLE_MIN; // = mm - 180
  ang = clampf(ang, ANGLE_MIN, ANGLE_MAX);
  // 소프트 리미트 적용
  ang = clampf(ang, ANGLE_SFT_MIN, ANGLE_SFT_MAX);
  return (int)(ang + 0.5f);
}

// 한 스텝 각속도 제한
float step_limit_angle(float current, float target, float max_deg_per_sec, float dt_ms) {
  float max_step = max_deg_per_sec * (dt_ms / 1000.0f);
  float diff = target - current;
  if (diff >  max_step) return current + max_step;
  if (diff < -max_step) return current - max_step;
  return target;
}

void setup() {
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  digitalWrite(PIN_TRIG, LOW);

  myservo.attach(PIN_SERVO);
  current_angle = 90.0f;
  myservo.write((int)current_angle);

  Serial.begin(57600);
  last_sampling_time = millis();
}

void loop() {
  if (millis() - last_sampling_time < INTERVAL) return;

  // 1) 초음파 측정(raw)
  float dist_raw = USS_measure(PIN_TRIG, PIN_ECHO); // mm

  // 2) 범위 필터: 18~36 cm만 유효. 밖이면 직전 유효값 유지.
  bool in_range = (dist_raw >= _DIST_MIN) && (dist_raw <= _DIST_MAX) && (dist_raw != 0.0);
  float dist_filtered = in_range ? dist_raw : dist_prev;
  if (in_range) dist_prev = dist_raw;

  // 3) EMA: y_k = α*x_k + (1-α)*y_{k-1}
  dist_ema = _EMA_ALPHA * dist_filtered + (1.0f - _EMA_ALPHA) * dist_ema;

  // 4) LED: 측정값이 '범위 안'에 있으면 점등(ON, active-low)
  digitalWrite(PIN_LED, in_range ? LOW : HIGH);

  // 5) 목표 각도 결정 (요구사항: 18↓->0°, 36↑->180°, 사이는 선형)
  int target_angle_i;
  if (dist_ema <= _DIST_MIN)       target_angle_i = ANGLE_MIN;
  else if (dist_ema >= _DIST_MAX)  target_angle_i = ANGLE_MAX;
  else                              target_angle_i = mm_to_angle(dist_ema);

  float target_angle = (float)target_angle_i;

  // 6) 데드밴드: 너무 가까우면 움직이지 않음(떨림 억제)
  if (fabs(target_angle - current_angle) <= ANGLE_DEADBAND) {
    target_angle = current_angle;
  }

  // 7) 각속도 제한 적용
  current_angle = step_limit_angle(current_angle, target_angle, MAX_DEG_PER_SEC, (float)INTERVAL);
  current_angle = clampf(current_angle, ANGLE_SFT_MIN, ANGLE_SFT_MAX);
  myservo.write((int)(current_angle + 0.5f));

  // 8) 플로터용 시리얼 출력
  Serial.print("Min:");   Serial.print(_DIST_MIN);
  Serial.print(",dist:"); Serial.print(dist_raw);
  Serial.print(",ema:");  Serial.print(dist_ema);
  Serial.print(",Servo:");Serial.print(myservo.read());
  Serial.print(",Max:");  Serial.print(_DIST_MAX);
  Serial.println("");

  last_sampling_time += INTERVAL;
}

float USS_measure(int TRIG, int ECHO)
{
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(PULSE_DURATION);
  digitalWrite(TRIG, LOW);
  return pulseIn(ECHO, HIGH, TIMEOUT) * SCALE; // mm
}
