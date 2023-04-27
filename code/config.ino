void saveConfigCallback () {
  Serial.println(F("Should save config"));
  shouldSaveConfig = true;
}

void initSPIFFS(){
  Serial.println("mounting FS...");
SPIFFS.format();
  if (SPIFFS.begin()) {
    Serial.println("mounted file system");
    if (SPIFFS.exists("/config_blynk.json")) {
      //file exists, reading and loading
      Serial.println("reading config file");
      File configFile = SPIFFS.open("/config_blynk.json", "r");
      if (configFile) {
        Serial.println("opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);
        configFile.readBytes(buf.get(), size);
        DynamicJsonDocument json(1024);
         auto deserializeError = deserializeJson(json, buf.get());
        serializeJson(json, Serial);
        if (! deserializeError) {
          Serial.println("\nparsed json");
          strcpy(blynk_token, json["blynk_token"]);
          strcpy(blynk_host, json["blynk_host"]);
          strcpy(blynk_port, json["blynk_port"]);
          strcpy(g_listrik, json["g_listrik"]);
        } else {
          Serial.println("failed to load json config");
        }
      }
    }

    
  } else {
    Serial.println("failed to mount FS");
  }
 }


void save_pengaturan(int a) {
    Serial.println("saving config");
    DynamicJsonDocument json(1024);
    json["blynk_token"] = blynk_token;
    json["blynk_host"]   = blynk_host;
    json["blynk_port"]   = blynk_port;
    json["g_listrik "]   = g_listrik;
    File configFile = SPIFFS.open("/config_blynk.json", "w");
    if (!configFile) 
    {
      Serial.println("file creation failed");
    } else {
      Serial.println("File Created!");
      serializeJson(json,configFile);
      configFile.close();
    //end save
      shouldSaveConfig = false;
    }
    configFile.close();
    
}

void initialize(){
  // put your setup code here, to run once:
  //SPIFFS.format();
  Serial.println("intialized");
  initSPIFFS();
  WiFiManagerParameter custom_blynk_token("blynk", "blynk token", blynk_token, 34);
  WiFiManagerParameter custom_blynk_host("Host", "blynk Host", blynk_host, 20);
  WiFiManagerParameter custom_blynk_port("Port", "blynk Port", blynk_port,7);
  WiFiManagerParameter custom_golongan("Tarif", "Tarif Listrik", g_listrik,15);
  WiFiManager wifiManager;
  wifiManager.setSaveConfigCallback(saveConfigCallback);
  wifiManager.addParameter(&custom_blynk_token);
  wifiManager.addParameter(&custom_blynk_host);
  wifiManager.addParameter(&custom_blynk_port);
  wifiManager.addParameter(&custom_golongan);
  if (!wifiManager.autoConnect("Air Quality", "12345678")) {
    digitalWrite(led_p, HIGH);
    Serial.println("failed to connect and hit timeout");
    delay(3000);
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(3000);
  }

  Serial.println("connected...yeey :)");
  strcpy(blynk_token, custom_blynk_token.getValue());
  strcpy(blynk_host, custom_blynk_host.getValue());
  strcpy(g_listrik, custom_golongan.getValue());
  strcpy(blynk_port, custom_blynk_port.getValue());
  portblynk = atoi(blynk_port);
  //save the custom parameters to FS
  if (shouldSaveConfig) {
    save_pengaturan(simpan_wifi);
  }
    inisial_ota();
}



void inisial_ota() {
 ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_FS
      type = "filesystem";
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
}
