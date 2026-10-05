#ifndef APP_CHFRSCANNER_H
#define APP_CHFRSCANNER_H

#include <stdbool.h>
#include <stdint.h>

// scan direction, if not equal SCAN_OFF indicates 
// that we are in a process of scanning channels/frequencies
extern int8_t            gScanStateDir;
extern bool              gScanKeepResult;
extern bool              gScanPauseMode;

#ifdef ENABLE_SCAN_RANGES
extern uint32_t          gScanRangeStart;
extern uint32_t          gScanRangeStop;
#endif

void CHFRSCANNER_Found(void);
void CHFRSCANNER_Stop(void);
void CHFRSCANNER_Start(const bool storeBackupSettings, const int8_t scan_direction);
void CHFRSCANNER_ContinueScanning(void);

#if defined(ENABLE_FEAT_F4HWN_RESUME_STATE) || defined(ENABLE_SCAN_RANGES)
    void CHFRSCANNER_ScanRange(void);
#endif

#ifdef ENABLE_SCAN_RANGES
    /* JA: 範囲スキャン中、今の周波数を除外する（F4HWN 5.9 / 6.0 と同じ。最大 64 件）。除外できたら true */
    bool CHFRSCANNER_ExcludeCurrentScanRange(void);
#endif

#ifdef ENABLE_FEAT_F4HWN
    extern uint32_t lastFoundFrqOrChan;
    extern uint32_t lastFoundFrqOrChanOld;
#endif

#endif