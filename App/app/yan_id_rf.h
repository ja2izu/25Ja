/*
 * Dondji Firmware — Yan ID AirCopy FSK TX/RX (mangosteen-compatible)
 */
#ifndef APP_YAN_ID_RF_H
#define APP_YAN_ID_RF_H

#include <stdbool.h>
#include <stdint.h>

#include "settings.h"

#ifdef ENABLE_MDC_YAN_ID
extern char    gYanId_RX[YAN_ID_LEN + 1];
extern uint8_t gYanId_RX_timeout;

bool YAN_RF_ReceiveEnabled(void);
bool YAN_RF_Send(void);
void YAN_RF_EnableRx(void);
void YAN_RF_DisableRx(void);
void YAN_RF_OnRadioInterrupt(uint16_t irq02_bits);
void YAN_RF_Tick10ms(void);
void YAN_RF_Tick500ms(void);
#else
/* JA: Yan ID を外したビルド。呼び出し側はそのままで何もしない */
static inline bool YAN_RF_ReceiveEnabled(void) { return false; }
static inline bool YAN_RF_Send(void) { return false; }
static inline void YAN_RF_EnableRx(void) {}
static inline void YAN_RF_DisableRx(void) {}
static inline void YAN_RF_OnRadioInterrupt(uint16_t irq02_bits) { (void)irq02_bits; }
static inline void YAN_RF_Tick10ms(void) {}
static inline void YAN_RF_Tick500ms(void) {}
#endif

#endif
