#include "rfal_platform.h"

#include <stdlib.h>
#include <string.h>

#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_intr_alloc.h"

static spi_device_handle_t nfc_spi;
static SemaphoreHandle_t spi_mutex = NULL;
static SemaphoreHandle_t irq_stat_mutex = NULL;
static TaskHandle_t rfal_irq_task_handle = NULL;
static platformIrqCallback_t st25r_irq_cb = NULL;


/* ============================================================
 *  Pin mapping
 * ============================================================ */

#define NFC_SPI_MOSI_PIN   CONFIG_NFC_SPI_MOSI_PIN
#define NFC_SPI_MISO_PIN   CONFIG_NFC_SPI_MISO_PIN
#define NFC_SPI_SCLK_PIN   CONFIG_NFC_SPI_SCLK_PIN
#define NFC_SPI_SS_PIN     CONFIG_NFC_SPI_SS_PIN
#define NFC_INTR_PIN       CONFIG_NFC_INTR_PIN
#define NFC_SPI_HOST       (spi_host_device_t)CONFIG_NFC_SPI_HOST

/* ============================================================
 *  SPI handle
 * ============================================================ */

/* ============================================================
 *  IRQ handling
 * ============================================================ */

// static portMUX_TYPE st25r_irq_mux = portMUX_INITIALIZER_UNLOCKED; // Removed spinlocks
// static portMUX_TYPE st25r_spi_mux = portMUX_INITIALIZER_UNLOCKED; // Removed spinlocks

static void IRAM_ATTR st25r_irq_isr(void *arg)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    if (rfal_irq_task_handle != NULL) {
        vTaskNotifyGiveFromISR(rfal_irq_task_handle, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

static void rfal_irq_task(void *arg)
{
    while (1) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        // ESP_LOGI("PLAT", "IRQ Task Notified"); // Uncomment for verbose debug
        if (st25r_irq_cb) {
            st25r_irq_cb();
        }
    }
}

/* ============================================================
 *  GPIO init
 * ============================================================ */

static void platformGpioInit(void)
{
    gpio_config_t cfg = {0};

    /* CS */
    cfg.pin_bit_mask = 1ULL << NFC_SPI_SS_PIN;
    cfg.mode = GPIO_MODE_OUTPUT;
    gpio_config(&cfg);
    gpio_set_level(NFC_SPI_SS_PIN, 1);

    /* IRQ */
    cfg.pin_bit_mask = 1ULL << NFC_INTR_PIN;
    cfg.mode = GPIO_MODE_INPUT;
    cfg.pull_up_en = GPIO_PULLUP_ENABLE;
    cfg.intr_type = GPIO_INTR_NEGEDGE;
    gpio_config(&cfg);
}

/* ============================================================
 *  SPI init
 * ============================================================ */

void platformSpiInit(void)
{
    // Initialize SPI mutex early
    if (spi_mutex == NULL) {
        spi_mutex = xSemaphoreCreateMutex();
    }

    spi_bus_config_t buscfg = {
        .mosi_io_num = NFC_SPI_MOSI_PIN,
        .miso_io_num = NFC_SPI_MISO_PIN,
        .sclk_io_num = NFC_SPI_SCLK_PIN,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 256,
    };

    spi_bus_initialize(NFC_SPI_HOST, &buscfg, SPI_DMA_CH_AUTO);

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 5 * 1000 * 1000, // Reduced to 5MHz for reliability
        .mode = 0,
        .spics_io_num = -1,   /* manual CS */
        .queue_size = 1,
    };

    spi_bus_add_device(NFC_SPI_HOST, &devcfg, &nfc_spi);

    platformGpioInit();
}

/* ============================================================
 *  SPI TX/RX (BUFFER MODE)
 * ============================================================ */

void platformSpiSelect(void)
{
    gpio_set_level(NFC_SPI_SS_PIN, 0);
}

void platformSpiDeselect(void)
{
    gpio_set_level(NFC_SPI_SS_PIN, 1);
}

void platformSpiTxRx(const uint8_t *txBuf, uint8_t *rxBuf, uint16_t len)
{
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));

    t.length = len * 8;
    t.tx_buffer = txBuf;
    t.rx_buffer = rxBuf;

    spi_device_transmit(nfc_spi, &t);
}

/* ============================================================
 *  Delay
 * ============================================================ */

/* ============================================================
 *  Delay
 * ============================================================ */

void platformDelay(uint32_t ms)
{
    TickType_t ticks = pdMS_TO_TICKS(ms);
    if (ticks == 0 && ms > 0) {
        ticks = 1;
    }
    vTaskDelay(ticks);
}

/* ============================================================
 *  IRQ BSP
 * ============================================================ */

void platformIrqST25RPinInitialize(void)
{
    esp_err_t err = gpio_install_isr_service(0);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        // Only log error if it's not already installed
        // ESP_LOGE("PLAT", "Failed to install ISR service: %d", err);
    }
    
    // Create IRQ status mutex
    if (irq_stat_mutex == NULL) {
        irq_stat_mutex = xSemaphoreCreateMutex();
    }

    // Configure GPIO
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_POSEDGE,
        .pin_bit_mask = (1ULL << ST25R_INT_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_down_en = 0,
        .pull_up_en = 0,
    };
    gpio_config(&io_conf);

    gpio_isr_handler_add(NFC_INTR_PIN, st25r_irq_isr, NULL);

    // Create IRQ handling task
    if (rfal_irq_task_handle == NULL) {
        xTaskCreate(rfal_irq_task, "rfal_irq_task", 4096, NULL, configMAX_PRIORITIES - 1, &rfal_irq_task_handle);
    }
}

void platformIrqST25RSetCallback(platformIrqCallback_t cb)
{
    st25r_irq_cb = cb;
}

void platformProtectST25RIrqStatus(void)
{
    if (irq_stat_mutex == NULL) {
        return; 
    }
    xSemaphoreTake(irq_stat_mutex, portMAX_DELAY);
}

void platformUnprotectST25RIrqStatus(void)
{
    if (irq_stat_mutex) {
        xSemaphoreGive(irq_stat_mutex);
    }
}

void platformProtectST25RComm(void)
{
    if (spi_mutex) {
        xSemaphoreTake(spi_mutex, portMAX_DELAY);
    }
}

void platformUnprotectST25RComm(void)
{
    if (spi_mutex) {
        xSemaphoreGive(spi_mutex);
    }
}

/* ============================================================
 *  Timer abstraction
 * ============================================================ */

platformTimer_t platformTimerCreate(uint32_t timeout_ms)
{
    TickType_t ticks = pdMS_TO_TICKS(timeout_ms);
    // Ensure small timeouts (e.g. 5ms) don't become 0 ticks
    if (ticks == 0 && timeout_ms > 0) {
        ticks = 1; 
    }
    // Add 1 extra tick to guarantee we wait AT LEAST timeout_ms
    // because current tick might be just about to expire.
    ticks += 1;
    
    return (xTaskGetTickCount() + ticks);
}

bool platformTimerIsExpired(platformTimer_t timer)
{
    // Safe comparison for wrap-around
    return ((int32_t)(xTaskGetTickCount() - timer) >= 0);
}

void platformTimerDestroy(platformTimer_t timer)
{
    /* No-op as we use simple integers */
    (void)timer;
}

/* ============================================================
 *  LEDs (unused)
 * ============================================================ */

void platformLedsInitialize(void)
{
}
