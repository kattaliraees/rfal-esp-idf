#pragma once

/* ============================================================
 *  Transport selection
 * ============================================================ */

/* We are using SPI, not I2C */
#define RFAL_USE_SPI

/* ST25R chip selection */
#define ST25R3911

/* ============================================================
 *  Supported RFAL modes
 * ============================================================ */



/* ============================================================
 *  RFAL feature enable
 * ============================================================ */

#define RFAL_FEATURE_NFCA                  true
#define RFAL_FEATURE_T1T                   false
#define RFAL_FEATURE_T2T                   true
#define RFAL_FEATURE_T4T                   true

#define RFAL_FEATURE_NFCV                  true

#define RFAL_FEATURE_ST25TB                false

#define RFAL_FEATURE_NFCB                  false
#define RFAL_FEATURE_NFCF                  false
#define RFAL_FEATURE_NFC_DEP               false

#define RFAL_FEATURE_WAKEUP_MODE           true
#define RFAL_FEATURE_LOWPOWER_MODE         false

/* ============================================================
 *  Buffer sizes
 * ============================================================ */

#define RFAL_FEATURE_NFC_RF_BUF_LEN        258U
#define RFAL_FEATURE_ISO_DEP_IBLOCK_MAX_LEN 256U
#define RFAL_FEATURE_ISO_DEP_APDU_MAX_LEN  512U

/* ============================================================
 *  Platform behavior (important)
 * ============================================================ */

/* We provide real implementations */


#define platformProtectWorker()
#define platformUnprotectWorker()

/* Optional but silence warnings */
#define platformGetSysTick()               (xTaskGetTickCount())
#define platformTimerGetRemaining(t)       (0U)

/* Logging disabled */
#define platformLog(...)
#define platformAssert(exp)
#define platformErrorHandle()
