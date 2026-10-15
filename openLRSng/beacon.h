#ifndef _BEACON_H_
#define _BEACON_H_

uint8_t beaconGetRSSI(void);
void beacon_tone(int16_t hz, int16_t len);
void beacon_send(bool static_tone);

static inline void beacon_delay_us(uint32_t us)
{
  while (us > 10000) {
    delayMicroseconds(10000);
    us -= 10000;
  }
  if (us) {
    delayMicroseconds((uint16_t)us);
  }
}

uint8_t beaconGetRSSI(void)
{
  uint16_t rssiSUM = 0;
  Green_LED_ON

  rfmSetChannel(0);
  rfmSetCarrierFrequency(rx_config.beacon_frequency);

  delay(1);
  rssiSUM += rfmGetRSSI();
  delay(1);
  rssiSUM += rfmGetRSSI();
  delay(1);
  rssiSUM += rfmGetRSSI();
  delay(1);
  rssiSUM += rfmGetRSSI();

  Green_LED_OFF
  return rssiSUM>>2;
}

void beacon_tone(int16_t hz, int16_t len)
{
  // len is in half seconds. Use 32 bit math: 500000/hz overflows int16 for
  // low tones (e.g. the 10Hz squelch breaker) and delayMicroseconds() is only
  // accurate up to ~16ms, so long half-periods are split into chunks.
  uint32_t d = 500000UL / (uint32_t)hz; // half period in us

  if (d < 1) {
    d = 1;
  }

  uint32_t cycles = ((uint32_t)len * 500000UL / d) >> 1;

  for (uint32_t i = 0; i < cycles; i++) {
    SDI_on;
    beacon_delay_us(d);
    SDI_off;
    beacon_delay_us(d);
  }
}

void beacon_send(bool static_tone)
{
  watchdogConfig(WATCHDOG_4S);
  Green_LED_ON
  
  rfmClearInterrupts();
  rfmClearIntStatus();
  rfmSetDirectOut(1); // enable direct output
  rfmSetChannel(0);
  rfmSetCarrierFrequency(rx_config.beacon_frequency);
  rfmSetPower(0x07);   // 7 set max power 100mW
  rfmSetTX();

  // 10Hz sub-audible tone to break squelch
  beacon_tone(10, 1);
  delay(250);

  if (static_tone) {
    uint8_t i=0;
    while (i++<20) {
      beacon_tone(440,1);
      watchdogReset();
    }
  } else {
    //close encounters tune
    //  G, A, F, F (lower octave), C
    //octave 3:  392  440  349  175   261

    beacon_tone(392, 2);  // 7 max power 100mw
    watchdogReset();

    rfmSetPower(0x05);   // 7 max power 100mW
    beacon_tone(440, 2);
    watchdogReset();

    rfmSetPower(0x04);   // 4 set mid power 13mW
    beacon_tone(349, 2);
    watchdogReset();

    rfmSetPower(0x02);   // 2 set min power 3mW
    beacon_tone(175, 2);
    watchdogReset();

    rfmSetPower(0x00);   // 0 set min power 1.3mW
    beacon_tone(261, 4);
    watchdogReset();
  }
  rfmSetReadyMode();
  rfmSetDirectOut(0);  // disable direct output
  Green_LED_OFF
  watchdogConfig(WATCHDOG_2S);
}

#endif
