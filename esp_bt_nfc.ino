
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#include <MFRC522.h>
#include <SPI.h> //biblioteca para comunicação do barramento SPI

#define SS_PIN 21
#define RST_PIN 22
// Definicoes pino modulo RC522
MFRC522 mfrc522(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};


#define SERVICE_UUID           "ab0828b1-198e-4351-b779-901fa0e0371e" // UART service UUID
#define CHARACTERISTIC_UUID_RX "4ac8a682-9736-4e5d-932b-e9b31405049c"
#define CHARACTERISTIC_UUID_TX "0972EF8C-7613-4075-AD52-756F33D4DA91"

BLECharacteristic *characteristicTX; //através desse objeto iremos enviar dados para o client
bool deviceConnected = false; //controle de dispositivo conectado
bool readCardData = false;




//callback para eventos das características
class CharacteristicCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *characteristic) {
      //retorna ponteiro para o registrador contendo o valor atual da caracteristica
      std::string rxValue = characteristic->getValue();
      //verifica se existe dados (tamanho maior que zero)
      if (rxValue.length() > 0) {

        for (int i = 0; i < rxValue.length(); i++) {
          Serial.print(rxValue[i]);
        }
        Serial.println();
        //L1 liga o LED | L0 desliga o LED
        if (rxValue.find("L1") != -1) {
          digitalWrite(LED_BUILTIN, HIGH);
        } else if (rxValue.find("L0") != -1) {
          digitalWrite(LED_BUILTIN, LOW);
        } else if (rxValue.find("PL") != -1) {
          for (int i = 0; i < 10; i++) {
            int statusLed = digitalRead(LED_BUILTIN) ? LOW : HIGH;
            digitalWrite(LED_BUILTIN, statusLed);
            delay(500);
          }
          digitalWrite(LED_BUILTIN, LOW);
        } else if (rxValue.find("READ") != -1) {
          readCardData = true;
        }


      }
    }//onWrite
};

//callback para receber os eventos de conexão de dispositivos
class ServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};


void setup() {


  // Inicia a serial
  Serial.begin(9600);
  SPI.begin(); // Init SPI bus

  // Inicia MFRC522
  mfrc522.PCD_Init();

  pinMode(LED_BUILTIN, OUTPUT);


  // Create the BLE Device
  BLEDevice::init("NFCReader"); // nome do dispositivo bluetooth

  // Create the BLE Server
  BLEServer *server = BLEDevice::createServer(); //cria um BLE server

  server->setCallbacks(new ServerCallbacks()); //seta o callback do server

  // Create the BLE Service
  BLEService *service = server->createService(SERVICE_UUID);

  // Create a BLE Characteristic para envio de dados
  characteristicTX = service->createCharacteristic(CHARACTERISTIC_UUID_TX, BLECharacteristic::PROPERTY_NOTIFY);

  characteristicTX->addDescriptor(new BLE2902());
  // Create a BLE Characteristic para recebimento de dados
  BLECharacteristic *characteristic = service->createCharacteristic(CHARACTERISTIC_UUID_RX, BLECharacteristic::PROPERTY_WRITE);

  characteristic->setCallbacks(new CharacteristicCallbacks());

  // Start the service
  service->start();

  // Start advertising (descoberta do ESP32)
  server->getAdvertising()->start();

  // Mensagens iniciais no serial monitor
  Serial.println("Se conecte ao dispositivo e aproxime o seu cartao do leitor...");
  Serial.println();

}

void loop() {



  // Aguarda a aproximacao do cartao
  if ( ! mfrc522.PICC_IsNewCardPresent())
  {
    return;
  }
  // Seleciona um dos cartoes
  if ( ! mfrc522.PICC_ReadCardSerial())
  {
    return;
  }

  digitalWrite(LED_BUILTIN, HIGH);

  // UID
  Serial.print(F("Card UID:"));
  std::string idCartaoHex = lerUidBytes(mfrc522.uid);

  Serial.print(idCartaoHex.c_str());
  Serial.println();

  //se existe algum dispositivo conectado

  if (deviceConnected) {
    characteristicTX->setValue(idCartaoHex); //seta o valor que a caracteristica notificará (enviar)
    characteristicTX->notify(); // Envia o valor para o smartphone
  }

  if (readCardData) {
    mfrc522.PICC_DumpToSerial(&(mfrc522.uid));
    /*
    // In this sample we use the second sector,
    // that is: sector #1, covering block #4 up to and including block #7
    byte sector         = 1;
    byte blockAddr      = 4;
    byte dataBlock[]    = {
      0x01, 0x02, 0x03, 0x04, //  1,  2,   3,  4,
      0x05, 0x06, 0x07, 0x08, //  5,  6,   7,  8,
      0x09, 0x0a, 0xff, 0x0b, //  9, 10, 255, 11,
      0x0c, 0x0d, 0x0e, 0x0f  // 12, 13, 14, 15
    };
    byte trailerBlock   = 7;
    MFRC522::StatusCode status;
    byte buffer[18];
    byte size = sizeof(buffer);

    // Authenticate using key A
    Serial.println(F("Authenticating using key A..."));
    status = (MFRC522::StatusCode) mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid));
    if (status != MFRC522::STATUS_OK) {
      Serial.print(F("PCD_Authenticate() failed: "));
      Serial.println(mfrc522.GetStatusCodeName(status));
      return;
    }

    // Show the whole sector as it currently is
    Serial.println(F("Current data in sector:"));
    mfrc522.PICC_DumpMifareClassicSectorToSerial(&(mfrc522.uid), &key, sector);
    Serial.println();
    */
  }





  delay(1000);
  digitalWrite(LED_BUILTIN, LOW);


}

std::string lerUidBytes(MFRC522::Uid uid) {

  char asciiString[uid.size * 2 + 1];

  for (byte i = 0; i < uid.size; i++) {
    sprintf(&asciiString[2 * i], "%02X", uid.uidByte[i]);
  }

  return (std::string) asciiString;

}
