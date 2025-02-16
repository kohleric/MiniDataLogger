// Bibliothèques pour la carte SD
#include "FS.h"
#include "SD.h"
#include "SPI.h"

// Bibliothèques pour les capteurs
#include "DFRobot_GNSS.h"
#include <Wire.h>
#include "Adafruit_SGP30.h"
#include <Adafruit_AHT10.h>
#include "SparkFunBME280.h"

// Définition des objets capteurs
DFRobot_GNSS_I2C gnss(&Wire , GNSS_DEVICE_ADDR);
Adafruit_SGP30 sgp;
Adafruit_AHT10 aht;
BME280 capteur;

File dataFile;      // Fichier de données
String fileName;    // Nom du fichier à utiliser

// 📌 Fonction pour générer un nom de fichier unique
String getNewFileName() {
    int fileNumber = 1;
    String newFileName;
    
    do {
        newFileName = "/data_" + String(fileNumber) + ".txt";
        fileNumber++;
    } while (SD.exists(newFileName.c_str())); // Vérifie si le fichier existe déjà
    
    return newFileName;
}

void setup() {
    Serial.begin(115200);

    // 📌 Initialisation de la carte SD
    if (!SD.begin()) {
        Serial.println("Erreur : Carte SD non détectée !");
        return;
    }
    Serial.println("Carte SD détectée avec succès !");

    // 📌 Création d'un fichier unique au démarrage
    fileName = getNewFileName();
    Serial.println("Création du fichier : " + fileName);
    dataFile = SD.open(fileName.c_str(), FILE_WRITE);
    if (dataFile) {
        dataFile.println("Temps (ms), Température (°C), Humidité (%), Pression (hPa), TVOC (ppb), eCO2 (ppm), Latitude, Longitude");
        dataFile.close();
        Serial.println("Fichier créé avec succès !");
    } else {
        Serial.println("Erreur d'ouverture du fichier !");
    }

    // 📌 Initialisation des capteurs
    Serial.println("Initialisation des capteurs...");
    
    // GPS
    while (!gnss.begin()) {
        Serial.println("GPS non détecté !");
        delay(1000);
    }
    gnss.enablePower();
    gnss.setGnss(eGPS_BeiDou_GLONASS);
    gnss.setRgbOn();

    // SGP30 (Qualité de l'air)
    if (!sgp.begin()) {
        Serial.println("Erreur : SGP30 non détecté !");
    }

    // AHT10 (Température & Humidité)
    if (!aht.begin()) {
        Serial.println("Erreur : AHT10 non détecté !");
    }

    // BME280 (Température, Humidité, Pression)
    Wire.begin(21, 22);
    capteur.settings.commInterface = I2C_MODE; 
    capteur.settings.I2CAddress = 0x76;
    capteur.settings.runMode = 3; 
    capteur.settings.tempOverSample = 1;
    capteur.settings.pressOverSample = 1;
    capteur.settings.humidOverSample = 1;
    capteur.begin();

    Serial.println("Capteurs initialisés !");
}

void loop() {
    unsigned long timeStamp = millis();

    // 📌 Récupération des données des capteurs

    // GPS
    sLonLat_t lat = gnss.getLat();
    sLonLat_t lon = gnss.getLon();

    // SGP30
    if (!sgp.IAQmeasure()) {
        Serial.println("Erreur mesure SGP30 !");
        return;
    }
    int tvoc = sgp.TVOC;
    int eco2 = sgp.eCO2;

    // AHT10 (Température et Humidité)
    sensors_event_t humidity, temp;
    aht.getEvent(&humidity, &temp);
    float temperature = temp.temperature;
    float humidite = humidity.relative_humidity;

    // BME280 (Pression)
    float pression = capteur.readFloatPressure() / 100.0; // Conversion en hPa

    // 📌 Affichage Serial Monitor
    Serial.print("Temps : "); Serial.print(timeStamp); Serial.print(" ms, ");
    Serial.print("Température : "); Serial.print(temperature); Serial.print(" °C, ");
    Serial.print("Humidité : "); Serial.print(humidite); Serial.print(" %, ");
    Serial.print("Pression : "); Serial.print(pression); Serial.print(" hPa, ");
    Serial.print("TVOC : "); Serial.print(tvoc); Serial.print(" ppb, ");
    Serial.print("eCO2 : "); Serial.print(eco2); Serial.print(" ppm, ");
    Serial.print("Lat : "); Serial.print(lat.latitudeDegree, 6); Serial.print(", ");
    Serial.print("Lon : "); Serial.println(lon.lonitudeDegree, 6);

    // 📌 Enregistrement des données sur la carte SD
    dataFile = SD.open(fileName.c_str(), FILE_APPEND);
    if (dataFile) {
        dataFile.print(timeStamp);
        dataFile.print(", ");
        dataFile.print(temperature);
        dataFile.print(", ");
        dataFile.print(humidite);
        dataFile.print(", ");
        dataFile.print(pression);
        dataFile.print(", ");
        dataFile.print(tvoc);
        dataFile.print(", ");
        dataFile.print(eco2);
        dataFile.print(", ");
        dataFile.print(lat.latitudeDegree, 6);
        dataFile.print(", ");
        dataFile.println(lon.lonitudeDegree, 6);
        dataFile.close();
        Serial.println("✅ Données enregistrées avec succès !");
    } else {
        Serial.println("❌ Erreur d'écriture sur la carte SD !");
    }

    delay(100); // Attente 1 seconde avant la prochaine mesure
}
