
#ifndef WEATHER_H
#define WEATHER_H
#include <Arduino.h>
#include <Wire.h>

#include "PubSubClient.h"
#include "Adafruit_BME280.h"
#include "rfm.h"


#define ERROR_BME280_NOT_FOUND 0x00000001lu
class Weather
{
    public:
    Weather();
    void Init(PubSubClient &myclient, Stream &mydbgprn, const char mqtt_root_topic[]);
    int GetValues();
    int PrintValues();
    int PublishValues(bool force);
    void loop();

    // values
    float val_pressure;
    float val_humidity_in;
    float val_humidity_out;
    float val_temp_in;
    float val_temp_out;
    float val_wind;
    float val_wind_avg;
    float val_wind_gust;
    float val_rain_total;
    float val_rain_1h;
    float val_rain_24h;
    uint32_t val_errorflags;

    //previous values
    float last_val_pressure = 1;
    float last_val_humidity_in;
    float last_val_humidity_out;
    float last_val_temp_in;
    float last_val_temp_out;
    float last_val_wind;
    float last_val_wind_avg;
    float last_val_wind_gust;
    float last_val_rain_total;
    float last_val_rain_1h;
    float last_val_rain_24h;
    uint32_t last_val_errorflags;

    //Topics
    char mqtt_pressure_topic[44];
    char mqtt_humidity_in_topic[44];
    char mqtt_humidity_out_topic[44];
    char mqtt_temp_in_topic[44];
    char mqtt_temp_out_topic[44];
    char mqtt_wind_topic[44];
    char mqtt_wind_avg_topic[44];
    char mqtt_wind_gust_topic[44];
    char mqtt_rain_total_topic[44];
    char mqtt_rain_1h_topic[44];
    char mqtt_rain_24h_topic[44];
    char mqtt_errorflags_topic[44];

    private:
    PubSubClient* client;
    Stream* dbgprinter;

    Adafruit_BME280 bme;
    RFM rfm;
};

#endif //WEATHER_H
