#include <Arduino.h>
#include "weather.h"
#include "rfm.h"

#define SEALEVELPRESSURE_HPA (1013.25)

Weather::Weather()
{
  //rfm.Init(*dbgprinter);
}

void Weather::Init(PubSubClient &myclient, Stream &mydbgprn, const char mqtt_root_topic[])
{
  client = &myclient;
  dbgprinter = &mydbgprn;
  dbgprinter->println("Weather Init:");

  val_errorflags = 0;
  last_val_errorflags = 0xFFFFFFFFlu; //force first send
  
  snprintf(mqtt_pressure_topic,sizeof(mqtt_pressure_topic),"%s/%s",mqtt_root_topic,"preasure");
  snprintf(mqtt_humidity_in_topic,sizeof(mqtt_humidity_in_topic),"%s/%s",mqtt_root_topic, "humidity_in");
  snprintf(mqtt_humidity_out_topic,sizeof(mqtt_humidity_out_topic),"%s/%s",mqtt_root_topic, "humidity_out");
  snprintf(mqtt_temp_in_topic,sizeof(mqtt_temp_in_topic),"%s/%s",mqtt_root_topic, "temp_in");
  snprintf(mqtt_temp_out_topic,sizeof(mqtt_temp_out_topic),"%s/%s",mqtt_root_topic, "temp_out");
  snprintf(mqtt_wind_topic,sizeof(mqtt_wind_topic),"%s/%s",mqtt_root_topic, "wind");
  snprintf(mqtt_wind_avg_topic,sizeof(mqtt_wind_avg_topic),"%s/%s",mqtt_root_topic, "wind_avg");
  snprintf(mqtt_wind_gust_topic,sizeof(mqtt_wind_gust_topic),"%s/%s",mqtt_root_topic, "wind_gust");
  snprintf(mqtt_rain_total_topic,sizeof(mqtt_rain_total_topic),"%s/%s",mqtt_root_topic, "rain_total");
  snprintf(mqtt_rain_1h_topic,sizeof(mqtt_rain_1h_topic),"%s/%s",mqtt_root_topic, "rain_h");
  snprintf(mqtt_rain_24h_topic,sizeof(mqtt_rain_24h_topic),"%s/%s",mqtt_root_topic, "rain_24h");
  snprintf(mqtt_errorflags_topic,sizeof(mqtt_errorflags_topic),"%s/%s",mqtt_root_topic, "errorflags");


  unsigned status;
  
  // default settings
  status = bme.begin(0x77, &Wire);  
  // You can also pass in a Wire library object like &Wire2
  // status = bme.begin(0x76, &Wire2)
  if (!status) {
      Serial.println("Could not find a valid BME280 sensor, check wiring, address, sensor ID!");
      Serial.print("SensorID was: 0x"); Serial.println(bme.sensorID(),16);
      Serial.print("        ID of 0xFF probably means a bad address, a BMP 180 or BMP 085\n");
      Serial.print("   ID of 0x56-0x58 represents a BMP 280,\n");
      Serial.print("        ID of 0x60 represents a BME 280.\n");
      Serial.print("        ID of 0x61 represents a BME 680.\n");
      val_errorflags |= ERROR_BME280_NOT_FOUND; //Error BME280 not found
  }

  // weather monitoring
  Serial.println("-- Weather Station Scenario --");
  Serial.println("forced mode, 1x temperature / 1x humidity / 1x pressure oversampling,");
  Serial.println("filter off");
  bme.setSampling(Adafruit_BME280::MODE_FORCED,
                  Adafruit_BME280::SAMPLING_X1, // temperature
                  Adafruit_BME280::SAMPLING_X1, // pressure
                  Adafruit_BME280::SAMPLING_X1, // humidity
                  Adafruit_BME280::FILTER_OFF   );

}

int Weather::GetValues()
{
  bme.takeForcedMeasurement(); // has no effect in normal mode
  
  dbgprinter->println("Weather GetValues:");
  val_pressure = bme.readPressure() / 100.0F;
  val_humidity_in = bme.readHumidity();
  val_humidity_out = random(100);
  val_temp_in = bme.readTemperature();
  val_temp_out = random(100)/2.0F -20.0;
  val_wind = random(100)/2.0F;
  val_wind_avg = random(100)/2.0F;
  val_wind_gust =random(100)/2.0F;
  val_rain_total = val_rain_total +random(10)/4;
  val_rain_1h = random(60)/10.0F;
  val_rain_24h = random(90)/10.0F;

  return 0;
}

