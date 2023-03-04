#include "main.h"

struct Settings {
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
MedianFilter filter(MOVING_AVERAGE_WINDOW);

int currentProfile = -1;
int profileIndex = 0;
unsigned long profileStartTime = 0L;
unsigned long lastProfileSend = 0L;
unsigned long lastProfileProgress = 0L;

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

    profiles[0].name = LEADED_SMD291AX50T3_NAME;
    profiles[0].profile = LEADED_SMD291AX50T3;
    profiles[1].name = UNLEADED_SMD291SNL_NAME;
    profiles[1].profile = UNLEADED_SMD291SNL;
}

void loop() {
    temperature = filter.add(ntc.analog2temp());
    if (target <= MIN_TEMP)
        pwm = 0.0;
    else if (filter.isReady())
        controller.run();
    analogWrite(PWM_PIN, (int)pwm);

    unsigned long t = millis();
    if (currentProfile >= 0) {
        double elapsed = (t - profileStartTime) / 1000.0;
        if ((profileIndex < (PROFILES_LENGTH - 2)) && (elapsed >= profiles[currentProfile].profile[profileIndex + 1][0])) profileIndex++;
        if (profileIndex < (PROFILES_LENGTH - 2)) {
            target = map(elapsed, profiles[currentProfile].profile[profileIndex][0], profiles[currentProfile].profile[profileIndex + 1][0],
                         profiles[currentProfile].profile[profileIndex][1], profiles[currentProfile].profile[profileIndex + 1][1]);
            if ((lastProfileProgress - t) >= 250L) {
                Serial.print("Z");
                Serial.println((int)(100.0 * elapsed / profiles[currentProfile].profile[PROFILES_LENGTH - 1][0]));
                lastProfileProgress = t;
            }
        } else {
            target = profiles[currentProfile].profile[profileIndex][1];
            currentProfile = -1;
            profileIndex = 0;
            profileStartTime = 0L;
            Serial.println("Reflow complete!");
            Serial.println("Z100");
        }
        if ((t - lastProfileSend) >= 1000L) {
            Serial.print("Reflow info:\n\tProfile: ");
            Serial.print(profiles[currentProfile].name);
            Serial.print("\n\tElapsed time: ");
            Serial.print(elapsed, 2);
            Serial.print("s\n\tProfile index: ");
            Serial.print(profileIndex);
            Serial.print("\n\tProfile time: ");
            Serial.print(profiles[currentProfile].profile[profileIndex][0], 2);
            Serial.print("s\n\tProfile temperature: ");
            Serial.print(profiles[currentProfile].profile[profileIndex][1], 2);
            Serial.print("\n\tInterpolated target temperature: ");
            Serial.println(target, 2);
            lastProfileSend = t;
        }
    }

    if (lastDerivativeCalc == 0L) {
        lastTemperature = temperature;
        lastDerivativeCalc = t;
    } else if ((t - lastDerivativeCalc) >= 1000L) {
        derivative = 1000.0 * (temperature - lastTemperature) / ((double)(t - lastDerivativeCalc));
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

void serialEvent() {
    while (Serial.available()) {
        if (Serial.read() != '$') continue;
        delay(1);
        switch (Serial.read()) {
            case 'S': {
                target = Serial.parseFloat();
                Serial.print("Target = ");
                Serial.println(target, 2);
                currentProfile = -1;
                profileIndex = 0;
                profileStartTime = 0L;
                Serial.println("Z0");
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
                Serial.print(MAX_TEMP, 2);
                Serial.print(",");
                unsigned int profilesSize = sizeof(profiles) / sizeof(profiles[0]);
                for (unsigned int i = 0; i < profilesSize; i++) {
                    Serial.print(profiles[i].name);
                    if (i != (profilesSize - 1)) Serial.print(",");
                }
                Serial.println();
                break;
            }

            case 'R': {
                currentProfile = Serial.parseInt();
                profileStartTime = millis();
                Serial.print("Start reflow: ");
                Serial.println(profiles[currentProfile].name);
                break;
            }
        }
    }
}
