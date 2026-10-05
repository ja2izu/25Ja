
#include "app/app.h"
#include "app/chFrScanner.h"
#include "functions.h"
#include "misc.h"
#include "settings.h"
//#include "debugging.h"

int8_t            gScanStateDir;
bool              gScanKeepResult;
bool              gScanPauseMode;

#ifdef ENABLE_SCAN_RANGES
uint32_t          gScanRangeStart;
uint32_t          gScanRangeStop;

/* JA: 範囲スキャンで除外した周波数（F4HWN 5.9 / 6.0 の実装を移植）。範囲の開始からの刻み番号を最大 64 件覚える（RAM 約 140 バイト） */
#define SCAN_RANGE_SKIP_MAX 64
#if (SCAN_RANGE_SKIP_MAX & (SCAN_RANGE_SKIP_MAX - 1)) != 0
    #error SCAN_RANGE_SKIP_MAX must be a power of two
#endif

typedef struct {
    uint16_t sample[SCAN_RANGE_SKIP_MAX];
    uint32_t start;
    uint32_t stop;
    uint16_t step;
    uint8_t  count;
    uint8_t  next;
} ScanRangeSkipList_t;

static ScanRangeSkipList_t scanRangeSkip;
#endif

typedef enum {
    SCAN_NEXT_CHAN_SCANLIST1 = 0,
    SCAN_NEXT_CHAN_SCANLIST2,
    SCAN_NEXT_CHAN_DUAL_WATCH,
    SCAN_NEXT_CHAN_MR,
    SCAN_NEXT_NUM
} scan_next_chan_t;

scan_next_chan_t    currentScanList;
uint32_t            initialFrqOrChan;
uint8_t             initialCROSS_BAND_RX_TX;

#ifndef ENABLE_FEAT_F4HWN
    uint32_t lastFoundFrqOrChan;
#else
    uint32_t lastFoundFrqOrChan;
    uint32_t lastFoundFrqOrChanOld;
#endif

static void NextFreqChannel(void);
static void NextMemChannel(void);

#ifdef ENABLE_SCAN_RANGES
static void ScanRangeSkipClear(void)
{
    scanRangeSkip.count = 0;
    scanRangeSkip.next = 0;
}

static void ScanRangeSkipSync(void)
{
    const uint16_t step = gRxVfo->StepFrequency;

    if (scanRangeSkip.start == gScanRangeStart &&
        scanRangeSkip.stop == gScanRangeStop &&
        scanRangeSkip.step == step)
    {
        return;
    }

    ScanRangeSkipClear();
    scanRangeSkip.start = gScanRangeStart;
    scanRangeSkip.stop  = gScanRangeStop;
    scanRangeSkip.step  = step;
}

static uint16_t ScanRangeSkipSampleForFrequency(uint32_t frequency)
{
    const uint16_t step = gRxVfo->StepFrequency;
    if (!gScanRangeStart || step == 0 || frequency < gScanRangeStart || frequency > gScanRangeStop)
        return 0;

    const uint32_t sample = (frequency - gScanRangeStart) / step;
    if (sample >= 0xFFFFu)
        return 0;

    return (uint16_t)(sample + 1);
}

static bool ScanRangeSkipContainsFrequency(uint32_t frequency)
{
    const uint16_t sample = ScanRangeSkipSampleForFrequency(frequency);
    if (sample == 0)
        return false;

    for (uint8_t i = 0; i < scanRangeSkip.count; ++i)
        if (scanRangeSkip.sample[i] == sample)
            return true;

    return false;
}

static uint32_t ScanRangeNextFrequency(void)
{
    uint32_t frequency;
    uint8_t guard = SCAN_RANGE_SKIP_MAX + 1;

    ScanRangeSkipSync();

    do
    {
        frequency = APP_SetFreqByStepAndLimits(gRxVfo, gScanStateDir, gScanRangeStart, gScanRangeStop);
        gRxVfo->freq_config_RX.Frequency = frequency;
    } while (--guard && ScanRangeSkipContainsFrequency(frequency));

    return frequency;
}

bool CHFRSCANNER_ExcludeCurrentScanRange(void)
{
    ScanRangeSkipSync();

    uint16_t sample = ScanRangeSkipSampleForFrequency(lastFoundFrqOrChan);
    if (sample == 0)
        sample = ScanRangeSkipSampleForFrequency(gRxVfo->freq_config_RX.Frequency);

    if (sample == 0)
        return false;

    for (uint8_t i = 0; i < scanRangeSkip.count; ++i)
        if (scanRangeSkip.sample[i] == sample)
            return true;

    scanRangeSkip.sample[scanRangeSkip.next] = sample;
    scanRangeSkip.next = (scanRangeSkip.next + 1) & (SCAN_RANGE_SKIP_MAX - 1);
    if (scanRangeSkip.count < SCAN_RANGE_SKIP_MAX)
        scanRangeSkip.count++;

    return true;
}
#endif

