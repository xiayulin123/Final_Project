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


#define I2S_WS 5   // Word Select
#define I2S_SCK 4  // Serial Clock
#define I2S_SD 41  // Data from INMP441


#define I2S_DOUT 42   // 对应 MAX98357A 的 DIN
#define I2S_BCLK 37    // 对应 MAX98357A 的 BCLK/SCK
#define I2S_LRC 35     // 对应 MAX98357A 的 LRC/WS

bool listening = true;
int16_t previousSample = 0;

bool recording = false;
File recordingFile;
const char* recordFilePath = "/record.raw";

// hotspot
const char* ssid = "BELL785";
const char* password = "";
// hotspot
// const char* serverName = "http://172.20.10.2:3000/esp32";  //MacBook's IP (will change with wifi)

//school wifi
// const char* ssid = "eduroam";

// const char* username = "; // full login
// const char* password = "!";



// const char* mp3URL = "https://firebasestorage.googleapis.com/v0/b/portfolio-73553.appspot.com/o/hello-46355.mp3?alt=media&token=76ac75dc-3a16-4cf5-941b-44fa12b6e49f";
// const char* savePath = "/hello.mp3";

const char* rawURL = "https://firebasestorage.googleapis.com/v0/b/portfolio-73553.appspot.com/o/hello.raw?alt=media&token=ae483453-b15e-4819-834a-806d4e8f1688";
const char* savePath = "/hello.raw";


void listFiles(fs::FS& fs, const char* dirname, uint8_t levels) {
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
  http.begin("http://172.20.10.2:3000/upload-raw");  // 修改为你的服务器地址
  http.addHeader("Content-Type", "application/octet-stream");

  // 发送 POST 请求
  int code = http.sendRequest("POST", &file, file.size());
  http.end();    // 释放 HTTP 资源
  file.close();  // 关闭文件句柄

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


void initI2SForPlayback() {
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = 16000,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 8,
    .dma_buf_len = 1024,
    .use_apll = false,
    .tx_desc_auto_clear = true,
    .fixed_mclk = 0
  };

  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_BCLK,
    .ws_io_num = I2S_LRC,
    .data_out_num = I2S_DOUT,
    .data_in_num = I2S_PIN_NO_CHANGE
  };

  i2s_driver_install(I2S_NUM_1, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_NUM_1, &pin_config);
}


void initFileSystem() {
  if (!LittleFS.begin(true)) {
    Serial.println("Mount LittleFS failed!");
    return;
  }
  Serial.println("LittleFS mounted successfully.");
  listFiles(LittleFS, "/", 2);
}

void downloadAudioFromServer() {
  if ((WiFi.status() == WL_CONNECTED)) {
    HTTPClient http;
    // http.begin(localServerURL);
    int httpCode = http.GET();

    if (httpCode == 200) {
      File file = LittleFS.open("/hello.raw", FILE_WRITE);
      if (!file) {
        Serial.println("无法打开文件进行写入");
        return;
      }

      WiFiClient* stream = http.getStreamPtr();
      uint8_t buffer[128];
      int len = http.getSize();
      Serial.println("正在下载音频文件...");

      while (http.connected() && len > 0) {
        int bytesRead = stream->readBytes(buffer, sizeof(buffer));
        if (bytesRead <= 0) break;
        file.write(buffer, bytesRead);
        len -= bytesRead;
      }

      file.close();
      Serial.println("下载完成并保存到 LittleFS");

      // 自动播放
      playAudioFile("/hello.raw");

    } else {
      Serial.printf("HTTP GET 失败，状态码：%d\n", httpCode);
    }

    http.end();
  } else {
    Serial.println("WiFi 尚未连接，无法下载");
  }
}


void playAudioFile(const char* filePath) {
  File file = LittleFS.open(filePath, "r");
  if (!file) {
    Serial.println("无法打开音频文件");
    return;
  }

  const size_t bufferSize = 512;
  uint8_t buffer[bufferSize];
  size_t bytesRead;

  Serial.println("开始播放音频...");
  // while (file.available()) {
  //   bytesRead = file.read(buffer, bufferSize);
  //   size_t bytesWritten;
  //   i2s_write(I2S_NUM_1, buffer, bytesRead, &bytesWritten, portMAX_DELAY);
  // }
  while (file.available()) {
    bytesRead = file.read(buffer, bufferSize);
    size_t bytesWritten;
    esp_err_t err = i2s_write(I2S_NUM_1, buffer, bytesRead, &bytesWritten, portMAX_DELAY);
    Serial.printf("Wrote %d bytes to I2S (err=%d)\n", bytesWritten, err);
  }

  file.close();
  Serial.println("播放完成");
}

void connectToWiFi() {
   // ***********
  // WIFI CONNECTIONS
  // ***********

  WiFi.disconnect(true);  // clear previous
  WiFi.mode(WIFI_STA);

  // EDUROAM
  // esp_wifi_sta_wpa2_ent_set_identity((uint8_t *)username, strlen(username));
  // esp_wifi_sta_wpa2_ent_set_username((uint8_t *)username, strlen(username));
  // esp_wifi_sta_wpa2_ent_set_password((uint8_t *)password, strlen(password));
  // esp_wifi_sta_wpa2_ent_enable();

  // WiFi.begin(ssid);
  // Serial.println("Connecting to eduroam...");

  WiFi.begin(ssid, password);
  Serial.println("Connecting to wifi...");

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
}

void handleSerialCommands() {
  static size_t totalBytes = 0;
  const size_t maxFileSize = 100000;

  if (Serial.available()) {
    char c = Serial.read();
    if (c == 's' && !recording) {
      recordingFile = LittleFS.open(recordFilePath, FILE_WRITE);
      if (!recordingFile) {
        Serial.println("Failed to open file for writing.");
        return;
      }
      recording = true;
      totalBytes = 0;
      Serial.println("Start recording...");
    }

    if (c == 'q' && recording) {
      recording = false;
      recordingFile.close();
      Serial.println("Recording stopped.");
      if (WiFi.status() == WL_CONNECTED) {
        uploadFile();
      } else {
        Serial.println("WiFi not connected. File not uploaded.");
      }
    }
  }

  if (recording) {
    uint8_t buffer[512];
    size_t bytesRead;
    i2s_read(I2S_NUM_0, buffer, sizeof(buffer), &bytesRead, portMAX_DELAY);
    recordingFile.write(buffer, bytesRead);
    totalBytes += bytesRead;

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
}

void setup() {
  Serial.begin(115200);
  delay(1000);  //avoid to be too fast to generate the message and cause baud rate is wrong,
  Serial.println("Starting INMP441 with I2S...");
  
  initFileSystem();

  initI2SForPlayback();

  // connectToWiFi();


}


const int sampleRate = 16000;
const float freq = 1000;
const int samples = 16000;
int16_t sineWave[samples];

void generateSineWave() {
  for (int i = 0; i < samples; i++) {
    sineWave[i] = 3000 * sinf(2 * PI * freq * i / sampleRate);
  }
}

void loop() {
  //handleSerialCommands();
  playAudioFile("/hello.raw");
  // size_t bytes_written;
  // i2s_write(I2S_NUM_1, sineWave, sizeof(sineWave), &bytes_written, portMAX_DELAY);
  // Serial.printf("Wrote %d samples\n", bytes_written / 2);
  delay(5000);
  // delay(10);
}
