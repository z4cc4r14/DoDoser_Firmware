/****************************************************************************
   Copyright (C) 2018 by Sandro Feliciani
 *                                                                          *
   This file is part of Box.
 ****************************************************************************/
#include <EEPROM.h>
#include <Arduino.h>
#include "MyType.h"
#include <Time.h>
#include <EEPROM.h>
#include <Wire.h>
#include <DS1307RTC.h>
#include <ArduinoJson.h>
//**********************************************************
//Initialization
//**********************************************************
byte Velocita = 200;
byte DataRX = -1;
uint32_t precTime;
pump_Type pumps[NUM_OF_PUMP];
uint8_t serialNum = 0;
line_type line[NUM_OF_LINE];
network_conf_type network_conf;
uint8_t flagSchedulerServed = 0;
uint8_t SaveNewStatus = 0;
int16_t SizeOfSchedulerFrame = sizeof(timerDose_type) * NUM_OF_SCHEDULER;
uint32_t schedulercounterwrite = 0;
uint32_t schedulercounteread = 0;
uint8_t precSended = 0;
uint8_t NumOfPkt = 0;
uint32_t globalCounter = 0;
char check;
char dataUART[MAX_DATA];
int y = 0;
int microTimerCounter = 0;
int counterStreamEnd = 10;
int CheckParam(int data, uint32_t maxLevel);
void eepromReadAllData();
void eepromReadScheData();
void eepromSaveAllData();
void returnError(char typeError);

/**
   @brief This function provide to inizialize all variable and she call al init
   function of shared library

*/
void setup()
{
  totTimeSum = 0;
  totTime = 0;
  //Init USB serial
  Serial.begin(57600);
  //Init debug serial
  Serial2.begin(9600);
  //Start HW serial for ESP8266 (change baud depending on firmware)
  Serial1.begin(115200);
  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW);
  //Inizializzo le strutture dati e leggo i dati dalla EEPROM
#if DEBUG == 1
  Serial2.println("START");
