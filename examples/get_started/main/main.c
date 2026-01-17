#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "rfal_rf.h"
#include "rfal_nfca.h"
#include "rfal_nfcv.h"
#include "rfal_t2t.h"
#include "st25r3911.h"
#include "st25r3911_com.h"

static const char *TAG = "NFC-PLG";

/* NFC polling configuration */
#define NFC_POLL_INTERVAL_MS    500
#define MAX_DEVICES             5

/* Helper function to print UID in hex format */
static void print_uid(const uint8_t *uid, uint8_t len)
{
    printf("UID: ");
    for (uint8_t i = 0; i < len; i++) {
        printf("%02X", uid[i]);
        if (i < len - 1) printf(":");
    }
    printf("\n");
}

/* Initialize RFAL and ST25R3911 */
static esp_err_t nfc_init(void)
{
    ReturnCode err;
    
    ESP_LOGI(TAG, "Initializing NFC...");
    
    /* Initialize SPI */
    platformSpiInit();
    
    /* Initialize ST25R3911 chip */
    err = st25r3911Initialize();
    if (err != RFAL_ERR_NONE) {
        ESP_LOGE(TAG, "ST25R3911 initialization failed: %d", err);
        return ESP_FAIL;
    }

    /* DEBUG: Read and print Chip ID */
    uint8_t chipID = 0;
    st25r3911ReadRegister(ST25R3911_REG_IC_IDENTITY, &chipID);
    ESP_LOGI(TAG, "ST25R3911 Chip ID: 0x%02X", chipID);
    
    /* Initialize RFAL */
    err = rfalInitialize();
    if (err != RFAL_ERR_NONE) {
        ESP_LOGE(TAG, "rfalInitialize failed: %d", err);
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "NFC initialized successfully");
    return ESP_OK;
}

/* Poll for NFC-A tags (ISO14443A - includes NTAG, MIFARE, etc.) */
static void poll_nfca_tags(void)
{
    ReturnCode err;
    rfalNfcaSensRes sensRes;
    rfalNfcaListenDevice nfcaDevList[MAX_DEVICES];
    uint8_t devCnt = 0;
    
    /* Initialize for NFC-A */
    err = rfalNfcaPollerInitialize();
    if (err != RFAL_ERR_NONE) {
        ESP_LOGW(TAG, "NFC-A init failed: %d", err);
        return;
    }
    
    /* Turn field ON and start GT timer */
    err = rfalFieldOnAndStartGT();
    if (err != RFAL_ERR_NONE) {
        ESP_LOGW(TAG, "Field ON failed: %d", err);
        return;
    }
    
    /* Wait for GT to expire */
    while (!rfalIsGTExpired()) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    
    /* Perform technology detection */
    err = rfalNfcaPollerTechnologyDetection(RFAL_COMPLIANCE_MODE_NFC, &sensRes);
    if (err != RFAL_ERR_NONE) {
        if (err != RFAL_ERR_TIMEOUT && err != RFAL_ERR_BUSY) {
             ESP_LOGW(TAG, "NFC-A technology detection error: %d", err);
        }
    }
    if (err == RFAL_ERR_NONE) {
        ESP_LOGI(TAG, "NFC-A tag detected!");
        
        /* Perform collision resolution to get device details */
        err = rfalNfcaPollerFullCollisionResolution(RFAL_COMPLIANCE_MODE_NFC, MAX_DEVICES, nfcaDevList, &devCnt);
        
        if (err == RFAL_ERR_NONE && devCnt > 0) {
            ESP_LOGI(TAG, "Found %d NFC-A device(s)", devCnt);
            
            for (uint8_t i = 0; i < devCnt; i++) {
                printf("\n=== Device %d ===\n", i + 1);
                print_uid(nfcaDevList[i].nfcId1, nfcaDevList[i].nfcId1Len);
                
                /* Determine tag type */
                switch (nfcaDevList[i].type) {
                    case RFAL_NFCA_T1T:
                        printf("Type: T1T (Topaz)\n");
                        break;
                    case RFAL_NFCA_T2T:
                        printf("Type: T2T (NTAG/MIFARE Ultralight)\n");
                        break;
                    case RFAL_NFCA_T4T:
                        printf("Type: T4T (MIFARE DESFire)\n");
                        break;
                    case RFAL_NFCA_NFCDEP:
                        printf("Type: NFC-DEP (P2P)\n");
                        break;
                    default:
                        printf("Type: Unknown (0x%02X)\n", nfcaDevList[i].type);
                        break;
                }
                
                printf("SAK: 0x%02X\n", nfcaDevList[i].selRes.sak);
                if (nfcaDevList[i].type == RFAL_NFCA_T2T) {
                    uint8_t rxBuf[RFAL_T2T_READ_DATA_LEN];
                    uint16_t rcvLen;
                    
                    // Read pages 4-7 (first 16 bytes of user memory)
                    err = rfalT2TPollerRead(6, rxBuf, sizeof(rxBuf), &rcvLen);
                    if (err == RFAL_ERR_NONE) {
                        for (int k = 0; k < 4; k++) { // 4 pages read (4,5,6,7)
                            printf("Page %d: %02X %02X %02X %02X\n", 4 + k, 
                                   rxBuf[k*4+0], rxBuf[k*4+1], rxBuf[k*4+2], rxBuf[k*4+3]);
                        }
                    } else {
                        ESP_LOGW(TAG, "Failed to read T2T data: %d", err);
                    }
                }
            }
        }
    }
    
    /* Turn field OFF */
    rfalFieldOff();
}

