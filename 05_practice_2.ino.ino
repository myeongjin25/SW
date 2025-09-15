#define PIN_LED 7

void setup() {
  pinMode(PIN_LED, OUTPUT); 
}

void loop() {
  int count = 0;
  while (1) {
      if (count == 0) {
        digitalWrite(PIN_LED, HIGH);
        delay(1000);
        count++;
        }
      else if (count <= 5) {
        digitalWrite(PIN_LED, LOW);
        delay(100);
        digitalWrite(PIN_LED, HIGH);
        delay(100);
        count ++;
        }
       else {
        digitalWrite(PIN_LED, LOW);
        }
      
    }
  

}