#endif
  network_conf.DHCP = 1;
  strncpy(network_conf.IP, "   ", 2);
  strncpy(network_conf.PORT, "   ", 2);
  strncpy(network_conf.NETMASK, "   ", 2);
  strncpy(network_conf.GATEWAY, "   ", 2);
  strncpy(network_conf.SSID, "NETGEAR14", 9);
  strncpy(network_conf.PASSWORD, "jaggedzoo147", 12);
  setupNetwork();
  int i = 0;
  for (i = 0; i < NUM_OF_SCHEDULER; i++)
  {
    i = CheckParam(i, NUM_OF_SCHEDULER);
    dose[i].ID = 0;
    dose[i].HOUR = 0;
    dose[i].MINUTE = 0;
    dose[i].EXEC = 0;
    dose[i].ID_LINE = 0;
    dose[i].QUANTITY = 0;
    dose[i].DAYOFWEEK = 0;
  }
  for (i = 0; i < NUM_OF_LINE; i++)
  {
    i = CheckParam(i, NUM_OF_PUMP);
    line[i].ID_PUMP = i;//associo ad ogni linea la rispettiva pompa
  }
  precTime = millis();
  globalCounter = 0;

  pumps[0].active = 0;
  pumps[0].state = 0;
  pumps[0].counter = 0;
  pumps[0].pin = 5;
  pumps[0].lastCounterValue = 0;

  pumps[1].active = 0;
  pumps[1].state = 0;
  pumps[1].counter = 0;
  pumps[1].pin = 9;
  pumps[1].lastCounterValue = 0;

  pumps[2].active = 0;
  pumps[2].state = 0;
  pumps[2].counter = 0;
  pumps[2].pin = 6;
  pumps[2].lastCounterValue = 0;

  pumps[3].active = 0;
  pumps[3].state = 0;
  pumps[3].counter = 0;
  pumps[3].pin = 10;
  pumps[3].lastCounterValue = 0;

  eepromReadAllData();
  //pin5 e pin6 in uscita
  pinMode(pumps[0].pin, OUTPUT);
  pinMode(pumps[1].pin, OUTPUT);
  pinMode(pumps[2].pin, OUTPUT);
  pinMode(pumps[3].pin, OUTPUT);
  //pin5 e pin6 bassi
  digitalWrite(pumps[0].pin, LOW);
  digitalWrite(pumps[1].pin, LOW);
  digitalWrite(pumps[2].pin, LOW);
  digitalWrite(pumps[3].pin, LOW);
  //nuova inizializzazione RTC
  setSyncProvider(RTC.get);
  if (timeStatus() != timeSet)
  {
    delay(200);
    returnError(ERROR_NO_RTC_COM);
  }
  check = 0;
  for (y = 0; y < MAX_DATA; y++)
    dataUART[y] = 0;
  Wire.begin();
  eepromReadScheData();
  // initialize timer1
  noInterrupts();           // disable all interrupts
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1  = 0;

  //OCR1A = 31250;            // compare match register 16MHz/256/2Hz
  OCR1A = 511;            // 511 con prescaler 8 otteniamo 1024 us = circa 1ms
  TCCR1B |= (1 << WGM12);   // CTC mode
  //TCCR1B |= (1 << CS12);    // 256 prescaler
  TCCR1B |= (1 << CS11);     //8bit prescaler
  TIMSK1 |= (1 << OCIE1A);  // enable timer compare interrupt
  interrupts();
  //setupNetwork();
  y = 0;
  counterStreamEnd = 800;
}
/**
   @brief Main loop function

*/
int numOfGraph = 0;
void loop()
{
  //**********************************************************Loop LAN
  loopNetwork();
  //**********************************************************
  int i = 0;
  globalCounter = millis();
  //Elaboro dati se presenti in seriale
  //Finchè ho dati li leggo e li carico in array
  while (Serial.available())
  {
    if (y < MAX_DATA)
    {
      dataUART[y++] = Serial.read();
     // if (dataUART[y] == 0x7D)
     // {  
        numOfGraph++;
        //Serial2.println(dataUART);
      //}
      /* if (dataUART[y] == '{')
        numOfGraph++;*/
      counterStreamEnd = 0;
    }
    else
    {
#if DEBUG == 1
      Serial2.println("OVERFLOW READ SERIAL");
#endif
      y = 0;
    }
  }




  if (counterStreamEnd == 45)
  {
    numOfGraph = 0;
    y = 0;
    //StaticJsonBuffer<MAX_DATA> jsonBuffer;
    const size_t bufferSize = 5 * JSON_ARRAY_SIZE(4) + JSON_ARRAY_SIZE(7) + JSON_ARRAY_SIZE(8) + JSON_OBJECT_SIZE(9) + 210;
    DynamicJsonBuffer jsonBuffer(bufferSize);

    JsonObject& root = jsonBuffer.parseObject(dataUART);
    if (!root.success())
    {
#if DEBUG == 1
      int i = 0;
      for (i = 0; i < 120; i++)
        Serial2.print(dataUART[i]);
      Serial2.println("parseObject() failed");
#endif
    }
    else
    {
      int i = 0;
#if DEBUG == 1
      Serial2.println("parseObject() success");
#endif
    }
    if (root["CMD"] == CMD_SET_TIME)
    {
#if DEBUG == 1
      Serial2.print("Impostata data e ora: ");
#endif
      //Nuova gestione rtc:
      tmElements_t tm;
      uint32_t Support = 0;
      //Year
      Support = root["Y"];//atoi(&data[4]);
#if DEBUG == 1
      Serial2.print(Support);
#endif
      if ( Support > 99)
        Support = Support - 1970;
      else
        Support += 30;
      tm.Year = Support;
      //Month
      Support = root["M"];
#if DEBUG == 1
      Serial2.print("-");
      Serial2.print(Support);
#endif
      tm.Month = Support;
      //Day
      Support = root["D"];
#if DEBUG == 1
      Serial2.print("-");
      Serial2.print(Support);
#endif
      tm.Day = Support;
#if DEBUG == 1
      Serial2.print(" ");
#endif
      //Hour
      Support = root["h"];
#if DEBUG == 1
      Serial2.print(Support);
#endif
      tm.Hour = Support;
      //Minute
      Support = root["m"];
#if DEBUG == 1
      Serial2.print(":");
      Serial2.print(Support);
#endif
      tm.Minute = Support;
      //Second
      Support = root["s"];
#if DEBUG == 1
      Serial2.print(":");
      Serial2.print(Support);
#endif
      tm.Second = Support;
      //Dow
      Support = root["dw"];
#if DEBUG == 1
      Serial2.print(":");
      Serial2.print(Support);
#endif
      tm.Wday = Support;//Sunday is 1
#if DEBUG == 1
      Serial2.print("\n");
#endif
      time_t nowTime = makeTime(tm);
      RTC.set(nowTime);
      setTime(nowTime);
      //segnalo al software che ho settato l'ora

      Serial.print("{\"CMD\":6,\"TYPE\":0}");

    }

    if (root["CMD"] == CMD_DOSE)
    {
      int numLine = 0;
      int x = 0;
      for (x = 0; x < NUM_OF_LINE; x++)
      {
        //cerco quale linea è
        if (line[x].ID_PUMP == root["PUMP"])
        {
          numLine = x;
          break;
        }
      }
      numLine = CheckParam(numLine, NUM_OF_LINE);
      if (numLine == -1)
      {
        //no linee associate alla pompa
        returnError(ERROR_NO_PUMP_ASSOCIATED);
        return;
      }
      if (line[numLine].ABILITATION == 0)
      {
        //Linea non abilitata
        returnError(ERROR_LINE_NOT_ABLE);
        return;
      }
      numLine;
      //verifico la congruenza del dato
      numLine = CheckParam(numLine, NUM_OF_LINE);
      if (pumps[numLine].state == 1)
      { //Se la pompa sta già dosando esco e non faccio nulla
        returnError(ERROR_LINE_NOT_FREE);
        return;
      }
      float quantity = root["QUANTITY"];
      //altrimenti doso...
      Dosing(numLine, (quantity * line[numLine].CALIBRATION));

    }


    if (root["CMD"] == CMD_REFILL)
    {
      int8_t nofp = root["PUMP"];
      //OVERFLOW RISK
      nofp = CheckParam(nofp, NUM_OF_LINE);
      line[nofp].LEVEL = 100;
      eepromSaveAllData();
      Serial.print("{\"CMD\":4}");
    }
    if (root["CMD"] == CMD_BREAK_DOSE)
    {
      uint8_t Support = root["PUMP"];
      if (stopDose(Support))
      {
        returnError(ERROR_PUMP_ALREADY_STOPED);
      }
    }




    if (root["CMD"] == CMD_CONFIG_CAPACITY)
    {
      //mi è stata mandata una configurazione
      int8_t o = 0;
      JsonArray& supp1 = root["capacity"];
      for (o = 0; o < 4; o++)
      {
        line[o].CAPACITY = supp1[o];
      }

      Serial.print("{\"CMD\":13}");
      eepromSaveAllData();
    }
    if (root["CMD"] == CMD_CONFIG_ABLE)
    {
      //mi è stata mandata una configurazione
      int8_t o = 0;
      JsonArray& supp2 = root["abilitation"];
      for (o = 0; o < 4; o++)
      {
        line[o].ABILITATION = supp2[o];
      }

      Serial.print("{\"CMD\":14}");
      eepromSaveAllData();
    }
    if (root["CMD"] == CMD_CONFIG_TIME_ABLE)
    {
      //mi è stata mandata una configurazione
      int8_t o = 0;
      JsonArray& supp3 = root["timer-abilitation"];
      for (o = 0; o < 4; o++)
      {
        line[o].TIMER_ABILITATION = supp3[o];
      }

      Serial.print("{\"CMD\":15}");
      eepromSaveAllData();
    }
    if (root["CMD"] == CMD_CONFIG_CALIB)
    {
      //mi è stata mandata una configurazione
      int8_t o = 0;
      JsonArray& supp4 = root["calibration"];
      for (o = 0; o < 4; o++)
      {
        line[o].CALIBRATION = supp4[o];
      }

      Serial.print("{\"CMD\":16}");
      eepromSaveAllData();
    }


    if (root["CMD"] == CMD_STATUS)
    {
#if DEBUG == 1
      Serial2.println("CMD_STATUS received");
#endif
      StaticJsonBuffer<500> jsonBuffer;
      JsonObject& root = jsonBuffer.createObject();
      root.set<int>("CMD", 10);
      //level
      JsonArray& data = root.createNestedArray("levels");
      data.add(line[0].LEVEL);
      data.add(line[1].LEVEL);
      data.add(line[2].LEVEL);
      data.add(line[3].LEVEL);
      //capacity
      JsonArray& data1 = root.createNestedArray("capacity");
      data1.add(line[0].CAPACITY);
      data1.add(line[1].CAPACITY);
      data1.add(line[2].CAPACITY);
      data1.add(line[3].CAPACITY);
      //abil
      JsonArray& data2 = root.createNestedArray("able");
      data2.add<int>(line[0].ABILITATION);
      data2.add<int>(line[1].ABILITATION);
      data2.add<int>(line[2].ABILITATION);
      data2.add<int>(line[3].ABILITATION);
      //timerAbil
      JsonArray& data3 = root.createNestedArray("timer-able");
      data3.add<int>(line[0].TIMER_ABILITATION);
      data3.add<int>(line[1].TIMER_ABILITATION);
      data3.add<int>(line[2].TIMER_ABILITATION);
      data3.add<int>(line[3].TIMER_ABILITATION);
      //calibration
      JsonArray& data4 = root.createNestedArray("calibration");
      data4.add<unsigned int>(line[0].CALIBRATION);
      data4.add<unsigned int>(line[1].CALIBRATION);
      data4.add<unsigned int>(line[2].CALIBRATION);
      data4.add<unsigned int>(line[3].CALIBRATION);
      //data e ora
      JsonArray& data5 = root.createNestedArray("timestamp");
      data5.add<unsigned int>(year());
      data5.add<unsigned int>(month());
      data5.add<unsigned int>(day());
      data5.add<unsigned int>(hour());
      data5.add<unsigned int>(minute());
      data5.add<unsigned int>(second());
      data5.add<unsigned int>(weekday());
      JsonArray& data6 = root.createNestedArray("pump-active");
      for (int i = 0; i < NUM_OF_PUMP ; i++)
        data6.add<unsigned int>(pumps[i].state);
      uint32_t timeTotSupp = totTime;// * 100) / totTimeSum;
      if (totTime != 0)
        root.set<unsigned long>("doseT", timeTotSupp);
      else
        root.set<unsigned int>("doseT", 0);
      root.printTo(Serial);
      // root.printTo(Serial2);
    }
    if (root["CMD"] == CMD_SEND_SCHEDULER)
    {
      //Ho appena ricevuto una richiesta di leggere la configurazione oraria
      if (root["NOI"] == 0)
        schedulercounteread = 0;
      uint16_t send_counter = root["NOI"];
      StaticJsonBuffer<500> jsonBuffer;
      JsonObject& root = jsonBuffer.createObject();
      root.set<int>("CMD", CMD_SEND_SCHEDULER);
      //level
      JsonArray& data = root.createNestedArray("BACON");
      data.add(dose[send_counter].ID);
      data.add(dose[send_counter].HOUR);
      data.add(dose[send_counter].MINUTE);
      data.add(dose[send_counter].EXEC);
      data.add(dose[send_counter].ID_LINE);
      data.add(dose[send_counter].QUANTITY);
      data.add(dose[send_counter].DAYOFWEEK);
      schedulercounteread++;
      root.set<int>("NOIC", schedulercounteread);
      root.printTo(Serial);
    }
    if (root["CMD"] == CMD_SCHEDULER)
    {
      if (root["NOI"] == 0)
        schedulercounterwrite = 0;
      uint16_t numOfSchedToWrite = root["NOI"];
      dose[numOfSchedToWrite].ID = root["BACON"][0];
      dose[numOfSchedToWrite].HOUR = root["BACON"][1];
      dose[numOfSchedToWrite].MINUTE = root["BACON"][2];
      dose[numOfSchedToWrite].EXEC = root["BACON"][3];
      dose[numOfSchedToWrite].ID_LINE = root["BACON"][4];
      dose[numOfSchedToWrite].QUANTITY = root["BACON"][5];
      dose[numOfSchedToWrite].DAYOFWEEK = root["BACON"][6];
      schedulercounterwrite++;
      StaticJsonBuffer<350> jsonBuffer;
      JsonObject& root = jsonBuffer.createObject();
      root.set<int>("CMD", CMD_SCHEDULER);
      root.set<int>("NOIC", schedulercounterwrite);
      root.printTo(Serial);
      if (schedulercounterwrite == NUM_OF_SCHEDULER)
      {
        int i = 0;
        uint8_t timerDoseRaw[1500];
        for (i = 0; i < 1500; i++)
          timerDoseRaw[i] = 0;
        for (i = 0; i < sizeof(dose); i++)
          memcpy(timerDoseRaw, &dose, i);
        for (i = EEPROM_SCHEDULER_DATA_ADDRESS_L; i < EEPROM_SCHEDULER_DATA_ADDRESS_H; i++)
        {
          EEPROM.write(i, timerDoseRaw[CheckParam((i - EEPROM_SCHEDULER_DATA_ADDRESS_L), EEPROM_SCHEDULER_DATA_ADDRESS_H)]);
        }
      }
    }
    if (root["CMD"] == CMD_CONFIG_IP)
    {
      //mi salvo la nuova configurazione del network e riavvio il wifi
      network_conf.DHCP = root["DHCP"];
      strncpy(network_conf.IP, root["IP"], sizeof(network_conf.IP));
      //   network_conf.PORT = root["PORT"];
      strncpy(network_conf.PORT, root["PORT"], sizeof(network_conf.PORT));
      strncpy(network_conf.NETMASK, root["NET"], sizeof(network_conf.NETMASK));
      strncpy(network_conf.GATEWAY, root["GAT"], sizeof(network_conf.GATEWAY));
      strncpy(network_conf.SSID, root["SSID"], sizeof(network_conf.SSID));
      strncpy(network_conf.PASSWORD, root["PWD"], sizeof(network_conf.PASSWORD));
      setupNetwork();

    }
    int i = 0;
    for (i = 0; i < 120; i++)
    {
      //   Serial2.print(dataUART[i]);
      dataUART[i] = 0;
    }
  }
  if ((globalCounter % 1000) == 0)
  {
    if (flagSchedulerServed == 0)
    {
      TimeScheduler();
      flagSchedulerServed = 1;
      totTime = 0;
      char Pstate[NUM_OF_PUMP];
      for (int i = 0; i < NUM_OF_PUMP; i++)
        Pstate[i] = 0;
      int state = 0;
      //ogni 400 msecondi controllo se devo mandare lo stato
      for (int i = 0; i < NUM_OF_PUMP; i++)
      {
        i = CheckParam(i, NUM_OF_PUMP);
        if (pumps[i].state == 1)
        {
          Pstate[i] = 1;
          state = 1;
          totTime = totTime + pumps[i].counter;
          totTimeSum = totTime;
        }
      }
      if ((state == 1) && (precSended != totTime))
      {
        //se sto dosando invio al software l'avanzamento del dosaggio
        precSended = totTime;
        state = 0;
      }
      if (totTime == 0 && (precSended != totTime))
      {
#if DEBUG == 1
        Serial2.println("FineDosaggio");
#endif
        //Ho finito l'erogazione
        precSended = totTime;
      }
    }
  }
  else
    flagSchedulerServed = 0;
  if (precTime != globalCounter)
  { //qui entro ogni 1 ms
    counterStreamEnd++;
    precTime = millis();
  }
  //questo flag indica se posso scrivere in memoria il nuovo dato inerente la quantità residua
  int8_t okToWrite = 0;
  //Questo flag indica se ho linee in erogazione
  int8_t isInErogation = 0;
  //Questo flag indica se i contatori sono effettivamente a 0
  int8_t totCounter = 0;
  for (i = 0; i < NUM_OF_PUMP; i++)
  { //Verifico se ci sono erogazioni in corso
    i = CheckParam(i, NUM_OF_PUMP);
    totCounter = totCounter + pumps[i].counter;
    if (pumps[i].state == 1 || pumps[i].active == 1)
      isInErogation = 1;
  }
  if (!isInErogation)
  { //Se NON ci sono erogazioni in corso do l'OK  per la scrittura in memoria dei dati
    if (totCounter == 0)
      okToWrite = 1;
  }
  if (okToWrite && SaveNewStatus)
  { //solo se ho finito tutte le erogazioni e le pompe sono in off salvo il nuovo dato in EEPROM
    digitalWrite(LED, LOW);
    //Ho finito l'erogazione, ora mando ultimo messaggio di chiusura dosaggio che fa chiudere la status bar
    eepromSaveAllData();
    SaveNewStatus = 0;
  }
}




