#include <SerialESP8266wifi.h>
#include "MyType.h"
#define esp8266_reset_pin 2 // Connect this pin to CH_PD on the esp8266, not reset. (let reset be unconnected)



// the last parameter sets the local echo option for the ESP8266 module..
SerialESP8266wifi wifi(Serial1, Serial1, esp8266_reset_pin, Serial);

void processCommand(WifiMessage msg);

uint8_t wifi_started = false;

// TCP Commands
const char RST[] PROGMEM = "RST";
const char IDN[] PROGMEM = "*IDN?";
/**
 * @brief 
 *
 *
 */
void setupNetwork() 
{

  while (!Serial)
  wifi.setTransportToTCP();// this is also default
  wifi.endSendWithNewline(false); // Will end all transmissions with a newline and carrage return ie println.. default is true
  wifi_started = wifi.begin();
//  if(network_conf.DHCP == 0)
//    wifi.config(network_conf.IP, network_conf.GATEWAY, network_conf.NETMASK);
  if (wifi_started) 
  {
    wifi.connectToAP(network_conf.SSID, network_conf.PASSWORD);
    wifi.startLocalServer(network_conf.PORT);
  } 
  else 
  {  
    // ESP8266 isn't working..
  }
}
/**
 * @brief 
 *
 *
 */
void loopNetwork() 
{
  static WifiConnection *connections;
  // check connections if the ESP8266 is there
  if (wifi_started)
    wifi.checkConnections(&connections);
  // check for messages if there is a connection
  for (int i = 0; i < MAX_CONNECTIONS; i++) 
  {
    if (connections[i].connected) 
    {
      // See if there is a message
      WifiMessage msg = wifi.getIncomingMessage();
      // Check message is there
      if (msg.hasData) 
      {
        // process the command
        processCommand(msg);
      }
    }
  }
}
/**
 * @brief 
 *
 *
 * @param param1
 */
void processCommand(WifiMessage msg) 
{
 
  // return buffer
  char espBuf[MSG_BUFFER_MAX];
  // scanf holders
  int set;
  char str[16];

  // Get command and setting
  sscanf(msg.message,"%15s %d",str,&set);

  if ( !strcmp_P(str,IDN) ) 
  {
    wifi.send(msg.channel,"ESP8266wifi Example");
  }
  // Reset system by temp enable watchdog
  else if ( !strcmp_P(str,RST) ) 
  {
    wifi.send(msg.channel,"SYSTEM RESET...");
    // soft reset by reseting PC
    asm volatile ("  jmp 0");
  }
  // Unknown command
  else 
  {
    wifi.send(msg.channel,"ERR");
  }
}
