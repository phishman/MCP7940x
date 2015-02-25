#include <MCP7940x.h>
#include <Wire.h>


RTC_MCP7940X RTC;
char buf[9];
uint8_t mac[6];

void setup () {
    Serial.begin(9600);
    Wire.begin();
    RTC.begin();
    
    // following line sets the RTC to the date & time this sketch was compiled
    //RTC.adjust(DateTime(__DATE__, __TIME__));
}

void loop () {
  
    print_mac();
    DateTime now = RTC.now();
    
    Serial.print(now.year(), DEC);
    Serial.print('/');
    Serial.print(now.month(), DEC);
    Serial.print('/');
    Serial.print(now.day(), DEC);
    Serial.print(' ');
    sprintf(buf,"%02d:%02d:%02d", now.hour(), now.minute(), now.second());
    Serial.println(buf);
   
    delay(3000);
}

void print_mac(void)
{
    RTC.getmac(mac);
    Serial.print(mac[0],HEX);
    Serial.print(mac[1],HEX);
    Serial.print(mac[2],HEX);
    Serial.print(mac[3],HEX);
    Serial.print(mac[4],HEX);
    Serial.print(mac[5],HEX);
    Serial.print(mac[6],HEX);
    Serial.println();
}