ISR(TIMER1_COMPA_vect)          // gestisco i counter tramite interrupt timer per ottenere minor errore
{
  //digitalWrite(LED, digitalRead(ledPin) ^ 1);   // toggle LED pin
  int i = 0;
  for (i = 0; i < NUM_OF_PUMP; i++)
  {
    i = CheckParam(i, NUM_OF_PUMP);
    if (pumps[i].active  == 1)
    { //abilito il countdown, inizio erogazione
      analogWrite(pumps[i].pin, Velocita);
      pumps[i].state = 1;
      pumps[i].active = 0;
    }
    if (pumps[i].state)
    {
      if (pumps[i].counter != 0)
      { //Gestisco il contatore per derementare il count down
        pumps[i].counter--;
      }
      if (pumps[i].counter == 0)
      { //fine erogazione
        pumps[i].state = 0;
        digitalWrite(pumps[i].pin, LOW);
        int x = 0;
        for (x = 0; x < NUM_OF_LINE; x++)
        {
          x = CheckParam(x, NUM_OF_LINE);
          if (line[x].ID_PUMP == i)
          {
            float support = (((float)pumps[i].lastCounterValue) / (float)line[x].CALIBRATION);
            //cerco quale linea è associata alla pompa che ha appena finito il ciclo e ne scalo il consumo dall'ammontare
            line[x].LEVEL = ((((line[x].LEVEL * line[x].CAPACITY) / 100) - support) * 100) / line[x].CAPACITY;
            SaveNewStatus = 1;
          }
        }
      }
    }
  }
}
/**
   @brief This function is used to dose a certain quantity of solution on defined line.


   @param param1 loneN the number of line we whant to dose.
   @param param2 The quantity of solution we whant to dose on line passed on @p param1.
   @return 0 if no error occurred, 1 if the configuration of line is not valid.
*/
int DoseLine(uint8_t lineN, float qty)
{
  uint8_t vlineN = CheckParam(lineN, NUM_OF_LINE);
  int returnValue=0;
  if (line[vlineN].CALIBRATION <= 0)
  { //Se la linea non ha un coefficiente di taratura valido
    returnError(ERROR_NO_CALIB_VALUE);
    return 1;
  }
  //se si tratta di una linea del modulo slave, invio il comando via RS485
  if (line[vlineN].ID_PUMP > 3)
  {
    //Invio allo slave il comando di dosaggio
    char support[5];
    struct {
      uint8_t ID_PUMP;
      uint32_t QTY;
    } extern_dose;
    extern_dose.ID_PUMP = line[vlineN].ID_PUMP;
    extern_dose.QTY = (line[vlineN].CALIBRATION * qty);
    memcpy(support, &extern_dose, 5);
    //*********************************************************
    /* Wire.beginTransmission(SLAVE_A_ADDRESS); // transmit to device #4
      Wire.write(support);        // sends five bytes
      Wire.endTransmission();    // stop transmitting*/
    //**********************************************************
  }
  else
  {
    //Eseguo il dosaggio in loco
    returnValue = Dosing(line[vlineN].ID_PUMP, line[vlineN].CALIBRATION * qty);
  }
  return returnValue;
}
/**
   @brief


   @param param1
   @return
*/
int stopDose(uint8_t pumpN)
{ //Sto stoppando l'erogazione, ricavo quanto sino ad adesso ho erogato per toglierlo dal monte residuo di liquido, e resetto il contatore di erogazione che stopperà poi il processo di erogazione
  int8_t vpumpN = CheckParam(pumpN, NUM_OF_LINE);
  if (pumps[vpumpN].counter == 0)
    return 1;
  pumps[vpumpN].lastCounterValue = pumps[vpumpN].lastCounterValue - pumps[vpumpN].counter;
  pumps[vpumpN].counter = 0;
  return 0;
}
/**
   @brief


   @param param1
   @param param2
   @return
*/
int Dosing(uint8_t pumpN, float timeToDose)
{
  int8_t vpumpN = CheckParam(pumpN, NUM_OF_PUMP);
  //If pump is already in dosing time exit with exeption
  if(pumps[vpumpN].active==1)
    return 1;
  digitalWrite(LED, HIGH);
  pumps[vpumpN].active = 1;
  pumps[vpumpN].counter = timeToDose;
  pumps[vpumpN].lastCounterValue = timeToDose;
  uint32_t totime = 0;
  int i;
  for (i = 0; i < NUM_OF_PUMP; i++)
  {
    if (pumps[i].active)
      totime = totime + pumps[i].counter;
  }
}
/**
   @brief


   @param param1
   @return void
*/
void returnError(char typeError)
{
  StaticJsonBuffer<100> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root.set<int>("CMD", CMD_ERROR_RETURN);
  root.set<int>("error-type", typeError);
  root.printTo(Serial);
}
/**
   @brief


   @param param1
   @param param2
   @return
*/
int CheckParam(int data, uint32_t maxLevel)
{
  if (data < maxLevel)
  {
    return data;
  }
  else
  {
#if DEBUG == 1
    Serial2.println("OVERFLOW");
#endif
    return maxLevel - 1;
  }
}
/**
   @brief


*/
void eepromSaveAllData()
{
  byte STORAGE[256];
  uint32_t support = 0;
  int i = 0;
  for (i = 0; i < 255; i++)
    STORAGE[i] = 0;
  int addr = 0;
  for (i = 0; i < NUM_OF_LINE; i++) //0-4
  {
    addr = i * NUM_OF_LINE;//0-16
    addr = CheckParam(addr, 255);
    support = (uint32_t)line[i].LEVEL;
    STORAGE[addr] = (uint8_t)(support >> 24);
    STORAGE[addr + 1] = (uint8_t)(support >> 16);
    STORAGE[addr + 2] = (uint8_t)(support >> 8);
    STORAGE[addr + 3] = (uint8_t)support;
  }
  //16
  int indexOfArray = NUM_OF_LINE * 4;//16 mi calcolo il numero di byte(4) per ogni dato(NUM_OF_PUMP) usato per storare i dati precedenti
  for (i = 0; i < NUM_OF_LINE; i++) //16-32
  {
    addr = (i * NUM_OF_LINE) + indexOfArray;
    addr = CheckParam(addr, 255);
    support = (uint32_t)line[i].CAPACITY;
    STORAGE[addr] = (uint8_t)(support >> 24);
    STORAGE[addr + 1] = (uint8_t)(support >> 16);
    STORAGE[addr + 2] = (uint8_t)(support >> 8);
    STORAGE[addr + 3] = (uint8_t)support;
  }
  //32
  indexOfArray = NUM_OF_LINE * 8;//mi calcolo il numero di byte(8) per ogni dato(NUM_OF_PUMP) usato per storare i dati precedenti
  for (i = 0; i < NUM_OF_LINE; i++)
  {
    addr = i + indexOfArray;
    addr = CheckParam(addr, 255);
    STORAGE[addr] = line[i].ABILITATION;
  }
  //36
  indexOfArray = NUM_OF_LINE * 9;//mi calcolo il numero di byte(8) per ogni dato(NUM_OF_PUMP) usato per storare i dati precedenti
  for (i = 0; i < NUM_OF_LINE; i++)
  {
    addr = i + indexOfArray;
    addr = CheckParam(addr, 255);
    STORAGE[addr] = line[i].TIMER_ABILITATION ;
  }
  indexOfArray = NUM_OF_LINE * 10;//16 mi calcolo il numero di byte(4) per ogni dato(NUM_OF_PUMP) usato per storare i dati precedenti
  for (i = 0; i < NUM_OF_LINE; i++) //16-32
  {
    addr = (i * NUM_OF_PUMP) + indexOfArray;
    addr = CheckParam(addr, 255);
    support = (uint32_t)line[i].CALIBRATION;
    STORAGE[addr] = (uint8_t)(support >> 24);
    STORAGE[addr + 1] = (uint8_t)(support >> 16);
    STORAGE[addr + 2] = (uint8_t)(support >> 8);
    STORAGE[addr + 3] = (uint8_t)support;
  }
  //Salvo in flash

  for (i = 0; i < 255; i++)
  {
    EEPROM.write(i, STORAGE[CheckParam(i, 255)]);
  }
}
/**
   @brief

*/
void eepromReadScheData()
{
  uint8_t timerDoseRaw[1500];
  int i = 0;
  for (i = 0; i < 1500; i++)
    timerDoseRaw[i] = 0;
#if DEBUG == 1
  Serial2.println("Lettura schedulazione oraria da EEPROM");
#endif
  //Leggo i dati inerenti le schedulazioni orarie dalla EEPROM
  for (i = EEPROM_SCHEDULER_DATA_ADDRESS_L; i < EEPROM_SCHEDULER_DATA_ADDRESS_H; i++)
    timerDoseRaw[CheckParam(i - EEPROM_SCHEDULER_DATA_ADDRESS_L, EEPROM_SCHEDULER_DATA_ADDRESS_H)] = EEPROM.read(i);
  memcpy(&dose, &timerDoseRaw, SizeOfSchedulerFrame);
  for (i = 0; i < NUM_OF_SCHEDULER; i++)
  {
#if DEBUG == 1
    Serial2.print(dose[i].ID);
    Serial2.print(" ");
    Serial2.print(dose[i].ID_LINE);
    Serial2.print(" ");
    Serial2.print(dose[i].HOUR);
    Serial2.print(" ");
    Serial2.print(dose[i].MINUTE);
    Serial2.print(" ");
    Serial2.print(dose[i].QUANTITY);
    Serial2.print(" ");
    Serial2.print(dose[i].DAYOFWEEK);
    Serial2.print(" ");
    Serial2.println(dose[i].EXEC);
#endif
  }
}
/**
   @brief


*/
void eepromReadAllData()
{
  byte STORAGE[256];
  uint32_t support = 0;
  int i = 0;
  for (i = 0; i < 256; i++)
    STORAGE[i] = 0;
  //leggo da flash
  for (i = 0; i < 256; i++)
    STORAGE[i] = EEPROM.read(i);
  support = 0;
  int adrr = 0;
  for (i = 0; i < NUM_OF_LINE; i++)
  {
    adrr = i * NUM_OF_LINE;
    support = (uint8_t)STORAGE[adrr];
    support = support << 8;
    support = support ^ (uint8_t)STORAGE[adrr + 1];
    support = support << 8;
    support = support ^ (uint8_t)STORAGE[adrr + 2];
    support = support << 8;
    support = support ^ (uint8_t)STORAGE[adrr + 3];
    line[i].LEVEL = (float)support;
  }
  //16
  int indexOfArray = NUM_OF_LINE * 4;//mi calcolo il numero di byte(4) per ogni dato(NUM_OF_PUMP) usato per storare i dati precedenti
  for (i = 0; i < NUM_OF_LINE; i++)
  {
    adrr = (i * NUM_OF_LINE) + indexOfArray;
    support = (uint8_t)STORAGE[adrr];
    support = support << 8;
    support = support ^ (uint8_t)STORAGE[adrr + 1];
    support = support << 8;
    support = support ^ (uint8_t)STORAGE[adrr + 2];
    support = support << 8;
    support = support ^ (uint8_t)STORAGE[adrr + 3];
    line[i].CAPACITY = (float)support;
  }
  //32
  indexOfArray = NUM_OF_LINE * 8;
  for (i = 0; i < NUM_OF_LINE; i++)
  {
    adrr = i + indexOfArray;
    line[i].ABILITATION = STORAGE[adrr];
  }
  indexOfArray = NUM_OF_LINE * 9;
  for (i = 0; i < NUM_OF_LINE; i++)
  {
    adrr = i + indexOfArray;
    line[i].TIMER_ABILITATION = STORAGE[adrr];
  }
  indexOfArray = NUM_OF_LINE * 10;//mi calcolo il numero di byte(4) per ogni dato(NUM_OF_PUMP) usato per storare i dati precedenti
  for (i = 0; i < NUM_OF_LINE; i++)
  {
    adrr = (i * NUM_OF_LINE) + indexOfArray;
    support = (uint8_t)STORAGE[adrr];
    support = support << 8;
    support = support ^ (uint8_t)STORAGE[adrr + 1];
    support = support << 8;
    support = support ^ (uint8_t)STORAGE[adrr + 2];
    support = support << 8;
    support = support ^ (uint8_t)STORAGE[adrr + 3];
    line[i].CALIBRATION = (float)support;
  }
}

