/* UTF-8 menu titles when UI language is Chinese */

#include <stddef.h>

#include "settings.h"
#include "ui/menu.h"

const char *UI_MENU_GetMenuTitle(const t_menu_item *item)
{
    if (item == NULL)
        return "";
    if (gUiLanguage != UI_LANGUAGE_CN)
        return item->name;

    switch (item->menu_id)
    {
    case MENU_STEP:         return "ステップ";
    case MENU_TXP:          return "送信出力";
    case MENU_R_DCS:        return "受信 DCS";
    case MENU_R_CTCS:       return "受信 CTCSS";
    case MENU_T_DCS:        return "送信 DCS";
    case MENU_T_CTCS:       return "送信 CTCSS";
    case MENU_SFT_D:        return "シフト 方向";
    case MENU_OFFSET:       return "シフト 周波数";
    case MENU_W_N:          return "ワイド/ナロー";
#ifndef ENABLE_FEAT_F4HWN
    case MENU_SCR:          return "秘話";
#endif
    case MENU_BCL:          return "ビジー ロック";
    case MENU_COMPAND:      return "音声 圧縮伸張";
    case MENU_AM:           return "変調方式";
#ifdef ENABLE_FEAT_F4HWN
    case MENU_TX_LOCK:      return "バンド外 送信禁止";
#endif
    case MENU_LIST_CH:      return "CHリスト";
    case MENU_MEM_CH:       return "CH保存";
    case MENU_DEL_CH:       return "CH削除";
    case MENU_MEM_NAME:     return "CH名 編集";
    case MENU_CN_MEM_NAME:  return "CH名 編集";
    case MENU_S_LIST:       return "スキャン リスト";
    case MENU_S_PRI:        return "優先 スキャン";
    case MENU_S_PRI_CH_1:   return "優先CH""1";
    case MENU_S_PRI_CH_2:   return "優先CH""2";
    case MENU_SC_REV:       return "スキャン 再開";
#ifndef ENABLE_FEAT_F4HWN
    #ifdef ENABLE_NOAA
    case MENU_NOAA_S:       return "NOAA-S";
    #endif
#endif
    case MENU_F1SHRT:       return "PF1短押";
    case MENU_F1LONG:       return "PF1長押";
    case MENU_F2SHRT:       return "PF2短押";
    case MENU_F2LONG:       return "PF2長押";
    case MENU_MLONG:        return "MENU 長押";
    case MENU_AUTOLK:       return "自動 ロック";
    case MENU_LANGUAGE:     return "表示言語";
    case MENU_TOT:          return "送信時間";
    case MENU_SAVE:         return "省電力";
    case MENU_BAT_TXT:      return "電池表示";
    case MENU_MIC:          return "マイク 感度"; /* 麦克风增益 — SPI 字库 */
    case MENU_MDF:          return "CH表示";
    case MENU_PONMSG:       return "起動画面";
    case MENU_BOOT_HINT:    return "起動表示";
    case MENU_BOOT_SOUND:   return "起動音";
    case MENU_BREATH:       return "LED点滅"; /* 飞机灯 */
    case MENU_ABR:          return "照明時間";
    case MENU_ABR_MIN:      return "照明最小";
    case MENU_ABR_MAX:      return "照明最大";
    case MENU_ABR_ON_TX_RX: return "送受照明";
    case MENU_BEEP:         return "キー音";
#ifdef ENABLE_VOICE
    case MENU_VOICE:        return "音声";
#endif
    case MENU_ROGER:        return "ラジャー 音";
#ifdef ENABLE_MDC_YAN_ID  // JA
    case MENU_MDC_ID:       return "MDC ID";
    case MENU_YAN_ID:       return "Yan ID";
    case MENU_YAN_ID_RX:    return "受信 Yan ID"; /* 接收 Yan ID */
    case MENU_MDC_ID_RX:    return "受信 MDC ID";
#endif
    case MENU_STE:          return "テール 消去";
    case MENU_RP_STE:       return "レピータ STE";
    case MENU_1_CALL:       return "ワンキー 呼出";
#ifdef ENABLE_ALARM
    case MENU_AL_MOD:       return "警報";
#endif
#ifdef ENABLE_DTMF_CALLING
    case MENU_ANI_ID:       return "ANI ID";
#endif
    case MENU_UPCODE:       return "開始符号";
    case MENU_DWCODE:       return "終了符号";
    case MENU_PTT_ID:       return "PTT ID";
    case MENU_D_ST:         return "DTMF ST";
#ifdef ENABLE_DTMF_CALLING
    case MENU_D_RSP:        return "DTMF 応答";
    case MENU_D_HOLD:       return "DTMF 保持";
#endif
    case MENU_D_PRE:        return "DTMF 前置時間";
#ifdef ENABLE_DTMF_CALLING
    case MENU_D_DCD:        return "DTMF 復号";
    case MENU_D_LIST:       return "DTMF 連絡先";
#endif
    case MENU_D_LIVE_DEC:   return "DTMF 受信表示";
#ifndef ENABLE_FEAT_F4HWN
    #ifdef ENABLE_AM_FIX
    case MENU_AM_FIX:       return "AM Fix";
    #endif
#endif
    case MENU_VOX:          return "VOX";
#ifdef ENABLE_FEAT_F4HWN
    case MENU_VOL:          return "本体情報";
#else
    case MENU_VOL:          return "電池電圧";
#endif
    case MENU_TDR:          return "受信方式<主画面>";
    case MENU_SQL:          return "スケルチ";
#ifdef ENABLE_FEAT_F4HWN
    case MENU_SET_PWR:      return "出力設定";
    case MENU_SET_PTT:      return "設定""PTT";
    case MENU_SET_TOT:      return "設定""TOT";
    case MENU_SET_EOT:      return "設定""EOT";
#ifdef ENABLE_FEAT_F4HWN_CTR
    case MENU_SET_CTR:      return "表示濃度";
#endif
    case MENU_SET_INV:      return "白黒反転";
    case MENU_SET_LCK:      return "ロック 範囲";
    case MENU_SET_MET:      return "メーター";
    case MENU_SET_GUI:      return "設定""GUI";
    case MENU_SET_TMR:      return "タイマー";
#ifdef ENABLE_FEAT_F4HWN_SLEEP
    case MENU_SET_OFF:      return "スリープ";
#endif
#ifdef ENABLE_FEAT_F4HWN_NARROWER
    case MENU_SET_NFM:      return "ナロー幅";
#endif
#ifdef ENABLE_FEAT_F4HWN_VOL
    case MENU_SET_VOL:      return "音量設定";
#endif
#ifdef ENABLE_FEAT_F4HWN_RESCUE_OPS
    case MENU_SET_KEY:      return "キー設定";
#endif
#ifdef ENABLE_NOAA
    case MENU_NOAA_S:       return "設定""NWR";
#endif
#ifdef ENABLE_FEAT_F4HWN_AUDIO
    case MENU_SET_AUD:      return "音質設定";
#endif
    case MENU_SET_NAV:      return "ナビキー";
#endif
    case MENU_RESET:        return "リセット";
    case MENU_F_LOCK:       return "バンド ロック";
#ifndef ENABLE_FEAT_F4HWN
    case MENU_200TX:        return "Tx200";
    case MENU_350TX:        return "Tx350";
    case MENU_500TX:        return "Tx500";
#endif
    case MENU_350EN:        return "350""有効";
#ifndef ENABLE_FEAT_F4HWN
    case MENU_SCREN:        return "秘話有効";
#endif
#ifdef ENABLE_F_CAL_MENU
    case MENU_F_CALI:       return "周波数 校正";
#endif
    case MENU_BATCAL:       return "電池校正";
    case MENU_BATTYP:       return "電池種類";
#ifdef ENABLE_SPECTRUM
    case MENU_SPECTRUM_MODE: return "スペアナ 表示";
#endif
#ifdef ENABLE_AUDIO_BAR
    case MENU_MIC_BAR:      return "送信表示"; /* 发射提示 */
#endif
    default:
        return item->name;
    }
}