int Weather::PrintValues()
{
  dbgprinter->println("Weather PrintValues:");
  dbgprinter->print("val_pressure:"); dbgprinter->println(val_pressure);
  dbgprinter->print("val_humidity_in:"); dbgprinter->println(val_humidity_in);
  dbgprinter->print("val_humidity_out:"); dbgprinter->println(val_humidity_out);
  dbgprinter->print("val_temp_in:"); dbgprinter->println(val_temp_in);
  dbgprinter->print("val_temp_out:"); dbgprinter->println(val_temp_out);
  dbgprinter->print("val_wind:"); dbgprinter->println(val_wind);
  dbgprinter->print("val_wind_avg:"); dbgprinter->println(val_wind_avg);
  dbgprinter->print("val_wind_gust:"); dbgprinter->println(val_wind_gust);
  dbgprinter->print("val_rain_total:"); dbgprinter->println(val_rain_total);
  dbgprinter->print("val_rain_1h:"); dbgprinter->println(val_rain_1h);
  dbgprinter->print("val_rain_24h:"); dbgprinter->println(val_rain_24h);

  return 0;
}

int Weather::PublishValues(bool force)
{
  int cnt = 0;
  char tmp[20];

  dbgprinter->println("Weather PublishValues:");
  dbgprinter->print("force=");
  force?dbgprinter->println("true"):dbgprinter->println("false");

  if((val_pressure != last_val_pressure) || force){
    snprintf(tmp,sizeof(tmp),"%6.2f",val_pressure);  
    client->publish(mqtt_pressure_topic, tmp);
    last_val_pressure = val_pressure;
    cnt++;
  }

  if((val_humidity_in != last_val_humidity_in) || force){
    snprintf(tmp,sizeof(tmp),"%6.2f",val_humidity_in);  
    client->publish(mqtt_humidity_in_topic, tmp);
    last_val_humidity_in = val_humidity_in;
    cnt++;
  }
  if((val_humidity_out != last_val_humidity_out) || force){
    snprintf(tmp,sizeof(tmp),"%6.2f",val_humidity_out);  
    client->publish(mqtt_humidity_out_topic, tmp);
    last_val_humidity_out = val_humidity_out;
    cnt++;
  }
  if((val_temp_in != last_val_temp_in) || force){
    snprintf(tmp,sizeof(tmp),"%6.2f",val_temp_in);  
    client->publish(mqtt_temp_in_topic, tmp);
    last_val_temp_in = val_temp_in;
    cnt++;
  }
  if((val_temp_out != last_val_temp_out) || force){
    snprintf(tmp,sizeof(tmp),"%6.2f",val_temp_out);  
    client->publish(mqtt_temp_out_topic, tmp);
    last_val_temp_out = val_temp_out;
    cnt++;
  }
  if((val_wind != last_val_wind) || force){
    snprintf(tmp,sizeof(tmp),"%6.2f",val_wind);  
    client->publish(mqtt_wind_topic, tmp);
    last_val_wind = val_wind;
    cnt++;
  }
  if((val_wind_avg != last_val_wind_avg) || force){
    snprintf(tmp,sizeof(tmp),"%6.2f",val_wind_avg);  
    client->publish(mqtt_wind_avg_topic, tmp);
    last_val_wind_avg = val_wind_avg;
    cnt++;
  }
  if((val_wind_gust != last_val_wind_gust) || force){
    snprintf(tmp,sizeof(tmp),"%6.2f",val_wind_gust);  
    client->publish(mqtt_wind_gust_topic, tmp);
    last_val_wind_gust = val_wind_gust;
    cnt++;
  }
  if((val_rain_total != last_val_rain_total) || force){
    snprintf(tmp,sizeof(tmp),"%6.2f",val_rain_total);  
    client->publish(mqtt_rain_total_topic, tmp);
    last_val_rain_total = val_rain_total;
    cnt++;
  }
  if((val_rain_1h != last_val_rain_1h) || force){
    snprintf(tmp,sizeof(tmp),"%6.2f",val_rain_1h);  
    client->publish(mqtt_rain_1h_topic, tmp);
    last_val_rain_1h = val_rain_1h;
    cnt++;
  }
  if((val_rain_24h != last_val_rain_24h) || force){
    snprintf(tmp,sizeof(tmp),"%6.2f",val_rain_24h);  
    client->publish(mqtt_rain_24h_topic, tmp);
    last_val_rain_24h = val_rain_24h;
    cnt++;
  }
  if((val_errorflags != last_val_errorflags) || force){
    snprintf(tmp,sizeof(tmp),"%u",val_errorflags);  
    client->publish(mqtt_errorflags_topic, tmp);
    last_val_errorflags = val_errorflags;
    cnt++;
  }

  return cnt;
}

void Weather::loop()
{
  //rfm.loop();
}