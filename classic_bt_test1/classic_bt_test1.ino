#include "BluetoothSerial.h" //Header File for Serial Bluetooth, will be added by default into Arduino

const int ledPin = 23; // Pin of ESP32
int incoming; // Bluetooth Message
BluetoothSerial ESP_BT; //Object for Bluetooth

void setup() {
  pinMode(ledPin, OUTPUT); // set pin modes
  Serial.begin(115200); // Start Serial monitor 
  ESP_BT.begin("ESP32"); // Name of Bluetooth Signal
  Serial.println("Bluetooth Device is Ready to Pair");
}

void loop() {
  
  if (ESP_BT.available()) // Check if we receive anything from Bluetooth
  {

    digitalWrite(ledPin,HIGH);
    
    incoming = ESP_BT.read(); // Read what we recevive 
    Serial.print("Received:"); Serial.println(incoming);
    if (incoming == 119)
      digitalWrite(ledPin, HIGH);
    if (incoming == 115)
      digitalWrite(ledPin, HIGH);
    if (incoming == 97)
      digitalWrite(ledPin, HIGH); 
    if (incoming == 100)
      digitalWrite(ledPin, LOW); 
    if (incoming == 32)
      digitalWrite(ledPin, HIGH);

    delay(500);
    digitalWrite(ledPin,LOW);
      
  }
  delay(20);
}