#if defined(ENABLE_FEAT_F4HWN_RESUME_STATE) || defined(ENABLE_SCAN_RANGES)
    void CHFRSCANNER_ScanRange(void) {
        gScanRangeStart = gScanRangeStart ? 0 : gTxVfo->pRX->Frequency;
        gScanRangeStop = gEeprom.VfoInfo[!gEeprom.TX_VFO].freq_config_RX.Frequency;
        if(gScanRangeStart > gScanRangeStop)
            SWAP(gScanRangeStart, gScanRangeStop);
    }
#endif

void CHFRSCANNER_Start(const bool storeBackupSettings, const int8_t scan_direction)
{
    if (storeBackupSettings) {
        initialCROSS_BAND_RX_TX = gEeprom.CROSS_BAND_RX_TX;
        gEeprom.CROSS_BAND_RX_TX = CROSS_BAND_OFF;
        gScanKeepResult = false;
    }
    
    RADIO_SelectVfos();

    gNextMrChannel   = gRxVfo->CHANNEL_SAVE;
    currentScanList = SCAN_NEXT_CHAN_SCANLIST1;
    gScanStateDir    = scan_direction;

    if (IS_MR_CHANNEL(gNextMrChannel))
    {   

        if(!RADIO_CheckValidList(gEeprom.SCAN_LIST_DEFAULT)) {
            RADIO_NextValidList(1);
        }

        // channel mode
        if (storeBackupSettings) {
            initialFrqOrChan = gRxVfo->CHANNEL_SAVE;
            lastFoundFrqOrChan = initialFrqOrChan;
        }
        NextMemChannel();
    }
    else
    {   // frequency mode
        if (storeBackupSettings) {
            initialFrqOrChan = gRxVfo->freq_config_RX.Frequency;
            lastFoundFrqOrChan = initialFrqOrChan;
        }
        NextFreqChannel();
    }

#ifdef ENABLE_FEAT_F4HWN
    lastFoundFrqOrChanOld = lastFoundFrqOrChan;
#endif

    gScanPauseDelayIn_10ms = scan_pause_delay_in_2_10ms;
    gScheduleScanListen    = false;
    gRxReceptionMode       = RX_MODE_NONE;
    gScanPauseMode         = false;
}

/*
void CHFRSCANNER_ContinueScanning(void)
{
    if (IS_FREQ_CHANNEL(gNextMrChannel))
    {
        if (gCurrentFunction == FUNCTION_INCOMING)
            APP_StartListening(gMonitor ? FUNCTION_MONITOR : FUNCTION_RECEIVE);
        else
            NextFreqChannel();  // switch to next frequency
    }
    else
    {
        if (gCurrentCodeType == CODE_TYPE_OFF && gCurrentFunction == FUNCTION_INCOMING)
            APP_StartListening(gMonitor ? FUNCTION_MONITOR : FUNCTION_RECEIVE);
        else
            NextMemChannel();    // switch to next channel
    }
    
    gScanPauseMode      = false;
    gRxReceptionMode    = RX_MODE_NONE;
    gScheduleScanListen = false;
}
*/

void CHFRSCANNER_ContinueScanning(void)
{
    if (gCurrentFunction == FUNCTION_INCOMING &&
        (IS_FREQ_CHANNEL(gNextMrChannel) || gCurrentCodeType == CODE_TYPE_OFF))
    {
        APP_StartListening(gMonitor ? FUNCTION_MONITOR : FUNCTION_RECEIVE);
    }
    else
    {
        IS_FREQ_CHANNEL(gNextMrChannel) ? NextFreqChannel() : NextMemChannel();
    }

    gScanPauseMode      = false;
    gRxReceptionMode    = RX_MODE_NONE;
    gScheduleScanListen = false;
}

