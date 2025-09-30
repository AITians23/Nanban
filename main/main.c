 #include <stdio.h>
#include "esp_log.h"
#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// BH1750 I2C address (default: ADD pin low or floating)
#define BH1750_ADDR 0x23
// BH1750 commands
#define BH1750_POWER_ON 0x01
#define BH1750_CONT_H_RES 0x10 // Continuous high-resolution mode (1 lux resolution)

// I2C configuration
#define I2C_MASTER_SCL_IO 21       // GPIO for SCL
#define I2C_MASTER_SDA_IO 20       // GPIO for SDA
#define I2C_MASTER_NUM I2C_NUM_0   // I2C port number
#define I2C_MASTER_FREQ_HZ 100000  // I2C clock frequency (100 kHz)

static const char *TAG = "BH1750";
 
esp_err_t i2c_master_init(void) {
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };
    esp_err_t err = i2c_param_config(I2C_MASTER_NUM, &conf);
    if (err != ESP_OK) {
        return err;
    }
    return i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);
}
//to write the commmand
esp_err_t bh1750_write_command(uint8_t cmd) {
    i2c_cmd_handle_t cmd_handle = i2c_cmd_link_create();
    i2c_master_start(cmd_handle);
    i2c_master_write_byte(cmd_handle, (BH1750_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd_handle, cmd, true);
    i2c_master_stop(cmd_handle);
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd_handle, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd_handle);
    return ret;
}
//to read the command
esp_err_t bh1750_read_lux(float *lux) {
    uint8_t data[2];
    i2c_cmd_handle_t cmd_handle = i2c_cmd_link_create();
    i2c_master_start(cmd_handle);
    i2c_master_write_byte(cmd_handle, (BH1750_ADDR << 1) | I2C_MASTER_READ, true);
    i2c_master_read(cmd_handle, data, 2, I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmd_handle);
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd_handle, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd_handle);

    if (ret == ESP_OK) {
        uint16_t raw_lux = (data[0] << 8) | data[1];
        *lux = raw_lux / 1.2; // Convert to lux (BH1750 formula for high-res mode)
    }
    return ret;
}

void app_main(void) {
    // Initialize I2C
    ESP_ERROR_CHECK(i2c_master_init());
    ESP_LOGI(TAG, "I2C initialized successfully");

    // Power on BH1750 and set to continuous high-resolution mode
    ESP_ERROR_CHECK(bh1750_write_command(BH1750_POWER_ON));
    ESP_ERROR_CHECK(bh1750_write_command(BH1750_CONT_H_RES));
    ESP_LOGI(TAG, "BH1750 initialized successfully");

    while (1) {
        float lux;
        esp_err_t ret = bh1750_read_lux(&lux);
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "Light: %.2f lx", lux);
        } else {
            ESP_LOGE(TAG, "Failed to read from BH1750: %s", esp_err_to_name(ret));
        }
        vTaskDelay(pdMS_TO_TICKS(1000)); // Delay 1 second
    }

    // Cleanup (unreachable in this example)
    i2c_driver_delete(I2C_MASTER_NUM);
}
