/*
 * Dondji Firmware
 *
 * Copyright (c) 2026 BD1AHN
 *
 * Licensed under the Apache License, Version 2.0
 *
 * Project:
 *     叮咚鸡 (Dondji)
 *
 * Maintainer:
 *     BD1AHN
 *
 * Official Website:
 *     https://ethanyan6.github.io/Dondji/
 *
 * The Dondji name, logo, and official project identity
 * are protected separately from the source code license.
 */


/* Copyright 2023 Dual Tachyon
 * https://github.com/DualTachyon
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifdef ENABLE_FMRADIO

#include <string.h>

#include "app/fm.h"
#include "driver/bk1080.h"
#include "driver/st7565.h"
#include "external/printf/printf.h"
#include "font.h"
#include "misc.h"
#include "settings.h"
#include "ui/fmradio.h"
#include "ui/helper.h"
#include "ui/inputbox.h"
#include "ui/ui.h"
#include "ui/dualvfo_u8g2_freq.h"

/* JA: FM 画面は 3 段、すべて中央寄せ
 *   上段: CH 番号（CH12）、または状態（CH12に保存? / CH12を削除? / 自動登録中 / サーチ）
 *   中段: 周波数（大）
 *   下段: 局名（SPI 0x012000 の局名表）。自動登録中は登録した局数 */
static void FM_UI_PrintCenter(const char *pStr, uint8_t y_start, uint8_t y_end)
{
#ifdef ENABLE_CHINESE
    UI_PrintStringSmallAtPixel(pStr, 0, LCD_WIDTH - 1, y_start, y_end, 0);
#else
    (void)y_end;
    UI_PrintStringSmallNormalAt(pStr, (uint8_t)((LCD_WIDTH - strlen(pStr) * 7u) / 2u), y_start - 8u);
#endif
}

/* 受信中の周波数が入っているプリセット（1-48）。なければ 0 */
static uint8_t FM_UI_PresetForPlaying(void)
{
    if (gEeprom.FM_IsMrMode)
        return (uint8_t)(gEeprom.FM_SelectedChannel + 1u);
    for (unsigned i = 0; i < FM_CHANNELS_MAX; i++) {
        if (gFM_Channels[i] == gEeprom.FM_FrequencyPlaying)
            return (uint8_t)(i + 1u);
    }
    return 0;
}

void UI_DisplayFM(void)
{
    char String[40] = {0};
    bool showName = true;
#ifdef ENABLE_CHINESE
    const bool ja = (gUiLanguage == UI_LANGUAGE_CN);
#else
    const bool ja = false;
#endif
    UI_DisplayClear();

#ifdef ENABLE_FEAT_F4HWN
    UI_DrawLineBuffer(gFrameBuffer, 0, 0, (int16_t)(LCD_WIDTH - 1u), 0, true);
#endif

    /* JA: バンド範囲（例 87.5-108MHz）を右上に小さく表示（Dondji v5.2.8 と同じ。3 段化で抜けていたため戻した） */
    {
        const uint16_t freqMin = BK1080_GetFreqLoLimit(gEeprom.FM_Band);
        const uint16_t freqMax = BK1080_GetFreqHiLimit(gEeprom.FM_Band);

        sprintf(String, "%d%s-%dMHz",
            freqMin / 10,
            gEeprom.FM_Band == 0 ? ".5" : "",
            freqMax / 10
        );
#ifdef ENABLE_FEAT_F4HWN
        const uint8_t text_width = DualVfoU8g2_GetSmallTextWidth(String);
        const uint8_t x = (uint8_t)(LCD_WIDTH - text_width - 2);
        DualVfoU8g2_DrawSmallText(String, x, 2, true);
#else
        UI_PrintStringSmallNormal(String, 1, 0, 0);
#endif
        String[0] = '\0';
    }

    /* ---- 上段 ---- */
    if (gAskToSave) {
        /* 保存先は上下キーで選ぶ（gFM_ChannelPosition） */
        sprintf(String, ja ? "CH%02uに保存?" : "SAVE CH%02u?", (unsigned)gFM_ChannelPosition + 1u);
    } else if (gAskToDelete) {
        sprintf(String, ja ? "CH%02uを削除?" : "DEL CH%02u?", (unsigned)gEeprom.FM_SelectedChannel + 1u);
    } else if (gFM_ScanState != FM_SCAN_OFF && gFM_AutoScan) {
        /* 自動登録: プリセットを全消去して帯域を下から探し、見つけた局を CH01 から順に登録 */
        strcpy(String, ja ? "自動登録中" : "AUTO SCAN");
        showName = false;
    } else if (gFM_ScanState != FM_SCAN_OFF) {
        /* サーチ: 局で止まる。止まった局は局名も出す */
        strcpy(String, ja ? "サーチ中" : "SEEK");
    } else {
        const uint8_t preset = FM_UI_PresetForPlaying();
        if (preset != 0u)
            sprintf(String, "CH%02u", (unsigned)preset);
    }
    if (String[0] != '\0')
        FM_UI_PrintCenter(String, 10, 21);

    /* ---- 中段: 周波数（大, 中央） ---- */
    if (gInputBoxIndex == 0) {
        sprintf(String, "%d.%d", gEeprom.FM_FrequencyPlaying / 10, gEeprom.FM_FrequencyPlaying % 10);
    } else {
        const char * ascii = INPUTBOX_GetAscii();
        sprintf(String, "%.3s.%.1s", ascii, ascii + 3);
        showName = false;
    }
    {
        uint8_t width = 0;
        for (const char *p = String; *p; p++)
            width += (*p == '.') ? 3u : 13u;
        UI_DisplayFrequency(String, (uint8_t)((LCD_WIDTH - width) / 2u), 2, false);
    }

    /* ---- 下段 ---- */
    if (gFM_ScanState != FM_SCAN_OFF && gFM_AutoScan) {
        sprintf(String, ja ? "%u局 登録" : "%u FOUND", (unsigned)gFM_ChannelPosition);
        FM_UI_PrintCenter(String, 42, 57);
    }
#ifdef ENABLE_FEAT_STERANIAN_DISP_RADIOSTATION_NAME
    else if (showName) {
        char station[FM_STATION_NAME_LEN + 1];
        if (FM_GetStationName(gEeprom.FM_FrequencyPlaying, station))
            FM_UI_PrintCenter(station, 42, 57);
    }
#endif
    (void)showName;

#ifdef ENABLE_FEAT_F4HWN
    if (gEeprom.KEY_LOCK && gKeypadLocked > 0) {
        UI_DisplayUnlockKeyboard(-10);
    }
#endif

    ST7565_BlitFullScreen();
}

#endif
