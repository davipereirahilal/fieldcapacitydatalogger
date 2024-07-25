#include <Arduino.h>
#include "SdFat.h"
#include "RTClib.h"
#include "HX711.h"


// SD_FAT_TYPE = 0 for SdFat/File as defined in SdFatConfig.h,
// 1 for FAT16/FAT32, 2 for exFAT, 3 for FAT16/FAT32 and exFAT.
#define SD_FAT_TYPE 1

// SDCARD_SS_PIN is defined for the built-in SD on some boards.
#ifndef SDCARD_SS_PIN 
const uint8_t SD_CS_PIN = SS;
#else  // SDCARD_SS_PIN
// Assume built-in SD is used.
const uint8_t SD_CS_PIN = SDCARD_SS_PIN;
#endif  // SDCARD_SS_PIN

// Try max SPI clock for an SD. Reduce SPI_CLOCK if errors occur.
#define SPI_CLOCK SD_SCK_MHZ(50)

// Try to select the best SD card configuration.
#if HAS_SDIO_CLASS
#define SD_CONFIG SdioConfig(FIFO_SDIO)
#elif  ENABLE_DEDICATED_SPI
#define SD_CONFIG SdSpiConfig(SS, DEDICATED_SPI, SPI_CLOCK)
#else  // HAS_SDIO_CLASS
#define SD_CONFIG SdSpiConfig(SD_CS_PIN, SHARED_SPI, SPI_CLOCK)
#endif  // HAS_SDIO_CLASS

#if SD_FAT_TYPE == 0
SdFat SD;
File dataFile;
#elif SD_FAT_TYPE == 1
SdFat32 SD;
File32 dataFile;
#elif SD_FAT_TYPE == 2
SdExFat SD;
ExFile dataFile;
#elif SD_FAT_TYPE == 3
SdFs SD;
FsFile dataFile;
#else  // SD_FAT_TYPE
#error Invalid SD_FAT_TYPE
#endif  // SD_FAT_TYPE

//const int chipSelect = 15;

bool rtcc_adjust = true;

const char* fileName = "datalog.csv";                    // Nome do arquivo de texto do datalog
long long lastMeasument = 0;
boolean debug = true;
char datetime [25] = "";
char line[500];
float Weight;

//---------------------------------------------------------------------
// HX711 circuit wiring
const int LOADCELL_DOUT_PIN = 17; // Definir quando for utilizar
const int LOADCELL_SCK_PIN = 16; // Definir quando for utilizar

RTC_DS1307 rtc;
HX711 loadcell;

float pesagem();
void headers();
void rtcc_config();
void sd_card_init();
void rtcc_test();

void setup() {
  Serial.begin(9600); // porta USB or Xbee      
  if (debug) Serial.println("start");  
  // CARTÃO SD
  sd_card_init();
  
  // RTCC and SD start:
  rtc.begin();
  // gravar pela primeira vez com esta função descomentada, para ajustar data e hora a partir do Windows. Verificar dados gravados no cartão. Em seguida, regravar com a função comentada.
  if (rtcc_adjust) rtcc_config(); 
  if (debug) rtcc_test();
  //Inicializa o HX711
  loadcell.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);

  lastMeasument = millis();
}

void loop() {
  if(millis() - lastMeasument > 1000 * 60 * 15)//15 min
  {
    lastMeasument = millis();
    Weight = pesagem();

    // RTCC             
    DateTime now = rtc.now();             
    
    sd_card_init();
    
    headers();    

    sprintf(datetime, "%04d/%02d/%02d;%02d:%02d:%02d", now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second());
    String Weight_str = (String)Weight;
    sprintf(line, "%s;%02d; \n",datetime, Weight);                                                
                
    if (debug) Serial.print(line);

    if (!dataFile.open(fileName, FILE_WRITE)) 
    {
      if (debug) Serial.println(F("file.open failed"));
    return;
    }

    dataFile.print(line); 
                
    if (debug) Serial.println("SD card writed");  // PROBLEM! não grava o sensor 4, só grava se fizer leituras consecutivas              

    dataFile.close();   
  }
}

float pesagem() {
  return (9.378 * (loadcell.get_units(10))-343.35);
}

void headers() 
{  
   // se o arquivo não existe
  if (!SD.exists(fileName)) 
  {
    if (debug) Serial.println("O arquivo não existe. Criando...");
    // Cria o arquivo
    if (!dataFile.open(fileName, FILE_WRITE)) 
    {
      if (debug) Serial.println(F("file.open failed"));
      return;
    }

    dataFile.println("Data;Hora;Id;Bateria[V];Temp_placa[C];Temp_solo[C];Const_Diel;Condutiv[uS/cm];Umidade[m3/m3*100];Salinidade[uS/cm]");  // Escreva o índice no arquivo
    if (debug) Serial.println("Arquivo criado");      

    dataFile.close();  // Fecha o arquivo
  } 

}


void rtcc_config()
{
  bool debug = true;
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__))); // ajusta automaticamente o RTCC
  
  DateTime now = rtc.now(); 
  
  headers();     // obtêm a identificação da ultima medição e adiciona um.

  sprintf(datetime, "%04d/%02d/%02d;%02d:%02d:%02d;", now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second());
  Serial.print(datetime);

  String Weight_str = (String)Weight;
  sprintf(line, "%s;%s \n",datetime, Weight_str);
            
  sd_card_init();
    
  if (debug) Serial.print(line);

  if (!dataFile.open(fileName, FILE_WRITE)) 
  {
    if (debug) Serial.println(F("file.open failed"));
  return;
  }

  dataFile.print(line);

  dataFile.close();
}

void rtcc_test()
{  
  DateTime now = rtc.now(); 
 
  sprintf(datetime, "%04d/%02d/%02d;%02d:%02d:%02d;", now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second());
  Serial.print(datetime);
}
void sd_card_init()
{
  if (debug) Serial.println("SD card initialized");
  // see if the card is present and can be initialized:
  if (!SD.begin(SD_CONFIG)) 
  {
    if (debug) SD.initErrorHalt(&Serial);
    // don't do anything more:  
  }
}