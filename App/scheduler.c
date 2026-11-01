/* Copyright 2023 Dual Tachyon
 * https://github.com/DualTachyon
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *     Unless required by applicable law or agreed to in writing, software
 *     distributed under the License is distributed on an "AS IS" BASIS,
 *     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *     See the License for the specific language governing permissions and
 *     limitations under the License.
 */

#include "scheduler.h"
#include "app/chFrScanner.h"
#ifdef ENABLE_FMRADIO
    #include "app/fm.h"
#endif
#include "app/scanner.h"
#include "audio.h"
#include "functions.h"
#include "helper/battery.h"
#include "misc.h"
#include "settings.h"

#include "driver/backlight.h"
#include "driver/gpio.h"

#define DECREMENT(cnt) \
    do {               \
        if (cnt > 0)   \
            cnt--;     \
    } while (0)

#define DECREMENT_AND_TRIGGER(cnt, flag) \
    do {                                 \
        if (cnt > 0)                     \
            if (--cnt == 0)              \
                flag = true;             \
    } while (0)

static volatile uint32_t gGlobalSysTickCounter;

#ifdef ENABLE_FASTER_CHANNEL_SCAN
/* JA: スキャンのフルチューン後の待ちを 1ms 単位で扱う。
 * 10ms 刻みの gScanPauseDelayIn_10ms は設定時点から最初の刻みまでの 0〜10ms が揺らぐので、
 * SysTick の VAL（カウントダウン）を使った 1us 単位の時刻で期限を判定する。 */
static volatile bool gJaScanWaitArmed;
static uint32_t      gJaScanWaitDeadlineUs;

static uint32_t JA_NowUs(void)
{
    uint32_t t1, t2, v, p1, p2;
    do {
        p1 = SCB->ICSR & SCB_ICSR_PENDSTSET_Msk;
        t1 = gGlobalSysTickCounter;
        v  = SysTick->VAL;
        t2 = gGlobalSysTickCounter;
        p2 = SCB->ICSR & SCB_ICSR_PENDSTSET_Msk;
    } while (t1 != t2 || p1 != p2);

    if (p2)     /* 周期をまたいだが SysTick_Handler が未実行 */
        t1++;

    return t1 * 10000u + (SysTick->LOAD - v) / (SystemCoreClock / 1000000u);
}

void JA_ScanWaitArm(uint16_t ms)
{
    gJaScanWaitDeadlineUs = JA_NowUs() + (uint32_t)ms * 1000u;
    gJaScanWaitArmed      = true;
}

void JA_ScanWaitDisarm(void)
{
    gJaScanWaitArmed = false;
}

/* メインループ（APP_Update）から毎回呼ぶ。期限が来たらスキャン再開を予約する */
void JA_ScanWaitPoll(void)
{
    if (!gJaScanWaitArmed)
        return;

    if (gScanStateDir == SCAN_OFF ||
        gScanPauseDelayIn_10ms != 0 ||            /* 他の処理が一時停止などを設定した */
        gCurrentFunction == FUNCTION_INCOMING ||
        gCurrentFunction == FUNCTION_RECEIVE)
    {
        gJaScanWaitArmed = false;
        return;
    }

    if (gCurrentFunction == FUNCTION_MONITOR || gCurrentFunction == FUNCTION_TRANSMIT)
        return;                                   /* 従来どおり、モニター・送信中は進めない */

    if ((int32_t)(JA_NowUs() - gJaScanWaitDeadlineUs) >= 0)
    {
        gJaScanWaitArmed    = false;
        gScheduleScanListen = true;
    }
}
#endif

