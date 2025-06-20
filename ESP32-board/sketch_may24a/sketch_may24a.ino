#include <WiFi.h>
extern "C" {
  #include "esp_wpa2.h"
}

#include <HTTPClient.h>
#include <ArduinoJson.h>

#include "FS.h"
#include "LittleFS.h"

#include "esp_flash.h"


// board 
#include <driver/i2s.h>


#define I2S_WS      5  // Word Select
#define I2S_SCK     4  // Serial Clock
#define I2S_SD      41  // Data from INMP441
bool listening = true;
int16_t previousSample = 0; 

bool recording = false;
File recordingFile;
const char* recordFilePath = "/record.raw";

// hotspot
const char* ssid = "Letdon的iPhone";
const char* password = "";
// hotspot 
// const char* serverName = "http://172.20.10.2:3000/esp32";  //MacBook's IP (will change with wifi)

//school wifi
// const char* ssid = "eduroam";

// const char* username = "; // full login
// const char* password = "!";



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

void uploadFile() {
  File file = LittleFS.open(recordFilePath, "r");
  if (!file) {
    Serial.println("Failed to open file for reading.");
    return;
  }

  HTTPClient http;
  http.begin("http://172.20.10.2:3000/upload-raw");   // 修改为你的服务器地址
  http.addHeader("Content-Type", "application/octet-stream");

  // 发送 POST 请求
  int code = http.sendRequest("POST", &file, file.size());
  http.end();   // 释放 HTTP 资源
  file.close(); // 关闭文件句柄

  // 根据返回码打印结果
  if (code == 200) {
    Serial.println("Upload succeeded.");
  } else {
    Serial.printf("Upload failed. HTTP code: %d\n", code);
  }

  // 无论成功或失败都删除本地文件
  if (LittleFS.remove(recordFilePath)) {
    Serial.println("Local file deleted.");
  } else {
    Serial.println("Failed to delete local file.");
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000); //avoid to be too fast to generate the message and cause baud rate is wrong,
  Serial.println("Starting INMP441 with I2S...");
  if (!LittleFS.begin(true)) {
  Serial.println("Mount LittleFS failed!");
  return;
}


  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = 16000,  // Can also try 44100
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,  // INMP441 is mono
    .communication_format = I2S_COMM_FORMAT_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 8,
    .dma_buf_len = 1024,
    .use_apll = false,
    .tx_desc_auto_clear = false,
    .fixed_mclk = 0
  };

  // 2. Define pin mapping
  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_SCK,
    .ws_io_num = I2S_WS,
    .data_out_num = I2S_PIN_NO_CHANGE,  // We only receive
    .data_in_num = I2S_SD
  };

  // 3. Install and start I2S
  i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_NUM_0, &pin_config);



  //   if (!LittleFS.begin(true)) {
  //   Serial.println("Failed to mount LittleFS");
  //   return;
  // }



  WiFi.disconnect(true);  // clear previous
  WiFi.mode(WIFI_STA);
  // esp_wifi_sta_wpa2_ent_set_identity((uint8_t *)username, strlen(username));
  // esp_wifi_sta_wpa2_ent_set_username((uint8_t *)username, strlen(username));
  // esp_wifi_sta_wpa2_ent_set_password((uint8_t *)password, strlen(password));
  // esp_wifi_sta_wpa2_ent_enable();
  

  // WiFi.begin(ssid);
  // Serial.println("Connecting to eduroam...");

  WiFi.begin(ssid, password); 
  Serial.println("Connecting to hotspot...");


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


//   // remote connect to download
//   HTTPClient http;
//   http.begin(mp3URL);
//   int httpCode = http.GET();

//   if (httpCode == 200) {
//     File file = LittleFS.open(savePath, FILE_WRITE);
//     if (!file) {
//       Serial.println("Failed to open file for writing");
//       return;
//     }

//     WiFiClient* stream = http.getStreamPtr();
//     uint8_t buffer[128];
//     int len = http.getSize();

//     while (http.connected() && len > 0) {
//       int bytesRead = stream->readBytes(buffer, sizeof(buffer));
//       file.write(buffer, bytesRead);
//       len -= bytesRead;
//     }

//     file.close();
//     Serial.println("Download complete and saved to LittleFS!");
//   } else {
//     Serial.printf("HTTP GET failed, code: %d\n", httpCode);
//   }

//   http.end();



// listFiles(LittleFS, "/", 2);


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
static size_t totalBytes = 0;  // 记录录音总大小（可选限制）
  const size_t maxFileSize = 100000; // 最大录音大小：100KB（可自定义）

  // 接收串口命令：s 开始录音，q 停止并上传
  if (Serial.available()) {
    char c = Serial.read();

    if (c == 's') {
      // 开始录音
      if (recording) {
        Serial.println("Already recording.");
        return;
      }

      recordingFile = LittleFS.open(recordFilePath, FILE_WRITE);
      if (!recordingFile) {
        Serial.println("Failed to open file for writing.");
        return;
      }

      recording = true;
      totalBytes = 0;
      Serial.println("Start recording...");
    }

    if (c == 'q') {
      // 停止录音并上传
      if (recording) {
        recording = false;
        recordingFile.close();
        Serial.println("Recording stopped.");

        if (WiFi.status() == WL_CONNECTED) {
          uploadFile();
        } else {
          Serial.println("WiFi not connected. File not uploaded.");
        }
      } else {
        Serial.println("Not currently recording.");
      }
    }
  }

  // 如果处于录音状态，则持续采集音频数据
  if (recording) {
    uint8_t buffer[512];
    size_t bytesRead;
    i2s_read(I2S_NUM_0, buffer, sizeof(buffer), &bytesRead, portMAX_DELAY);

    recordingFile.write(buffer, bytesRead);
    totalBytes += bytesRead;

    // 防止文件太大（自动停止）
    if (totalBytes > maxFileSize) {
      Serial.println("Max file size reached, auto stopping.");
      recording = false;
      recordingFile.close();

      if (WiFi.status() == WL_CONNECTED) {
        uploadFile();
      } else {
        Serial.println("WiFi not connected. File not uploaded.");
      }
    }
  }

  delay(10);
}
