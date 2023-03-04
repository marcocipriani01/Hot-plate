#include <thermistor.h>

#define NTC_PIN A7
#define PWM_PIN 3

int pwm = 0;
thermistor ntc(NTC_PIN, 0);
unsigned long startTime = 0L;
unsigned long t = 0L;

void setup() {
  Serial.begin(115200);
  pinMode(PWM_PIN, OUTPUT);
  digitalWrite(PWM_PIN, LOW);
  analogWrite(PWM_PIN, 0);

  startTime = millis();
  unsigned long t = startTime;
  while ((t - startTime) <= 2000) {
    Serial.print((t - startTime) / 1000.0, 2);
    Serial.print(",");
    Serial.print(0);
    Serial.print(",");
    Serial.println(readTemperature());
    delay(100);
    t = millis();
  }

  analogWrite(PWM_PIN, pwm = 200);
  while (1) {
    Serial.print((t - startTime) / 1000.0, 2);
    Serial.print(",");
    Serial.print(pwm);
    Serial.print(",");
    Serial.println(readTemperature());
    delay(100);
    t = millis();
  }
}

void loop() {

}

double readTemperature() {
  double sum = 0.0;
  for (int i = 0; i < 5; i++) {
    sum += ntc.analog2temp();
    delay(10);
  }
  return sum / 5.0;
}