// we come here every 10ms
void SysTick_Handler(void)
{
    gGlobalSysTickCounter++;
    
    gNextTimeslice = true;

    if ((gGlobalSysTickCounter % 50) == 0) {
        gNextTimeslice_500ms = true;

#ifdef ENABLE_FEAT_F4HWN
        DECREMENT_AND_TRIGGER(gVfoSaveCountdown_10ms, gScheduleVfoSave);
        DECREMENT_AND_TRIGGER(gTxTimerCountdownAlert_500ms - ALERT_TOT * 2, gTxTimeoutReachedAlert);
        #ifdef ENABLE_FEAT_F4HWN_RX_TX_TIMER
            /* 接收时正计时，与系统原逻辑一致 */
            if (FUNCTION_IsRx())
                DECREMENT(gRxTimerCountdown_500ms);
        #endif
#endif
        
        // 仅在发射时递减，松开 PTT 后发射计时停止
        if (gCurrentFunction == FUNCTION_TRANSMIT)
            DECREMENT_AND_TRIGGER(gTxTimerCountdown_500ms, gTxTimeoutReached);
        DECREMENT(gSerialConfigCountDown_500ms);
    }

    if ((gGlobalSysTickCounter & 3) == 0)
        gNextTimeslice40ms = true;

#ifdef ENABLE_NOAA
    DECREMENT(gNOAACountdown_10ms);
#endif

    if (gScanListNameCountdown_10ms > 0 && --gScanListNameCountdown_10ms == 0)
        gUpdateDisplay = true;   /* JA: ポップアップを消すため再描画 */

    DECREMENT_AND_TRIGGER(gJaScanGroupSelCountdown_10ms, gJaScanGroupSelTimeout);   /* JA: クイックスキャンの無操作 */

    DECREMENT(gFoundCDCSSCountdown_10ms);

    DECREMENT(gFoundCTCSSCountdown_10ms);

    if (gCurrentFunction == FUNCTION_FOREGROUND)
        DECREMENT_AND_TRIGGER(gBatterySaveCountdown_10ms, gSchedulePowerSave);

    if (gCurrentFunction == FUNCTION_POWER_SAVE)
        DECREMENT_AND_TRIGGER(gPowerSave_10ms, gPowerSaveCountdownExpired);

    if (gScanStateDir == SCAN_OFF && !gCssBackgroundScan && gEeprom.DUAL_WATCH != DUAL_WATCH_OFF)
        if (gCurrentFunction != FUNCTION_MONITOR && gCurrentFunction != FUNCTION_TRANSMIT && gCurrentFunction != FUNCTION_RECEIVE)
            DECREMENT_AND_TRIGGER(gDualWatchCountdown_10ms, gScheduleDualWatch);

#ifdef ENABLE_NOAA
    if (gScanStateDir == SCAN_OFF && !gCssBackgroundScan && gEeprom.DUAL_WATCH == DUAL_WATCH_OFF)
        if (gIsNoaaMode && gCurrentFunction != FUNCTION_MONITOR && gCurrentFunction != FUNCTION_TRANSMIT)
            if (gCurrentFunction != FUNCTION_RECEIVE)
                DECREMENT_AND_TRIGGER(gNOAA_Countdown_10ms, gScheduleNOAA);
#endif

    if (gScanStateDir != SCAN_OFF)
        if (gCurrentFunction != FUNCTION_MONITOR && gCurrentFunction != FUNCTION_TRANSMIT)
            DECREMENT_AND_TRIGGER(gScanPauseDelayIn_10ms, gScheduleScanListen);

#ifdef ENABLE_FEAT_F4HWN_SCAN_FASTER
    // Scan stall watchdog: if the scan-resume countdown has expired
    // (gScanPauseDelayIn_10ms == 0) but no resume was actually scheduled
    // (gScheduleScanListen == false) and no real reception is happening
    // (squelch closed -> green LED off, not in RX/TX/MONITOR), the scan
    // loop is stuck. Force a resume after this many 10ms ticks. The
    // gScanPauseDelayIn_10ms == 0 guard ensures we never override the
    // user-configured ScnRev (SCAN_RESUME_MODE) pause: while that pause
    // is decrementing, the watchdog stays asleep.
    #define SCAN_FAST_STALL_WATCHDOG_10ms  25   // 250 ms
    static uint16_t scanFastStallCounter;

    if (gSetting_set_scn
        && gScanStateDir != SCAN_OFF
        && gScanPauseDelayIn_10ms == 0
        && !gScheduleScanListen
        && !g_SquelchLost
        && gCurrentFunction != FUNCTION_RECEIVE
        && gCurrentFunction != FUNCTION_TRANSMIT
        && gCurrentFunction != FUNCTION_MONITOR)
    {
        if (++scanFastStallCounter >= SCAN_FAST_STALL_WATCHDOG_10ms) {
            scanFastStallCounter = 0;
            gScheduleScanListen  = true;
        }
    }
    else
    {
        scanFastStallCounter = 0;
    }
#endif

    DECREMENT_AND_TRIGGER(gTailNoteEliminationCountdown_10ms, gFlagTailNoteEliminationComplete);

#ifdef ENABLE_VOICE
    DECREMENT_AND_TRIGGER(gCountdownToPlayNextVoice_10ms, gFlagPlayQueuedVoice);
#endif

#ifdef ENABLE_FMRADIO
    if (gFM_ScanState != FM_SCAN_OFF && gCurrentFunction != FUNCTION_MONITOR)
        if (gCurrentFunction != FUNCTION_TRANSMIT && gCurrentFunction != FUNCTION_RECEIVE)
            DECREMENT_AND_TRIGGER(gFmPlayCountdown_10ms, gScheduleFM);
#endif

#ifdef ENABLE_VOX
    DECREMENT(gVoxStopCountdown_10ms);
#endif

    DECREMENT(boot_counter_10ms);
}
