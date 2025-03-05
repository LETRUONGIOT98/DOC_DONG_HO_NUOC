#include <esp_camera.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <base64.h>
#include <ArduinoJson.h>
#include <WebServer.h>
#include "SD_MMC.h"  // Thư viện hỗ trợ thẻ nhớ SD
// Cấu hình camera
#define CAMERA_MODEL_AI_THINKER
#include "camera_pins.h"

// Thay đổi với thông tin mạng của bạn và API
const char* ssid = "Le Truong";
const char* password = "01061998";
const char* apiKey = "K85134829288957";
const char* serverName = "https://api.ocr.space/parse/image";
WebServer server(80);

void startCameraServer() {
  // Đường dẫn để truy cập video stream
  server.on("/stream", HTTP_GET, [](){
    WiFiClient client = server.client();

    // Header của HTTP để truyền video MJPEG
    String response = "HTTP/1.1 200 OK\r\n";
    response += "Content-Type: multipart/x-mixed-replace; boundary=frame\r\n\r\n";
    server.sendContent(response);

    while (true) {
      camera_fb_t * fb = esp_camera_fb_get(); // Lấy frame từ camera
      if (!fb) {
        Serial.println("Camera capture failed");
        return;
      }

      // Header của mỗi frame
      response = "--frame\r\n";
      response += "Content-Type: image/jpeg\r\n\r\n";
      server.sendContent(response);

      // Gửi dữ liệu frame
      server.client().write(fb->buf, fb->len);
      server.sendContent("\r\n");

      esp_camera_fb_return(fb); // Trả lại bộ nhớ sau khi gửi
      delay(100); // Điều chỉnh độ trễ để đảm bảo livestream mượt mà
    }
  });

  server.begin(); // Bắt đầu server
}

void setupWiFi() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected to WiFi");
}

// Chuyển đổi ảnh sang base64
String encodeImageToBase64(camera_fb_t *fb) {
  String base64Image = base64::encode(fb->buf, fb->len);
    // Thêm tiền tố
    String base64Data = "data:image/jpeg;base64," + base64Image;

  return base64Data; // Thay đổi định dạng thành PNG

}
// Gửi yêu cầu OCR
String ocrRequest(const String& imageBase64) {
  HTTPClient http;
  String payload = "apikey=" + String(apiKey) + "&base64Image=" + imageBase64;
  payload += "&isTable=True&isOverlayRequired=True"; // Thêm các tham số này

  Serial.println("Payload: " + payload); // In payload để kiểm tra

  http.begin(serverName);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  int httpResponseCode = http.POST(payload);
  Serial.print("HTTP Response Code: ");
  Serial.println(httpResponseCode);

  String response = "";
  if (httpResponseCode > 0) {
    response = http.getString();
    Serial.println("OCR API response: " + response); // In ra phản hồi
  } else {
    Serial.println("Error on HTTP request");
  }

  http.end();
  return response;
}






// Xử lý và làm sạch văn bản từ OCR
void cleanText(String& text) {
  text.replace(" m 3", " m3");
  text.replace(" / h", "/h");
  text.replace("-", ".");
  text.trim();
}

// Phân tách số từ văn bản
void parseNumbers(String& text, String& string1, String& string2) {
  int firstSpaceIndex = text.indexOf(' ');
  if (firstSpaceIndex != -1) {
    string1 = text.substring(0, firstSpaceIndex);
    string2 = text.substring(firstSpaceIndex + 1);
  } else {
    string1 = text;
    string2 = "";
  }
}

void setup() {
  Serial.begin(115200);
  /////////////////HÀM SET KHỞI ĐỘNG CAMERA////////////
  pinMode(0, INPUT_PULLUP);
  initSDCard();
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  // if PSRAM IC present, init with UXGA resolution and higher JPEG quality
  //                      for larger pre-allocated frame buffer.
   
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  
    /*config.frame_size = FRAMESIZE_QVGA;
    config.frame_size = FRAMESIZE_UXGA; // Độ phân giải cao (1600x1200)
    config.jpeg_quality = 20; // Giảm chất lượng nhưng vẫn giữ độ phân giải*/
  //config.pixel_format = PIXFORMAT_RGB888; // Sử dụng RGB888 cho định dạng PNG

  // BẮT ĐẦU KHỞI ĐỘNG CAMERA///
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  sensor_t * s = esp_camera_sensor_get();
  // initial sensors are flipped vertically and colors are a bit saturated
 
  // drop down frame size for higher initial frame rate
  s->set_framesize(s, FRAMESIZE_VGA);
  setupWiFi();
  //startCameraServer();
  //startCameraServer();
  Serial.print("Camera Ready! Use 'http://");
  Serial.print(WiFi.localIP());  //Hiển thị IP
  phantich();
}

void loop() {
  //server.handleClient(); 
 // Serial.print("Heap available: ");
//Serial.println(ESP.getFreeHeap());
}

void initSDCard() {
  if (!SD_MMC.begin("/sdcard", true)) {
    Serial.println("Không thể khởi tạo thẻ nhớ SD");
    return;
  }
  Serial.println("Thẻ nhớ SD đã được khởi tạo thành công");
}

void saveImageToSD(camera_fb_t *fb) {
  // Tạo tên file duy nhất cho mỗi ảnh
  static int fileIndex = 0;
  String path = "/image_" + String(fileIndex++) + ".jpg";
  
  // Mở file trên thẻ nhớ để ghi
  File file = SD_MMC.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("Không thể mở file để ghi trên thẻ nhớ SD");
    return;
  }

  // Ghi dữ liệu ảnh vào file
  file.write(fb->buf, fb->len);
  file.close();
  Serial.println("Đã lưu ảnh vào thẻ nhớ SD với tên file: " + path);
}

void phantich() {
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Không thể chụp ảnh từ camera");
    return;
  }

  // Lưu ảnh vào thẻ nhớ SD
  saveImageToSD(fb);

  // Tiếp tục xử lý OCR như bình thường
  String imageBase = encodeImageToBase64(fb);
  Serial.println(imageBase);
  String ocrResult = ocrRequest(imageBase); // Chỉ truyền một tham số

  // In ra payload để kiểm tra
  Serial.println("OCR API response: " + ocrResult);
  
  // Xử lý JSON kết quả
  DynamicJsonDocument doc(2048);
  deserializeJson(doc, ocrResult);
  if (doc.containsKey("ParsedResults") && doc["ParsedResults"].size() > 0) {
    String parsedResult = doc["ParsedResults"][0]["ParsedText"].as<String>();
    Serial.println("Original Parsed Result:");
    Serial.println(parsedResult);
   
    cleanText(parsedResult);

    String string1, string2;
    parseNumbers(parsedResult, string1, string2);

    Serial.println("Cleaned Result:");
    Serial.println("String1: " + string1);
    Serial.println("String2: " + string2);
  } else {
    Serial.println("Không có kết quả nào được tìm thấy");
  }

  esp_camera_fb_return(fb);
}
