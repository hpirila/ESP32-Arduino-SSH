#include <WiFi.h>
#include "ssh.hpp"

const unsigned int configSTACK = 40960;
TaskHandle_t sshHandle = NULL;

void sshTask(void* pvParameter) {
  SSH ssh{};

  // Pick one of the ssh authentication methods to connect
  Serial.println("SSH Connecting to server...");

  // With password (in server side create password for the user and allow
  // password authentication in /etc/ssh/sshd_config)
  ssh.connectWithPassword("ec2-111-112-113-114.ap-southeast-1.compute.amazonaws.com", "ubuntu","System#1");

  // With public key
  // ssh.connectWithKey("101.102.103.104", "ubuntu","/spiffs/key1.pub","/spiffs/key1");
  // With public key, encrypted private key
  // ssh.connectWithKey("192.168.1.200", "hpirila","/spiffs/key2.pub","/spiffs/key2","MyPassPhrase");

  if (ssh.isConnected) {
    Serial.println("SSH is connected!\n");

    Serial.println("Lets create a test file in server");
    ssh.sendCommand("echo \"This is a test file for ESP32 Arduino SSH wrapper class\" > testFile1");

    Serial.println("Copying testFile1 from server to ESP32 spiffs file system");
    ssh.scpGetFile("testFile1", "/spiffs/testFile1");

    Serial.println("Copying testFile1 from ESP32 back to server with new name testFile2");
    ssh.scpPutFile("/spiffs/testFile1", "testFile2");

    Serial.println("Compare testFile1 and testFile2 and print the result to result.txt\n");
    Serial.println("Login to server and cat result.txt");
    Serial.println("It should say testFile1 and testFile2 are identical.\n");
    ssh.sendCommand("diff -s testFile1 testFile2 > result.txt");

    // ssh.scpGetFile("key1", "/spiffs/key1");
    // ssh.scpGetFile("key1.pub", "/spiffs/key1.pub");
    // ssh.scpGetFile("key2", "/spiffs/key2");
    // ssh.scpGetFile("key2.pub", "/spiffs/key2.pub");

  } else {
    Serial.println("SSH connection failed.");
  }

  Serial.println("Close ssh connection");
  ssh.end();
  Serial.println("Kill ssh task");
  vTaskDelete(NULL);
}

void setup(void) {
  Serial.begin(115200);

  WiFi.begin("ssid", "Wifi_password");

  Serial.println("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(100);
  }
  Serial.println("\nConnected to the WiFi network");
  Serial.print("Local ESP32 IP: ");
  Serial.println(WiFi.localIP());

  xTaskCreatePinnedToCore(sshTask, "ctl", configSTACK, NULL,
                          (tskIDLE_PRIORITY + 3), &sshHandle,
                          portNUM_PROCESSORS - 1);
}

void loop(void) {
  delay(1);
}
