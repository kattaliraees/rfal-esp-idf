#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "rfal_custom_config.h"

/* ============================================================
 *  GPIO abstraction
 * ============================================================ */

typedef void* platformGpioPort_t;

#define platformGpioSet(port, pin)        gpio_set_level((gpio_num_t)(pin), 1)
#define platformGpioClear(port, pin)      gpio_set_level((gpio_num_t)(pin), 0)

#define platformGpioToggle(port, pin)     \
    gpio_set_level((gpio_num_t)(pin), !gpio_get_level((gpio_num_t)(pin)))

#define platformGpioIsHigh(port, pin)     (gpio_get_level((gpio_num_t)(pin)) == 1)
#define platformGpioIsLow(port, pin)      (gpio_get_level((gpio_num_t)(pin)) == 0)

/* ============================================================
 *  ST25R IRQ pin
 * ============================================================ */

#define ST25R_INT_PORT   NULL
#define ST25R_INT_PIN    GPIO_NUM_38

/* ============================================================
 *  Delay
 * ============================================================ */

void platformDelay(uint32_t ms);

/* ============================================================
 *  SPI (BUFFER MODE – RFAL REQUIRED)
 * ============================================================ */

void platformSpiInit(void);
void platformSpiSelect(void);
void platformSpiDeselect(void);
void platformSpiTxRx(const uint8_t *txBuf, uint8_t *rxBuf, uint16_t len);

/* ============================================================
 *  IRQ handling
 * ============================================================ */

typedef void (*platformIrqCallback_t)(void);

void platformIrqST25RPinInitialize(void);
void platformIrqST25RSetCallback(platformIrqCallback_t cb);

void platformProtectST25RIrqStatus(void);
void platformUnprotectST25RIrqStatus(void);

void platformProtectST25RComm(void);
void platformUnprotectST25RComm(void);

/* ============================================================
 *  Timer abstraction
 * ============================================================ */

typedef uint32_t platformTimer_t;

platformTimer_t platformTimerCreate(uint32_t timeout_ms);
bool platformTimerIsExpired(platformTimer_t timer);
void platformTimerDestroy(platformTimer_t timer);

/* ============================================================
 *  LEDs (optional)
 * ============================================================ */

void platformLedsInitialize(void);

/* ============================================================
 *  ESP-IDF includes (last)
 * ============================================================ */

#include "driver/gpio.h"
