#include "iic_driver.h"

#include <configuration.h>

#include "Adafruit_AHTX0.h"
#include <Wire.h>

static Adafruit_AHTX0 AHT10;

void iic_init()
{
    Wire.begin(PIN_NUM_IIC_SDA, PIN_NUM_IIC_SDL);

    if (!AHT10.begin(&Wire))
    {
        Serial.println("[ERROR] AHT10 初始化失败");
        return;
    }
    Serial.println("[INFO] AHT10 初始化成功");
    return;
}

bool aht10_read(float* temperature, float* humidity)
{
    sensors_event_t humidity_event, temp_event;

    if (!AHT10.getEvent(&humidity_event, &temp_event))
    {
        // Serial.println("[ERROR] AHT10 读取失败");
        return false;
    }

    *temperature = temp_event.temperature;
    *humidity = humidity_event.relative_humidity;
    return true;
}
