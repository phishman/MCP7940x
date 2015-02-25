
#include <Wire.h>
#include <avr/pgmspace.h>
#include "mcp7940x.h"

//////////////////////////////////////////////////////////
// utility code, some of this could be exposed in the DateTime API if needed

static const uint8_t daysInMonth [] PROGMEM = { 31,28,31,30,31,30,31,31,30,31,30,31 };

// number of days since 2000/01/01, valid for 2001..2099
static uint16_t date2days(uint16_t y, uint8_t m, uint8_t d) {
    if (y >= 2000)
        y -= 2000;
    uint16_t days = d;
    for (uint8_t i = 1; i < m; ++i)
        days += pgm_read_byte(daysInMonth + i - 1);
    if (m > 2 && y % 4 == 0)
        ++days;
    return days + 365 * y + (y + 3) / 4 - 1;
}

static long time2long(uint16_t days, uint8_t h, uint8_t m, uint8_t s) {
    return ((days * 24L + h) * 60 + m) * 60 + s;
}


//////////////////////////////////////////////////////
// DateTime implementation - ignores time zones and DST changes
// NOTE: also ignores leap seconds, see http://en.wikipedia.org/wiki/Leap_second

DateTime::DateTime (long t) {
    ss = t % 60;
    t /= 60;
    mm = t % 60;
    t /= 60;
    hh = t % 24;
    uint16_t days = t / 24;
    uint8_t leap;
    for (yOff = 0; ; ++yOff) {
        leap = yOff % 4 == 0;
        if (days < 365 + leap)
            break;
        days -= 365 + leap;
    }
    for (m = 1; ; ++m) {
        uint8_t daysPerMonth = pgm_read_byte(daysInMonth + m - 1);
        if (leap && m == 2)
            ++daysPerMonth;
        if (days < daysPerMonth)
            break;
        days -= daysPerMonth;
    }
    d = days + 1;
}

DateTime::DateTime (uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t min, uint8_t sec) {
    if (year >= 2000)
        year -= 2000;
    yOff = year;
    m = month;
    d = day;
    hh = hour;
    mm = min;
    ss = sec;
}

static uint8_t conv2d(const char* p) {
    uint8_t v = 0;
    if ('0' <= *p && *p <= '9')
        v = *p - '0';
    return 10 * v + *++p - '0';
}

// A convenient constructor for using "the compiler's time":
//   DateTime now (__DATE__, __TIME__);
// NOTE: using PSTR would further reduce the RAM footprint
DateTime::DateTime (const char* date, const char* time) {
    // sample input: date = "Dec 26 2009", time = "12:34:56"
    yOff = conv2d(date + 9);
    // Jan Feb Mar Apr May Jun Jul Aug Sep Oct Nov Dec 
    switch (date[0]) {
        case 'J': m = date[1] == 'a' ? 1 : m = date[2] == 'n' ? 6 : 7; break;
        case 'F': m = 2; break;
        case 'A': m = date[2] == 'r' ? 4 : 8; break;
        case 'M': m = date[2] == 'r' ? 3 : 5; break;
        case 'S': m = 9; break;
        case 'O': m = 10; break;
        case 'N': m = 11; break;
        case 'D': m = 12; break;
    }
    d = conv2d(date + 4);
    hh = conv2d(time);
    mm = conv2d(time + 3);
    ss = conv2d(time + 6);
}

uint8_t DateTime::dayOfWeek() const {    
    uint16_t day = get() / SECONDS_PER_DAY;
    return (day + 6) % 7; // Jan 1, 2000 is a Saturday, i.e. returns 6
}

long DateTime::get() const {
    uint16_t days = date2days(yOff, m, d);
    return time2long(days, hh, mm, ss);
}
////////////////////////////////////////////////////////

////////////////////////////////////////////////////////
// RTC_MCP7940X implementation

void RTC_MCP7940X::adjust(const DateTime& dt) {
    uint8_t buffer[8]; 
    
    buffer[0] = bin2bcd(dt.second())|0x80;
    buffer[1] = bin2bcd(dt.minute());
    buffer[2] = bin2bcd(dt.hour());
    buffer[3] = bin2bcd(1|0x08);
    buffer[4] = bin2bcd(dt.day());	
    buffer[5] = bin2bcd(dt.month());
    buffer[6] = bin2bcd(dt.year() - 2000);
    buffer[7] = 0;
    
	writeRegisterBlock(MCP7940X_ADDRESS, 0, buffer, 8);
}

