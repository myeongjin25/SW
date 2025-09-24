// Arduino pin assignment
#define PIN_LED  9
#define PIN_TRIG 12   // sonar sensor TRIGGER
#define PIN_ECHO 13   // sonar sensor ECHO

// configurable parameters
#define SND_VEL 346.0     // sound velocity (m/s)
#define INTERVAL 25       // sampling interval (msec)
#define PULSE_DURATION 10 // ultra-sound pulse duration (usec)
#define _DIST_MIN 100.0   // mm
#define _DIST_MAX 300.0   // mm

#define TIMEOUT 30000     // max echo wait (usec) ~ 5m
#define SCALE (0.001 * 0.5 * SND_VEL) // convert duration to distance (mm)

unsigned long last_sampling_time = 0;

void setup() {
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  digitalWrite(PIN_TRIG, LOW);
  
  Serial.begin(57600);
}

void loop() { 
  if (millis() - last_sampling_time < INTERVAL) return;
  last_sampling_time = millis();

  float distance = USS_measure(PIN_TRIG, PIN_ECHO); // read distance
  int brightness = 255; // default: off

  if (distance >= _DIST_MIN && distance <= _DIST_MAX) {
    if (distance <= 200.0) {
      brightness = map((int)distance, 100, 200, 255, 0);
    } else {
      brightness = map((int)distance, 200, 300, 0, 255);
    }
  }

  analogWrite(PIN_LED, brightness);

  Serial.print("distance(mm): ");
  Serial.print(distance);
  Serial.print(" , brightness: ");
  Serial.println(brightness);
}

// get a distance reading from USS. return value is in mm.
float USS_measure(int TRIG, int ECHO) {
  digitalWrite(TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(PULSE_DURATION);
  digitalWrite(TRIG, LOW);

  long duration = pulseIn(ECHO, HIGH, TIMEOUT); // usec
  return duration * SCALE; // mm
}