/* Poll for NFC-V tags (ISO15693 - includes ICODE, etc.) */
static void poll_nfcv_tags(void)
{
    ReturnCode err;
    rfalNfcvInventoryRes invRes;
    
    /* Initialize for NFC-V */
    err = rfalNfcvPollerInitialize();
    if (err != RFAL_ERR_NONE) {
        ESP_LOGW(TAG, "NFC-V init failed: %d", err);
        return;
    }
    
    /* Turn field ON and start GT timer */
    err = rfalFieldOnAndStartGT();
    if (err != RFAL_ERR_NONE) {
        ESP_LOGW(TAG, "Field ON failed: %d", err);
        return;
    }
    
    /* Wait for GT to expire */
    while (!rfalIsGTExpired()) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    
    /* Check for NFC-V tag presence */
    err = rfalNfcvPollerCheckPresence(&invRes);
    if (err == RFAL_ERR_NONE) {
        ESP_LOGI(TAG, "NFC-V tag detected!");
        printf("\n=== NFC-V Tag ===\n");
        print_uid(invRes.UID, RFAL_NFCV_UID_LEN);
    }
    
    /* Turn field OFF */
    rfalFieldOff();
}

/* Main NFC polling task */
static void nfc_poll_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Starting NFC polling...");
    
    while (1) {
        /* Run RFAL worker continuously */
        rfalWorker();

        /* Poll for NFC-A tags */
        poll_nfca_tags();
        
        vTaskDelay(pdMS_TO_TICKS(100));
        
        /* Poll for NFC-V tags */
        poll_nfcv_tags();
        
        /* Wait before next poll cycle */
        vTaskDelay(pdMS_TO_TICKS(NFC_POLL_INTERVAL_MS));
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "NFC-PLG Example Application");
    ESP_LOGI(TAG, "===========================");
    
    /* Initialize NFC hardware and RFAL library */
    if (nfc_init() != ESP_OK) {
        ESP_LOGE(TAG, "NFC initialization failed!");
        return;
    }
    
    /* Create NFC polling task */
    xTaskCreate(nfc_poll_task, "nfc_poll", 4096, NULL, 5, NULL);
    
    ESP_LOGI(TAG, "Application started. Place an NFC tag near the reader...");
}
