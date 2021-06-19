#include <Arduino.h>
#include <SPI.h>
#include "weather.h"
#include "rfm.h"

#define RFM_CS D0

RFM::RFM()
{

}

void RFM::Init(Stream &mydbgprn)
{
  dbgprinter = &mydbgprn;
  dbgprinter->println("RFM Init:");

  // radio.initialize();
  // radio.setFrequencyMHz(433.9);
  // radio.receiveBegin();
  pinMode(RFM_CS,OUTPUT);
  settings = SPISettings(4000000, MSBFIRST, SPI_MODE0);
  RFMSPI.begin();
  dbgprinter->println(F("Init RFM69"));
  rfm69Init();

  dbgprinter->println(F("start"));
  t0 = micros();
}

void RFM::loop()
{
  return;

  bool s = true; //radio.poll();
  uint32_t t = micros();
  uint32_t d = t - t0;

  if (s0 != s) {

    // end of 0
    if (s == 1) {

      if (gotone) {
        if (d > 1900 - TOL && d < 1900 + TOL) {
          val <<= 1;
          bits++;
        } else if (d > 4450 - TOL && d < 4450 + TOL) {
          val <<= 1;
          val |= 1;
          bits++;
        } else if (d > 9450 - TOL && d < 9450 + TOL) {
          if (bits == 28) {
//            char buf[50];
//            byte bl = sprintf(buf, "%04x", val >> 16);
//            buf[bl] = 0;
//            Serial.print(buf);
//            sprintf(buf, "%04x", val);
//            Serial.print(buf);
//            Serial.print(F(" = "));
            Serial.print(((val >> 4) & 0xfff) / 10.0);
            Serial.println(F("\260C"));
          }
          val = 0;
          bits = 0;
        }
      }

    // end of 1
    } else {

      if (d > 550 - TOL && d < 550 + TOL) {
        gotone = true;
      } else {
        gotone = false;
      }

    }

    t0 = t;
    s0 = s;
  }
}


//private functions
uint8_t RFM::sendSPI(uint8_t address, uint8_t data)
{
  RFMSPI.beginTransaction(settings);
  digitalWrite(RFM_CS,LOW);
  RFMSPI.transfer(address);
  uint8_t res = RFMSPI.transfer(data);
  digitalWrite(RFM_CS,HIGH);
  RFMSPI.endTransaction();
  return res;
}

void RFM::writeReg(uint8_t address, uint8_t data)
{
  sendSPI(address | 0x80, data);
}

uint8_t RFM::readReg(uint8_t address)
{
  return sendSPI(address & 0x7F, 0);
}


bool RFM::rfm69Init()
{
  const byte CONFIG[][2] =
  {
    /* 0x01 */ { RFM69_REG_OPMODE, RFM69_OPMODE_SEQUENCER_OFF | RFM69_OPMODE_LISTEN_OFF | RFM69_OPMODE_STANDBY },
    /* 0x02 */ { RFM69_REG_DATAMODUL, RFM69_DATAMODUL_DATAMODE_CONTINUOUSNOBSYNC | RFM69_DATAMODUL_MODULATIONTYPE_OOK | RFM69_DATAMODUL_MODULATIONSHAPING_00 }, // no shaping
    /* 0x03 */ { RFM69_REG_BITRATEMSB, 0x03}, // bitrate: 32768 Hz
    /* 0x04 */ { RFM69_REG_BITRATELSB, 0xD1},
    /* 0x19 */ { RFM69_REG_RXBW, RFM69_RXBW_DCCFREQ_010 | RFM69_RXBW_MANT_24 | RFM69_RXBW_EXP_4}, // BW: 10.4 kHz
    /* 0x1B */ { RFM69_REG_OOKPEAK, RFM69_OOKPEAK_THRESHTYPE_PEAK | RFM69_OOKPEAK_PEAKTHRESHSTEP_000 | RFM69_OOKPEAK_PEAKTHRESHDEC_000 },
    /* 0x1D */ { RFM69_REG_OOKFIX, 6 }, // Fixed threshold value (in dB) in the OOK demodulator
    /* 0x29 */ { RFM69_REG_RSSITHRESH, 140 }, // RSSI threshold in dBm = -(REG_RSSITHRESH / 2)
    /* 0x6F */ { RFM69_REG_TESTDAGC, RFM69_DAGC_IMPROVED_LOWBETA0 }, // run DAGC continuously in RX mode, recommended default for AfcLowBetaOn=0
    {255, 0}
  };

    for (byte i = 0; CONFIG[i][0] != 255; i++)
      writeReg(CONFIG[i][0], CONFIG[i][1]);

}