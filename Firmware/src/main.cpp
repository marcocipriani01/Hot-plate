#include "main.h"

struct Struct {
    uint8_t marker;
    double kP;
    double kI;
    double kD;
    double bangBangRange;
} settings;

double temperature = 0.0;
double lastTemperature = 0.0;
double target = 0.0;
double pwm = 0.0;
double derivative = 0.0;

unsigned long lastSend = 0L;
unsigned long lastDerivativeCalc = 0L;

thermistor ntc(NTC_PIN, 0);
AutoPID controller(&temperature, &target, &pwm, 0, 255, DEFAULT_PID_KP, DEFAULT_PID_KI, DEFAULT_PID_KD);

void setup() {
    pinMode(PWM_PIN, OUTPUT);
    digitalWrite(PWM_PIN, LOW);
    analogWrite(PWM_PIN, 0);

    Serial.begin(115200);

    EEPROM.get(EEPROM_START, settings);
    if (settings.marker != EEPROM_MARKER) {
        settings.marker = EEPROM_MARKER;
        settings.kP = DEFAULT_PID_KP;
        settings.kI = DEFAULT_PID_KI;
        settings.kD = DEFAULT_PID_KD;
        settings.bangBangRange = DEFAULT_BANG_BANG_RANGE;
        EEPROM.put(EEPROM_START, settings);
    }

    controller.setGains(settings.kP, settings.kI, settings.kD);
    controller.setBangBang(settings.bangBangRange);
    controller.setTimeStep(PID_SAMPLE_MS);
}

void loop() {
    temperature = readTemperature();
    if (target <= MIN_TEMP)
        pwm = 0.0;
    else
        controller.run();
    analogWrite(PWM_PIN, (int) pwm);

    unsigned long t = millis();
    if (lastDerivativeCalc == 0L) {
        lastTemperature = temperature;
        lastDerivativeCalc = t;
    } else if ((t - lastDerivativeCalc) >= 1000L) {
        derivative = 1000.0 * (temperature - lastTemperature) / ((double) (t - lastDerivativeCalc));
        lastTemperature = temperature;
        lastDerivativeCalc = t;
    }
    if ((t - lastSend) >= 100L) {
        Serial.print("A");
        Serial.print(temperature, 2);
        Serial.print(",");
        Serial.print(derivative, 2);
        Serial.print(",");
        Serial.println(pwm);
        lastSend = t;
    }
}

double readTemperature() {
    double sum = 0.0;
    for (int i = 0; i < 5; i++) {
        sum += ntc.analog2temp();
        delay(10);
    }
    return sum / 5.0;
}

void serialEvent() {
    while (Serial.available()) {
        if (Serial.read() != '$') continue;
        delay(1);
        switch (Serial.read()) {
            case 'S': {
                target = Serial.parseFloat();
                Serial.print("Target = ");
                Serial.println(target, 2);
                break;
            }

            case 'T': {
                settings.kP = Serial.parseFloat();
                settings.kI = Serial.parseFloat();
                settings.kD = Serial.parseFloat();
                settings.bangBangRange = Serial.parseFloat();
                controller.setGains(settings.kP, settings.kI, settings.kD);
                controller.setBangBang(settings.bangBangRange);
                EEPROM.put(EEPROM_START, settings);
                Serial.print("kP = ");
                Serial.print(settings.kP, 5);
                Serial.print(", kI = ");
                Serial.print(settings.kI, 5);
                Serial.print(", kD = ");
                Serial.println(settings.kD, 5);
                Serial.print("Bang-bang range = ");
                Serial.println(settings.bangBangRange, 2);
                break;
            }

            case 'E': {
                Serial.print("E");
                Serial.print(settings.kP, 5);
                Serial.print(",");
                Serial.print(settings.kI, 5);
                Serial.print(",");
                Serial.print(settings.kD, 5);
                Serial.print(",");
                Serial.print(settings.bangBangRange, 5);
                Serial.print(",");
                Serial.print(MIN_TEMP, 2);
                Serial.print(",");
                Serial.println(MAX_TEMP, 2);
            }
        }
    }
}