void CHFRSCANNER_Found(void)
{
    if (gEeprom.SCAN_RESUME_MODE > 80) {
        if (!gScanPauseMode) {
            gScanPauseDelayIn_10ms = scan_pause_delay_in_5_10ms * (gEeprom.SCAN_RESUME_MODE - 80) * 5;
            gScanPauseMode = true;
        }
    } else {
        gScanPauseDelayIn_10ms = 0;
    }

    // gScheduleScanListen is always false...
    gScheduleScanListen = false;

    /*
    if(gEeprom.SCAN_RESUME_MODE > 1 && gEeprom.SCAN_RESUME_MODE < 26)
    {
        if (!gScanPauseMode)
        {
            gScanPauseDelayIn_10ms = scan_pause_delay_in_5_10ms * (gEeprom.SCAN_RESUME_MODE - 1) * 5;
            gScheduleScanListen    = false;
            gScanPauseMode         = true;
        }
    }
    else
    {
        gScanPauseDelayIn_10ms = 0;
        gScheduleScanListen    = false;
    }
    */

    /*
    switch (gEeprom.SCAN_RESUME_MODE)
    {
        case SCAN_RESUME_TO:
            if (!gScanPauseMode)
            {
                gScanPauseDelayIn_10ms = scan_pause_delay_in_1_10ms;
                gScheduleScanListen    = false;
                gScanPauseMode         = true;
            }
            break;

        case SCAN_RESUME_CO:
        case SCAN_RESUME_SE:
            gScanPauseDelayIn_10ms = 0;
            gScheduleScanListen    = false;
            break;
    }
    */

#ifdef ENABLE_FEAT_F4HWN
    lastFoundFrqOrChanOld = lastFoundFrqOrChan;
#endif

    if (IS_MR_CHANNEL(gRxVfo->CHANNEL_SAVE)) { //memory scan
        lastFoundFrqOrChan = gRxVfo->CHANNEL_SAVE;
    }
    else { // frequency scan
        lastFoundFrqOrChan = gRxVfo->freq_config_RX.Frequency;
    }


    gScanKeepResult = true;
}

void CHFRSCANNER_Stop(void)
{
    if(initialCROSS_BAND_RX_TX != CROSS_BAND_OFF) {
        gEeprom.CROSS_BAND_RX_TX = initialCROSS_BAND_RX_TX;
        initialCROSS_BAND_RX_TX = CROSS_BAND_OFF;
    }
    gBackup_CROSS_BAND_RX_TX = CROSS_BAND_OFF;
    
    gScanStateDir = SCAN_OFF;

    const uint32_t chFr = gScanKeepResult ? lastFoundFrqOrChan : initialFrqOrChan;
    const bool channelChanged = chFr != initialFrqOrChan;
    if (IS_MR_CHANNEL(gNextMrChannel)) {
        gEeprom.MrChannel[gEeprom.RX_VFO]     = chFr;
        gEeprom.ScreenChannel[gEeprom.RX_VFO] = chFr;
        RADIO_ConfigureChannel(gEeprom.RX_VFO, VFO_CONFIGURE_RELOAD);

        if(channelChanged) {
            SETTINGS_SaveVfoIndices();
            gUpdateStatus = true;
        }
    }
    else {
        gRxVfo->freq_config_RX.Frequency = chFr;
        RADIO_ApplyOffset(gRxVfo);
        RADIO_ConfigureSquelchAndOutputPower(gRxVfo);
        if(channelChanged) {
            SETTINGS_SaveChannel(gRxVfo->CHANNEL_SAVE, gEeprom.RX_VFO, gRxVfo, 1);
        }
    }

    #ifdef ENABLE_FEAT_F4HWN_RESUME_STATE
        gEeprom.CURRENT_STATE = 0;
        SETTINGS_WriteCurrentState();
    #endif

    RADIO_SetupRegisters(true);
    gUpdateDisplay = true;
}

static void NextFreqChannel(void)
{
#ifdef ENABLE_SCAN_RANGES
    if(gScanRangeStart) {
        gRxVfo->freq_config_RX.Frequency = ScanRangeNextFrequency();   // JA: 除外した周波数を飛ばす
    }
    else
#endif
        gRxVfo->freq_config_RX.Frequency = APP_SetFrequencyByStep(gRxVfo, gScanStateDir);

    RADIO_ApplyOffset(gRxVfo);
    RADIO_ConfigureSquelchAndOutputPower(gRxVfo);
    RADIO_SetupRegisters(true);

#ifdef ENABLE_FASTER_CHANNEL_SCAN
    gScanPauseDelayIn_10ms = 9;   // 90ms
#else
    gScanPauseDelayIn_10ms = scan_pause_delay_in_6_10ms;
#endif

    gUpdateDisplay     = true;
}

