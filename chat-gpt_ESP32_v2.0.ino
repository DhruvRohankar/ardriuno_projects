/*
 * ESP32 ChatGPT Voice Assistant
 * * Hardware:
 * - ESP32
 * - I2S Microphone (e.g., INMP441)
 * - I2S Amplifier (e.g., MAX98357A)
 * - Push-to-Wake/Talk Button
 * * Logic:
 * 1. Deep Sleep.
 * 2. Button press wakes the ESP32.
 * 3. Play a "start" tone.
 * 4. Record audio while the button is held.
 * 5. When released, play a "stop" tone.
 * 6. Send audio to Whisper API (STT).
 * 7. Get text, send it to ChatGPT API.
 * 8. Get text response, send it to TTS API.
 * 9. Get audio stream, play it on the speaker.
 * 10. Go back to deep sleep.
 */

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "driver/i2s.h"

// --- 1. CONFIGURATION ---

// -- Wi-Fi Credentials
const char* WIFI_SSID = "AndriodShare_J3";
const char* WIFI_PASSWORD = "72990022";

// -- OpenAI API Key
const char* OPENAI_API_KEY = "YOUR_OPENAI_API_KEY"; // Keep this SECRET

// -- I2S Pin Configuration
#define I2S_WS_PIN    25  // Word Select (LRC)
#define I2S_SCK_PIN   26  // Serial Clock (BCLK)
#define I2S_SD_IN_PIN 33  // Serial Data In (from Mic)
#define I2S_SD_OUT_PIN 22 // Serial Data Out (to Speaker)
#define I2S_PORT      I2S_NUM_0

// -- Wake Button Pin
#define WAKE_BUTTON_PIN GPIO_NUM_32

// -- Audio Recording Settings
#define SAMPLE_RATE 16000
#define BITS_PER_SAMPLE I2S_BITS_PER_SAMPLE_16BIT
#define RECORDING_DURATION_SEC 5 // Max recording time
// Calculate buffer size: 5 seconds of 16-bit audio at 16kHz
const int RECORDING_BUFFER_SIZE = SAMPLE_RATE * (16 / 8) * RECORDING_DURATION_SEC;
byte* recordingBuffer; // Will be allocated in setup

// -- API Endpoints
const char* WHISPER_API_URL = "https://api.openai.com/v1/audio/transcriptions"; // REVERTED back to transcriptions
const char* CHATGPT_API_URL = "https://api.openai.com/v1/chat/completions";
const char* TTS_API_URL = "https://api.openai.com/v1/audio/speech";

// Global HTTP client
HTTPClient http;
WiFiClient client;

// --- 2. DEEP SLEEP & WAKEUP ---

void goToSleep() {
  Serial.println("Going to sleep now.");
  Serial.flush();
  
  // Configure the button as the wakeup source
  esp_sleep_enable_ext0_wakeup(WAKE_BUTTON_PIN, 0); // Wake up when pin is LOW (button pressed)
  
  // Enter deep sleep
  esp_deep_sleep_start();
}

void printWakeupReason() {
  esp_sleep_wakeup_cause_t wakeup_reason;
  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch(wakeup_reason) {
    case ESP_SLEEP_WAKEUP_EXT0: 
      Serial.println("Wakeup caused by external signal using RTC_IO"); 
      break;
    default: 
      Serial.printf("Wakeup was not caused by deep sleep: %d\n", wakeup_reason); 
      break;
  }
}

// --- 3. I2S & AUDIO FUNCTIONS ---

void setupI2S() {
  // -- I2S Common Config
  i2s_config_t i2sConfig = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_RX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = BITS_PER_SAMPLE,
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 8,
    .dma_buf_len = 64,
    .use_apll = false,
    .tx_desc_auto_clear = true
  };

  // -- I2S Pin Config
  i2s_pin_config_t i2sPins = {
    .bck_io_num = I2S_SCK_PIN,
    .ws_io_num = I2S_WS_PIN,
    .data_out_num = I2S_SD_OUT_PIN,
    .data_in_num = I2S_SD_IN_PIN
  };

  // Install and start I2S driver
  i2s_driver_install(I2S_PORT, &i2sConfig, 0, NULL);
  i2s_set_pin(I2S_PORT, &i2sPins);
  i2s_set_clk(I2S_PORT, SAMPLE_RATE, BITS_PER_SAMPLE, I2S_CHANNEL_MONO);
  i2s_zero_dma_buffer(I2S_PORT);
}

// Simple function to play a "beep" to indicate start/stop
void playBeep(int freq, int duration) {
  // This is a placeholder. A real beep requires generating
  // a sine wave and sending it to the I2S port.
  // For now, we'll just log it.
  Serial.printf("BEEP (Freq: %d, Dur: %d)\n", freq, duration);
  // Example of playing silence (clearing buffer)
  i2s_zero_dma_buffer(I2S_PORT);
}

