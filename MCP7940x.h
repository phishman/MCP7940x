#ifndef MCP7940X
#define MCP7940X


#if defined(ARDUINO) && ARDUINO >= 100
  #include "Arduino.h"
  #ifndef I2CWRITE(x)
    #define I2CWRITE(x) Wire.write(x)
  #endif
  #ifndef I2CREAD()
    #define I2CREAD() Wire.read()
  #endif
#else
  #include "WProgram.h"
  #ifndef I2CWRITE(x)
    #define I2CWRITE(x) Wire.send(x)
  #endif
  #ifndef I2CREAD()
    #define I2CREAD() Wire.receive()
  #endif
#endif

#include <inttypes.h>
#include <avr/pgmspace.h>


/////////////////////////////////////////////////////////////
// Common/Generic Class independent I2C functions to support old and new IDE versions
uint8_t readRegister(uint8_t address, uint8_t offset);
uint8_t readRegister(uint8_t address, uint8_t offset, bool RS);
void writeRegister(uint8_t address, uint8_t offset, uint8_t value);
void writeRegister(uint8_t address, uint8_t offset, uint8_t value, bool RS);
int readRegisterBlock(uint8_t address, uint8_t offset, uint8_t *data, uint8_t count);
int readRegisterBlock(uint8_t address, uint8_t offset, uint8_t *data, uint8_t count, bool RS);
int readRegisterBlockW(uint8_t address, unsigned int offset, uint8_t *data, uint8_t count);
int readRegisterBlockW(uint8_t address, unsigned int offset, uint8_t *data, uint8_t count, bool RS);
int writeRegisterBlock(uint8_t address, uint8_t offset, uint8_t *data, uint8_t count);
int writeRegisterBlock(uint8_t address, uint8_t offset, uint8_t *data, uint8_t count, bool RS);
int writeRegisterBlockW(uint8_t address, unsigned int offset, uint8_t *data, uint8_t count);
int writeRegisterBlockW(uint8_t address, unsigned int offset, uint8_t *data, uint8_t count, bool RS);
/////////////////////////////////////////////////////////////

///////////////////////////////////////////////////
// RTC Definitions
#define MCP7940X_ADDRESS	0x6F
#define MCP7940X_MAC		0x57

#define MCP7940X_OSC_BAT	0x03	// bits 0-2 Day, bit 3=Bat En, bit 4=Vbat, bit 5=OSCON
#define MCP7940X_CTRLREG	0x07
#define MCP7940X_CALREG		0x08
#define MCP7940X_ALM0		0x0A
#define MCP7940X_ALM0_POL	0x0D	// bits 0-2 Day
#define MCP7940X_ALM1		0x11
#define MCP7940X_ALM1_POL	0x14	// bits 0-2 Day

#define MCP7940X_MFP_LOGIC_LOW	0x00
#define MCP7940X_MFP_LOGIC_HIGH	0x80

#define MCP7940X_MFP_SQWE_ON	0x40
#define MCP7940X_MFP_SQWE_OFF	0x00

#define MCP7940X_MFP_ALM1		0x20
#define MCP7940X_MFP_ALM0		0x10

#define MCP7940X_MFP_EXTOSC_ON	0x08
#define MCP7940X_MFP_EXTOSC_OFF 0x00

#define MCP7940X_MFP_1HZ		0x00
#define MCP7940X_MFP_4096HZ		0x01
#define MCP7940X_MFP_8192HZ		0x02
#define MCP7940X_MFP_32768HZ	0x03
#define MCP7940X_MFP_CALOUT		0x04

#define MCP7940X_ALMPOL_LL_L	0x00
#define MCP7940X_ALMPOL_LL_H	0x80

#define MCP7940X_ALMPOL_CFG		0x07	// AND mask for config bits
#define MCP7940X_ALMPOL_CFG_SEC	0x00
#define MCP7940X_ALMPOL_CFG_MIN	0x01
#define MCP7940X_ALMPOL_CFG_HR	0x02
#define MCP7940X_ALMPOL_CFG_DAY	0x03	// interrupt at midnight
#define MCP7940X_ALMPOL_CFG_DATE	0x04
#define MCP7940X_ALMPOL_CFG_ALL		0x07  // match sec, min, hr, day, date, month

#define MCP7940X_ALMPOL_INT_FLAG	0x08
#define MCP7940X_ALMPOL_INT_CLR		0xF7	// AND mask to clear bit


#define SECONDS_PER_DAY     86400L

/////////////////////////////////////////////////////////////
// RTC Class Definitions
// Simple general-purpose date/time class (no TZ / DST / leap second handling!)
class DateTime {
public:
    DateTime (long t =0);
    DateTime (uint16_t year, uint8_t month, uint8_t day,
                uint8_t hour =0, uint8_t min =0, uint8_t sec =0);
    DateTime (const char* date, const char* time);

    uint16_t year() const       { return 2000 + yOff; }
    uint8_t month() const       { return m; }
    uint8_t day() const         { return d; }
    uint8_t hour() const        { return hh; }
    uint8_t minute() const      { return mm; }
    uint8_t second() const      { return ss; }
    uint8_t dayOfWeek() const;

    // 32-bit times as seconds since 1/1/2000
    long get() const;   

protected:
    uint8_t yOff, m, d, hh, mm, ss;
};


// RTC based on the MCP7940X chip connected via I2C and the Wire library
class RTC_MCP7940X {
public:
    static void begin() {}
    static void adjust(const DateTime& dt);
    static DateTime now();
	
	static DateTime GetPwrOn();
	static DateTime GetPwrFail();
	static void ClearPowerFail();
	
	// RAM registers read/write functions. Address locations 08h to 3Fh.
    // Max length = 56 bytes.
    static uint8_t readByteInRam(uint8_t address);
    static void readBytesInRam(uint8_t address, uint8_t length, uint8_t* p_data);
    static void writeByteInRam(uint8_t address, uint8_t data);
    static void writeBytesInRam(uint8_t address, uint8_t length, uint8_t* p_data);
	static void WriteSRAM(uint8_t *BUFFER, uint8_t Index, uint8_t Count);
	static void ReadSRAM(uint8_t *BUFFER, uint8_t Index, uint8_t Count);
	
	static void setmac(uint8_t *MAC);
	static void setmac(uint8_t *MAC, uint8_t Count);
	static void getmac(uint8_t *MAC);
	static void getmac6(uint8_t *MAC);
	static void getmac(uint8_t *MAC, uint8_t Count);
	static void getmac(uint8_t *MAC, uint8_t Count, uint8_t type);
	
	static void SetSQW(uint8_t rate);
    // utility functions
    static uint8_t bcd2bin (uint8_t val) { return val - 6 * (val >> 4); }
    static uint8_t bin2bcd (uint8_t val) { return val + 6 * (val / 10); }
};
//////////////////////////////////////////////////////////////

#endif