DateTime RTC_MCP7940X::now() {
  	uint8_t buffer[7];
	
	readRegisterBlock(MCP7940X_ADDRESS, 0, buffer, 7);

    uint8_t ss = bcd2bin(buffer[0]&0x7F);
    uint8_t mm = bcd2bin(buffer[1]);
    uint8_t hh = bcd2bin(buffer[2]);
    uint8_t d = bcd2bin(buffer[4]);
    uint8_t m = bcd2bin(buffer[5]&0x1F);
    uint16_t y = bcd2bin(buffer[6]) + 2000;
    
    return DateTime (y, m, d, hh, mm, ss);
}

DateTime RTC_MCP7940X::GetPwrOn() {
	uint8_t buffer[4];
	
	readRegisterBlock(MCP7940X_ADDRESS, 0x1C, buffer, 4);

    uint8_t ss = 0;
    uint8_t mm = bcd2bin(buffer[0]);
    uint8_t hh = bcd2bin(buffer[1]);
    uint8_t d = bcd2bin(buffer[2]);
    uint8_t m = bcd2bin(buffer[3]&0x1F);
    uint16_t y = 0;
    return DateTime (y, m, d, hh, mm, ss);
}

DateTime RTC_MCP7940X::GetPwrFail() {
  	uint8_t buffer[4];
	
	readRegisterBlock(MCP7940X_ADDRESS, 0x18, buffer, 4);

    uint8_t ss = 0;
    uint8_t mm = bcd2bin(buffer[0]);
    uint8_t hh = bcd2bin(buffer[1]);
    uint8_t d = bcd2bin(buffer[2]);
    uint8_t m = bcd2bin(buffer[3]&0x1F);
    uint16_t y = 0;
    return DateTime (y, m, d, hh, mm, ss);
}

void RTC_MCP7940X::ClearPowerFail() {

	uint8_t dayfield = readRegister(MCP7940X_ADDRESS, 0x03);
	dayfield &= 0xEF;	
	writeRegister(MCP7940X_ADDRESS, 0x03, dayfield);
}

void RTC_MCP7940X::WriteSRAM(uint8_t *BUFFER, uint8_t Index, uint8_t Count) {
	if((0x20+Index+Count) > 0x5F) {
	  Count = 0x3F + Index;
	}
	writeRegisterBlock(MCP7940X_ADDRESS, (0x20+Index), BUFFER, Count);
}

void RTC_MCP7940X::ReadSRAM(uint8_t *BUFFER, uint8_t Index, uint8_t Count) {
	readRegisterBlock(MCP7940X_ADDRESS, (0x20+Index), BUFFER, Count);
}

void RTC_MCP7940X::setmac(uint8_t *MAC) {
    setmac(MAC, 6);
}

void RTC_MCP7940X::setmac(uint8_t *MAC, uint8_t Count) {
	writeRegister(MCP7940X_ADDRESS, 0x09, 0x55);	//unlock sequence one	
	delay(5);
	
	writeRegister(MCP7940X_ADDRESS, 0x09, 0xAA);	//unlock sequence two	
	delay(5);

	writeRegisterBlock(MCP7940X_ADDRESS, 0xF0, MAC, Count);
	delay(5);
	
	writeRegister(MCP7940X_ADDRESS, 0x09, 0x55);		//re-lock sequence one								
    delay(5);											//Repeated unlock sequence to lock
	
	writeRegister(MCP7940X_ADDRESS, 0x09, 0xAA);		//re-lock sequence two
}
	

void  RTC_MCP7940X::getmac(uint8_t *MAC) {
	getmac(MAC,6);
}

void  RTC_MCP7940X::getmac6(uint8_t *MAC) {
  #ifdef MCP79402
	getmac(MAC,6,8);
  #else
    getmac(MAC,6,6);
  #endif
}

void  RTC_MCP7940X::getmac(uint8_t *MAC, uint8_t count) {
  #ifdef MCP79402
	getmac(MAC,count,8);
  #else
    getmac(MAC,count,6);
  #endif
}

void  RTC_MCP7940X::getmac(uint8_t *MAC, uint8_t Count, uint8_t type) {
	if(type == 8)
	  readRegisterBlock(MCP7940X_MAC, 0xF0, MAC, Count);
	if(type == 6) {
	  Count = 6;
	  readRegisterBlock(MCP7940X_MAC, 0xF2, MAC, Count);
	}
}

