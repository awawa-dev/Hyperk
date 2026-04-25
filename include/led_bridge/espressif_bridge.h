/* espressif_bridge.h
*
*  MIT License
*
*  Copyright (c) 2026 awawa-dev
*
*  Project homesite: https://github.com/awawa-dev/Hyperk
*
*  Permission is hereby granted, free of charge, to any person obtaining a copy
*  of this software and associated documentation files (the "Software"), to deal
*  in the Software without restriction, including without limitation the rights
*  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
*  copies of the Software, and to permit persons to whom the Software is
*  furnished to do so, subject to the following conditions:
*
*  The above copyright notice and this permission notice shall be included in all
*  copies or substantial portions of the Software.

*  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
*  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
*  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
*  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
*  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
*  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
*  SOFTWARE.
*/

#pragma once

#include "led_strip.h"
#include "driver/spi_master.h"
#include "led_bridge.h"

struct espressif_bridge : public led_bridge
{
    led_strip_handle_t _handle = nullptr;
    uint16_t _totalLedsNumber = 0;
    LedType _ledsType = LedType::WS2812;

    const spi_host_device_t SPI_HOST = SPI2_HOST;
    spi_device_handle_t _spi_handle = nullptr;
    size_t _spi_buffer_size = 0;
    uint8_t* _spi_led_buffer = nullptr;

    int getLedsNumber() override
    {
        return _totalLedsNumber;
    }

    void clearAll() override
    {
        if (_handle != nullptr)
        {
            led_strip_clear(_handle);
        }
        else if (_spi_handle)
        {
            for (int i = 0; i < _totalLedsNumber; i++) {
                setLedRgb(i, 0 ,0, 0);
            }
            executeRenderLed(true);
        }
    }

    bool canRender() override
    {
        return true;
    }

    bool executeRenderLed(bool isNewFrame) override
    {
        if (_handle != nullptr)
        {
            led_strip_refresh(_handle);
        }
        else if (_spi_handle)
        {
            spi_transaction_t t;
            memset(&t, 0, sizeof(t));
            
            t.length = _spi_buffer_size * 8; 
            t.tx_buffer = _spi_led_buffer;

            spi_device_transmit(_spi_handle, &t);
        }
        return true;
    }

    void releaseDriverResources() override
    {        
        delay(50);

        if (_handle != nullptr)
        {
            led_strip_del(_handle);
            _handle = nullptr;
        }

        if (_spi_handle) {
            spi_bus_remove_device(_spi_handle);
            spi_bus_free(SPI_HOST);
        }
                
        if (_spi_led_buffer) {
            heap_caps_free(_spi_led_buffer);
            _spi_led_buffer = nullptr;
            _spi_buffer_size = 0;
        }
        
        delay(50);
    }

