// ====== Pin assignment ======
#define PIN_LED   9
#define PIN_TRIG 12
#define PIN_ECHO 13

// ====== Configurable parameters ======
#define SND_VEL 346.0      // m/s @ 24°C
#define INTERVAL 25        // ms (sampling interval)
#define PULSE_DURATION 10  // us (trigger pulse width)
#define _DIST_MIN 100      // mm
#define _DIST_MAX 300      // mm

// TIMEOUT: half the INTERVAL, in usec (정수)
#define TIMEOUT ((INTERVAL / 2) * 1000UL)

// duration(us) -> distance(mm)
// 0.001(mm/us) * 0.5(왕복 보정) * 346(m/s)
#define SCALE (0.001 * 0.5 * SND_VEL)

// ===== Median Filter Window =====
#define MEDIAN_WINDOW 30

// ===== EMA: 그래프 비교용 출력만. 동작에는 미사용 =====
#define _EMA_ALPHA 0.8     // 0~1

// ===== Globals =====
unsigned long last_sampling_time = 0; // ms

// 중위수용 순환 버퍼
float win_buf[MEDIAN_WINDOW];
int   win_count = 0;
int   win_index = 0;

// 그래프 비교용 EMA
float dist_ema = 0.0f;

// ---------- 초음파 측정 ----------
float USS_measure(int TRIG, int ECHO)
{
  digitalWrite(TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(PULSE_DURATION);
  digitalWrite(TRIG, LOW);

  unsigned long duration = pulseIn(ECHO, HIGH, TIMEOUT); // usec
  return (float)duration * SCALE; // mm
}

// ---------- 버퍼에 샘플 추가 ----------
void median_push(float x)
{
  win_buf[win_index] = x;
  win_index = (win_index + 1) % MEDIAN_WINDOW;
  if (win_count < MEDIAN_WINDOW) win_count++;
}

// ---------- 중위수 계산 ----------
float median_get()
{
  if (win_count == 0) return 0.0f;

  float tmp[MEDIAN_WINDOW];
  for (int i = 0; i < win_count; i++) tmp[i] = win_buf[i];

  // 삽입정렬
  for (int i = 1; i < win_count; i++) {
    float key = tmp[i];
    int j = i - 1;
    while (j >= 0 && tmp[j] > key) {
      tmp[j + 1] = tmp[j];
      j--;
    }
    tmp[j + 1] = key;
  }

  if (win_count % 2 == 1) return tmp[win_count / 2];
  int r = win_count / 2;
  return 0.5f * (tmp[r - 1] + tmp[r]);
}

// ---------- LED 삼각형 매핑 (Active-Low용 PWM) ----------
// 100/300mm -> 최소 밝기(꺼짐쪽: PWM≈255)
// 200mm     -> 최대 밝기(가장 밝음: PWM=0)
// 150/250mm -> 절반 밝기
uint8_t dist_to_pwm_active_low(float d_mm)
{
  // [100,300] 범위로 클램프: 범위를 벗어나면 가장 어두운 쪽으로 눌림
  if (d_mm < _DIST_MIN) d_mm = _DIST_MIN;
  if (d_mm > _DIST_MAX) d_mm = _DIST_MAX;

  const float mid   = (_DIST_MIN + _DIST_MAX) * 0.5f;   // 200
  const float half  = (_DIST_MAX - _DIST_MIN) * 0.5f;   // 100

  // 삼각형 밝기: b in [0..1], 1에서 가장 밝음(200mm)
  float b;
  if (d_mm <= mid) {
    b = (d_mm - _DIST_MIN) / half;       // 100->0, 200->1
  } else {
    b = (_DIST_MAX - d_mm) / half;       // 200->1, 300->0
  }
  if (b < 0) b = 0; if (b > 1) b = 1;

  // Active-Low PWM 변환: 밝기 1 → PWM 0, 밝기 0 → PWM 255
  int pwm = (int)( (1.0f - b) * 255.0f + 0.5f );
  if (pwm < 0) pwm = 0; if (pwm > 255) pwm = 255;
  return (uint8_t)pwm;
}

void setup() {
  pinMode(PIN_LED,  OUTPUT);
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  digitalWrite(PIN_TRIG, LOW);

  Serial.begin(57600);

  last_sampling_time = millis();

  // 초기 LED OFF (active-low 기준 255)
  analogWrite(PIN_LED, 255);
}

void loop() {
  if (millis() - last_sampling_time < INTERVAL) return;

  // 1) 원시 측정값 (timeout 시 0이 들어올 수 있음)
  float dist_raw = USS_measure(PIN_TRIG, PIN_ECHO);

  // 2) 중위수 필터 (직전 유효값 보정 없음!)
  median_push(dist_raw);
  float dist_median = median_get();

  // 3) EMA: 그래프 비교용
  dist_ema = (_EMA_ALPHA * dist_raw) + ((1.0f - _EMA_ALPHA) * dist_ema);

  // 4) LED 밝기 적용: "중위수 결과"를 기준으로 삼각형 매핑
  uint8_t pwm = dist_to_pwm_active_low(dist_median);
  analogWrite(PIN_LED, pwm);

  // 5) 시리얼 플로터 출력 (요구 형식 유지)
  Serial.print("Min:");      Serial.print(_DIST_MIN);
  Serial.print(",raw:");     Serial.print(dist_raw);
  Serial.print(",ema:");     Serial.print(dist_ema);     // 비교용
  Serial.print(",median:");  Serial.print(dist_median);  // 중위수 필터 결과
  Serial.print(",Max:");     Serial.print(_DIST_MAX);
  Serial.println("");

  last_sampling_time = millis();
}