// Records audio from I2S mic while the button is held down
// Returns the total number of bytes read
int recordAudio() {
  int bytesRead = 0;
  size_t bytes_read_from_i2s;

  Serial.println("Recording... Press and hold button.");
  playBeep(440, 100); // Start beep

  // Wait for button press (this is a simple implementation)
  // A better way is to use interrupts, but this works.
  while(digitalRead(WAKE_BUTTON_PIN) == HIGH) {
    delay(10);
  }

  Serial.println("Button pressed! Recording audio...");
  
  // Read from I2S mic while button is held down (LOW)
  while (digitalRead(WAKE_BUTTON_PIN) == LOW && bytesRead < RECORDING_BUFFER_SIZE) {
    i2s_read(I2S_PORT, (void*)(recordingBuffer + bytesRead),
             1024, &bytes_read_from_i2s, portMAX_DELAY);
    
    if (bytes_read_from_i2s > 0) {
      bytesRead += bytes_read_from_i2s;
    }
    // Small delay to prevent watchdog timer reset
    vTaskDelay(pdMS_TO_TICKS(1)); 
  }

  Serial.printf("Recording stopped. Total bytes read: %d\n", bytesRead);
  playBeep(880, 100); // Stop beep
  return bytesRead;
}

// Plays audio stream coming from an HTTPClient
void playAudioStream(WiFiClient* stream) {
  uint8_t buffer[1024];
  size_t bytesWritten;
  int bytesRead = 0;

  Serial.println("Playing audio stream...");

  while (stream->connected() || stream->available()) {
    if (stream->available()) {
      int len = stream->read(buffer, sizeof(buffer));
      if (len > 0) {
        bytesRead += len;
        // Write the audio data directly to the I2S speaker
        i2s_write(I2S_PORT, buffer, len, &bytesWritten, portMAX_DELAY);
      }
    }
    vTaskDelay(pdMS_TO_TICKS(1)); // Give other tasks time
  }
  Serial.printf("Audio playback finished. Total bytes: %d\n", bytesRead);
  i2s_zero_dma_buffer(I2S_PORT); // Clear buffer
}

// --- 4. API CALL FUNCTIONS ---

// Sends recorded audio to Whisper and gets text back
// NOTE: We use the "translations" endpoint. This auto-detects
// any language (including Hindi, Tamil, etc.) and translates it to English.
// This is the most robust way, as our TTS can only *speak* English well.
String sendToWhisper(int audioDataLength) {
  Serial.println("Sending audio to Whisper API for translation...");
  int headerSize = 44;
  byte wavHeader[headerSize];
  // This is a complex but necessary step.
  // You must create a valid WAV header for your raw PCM audio.
  // ... (Code to create WAV header) ...
// For brevity, this part is simplified.
// You will need to find a function to "createWavHeader"

// --- 4. API CALL FUNCTIONS ---

// Sends recorded audio to Whisper and gets text back
// NOTE: We use the "transcriptions" endpoint. This auto-detects
// the language and returns the text in that language.
// We will pass references to store both the text and the detected language.
bool sendToWhisper(int audioDataLength, String& transcribedText, String& detectedLanguage)
 {
  Serial.println("Sending audio to Whisper API for transcription...");
  
  // --- This is a placeholder ---
  // A real implementation is much more complex.
  // Search for "ESP32 HTTPClient multipart/form-data"
  
  /*
  http.begin(client, WHISPER_API_URL); // This now points to /transcriptions
  http.addHeader("Authorization", "Bearer " + String(OPENAI_API_KEY));
  http.addHeader("Content-Type", "multipart/form-data; boundary=myboundary");

  String bodyPrefix = "--myboundary\r\n"
                      "Content-Disposition: form-data; name=\"model\"\r\n\r\n"
                      "whisper-1\r\n"
                      "--myboundary\r\n"
                      "Content-Disposition: form-data; name=\"file\"; filename=\"audio.wav\"\r\n"
                      "Content-Type: audio/wav\r\n\r\n";
  String bodySuffix = "\r\n--myboundary--\r\n";

  int contentLength = bodyPrefix.length() + headerSize + audioDataLength + bodySuffix.length();

  http.POST( ... ); // This part is very tricky!
  */
  
  // --- End Placeholder ---
  
  // For this skeleton, we'll return a dummy string (as if user spoke in Hindi)
  transcribedText = "पेरिस में मौसम कैसा है?";
  detectedLanguage = "hi"; // "hi" is the ISO 639-1 code for Hindi
  Serial.println("Whisper (dummy) transcription: " + transcribedText);
  Serial.println("Whisper (dummy) language: " + detectedLanguage);
  return true;
  
  // --- Real implementation would look like this ---
  /*
  int httpCode = http.POST( ... ); // Send the complex request
  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    Serial.println("Whisper Response: " + payload);
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, payload);

    // CRITICAL: Get both text AND language
    transcribedText = doc["text"].as<String>();
    detectedLanguage = doc["language"].as<String>();
    
    http.end();
    return true; // Success
  } else {
    Serial.printf("Whisper API error: %d\n", httpCode);
    http.end();
    return false; // Failure
  }
  */
}

// Sends text to ChatGPT and gets text back
String sendToChatGPT(String prompt) {
  Serial.println("Sending prompt to ChatGPT API...");
  
  // Create JSON payload
  DynamicJsonDocument doc(1024);
  doc["model"] = "gpt-3.5-turbo"; // or "gpt-4o-mini"
  JsonArray messages = doc.createNestedArray("messages");

  // --- UPDATED SYSTEM PROMPT ---
  // This tells ChatGPT to reply in the *same language* it was
  // spoken to in.
  JsonObject systemMessage = messages.createNestedObject();
  systemMessage["role"] = "system";
  systemMessage["content"] = "You are a helpful voice assistant. You must respond concisely in the same language as the user's prompt.";
  // --- END UPDATED ---

  JsonObject userMessage = messages.createNestedObject();
  userMessage["role"] = "user";
  userMessage["content"] = prompt;
  
  String jsonPayload;
  serializeJson(doc, jsonPayload);

  // Send request
  http.begin(client, CHATGPT_API_URL);
  http.addHeader("Authorization", "Bearer " + String(OPENAI_API_KEY));
  http.addHeader("Content-Type", "application/json");

  int httpCode = http.POST(jsonPayload);

  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    Serial.println("ChatGPT Response: " + payload);
    
    // Parse the response
    deserializeJson(doc, payload);
    String gptResponse = doc["choices"][0]["message"]["content"];
    http.end();
    return gptResponse;
  } else {
    Serial.printf("ChatGPT API error: %d\n", httpCode);
    String payload = http.getString(); // Get error message
    Serial.println(payload);
    http.end();
    return "I'm sorry, I encountered an error.";
  }
}