    void initializeLedDriver(LedType cfgLedType, uint16_t cfgLedNumLeds, uint8_t cfgLedDataPin, uint8_t cfgLedClockPin,
                            uint8_t calGain, uint8_t calRed, uint8_t calGreen, uint8_t calBlue) override
    {
        _totalLedsNumber = cfgLedNumLeds;
        _ledsType = cfgLedType;

        if (_ledsType == LedType::SK6812)
        {
            setParamsAndPrepareCalibration(calGain, calRed, calGreen, calBlue);
        }

        if (_ledsType == LedType::WS2812 || _ledsType == LedType::SK6812)
        {
            led_strip_config_t strip_config = {
                .strip_gpio_num = cfgLedDataPin,
                .max_leds = cfgLedNumLeds,
                .led_model = (_ledsType == LedType::SK6812) ? LED_MODEL_SK6812 : LED_MODEL_WS2812,
                .color_component_format = (_ledsType == LedType::SK6812) ? LED_STRIP_COLOR_COMPONENT_FMT_GRBW : LED_STRIP_COLOR_COMPONENT_FMT_GRB,
                .flags = {
                    .invert_out = false,
                }
            };

            led_strip_spi_config_t spi_config = {
                .clk_src = SPI_CLK_SRC_DEFAULT,
                .spi_bus = SPI2_HOST,
                .flags = {
                    .with_dma = true,
                }
            };

            led_strip_new_spi_device(&strip_config, &spi_config, &_handle);
        }
        else
        { // SPI (APA102 / SK9822)
            _spi_buffer_size = 4 + (cfgLedNumLeds * 4) + ((cfgLedNumLeds / 16) + 1);

            spi_bus_config_t buscfg = {
                .mosi_io_num = cfgLedDataPin,
                .miso_io_num = -1,
                .sclk_io_num = cfgLedClockPin,
                .quadwp_io_num = -1,
                .quadhd_io_num = -1,
                .max_transfer_sz = static_cast<int>(_spi_buffer_size)
            };

            spi_device_interface_config_t devcfg = {
                .mode = 0,
                .clock_speed_hz = 10 * 1000 * 1000,
                .spics_io_num = -1,
                .queue_size = 7,
            };            

            if (spi_bus_initialize(SPI_HOST, &buscfg, SPI_DMA_CH_AUTO) == ESP_OK) {
                if (spi_bus_add_device(SPI_HOST, &devcfg, &_spi_handle) == ESP_OK) {   
                    
                    _spi_led_buffer = (uint8_t*)heap_caps_malloc(_spi_buffer_size, MALLOC_CAP_DEFAULT | MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA);
                    if (_spi_led_buffer == nullptr) {
                        spi_bus_remove_device(_spi_handle);
                        _spi_handle = nullptr;
                        spi_bus_free(SPI_HOST);
                    } else {                        
                        memset(_spi_led_buffer, 0, _spi_buffer_size);
                        for (size_t i = 4 + (cfgLedNumLeds * 4); i < _spi_buffer_size; i++) {
                            _spi_led_buffer[i] = 0xFF;
                        }
                    }
                }
            }

            if (_spi_handle == nullptr) {
                _spi_buffer_size = 0;
            }
        }        
    }

    inline void setLedRgb(int index, uint8_t r, uint8_t g, uint8_t b) override
    {
        if (_ledsType == LedType::SK6812)
        {            
            if (index >= _totalLedsNumber || _handle == nullptr) return;
            const ColorRgbw calibrated = rgb2rgbw(r, g, b);
            led_strip_set_pixel_rgbw(_handle, index, calibrated.R, calibrated.G, calibrated.B, calibrated.W);
        }
        else if (_ledsType == LedType::WS2812)
        {
            if (index >= _totalLedsNumber || _handle == nullptr) return;
            led_strip_set_pixel(_handle, index, r, g, b);
        }
        else if (_ledsType == LedType::APA102)
        {
            if (index >= _totalLedsNumber || _spi_led_buffer == nullptr) return;

            int offset = 4 + (index * 4);    
            _spi_led_buffer[offset]     = 0xFF;
            _spi_led_buffer[offset + 1] = b; 
            _spi_led_buffer[offset + 2] = g;
            _spi_led_buffer[offset + 3] = r;
        }
    }

    inline void setLedRgbw(int index, uint8_t r, uint8_t g, uint8_t b, uint8_t w) override
    {
        if (_ledsType == LedType::SK6812)
        {
            if (index >= _totalLedsNumber || _handle == nullptr) return;
            led_strip_set_pixel_rgbw(_handle, index, r, g, b, w);
        }
        else if (_ledsType == LedType::WS2812)
        {
            if (index >= _totalLedsNumber || _handle == nullptr) return;
            led_strip_set_pixel(_handle, index, r, g, b);
        }
        else if (_ledsType == LedType::APA102)
        {
            if (index >= _totalLedsNumber || _spi_led_buffer == nullptr) return;

            int offset = 4 + (index * 4);
            _spi_led_buffer[offset]     = 0xFF;
            _spi_led_buffer[offset + 1] = b; 
            _spi_led_buffer[offset + 2] = g;
            _spi_led_buffer[offset + 3] = r;
        }
    }
};