
#ifndef RFM_H
#define RFM_H
#include <Arduino.h>
//#include <RFM69OOK.h>
#include <SPI.h>
#include <RFM69registers.h>

#define TOL 50 // +- tolerance
#define RFMSPI SPI
#define RF69OOK_FSTEP 61.03515625 // == FXOSC/2^19 = 32mhz/2^19 (p13 in DS)

class RFM
{
    public:
    RFM();
    void Init(Stream &mydbgprn);
    void loop();
    void setFrequencyMHz(float f);
    void setFrequency(uint32_t freqHz);

    private:
    uint8_t sendSPI(uint8_t address, uint8_t data);
    void writeReg(uint8_t address, uint8_t data);
    uint8_t readReg(uint8_t address);
    bool rfm69Init();
    
    SPISettings settings;
    
    Stream* dbgprinter;
    //RFM69OOK radio;

    uint32_t val = 0;
    uint32_t t0 = 0;
    byte bits = 0;
    byte s0 = 0;
    bool gotone = false;

};

#endif