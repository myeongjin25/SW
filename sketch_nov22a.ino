#include <stdlib.h>

#define PIN_IR A0

int cmp_uint(const void *a, const void *b) {
  unsigned int aa = *(const unsigned int *)a;
  unsigned int bb = *(const unsigned int *)b;

  if (aa < bb) return -1;
  if (aa > bb) return 1;
  return 0;
}

unsigned int ir_sensor_filtered(unsigned int n,
                                float position,
                                int verbose)
{
  if (n == 0) return 0;
  if (n > 100) n = 100;

  if (position < 0.0f) position = 0.0f;
  if (position > 1.0f) position = 1.0f;

  unsigned int buf[100];
  unsigned int i;
  unsigned int index;
  unsigned int result;
  unsigned long t_start = 0;
  unsigned long t_end   = 0;

  if (verbose == 2) {
    t_start = micros();
  }

  for (i = 0; i < n; i++) {
    buf[i] = analogRead(PIN_IR);

    if (verbose == 1) {
      Serial.print("raw[");
      Serial.print(i);
      Serial.print("] = ");
      Serial.println(buf[i]);
    }
  }

  qsort(buf, n, sizeof(unsigned int), cmp_uint);

  if (verbose == 1) {
    Serial.print("sorted: ");
    for (i = 0; i < n; i++) {
      Serial.print(buf[i]);
      if (i < n - 1) Serial.print(", ");
    }
    Serial.println();
  }

  index  = (unsigned int)(position * (n - 1) + 0.5f);
  result = buf[index];

  if (verbose == 2) {
    t_end = micros();
    Serial.print("ir_sensor_filtered elapsed(us) = ");
    Serial.println(t_end - t_start);
  }

  return result;
}

float volt_to_distance(unsigned int a_value) {
  return (6762.0 / (a_value - 9) - 4.0) * 10.0;
}

void setup() {
  Serial.begin(1000000); 
}

void loop() {
  if (Serial.available() > 0) {
    char cmd = Serial.read();

    unsigned int filtered;
    float dist;
    int verbose = 0;

    if (cmd == '0') {
      verbose = 0;
    } else if (cmd == '1') {
      verbose = 1;
    } else if (cmd == '2') {
      verbose = 2;
    } else {
      return;
    }

    filtered = ir_sensor_filtered(7, 0.5f, verbose);
    dist     = volt_to_distance(filtered);

    Serial.print("FLT:");
    Serial.print(filtered);
    Serial.print("  DIST[mm]: ");
    Serial.println(dist);
  }
}
