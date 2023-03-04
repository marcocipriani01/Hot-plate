#include "main.h"

thermistor ntc(NTC_PIN, 0);

double target = 20.0;
double kP = 0.0;
double kI = 0.0;
double kD = 0.0;
double lastTemp = 0.0;
double derivative = 0.0;

unsigned long lastSend = 0L;
unsigned long lastDerivativeCalc = 0L;

PID_v2 controller(kP, kI, kD, PID::Direct);

void setup() {
    pinMode(PWM_PIN, OUTPUT);
    digitalWrite(PWM_PIN, LOW);
    analogWrite(PWM_PIN, 0);
    Serial.begin(115200);
    controller.Start(readTemperature(), 0, target);
    controller.SetOutputLimits(0, 255);
}

void loop() {
    double temperature = readTemperature();
    int pwm = (temperature >= (target - 0.2)) ? 0 : controller.Run(temperature);
    analogWrite(PWM_PIN, pwm);

    unsigned long t = millis();
    if (lastDerivativeCalc == 0L) {
        lastTemp = temperature;
        lastDerivativeCalc = t;
    } else if ((t - lastDerivativeCalc) >= 1000L) {
        derivative = 1000.0 * (temperature - lastTemp) / ((double) (t - lastDerivativeCalc));
        lastTemp = temperature;
        lastDerivativeCalc = t;
    }
    if ((t - lastSend) >= 100L) {
        Serial.print("A");
        Serial.print(temperature, 1);
        Serial.print(",");
        Serial.print(derivative, 1);
        Serial.print(",");
        Serial.println(pwm);
        lastSend = t;
    }
    delay(10);
}

double readTemperature() {
    double sum = 0.0;
    for (int i = 0; i < 5; i++) {
        sum += ntc.analog2temp();
        delay(1);
    }
    return sum / 5.0;
}

void serialEvent() {
    while (Serial.available()) {
        if (Serial.read() != '$') continue;
        delay(1);
        switch (Serial.read()) {
            case 'S': {
                target = Serial.parseInt() / 100.0;
                controller.Setpoint(target);
                Serial.print("Target=");
                Serial.println(target, 2);
                break;
            }

            case 'T': {
                kP = Serial.parseInt() / 1000.0;
                kI = Serial.parseInt() / 1000.0;
                kD = Serial.parseInt() / 1000.0;
                controller.SetTunings(kP, kI, kD);
                Serial.print("kP=");
                Serial.print(kP, 2);
                Serial.print(", kI=");
                Serial.print(kI, 2);
                Serial.print(", kD=");
                Serial.println(kD, 2);
                break;
            }
        }
    }
}