// Sends text to TTS and plays the audio stream
void sendToTTSAndPlay(String text) {
  Serial.println("Sending text to TTS API...");

  // Create JSON payload
  DynamicJsonDocument doc(512);
  doc["model"] = "tts-1"; // or tts-1-hd
  doc["input"] = text;
  doc["voice"] = "alloy"; // Other voices: echo, fable, onyx, nova, shimmer
  
  String jsonPayload;
  serializeJson(doc, jsonPayload);

  // Send request
  http.begin(client, TTS_API_URL);
  http.addHeader("Authorization", "Bearer " + String(OPENAI_API_KEY));
  http.addHeader("Content-Type", "application/json");

  int httpCode = http.POST(jsonPayload);

  if (httpCode == HTTP_CODE_OK) {
    Serial.println("Got audio stream from TTS...");
    // The response is a binary audio stream.
    // We get the WiFiClient stream and pass it to the player.
    WiFiClient* stream = http.getStreamPtr();
    playAudioStream(stream);

  } else {
    Serial.printf("TTS API error: %d\n", httpCode);
    String payload = http.getString(); // Get error message
    Serial.println(payload);
  }
  
  http.end();
}

// --- 5. MAIN SETUP & LOOP ---

void setup() {
  Serial.begin(115200);
  while (!Serial);
  Serial.println("Booting up...");

  // Check why we woke up
  printWakeupReason();

  // Configure the button pin
  // Use internal pull-up, so button connects Pin to GND
  pinMode(WAKE_BUTTON_PIN, INPUT_PULLUP);

  // Allocate memory for recording
  recordingBuffer = (byte*)malloc(RECORDING_BUFFER_SIZE);
  if (!recordingBuffer) {
    Serial.println("Failed to allocate recording buffer!");
    return;
  }
  Serial.println("Recording buffer allocated.");

  // Connect to Wi-Fi
  Serial.printf("Connecting to %s ", WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" Connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Setup I2S
  setupI2S();
  Serial.println("I2S drivers installed.");

  // --- Main Logic ---
  // This runs once after waking up.
  
  // 1. Record Audio
  int audioLen = recordAudio();
  if (audioLen == 0) {
    Serial.println("No audio recorded.");
    goToSleep();
    return;
  }

  // 2. Send to Whisper (STT)
  // We need to store both the text and the language
  String userPrompt = "";
  String userLanguage = "";

  bool whisperSuccess = sendToWhisper(audioLen, userPrompt, userLanguage);
  
  if (!whisperSuccess || userPrompt.length() == 0) {
    Serial.println("Whisper API failed.");
    goToSleep();
    return;
  }
  Serial.println("You said: " + userPrompt);
  Serial.println("Detected language: " + userLanguage);

  // 3. Send to ChatGPT
  // We send the prompt in the user's language (e.g., Hindi)
  String gptResponse = sendToChatGPT(userPrompt);
  Serial.println("ChatGPT says: " + gptResponse);

  // 4. Send to TTS and Play
  // The TTS model is multilingual. If gptResponse is in Hindi,
  // it will speak in Hindi.
  sendToTTSAndPlay(gptResponse);

  // 5. Go back to sleep
  goToSleep();
}

void loop() {
  // The loop is empty because the ESP32
  // spends all its time in deep sleep or
  // running the 'setup()' function after a wakeup.
  vTaskDelay(1000);
}