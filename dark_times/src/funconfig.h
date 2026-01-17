#ifndef _FUNCONFIG_H
#define _FUNCONFIG_H

// 1. Target Chip
#define CH32V003           1

// 2. System Clock Source (Internal 48MHz is standard for V003)
// Since I2C logic in the previous code assumed 24MHz for calculation,
// let's stick to 24MHz or adjust the I2C prescaler.
// Let's use 48MHz for system, but we must adjust I2C setup in main.c logic.
#define FUNCONF_USE_HSI 1
#define FUNCONF_SYSTEM_CORE_CLOCK 24000000

// 3. Debugging
// Enables printf() to work over the SWIO (PD1) pin automatically!
#define FUNCONF_USE_DEBUG_PRINTF 1

#endif