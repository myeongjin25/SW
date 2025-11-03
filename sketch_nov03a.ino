#include <Servo.h>

// Arduino pin assignment
#define PIN_IR    0         // IR sensor at Pin A0
#define PIN_LED   9
#define PIN_SERVO 10

// ===== Servo pulse map (typical SG90: 1000/1500/2000us) =====
#define _DUTY_MIN 1000  // servo full clock-wise position (0 degree)
#define _DUTY_NEU 1500  // servo neutral position (90 degree)
#define _DUTY_MAX 2000  // servo full counter-clockwise position (180 degree)

// ===== Distance range (mm) =====
#define _DIST_MIN  100.0   // 100 mm (10 cm)
#define _DIST_MAX  250.0   // 250 mm (25 cm)

// ===== Filters =====
#define EMA_ALPHA  0.25    // 0~1 (크면 민감, 작으면 더 매끈)

// ===== Loop timing =====
#define LOOP_INTERVAL 20   // msec (>= 20ms 권장)

Servo myservo;
unsigned long last_loop_time;   // unit: msec

float dist_prev = _DIST_MIN;
float dist_ema  = _DIST_MIN;

void setup()
{
  pinMode(PIN_LED, OUTPUT);

  myservo.attach(PIN_SERVO);
  myservo.writeMicroseconds(_DUTY_NEU);

  Serial.begin(1000000);      // 1,000,000 bps (최대)
  last_loop_time = millis();  // 루프 타이머 초기화
}

void loop()
{
  unsigned long time_curr = millis();
  if (time_curr < (last_loop_time + LOOP_INTERVAL)) return;
  last_loop_time += LOOP_INTERVAL;

  int duty;
  float a_value, dist_raw, dist_filt;

  // A0(IR) -> 거리(mm) 변환 (과제 안내식 반영: -60.0 보정 포함)
  a_value = analogRead(PIN_IR);
  dist_raw = ((6762.0 / (a_value - 9.0)) - 4.0) * 10.0 - 60.0;

  // ----- Range Filter (10cm ~ 25cm) + LED -----
  if (dist_raw >= _DIST_MIN && dist_raw <= _DIST_MAX) {
    dist_filt = dist_raw;            // 범위 안: 값 채택
    digitalWrite(PIN_LED, HIGH);     // LED ON
  } else {
    dist_filt = dist_prev;           // 범위 밖: 이전값 유지(홀드)
    digitalWrite(PIN_LED, LOW);      // LED OFF
  }
  dist_prev = dist_filt;

  // ----- EMA Filter -----
  // ema(t) = α*ema(t-1) + (1-α)*x(t)  (α가 클수록 더 느리게 변함)
  dist_ema = EMA_ALPHA * dist_ema + (1.0 - EMA_ALPHA) * dist_filt;

  // ----- map() 미사용: 선형 변환 -----
  // duty = MIN + (x - xmin) * (MAX - MIN) / (xmax - xmin)
  float span_dist = (_DIST_MAX - _DIST_MIN);
  float span_duty = (_DUTY_MAX - _DUTY_MIN);
  float duty_f = _DUTY_MIN + (dist_ema - _DIST_MIN) * (span_duty / span_dist);

  // 클램핑
  if (duty_f < _DUTY_MIN) duty_f = _DUTY_MIN;
  if (duty_f > _DUTY_MAX) duty_f = _DUTY_MAX;

  duty = (int)(duty_f + 0.5f);
  myservo.writeMicroseconds(duty);

  // ----- Serial Plotter용 출력 포맷 유지 -----
  Serial.print("_DUTY_MIN:");  Serial.print(_DUTY_MIN);
  Serial.print("_DIST_MIN:");  Serial.print(_DIST_MIN);
  Serial.print(",IR:");        Serial.print(a_value);
  Serial.print(",dist_raw:");  Serial.print(dist_raw);
  Serial.print(",ema:");       Serial.print(dist_ema);
  Serial.print(",servo:");     Serial.print(duty);
  Serial.print(",_DIST_MAX:"); Serial.print(_DIST_MAX);
  Serial.print(",_DUTY_MAX:"); Serial.print(_DUTY_MAX);
  Serial.println("");
}
