#include <hd44780.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <hd44780.h>
#include <hd44780ioClass/hd44780_HC1627_I2C.h>
#include <SD.h>
#define SS_PIN 10
#define RST_PIN 9
#define SD_CS 3
#define LCD_COLUMNS 16
#define LCD_ROWS 2
struct TessereColorate {
  String codice;
  String colore;
};
MFRC522 mfrc522(SS_PIN, RST_PIN); // istanza della classe
hd44780_HC1627_I2C lcd(0x27);//init. display lcd 16= caratteri per riga, 2= righe
MFRC522::MIFARE_Key key;
// array per la memorizzazione dell'ID RF
byte nuidPICC[4];
bool masterReset = true;
int numbers[4];
int avanzamento = 0;
TessereColorate* tc;
int timer = 620; //tempo massimo per risoluzione obiettivo (10 min + 20 di reset);
int ls = 0;


void setup() {
  //-----------------------------------------------------------------INIZIALIZZAZIONI-------------------------------------------------------//
  Serial.begin(9600); //!!solo debug!!
  SPI.begin(); // Init SPI bus  
  mfrc522.PCD_Init(); // Init MFRC522
  //-----------------------------------------------------------INIZIALIZZAZIONE E TEST SCHEDA SD--------------------------------------------//
  if (SD.begin(SD_CS) == 0)
  {
    Serial.println("Errore scheda SD");
    while (true);
  }
  else
  {
    Serial.println("Scheda SD OK");
    delay(3000);
  }
  //---------------------------------------------------APERTURA E CARICAMENTO FILE DA SD-----------------------------------------------------//
  File file;
  file = SD.open("tessereColori.txt", FILE_READ);
  if (!file) {
    Serial.println("Errore: impossibile aprire il file.");
    lcd.print("File error");
    delay(10000);
  }
  String line;
  while (file.available()) {
    line = file.readStringUntil('\n');
    tc[ls].codice = line;
    line = file.readStringUntil('\n');
    tc[ls].colore = line;
    ls++;
  }
  Serial.println("Caricati codici e colori totale:" + ls + 1);
  delay(1000);
  file.close();
  //--------------------------------------------INIZIALIZZAZIONE E TEST DISPLAY--------------------------------------------------//
  int status = lcd.begin(LCD_COLUMNS, LCD_ROWS); //init LCD Display
  if (status) // non zero status means it was unsuccesful
  {
    // begin() failed so blink error code using the onboard LED if possible
    Serial.println("LCD NON OK");
    hd44780::fatalError(status); // does not return
  }
  Serial.println("LCD OK");
  lcd.backlight();
  delay(3000);
}

void loop() {
  //------------------------------------------------GENERAZIONE DI TRE NUMERI RANDOM-------------------------------------------------------//
  if (masterReset) {
    // Genera un array di 4 numeri casuali unici da 1 a 4
   /* for (int i = 0; i < 3; i++) {
      int randomNumber;
      do {
        randomNumber = random(1, 4);
        bool alreadyExists = false;
        for (int j = 0; j < i; j++) {
          if (numbers[j] == randomNumber) {
            alreadyExists = true;
            break;
          }
        }
        if (!alreadyExists) {
          break;
        }
      } while (true);
      numbers[i] = randomNumber;
    }*/
    randomSeed(analogRead(millis())); // inizializzazione Seed per generazione numeri casuali
    for (int i = 0; i < 4; i++) {
    numbers[i] = random(1, 5); // Genera un numero casuale compreso tra 1 e 4
    delay(500);
  }
    masterReset = false;
  }
//-----------------------------------------------SE NON VIENE RILEVATA ALCUNA TESSERA------------------------------------------------------------------//
if ( ! mfrc522.PICC_IsNewCardPresent()) {
  lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Tempo: " + timer);
    lcd.setCursor(1, 0);
    lcd.print("Tessera" + tc[numbers[avanzamento]].colore);
    timer -= 1;
    delay(1000);
    if (timer <= 0) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Tempo Scaduto!");
      lcd.print("Bomba Esplosa!");
      //far suonare cicalino o qualcosa che fa rumore------------------------------------------completare---------------------------------------------
  }
  return;
}
//--------------------------------------------SE VIENE RILEVATA UNA TESSERA---------------------------------------------------------//
  if (mfrc522.uid.uidByte[0] != nuidPICC[0] ||
      mfrc522.uid.uidByte[1] != nuidPICC[1] ||
      mfrc522.uid.uidByte[2] != nuidPICC[2] ||
      mfrc522.uid.uidByte[3] != nuidPICC[3] )
    //Serial.println(F("A new card has been detected."));

    // Store NUID into nuidPICC array
    for (byte i = 0; i < 4; i++) {
      nuidPICC[i] = mfrc522.uid.uidByte[i];
    }

  String decData = byteToDecimal(mfrc522.uid.uidByte, mfrc522.uid.size);
  lcd.clear();//!!solo debug!!
  lcd.setCursor(0, 0); //!!solo debug!!
  lcd.print(decData); //!!solo debug!!
  delay(30000);//!!solo debug!!

  // Halt PICC
  mfrc522.PICC_HaltA();

  // Stop encryption on PCD
  mfrc522.PCD_StopCrypto1();
//----------------------------------------------------VERIFICA SE LA TESSERA E' MASTER------------------------------------------//
if (decData == tc[0].codice) {
      timer = 620;
      masterReset = true;
      return;
    }
//--------------------------------------------------VERIFICA TESSERA CORRETTA------------------------------------------------//
if (decData == tc[numbers[avanzamento]].codice) {
      avanzamento += 1;
      if (avanzamento >= 4) {
        lcd.setCursor(0, 0);
        lcd.print("Bomba disinnescata!");
        lcd.setCursor(1, 0);
        lcd.print("Tempo:" + (620 - timer) / 60);
        delay(3000);
      }
      masterReset = false;
      return;
    }
    else {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Sequenza Errata");
      timer-=30;
      return;
    }
}

//-----------------------------------------------FUNZIONE PER CONVERSIONE DI UN HEX A DECIMALE--------------------------------//
String byteToDecimal(byte *buffer, byte bufferSize) {
  String code = "";
  for (byte i = 0; i < bufferSize; i++) {
    code += buffer[i];
  }
  return code;
}