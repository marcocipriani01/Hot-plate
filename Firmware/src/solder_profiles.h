#ifndef SODLER_PROFILES_H
#define SODLER_PROFILES_H

#include <Arduino.h>

#define PROFILES_SIZE 6

typedef struct {
    String name;
    const double (*profile)[PROFILES_SIZE][2];
} SolderProfile;

SolderProfile solderProfiles[2];

// Leaded solder paste
// https://www.mouser.it/ProductDetail/Chip-Quik/SMD291AX50T3?qs=Wj%2FVkw3K%252BMDjfczTJjwRyQ%3D%3D
#define LEADED_SMD291AX50T3_NAME "Leaded SMD291AX50T3"
const double LEADED_SMD291AX50T3[PROFILES_SIZE][2] = {
    {0, 25},
    {30, 100},
    {120, 150},
    {150, 183},
    {185, 230},
    {210, 235}
};

// Unleaded solder paste
// https://www.mouser.it/ProductDetail/Chip-Quik/SMD291SNL?qs=sGAEpiMZZMtyU1cDF2RqUEx4HVIhVm2RrWlsW7zykWs%3D
#define UNLEADED_SMD291SNL_NAME "Unleaded SMD291SNL"
const double UNLEADED_SMD291SNL[PROFILES_SIZE][2] = {
    {0, 25},
    {90, 150},
    {180, 175},
    {210, 217},
    {225, 240},
    {240, 250}
};

#endif