static void NextMemChannel(void)
{
    static uint16_t prev_mr_chan = 0;
    const bool      enabled      = (gEeprom.SCAN_LIST_DEFAULT > 0 && gEeprom.SCAN_LIST_DEFAULT <= MR_CHANNELS_LIST + 1) ? gEeprom.SCAN_LIST_ENABLED : true;
    const int16_t   chan1        = (gEeprom.SCAN_LIST_DEFAULT > 0 && gEeprom.SCAN_LIST_DEFAULT <= MR_CHANNELS_LIST + 1 && gEeprom.SCANLIST_PRIORITY_CH[0] != MR_CHANNELS_MAX) ? gEeprom.SCANLIST_PRIORITY_CH[0] : -1;
    const int16_t   chan2        = (gEeprom.SCAN_LIST_DEFAULT > 0 && gEeprom.SCAN_LIST_DEFAULT <= MR_CHANNELS_LIST + 1 && gEeprom.SCANLIST_PRIORITY_CH[1] != MR_CHANNELS_MAX) ? gEeprom.SCANLIST_PRIORITY_CH[1] : -1;
    const uint16_t  prev_chan    = gNextMrChannel;
    uint16_t        chan         = 0;

    //char str[64] = "";

    if (enabled)
    {
        switch (currentScanList)
        {
            case SCAN_NEXT_CHAN_SCANLIST1:
                prev_mr_chan = gNextMrChannel;
    
                //sprintf(str, "-> Chan1 %d\n", chan1 + 1);
                //LogUart(str);

                if (chan1 >= 0)
                {
                    if (RADIO_CheckValidChannel(chan1, false, gEeprom.SCAN_LIST_DEFAULT))
                    {
                        currentScanList = SCAN_NEXT_CHAN_SCANLIST1;
                        gNextMrChannel   = chan1;
                        break;
                    }
                }

                [[fallthrough]];
            case SCAN_NEXT_CHAN_SCANLIST2:

                //sprintf(str, "-> Chan2 %d\n", chan2 + 1);
                //LogUart(str);

                if (chan2 >= 0)
                {
                    if (RADIO_CheckValidChannel(chan2, false, gEeprom.SCAN_LIST_DEFAULT))
                    {
                        currentScanList = SCAN_NEXT_CHAN_SCANLIST2;
                        gNextMrChannel   = chan2;
                        break;
                    }
                }

                [[fallthrough]];
            /*
            case SCAN_NEXT_CHAN_SCANLIST3:
                if (chan3 >= 0)
                {
                    if (RADIO_CheckValidChannel(chan3, false, 0))
                    {
                        currentScanList = SCAN_NEXT_CHAN_SCANLIST3;
                        gNextMrChannel   = chan3;
                        break;
                    }
                }
                [[fallthrough]];
            */
            // this bit doesn't yet work if the other VFO is a frequency
            case SCAN_NEXT_CHAN_DUAL_WATCH:
                // dual watch is enabled - include the other VFO in the scan
//              if (gEeprom.DUAL_WATCH != DUAL_WATCH_OFF)
//              {
//                  chan = (gEeprom.RX_VFO + 1) & 1u;
//                  chan = gEeprom.ScreenChannel[chan];
//                  if (IS_MR_CHANNEL(chan))
//                  {
//                      currentScanList = SCAN_NEXT_CHAN_DUAL_WATCH;
//                      gNextMrChannel   = chan;
//                      break;
//                  }
//              }

            default:
            case SCAN_NEXT_CHAN_MR:
                currentScanList = SCAN_NEXT_CHAN_MR;
                gNextMrChannel   = prev_mr_chan;
                chan             = 0xFFFF;
                break;
        }
    }

    if (!enabled || chan == 0xFFFF)
    {       
        chan = RADIO_FindNextChannel(gNextMrChannel + gScanStateDir, gScanStateDir, true, gEeprom.SCAN_LIST_DEFAULT);
        if (chan == 0xFFFF)
        {   // no valid channel found
            chan = MR_CHANNEL_FIRST;
        }
        
        gNextMrChannel = chan;

        //sprintf(str, "----> Chan %d\n", chan + 1);
        //LogUart(str);
    }

    if (gNextMrChannel != prev_chan)
    {
        gEeprom.MrChannel[    gEeprom.RX_VFO] = gNextMrChannel;
        gEeprom.ScreenChannel[gEeprom.RX_VFO] = gNextMrChannel;

        RADIO_ConfigureChannel(gEeprom.RX_VFO, VFO_CONFIGURE_RELOAD);
        RADIO_SetupRegisters(true);

        gUpdateDisplay = true;
    }

#ifdef ENABLE_FASTER_CHANNEL_SCAN
    gScanPauseDelayIn_10ms = 9;  // 90ms .. <= ~60ms it misses signals (squelch response and/or PLL lock time) ?
#else
    gScanPauseDelayIn_10ms = scan_pause_delay_in_3_10ms;
#endif

    if (enabled)
        if (++currentScanList >= SCAN_NEXT_NUM)
            currentScanList = SCAN_NEXT_CHAN_SCANLIST1;  // back round we go
}
