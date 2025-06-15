#include <WiFi.h>
extern "C" {
  #include "esp_wpa2.h"
}

#include <HTTPClient.h>
#include <ArduinoJson.h>

#include "FS.h"
#include "LittleFS.h"

#include "esp_flash.h"

// hotspot
// const char* ssid = "Letdon的iPhone";
// const char* password = "doubibax";

// const char* serverName = "http://172.20.10.2:3000/esp32";  //MacBook's IP (will change with wifi)

//school wifi
const char* ssid = "eduroam";
const char* username = ""; // full login
const char* password = "";


const char* mp3URL = "https://firebasestorage.googleapis.com/v0/b/portfolio-73553.appspot.com/o/hello-46355.mp3?alt=media&token=76ac75dc-3a16-4cf5-941b-44fa12b6e49f";
const char* savePath = "/hello.mp3";

void listFiles(fs::FS &fs, const char * dirname, uint8_t levels) {
  Serial.printf("Listing directory: %s\n", dirname);

  File root = fs.open(dirname);
  if (!root) {
    Serial.println("Failed to open directory");
    return;
  }
  if (!root.isDirectory()) {
    Serial.println("Not a directory");
    return;
  }

  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      Serial.printf("📁 [DIR]  %s\n", file.name());
      if (levels > 0) {
        listFiles(fs, file.name(), levels - 1);
      }
    } else {
      Serial.printf("📄 [FILE] %s  [%d bytes]\n", file.name(), file.size());
    }
    file = root.openNextFile();
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000); //avoid to be too fast to generate the message and cause baud rate is wrong,
    if (!LittleFS.begin(true)) {
    Serial.println("Failed to mount LittleFS");
    return;
  }


  Serial.printf("Total space: %d bytes\n", LittleFS.totalBytes());
Serial.printf("Used space:  %d bytes\n", LittleFS.usedBytes());


  WiFi.disconnect(true);  // clear previous
  WiFi.mode(WIFI_STA);
  esp_wifi_sta_wpa2_ent_set_identity((uint8_t *)username, strlen(username));
  esp_wifi_sta_wpa2_ent_set_username((uint8_t *)username, strlen(username));
  esp_wifi_sta_wpa2_ent_set_password((uint8_t *)password, strlen(password));
  esp_wifi_sta_wpa2_ent_enable();

  WiFi.begin(ssid);
  Serial.println("Connecting to eduroam...");

  int tries = 0;
  while (WiFi.status() != WL_CONNECTED && tries < 20) {
    delay(1000);
    Serial.print(".");
    tries++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnected!");
    Serial.println("IP Address: " + WiFi.localIP().toString());
  } else {
    Serial.println("\nFailed to connect.");
  }


  // remote connect to download
  HTTPClient http;
  http.begin(mp3URL);
  int httpCode = http.GET();

  if (httpCode == 200) {
    File file = LittleFS.open(savePath, FILE_WRITE);
    if (!file) {
      Serial.println("Failed to open file for writing");
      return;
    }

    WiFiClient* stream = http.getStreamPtr();
    uint8_t buffer[128];
    int len = http.getSize();

    while (http.connected() && len > 0) {
      int bytesRead = stream->readBytes(buffer, sizeof(buffer));
      file.write(buffer, bytesRead);
      len -= bytesRead;
    }

    file.close();
    Serial.println("Download complete and saved to LittleFS!");
  } else {
    Serial.printf("HTTP GET failed, code: %d\n", httpCode);
  }

  http.end();



listFiles(LittleFS, "/", 2);


  // if (!LittleFS.begin(true)) {
  //   Serial.println("Failed to mount LittleFS");
  //   return;
  // }
  // Serial.println("LittleFS mounted");


  // // 写入一个文件
  // File file = LittleFS.open("/test.txt", "w");
  // if (!file) {
  //   Serial.println("Failed to open file for writing");
  //   return;
  // }
  // file.println("This file was written from code!");
  // file.close();
  // Serial.println("File written");

  // // 读取它验证内容
  // file = LittleFS.open("/test.txt", "r");
  // Serial.println("File content:");
  // while (file.available()) Serial.write(file.read());
  // file.close();

  // listFiles(LittleFS, "/", 2);

  // Serial.begin(115200);
  // WiFi.begin(ssid, password);

  // while (WiFi.status() != WL_CONNECTED) {
  //   delay(500);
  //   Serial.print(".");
  // }
  // Serial.println("\nConnected to WiFi");
}

void loop() {
  // if (WiFi.status() == WL_CONNECTED) {
  //   HTTPClient http;
  //   http.begin(serverName);
  //   http.addHeader("Content-Type", "application/json");

  //   StaticJsonDocument<200> jsonDoc;
  //   jsonDoc["message"] = "Hi";

  //   String requestBody;
  //   serializeJson(jsonDoc, requestBody);

  //   int httpResponseCode = http.POST(requestBody);

  //   if (httpResponseCode > 0) {
  //     String response = http.getString();
  //     Serial.print("Response: ");
  //     Serial.println(response);
  //   } else {
  //     Serial.print("Error code: ");
  //     Serial.println(httpResponseCode);
  //   }

  //   http.end();
  // }

  // File file = LittleFS.open("/test.txt", "r");
  // if (!file) {
  //   Serial.println("Failed to open file for reading");
  //   return;
  // }

  // Serial.println("File content:");
  // while (file.available()) {
  //   Serial.write(file.read());
  // }
  // file.close();
  // delay(5000);
}