void RTC_MCP7940X::SetSQW(uint8_t rate) {
  rate &= 0x07;
  writeRegister(MCP7940X_ADDRESS, MCP7940X_CTRLREG, rate|MCP7940X_MFP_SQWE_ON|MCP7940X_MFP_LOGIC_HIGH);
}
////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////
// Generic I2C routines

uint8_t readRegister(uint8_t address, uint8_t offset) {
  uint8_t data;
  readRegisterBlock(address, offset, &data, 1);
  return(data);
}

uint8_t readRegister(uint8_t address, uint8_t offset, bool RS) {
  uint8_t data;
  readRegisterBlock(address, offset, &data, 1, RS);
  return(data);
}

int readRegisterBlock(uint8_t address, uint8_t offset, uint8_t *data, uint8_t count) {
  return(readRegisterBlock(address, offset, data, count, true));
}

int readRegisterBlock(uint8_t address, uint8_t offset, uint8_t *data, uint8_t count, bool RS) {    
	/* Read and return the value in the register at the given address. 
     */
    Wire.beginTransmission(address);
    I2CWRITE((byte) offset);
	#if defined(ARDUINO) && ARDUINO >= 100
      Wire.endTransmission(RS);
	#else
	  Wire.endTransmission();
	#endif
    Wire.requestFrom(address, count);

    int ndx = 0;
	unsigned long before = millis();
	while ((ndx < count) && ((millis() - before) < 1000))
	{
    	if (Wire.available()) data[ndx++] = I2CREAD();
	}
	return ndx; 
}

int readRegisterBlockW(uint8_t address, unsigned int offset, uint8_t *data, uint8_t count) {
  return(readRegisterBlockW(address, offset, data, count, (bool)true));
}

int readRegisterBlockW(uint8_t address, unsigned int offset, uint8_t *data, uint8_t count, bool RS) {    
	/* Read and return the value in the register at the given address. 
     */
    Wire.beginTransmission(address);
    I2CWRITE((uint16_t)(offset >> 8));
	I2CWRITE((uint16_t)(offset & 0xFF));
	#if defined(ARDUINO) && ARDUINO >= 100
      Wire.endTransmission(RS);
	#else
	  Wire.endTransmission();
	#endif
    Wire.requestFrom(address, count);

    int ndx = 0;
	unsigned long before = millis();
	while ((ndx < count) && ((millis() - before) < 1000))
	{
    	if (Wire.available()) data[ndx++] = I2CREAD();
	}
	return ndx; 
}

void writeRegister(uint8_t address, uint8_t offset, uint8_t value) {
	writeRegisterBlock(address, offset, &value, 1);
}

void writeRegister(uint8_t address, uint8_t offset, uint8_t value, bool RS) {
	writeRegisterBlock(address, offset, &value, 1, RS);
}

int writeRegisterBlock(uint8_t address, uint8_t offset, uint8_t *data, uint8_t count) {
	writeRegisterBlock(address, offset, data, count, true);
}

int writeRegisterBlock(uint8_t address, uint8_t offset, uint8_t *data, uint8_t count, bool RS) {
	/* Write the given value to the register at the given address. 
     */
    Wire.beginTransmission(address);
    I2CWRITE(offset);
	uint8_t ndx;
	for(ndx=0;ndx<count;ndx++) {
      I2CWRITE(data[ndx]);
	}
    #if defined(ARDUINO) && ARDUINO >= 100
      Wire.endTransmission(RS);
	#else
	  Wire.endTransmission();
	#endif
	return ndx;
}

int writeRegisterBlockW(uint8_t address, unsigned int offset, uint8_t *data, uint8_t count) {
  return(writeRegisterBlockW(address, offset, data, count, true));
}

int writeRegisterBlockW(uint8_t address, unsigned int offset, uint8_t *data, uint8_t count, bool RS) {
	/* Write the given value to the register at the given address. 
     */
    Wire.beginTransmission(address);
    I2CWRITE((uint16_t)(offset >> 8));
	I2CWRITE((uint16_t)(offset & 0xFF));
	uint8_t ndx;
	for(ndx=0;ndx<count;ndx++) {
      I2CWRITE(data[ndx]);
	}
    #if defined(ARDUINO) && ARDUINO >= 100
      Wire.endTransmission(RS);
	#else
	  Wire.endTransmission();
	#endif
	return ndx;
}
//////////////////////////////////////////////////////////