/* multi_esp32_led_strip_bridge.h
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
#include "double_buffer.h"

#if !defined(CONFIG_IDF_TARGET_ESP32C2)
    #define BOARD_HAS_RMT_SUPPORT
#endif

#if defined(SOC_SPI_PERIPH_NUM) && (SOC_SPI_PERIPH_NUM <= 2)
    #define SPI3_HOST SPI2_HOST
#endif

template<bool DOUBLEBUFFER_SUPPORT>
struct multi_esp32_led_strip_bridge : public led_bridge, InternalBuffer<DOUBLEBUFFER_SUPPORT>
{
    struct ExtSegments{
        LedConfig::Segment segment;
        led_strip_handle_t handle = NULL;
        int ledCount = 0;
    };
    std::vector<ExtSegments> _segments;

    uint16_t _totalLedsNumber = 0;
    LedType _ledsType = LedType::WS2812;    

    struct {
        const spi_host_device_t SELECTED_SPI_HOST = SPI2_HOST;
        spi_device_handle_t spi_handle = nullptr;
        size_t spi_buffer_size = 0;
        uint8_t* spi_led_buffer = nullptr;
    } cfgSpi;

    int getLedsNumber() override
    {
        return _totalLedsNumber;
    }

    void clearAll() override
    {
        if (_segments.size() || cfgSpi.spi_handle != nullptr)
        {
            for (int i = 0; !canRender() && i < 200; i++) {
                Log::debug("-");
                delay(1);
            }

            for (int i = 0; i < _totalLedsNumber; i++) {
                setLedRgb(i, 0 ,0, 0);
            }
            executeRenderLed();

            for (int i = 0; !canRender() && i < 200; i++) {
                Log::debug("+");
                delay(1);                
            }

            Log::debug("leds cleared");
        }
    }

    bool canRender() override
    {
        if (cfgSpi.spi_handle == nullptr)
            for(const auto& segment : _segments) 
                if (!led_strip_is_rendering_done(segment.handle)) {
                    return false;
                }
        
        return true;
    }

    int segmentSupported() override
    {
        return hardwareInfo.maxSegments();
    }

    bool supportsDoubleBuffering() override {
        if constexpr(DOUBLEBUFFER_SUPPORT){
            return true;
        }
        else {
            return false;
        }
    }  

    void executeRenderLed() override
    {
        if (_segments.size())
        {
            if constexpr (DOUBLEBUFFER_SUPPORT) {
                for(int i = 0; i < _totalLedsNumber; i++) {
                    uint8_t r, g, b, w;
                    this->doubleBuffer.getPixel(i, r, g, b, w);
                    internalSetLedRgbw(i, r, g, b, w);
                }
            }

            for(auto& segment : _segments) {
                if (segment.handle){
                    if (led_strip_refresh(segment.handle) != ESP_OK) {
                        Log::debug("led_strip_refresh failed");
                    }
                }
            }
        }
        else if (cfgSpi.spi_handle)
        {
            spi_transaction_t t;
            memset(&t, 0, sizeof(t));
            
            t.length = cfgSpi.spi_buffer_size * 8; 
            t.tx_buffer = cfgSpi.spi_led_buffer;

            spi_device_transmit(cfgSpi.spi_handle, &t);
        }
    }

    void releaseDriverResources() override
    {        
        delay(50);

        for(auto& segment : _segments)
            if (segment.handle)
            {
                led_strip_del(segment.handle);
                segment.handle = nullptr;
            }
        _segments.clear();

        if (cfgSpi.spi_handle) {
            spi_bus_remove_device(cfgSpi.spi_handle);
            spi_bus_free(cfgSpi.SELECTED_SPI_HOST);
            cfgSpi.spi_handle = nullptr;
        }
                
        if (cfgSpi.spi_led_buffer) {
            heap_caps_free(cfgSpi.spi_led_buffer);
            cfgSpi.spi_led_buffer = nullptr;
            cfgSpi.spi_buffer_size = 0;
        }

        if constexpr (DOUBLEBUFFER_SUPPORT) {
            this->doubleBuffer.releaseMemory();
        }
        
        delay(50);
    }

    void initializeLedDriver(LedType cfgLedType, uint16_t cfgLedNumLeds, const std::vector<LedConfig::Segment>& cfgSegments,
                            uint8_t calGain, uint8_t calRed, uint8_t calGreen, uint8_t calBlue) override
    {
        if (cfgSegments.size() < 0) return;

        if constexpr (DOUBLEBUFFER_SUPPORT) {
            if (!this->doubleBuffer.init(cfgLedNumLeds)) return;
            Log::debug("Enabled support for double buffering");
        }

        _totalLedsNumber = cfgLedNumLeds;
        _ledsType = cfgLedType;

        if (_ledsType == LedType::SK6812)
        {
            setParamsAndPrepareCalibration(calGain, calRed, calGreen, calBlue);
        }

        if (_ledsType == LedType::WS2812 || _ledsType == LedType::SK6812)
        {
            bool error = false;
            SegmentCapabilities dynamicResources;
            bool hasDmaLeft = hardwareInfo.rmt_has_dma;
            for(int i = 0; i < cfgSegments.size() && !error; i++) {
                const auto& seg = cfgSegments[i];

                typename multi_esp32_led_strip_bridge<DOUBLEBUFFER_SUPPORT>::SegmentCapabilities::Channels channel = dynamicResources.getFree();

                if (channel != SegmentCapabilities::Channels::NONE) {
                    int nextIndex = (i + 1 < cfgSegments.size()) ? cfgSegments[i + 1].startIndex : cfgLedNumLeds;

                    led_strip_config_t strip_config = {
                        .strip_gpio_num = seg.data,
                        .max_leds = static_cast<uint32_t>(std::max(nextIndex - seg.startIndex, 0)),
                        .led_model = (_ledsType == LedType::SK6812) ? LED_MODEL_SK6812 : LED_MODEL_WS2812,
                        .color_component_format = (_ledsType == LedType::SK6812) ? LED_STRIP_COLOR_COMPONENT_FMT_GRBW : LED_STRIP_COLOR_COMPONENT_FMT_GRB,
                        .flags = {
                            .invert_out = false,
                        }
                    };

                    led_strip_handle_t led_strip_handle = NULL;

                    if (channel == SegmentCapabilities::Channels::SPI) {
                         led_strip_spi_config_t spi_config = {
                            .clk_src = SPI_CLK_SRC_DEFAULT,
                            .spi_bus = ((i == 0) ? SPI2_HOST : SPI3_HOST),
                            .flags = {
                                .with_dma = true,
                            }
                        };

                        if (led_strip_new_spi_device(&strip_config, &spi_config, &led_strip_handle) != ESP_OK) {
                            led_strip_handle = NULL;
                            Log::debug("led_strip_new_spi_device failed for interface:", spi_config.spi_bus);
                        }
                    }
                    #if defined(BOARD_HAS_RMT_SUPPORT)
                    else if (channel == SegmentCapabilities::Channels::RMT) {
                        led_strip_rmt_config_t rmt_config = {
                            .clk_src = RMT_CLK_SRC_DEFAULT,
                            .resolution_hz = 10 * 1000 * 1000,
                            .mem_block_symbols = hardwareInfo.getRmtSize(cfgSegments.size(), hasDmaLeft),
                            .flags = {
                                .with_dma = hasDmaLeft,
                            }
                        };

                        hasDmaLeft = false;

                        if (led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip_handle) != ESP_OK) {
                            led_strip_handle = NULL;
                            Log::debug("led_strip_new_rmt_device failed");
                        }
                    }
                    #endif

                    if (led_strip_handle == NULL) {
                        error = true;
                        break;
                    }
                    else {
                        _segments.push_back({seg, led_strip_handle, static_cast<int>(strip_config.max_leds)});
                        Log::debug("Created Neopixel segment for ", strip_config.max_leds, " LEDS at: ", seg.startIndex, ", GPIO: ",  seg.data, 
                                    ", driver: ", ((channel == SegmentCapabilities::Channels::RMT) ? "RMT" : "SPI" ));
                    }
                }
                else {
                    Log::debug("Cannot segment at: ",i ,". Out of free hardware resources.");
                }
            }

            if (error) {
                releaseDriverResources();
            }            
        }
        else
        { // SPI (APA102 / SK9822)
            const auto& segment = cfgSegments.front();
            uint8_t cfgLedDataPin = segment.data;
            uint8_t cfgLedClockPin = segment.clock;

            cfgSpi.spi_buffer_size = 4 + (cfgLedNumLeds * 4) + ((cfgLedNumLeds / 16) + 1);

            spi_bus_config_t buscfg = {
                .mosi_io_num = cfgLedDataPin,
                .miso_io_num = -1,
                .sclk_io_num = cfgLedClockPin,
                .quadwp_io_num = -1,
                .quadhd_io_num = -1,
                .max_transfer_sz = static_cast<int>(cfgSpi.spi_buffer_size)
            };

            spi_device_interface_config_t devcfg = {
                .mode = 0,
                .clock_speed_hz = 10 * 1000 * 1000,
                .spics_io_num = -1,
                .queue_size = 7,
            };            

            if (spi_bus_initialize(cfgSpi.SELECTED_SPI_HOST, &buscfg, SPI_DMA_CH_AUTO) == ESP_OK) {
                if (spi_bus_add_device(cfgSpi.SELECTED_SPI_HOST, &devcfg, &cfgSpi.spi_handle) == ESP_OK) {   
                    
                    cfgSpi.spi_led_buffer = (uint8_t*)heap_caps_malloc(cfgSpi.spi_buffer_size, MALLOC_CAP_DEFAULT | MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA);

                    if (cfgSpi.spi_led_buffer != nullptr)
                    {
                        memset(cfgSpi.spi_led_buffer, 0, cfgSpi.spi_buffer_size);
                        for (size_t i = 4 + (cfgLedNumLeds * 4); i < cfgSpi.spi_buffer_size; i++) {
                            cfgSpi.spi_led_buffer[i] = 0xFF;
                        }
                        Log::debug("Created SPI segment, GPIO: ",  cfgLedDataPin, ", CLOCK: ", cfgLedClockPin);
                    }
                    else {
                        Log::debug("SPI: heap_caps_malloc failed");                        
                    }
                }
                else {
                    Log::debug("SPI: spi_bus_add_device failed");
                }
            }
            else {
                Log::debug("SPI: spi_bus_initialize failed");
            }

            if (cfgSpi.spi_led_buffer == nullptr) {
                releaseDriverResources();
            }
        }        
    }

    inline led_strip_handle_t findHandle(int& index) const {
        if (index >= 0 && index < _totalLedsNumber){
            for(const auto& seg : _segments)
                if (index < seg.ledCount) {
                    return seg.handle;
                }
                else {
                    index -= seg.ledCount;
                }
        }

        return nullptr;
    }

    void setLedRgb(int index, uint8_t r, uint8_t g, uint8_t b) override {
        if constexpr (DOUBLEBUFFER_SUPPORT) if (cfgSpi.spi_handle == nullptr) {
            uint8_t w = 0;
            if (_ledsType == LedType::SK6812)
            {                            
                const ColorRgbw calibrated = rgb2rgbw(r, g, b);
                r = calibrated.R; g = calibrated.G; b = calibrated.B; w = calibrated.W;
            }
            this->doubleBuffer.setPixel(index, r, g, b, w);
            return;                        
        }
        internalSetLedRgb(index, r, g, b);
    }

    void setLedRgbw(int index, uint8_t r, uint8_t g, uint8_t b, uint8_t w) override {
        if constexpr (DOUBLEBUFFER_SUPPORT) if (cfgSpi.spi_handle == nullptr) {
            this->doubleBuffer.setPixel(index, r, g, b, w);
            return;
        }
        internalSetLedRgbw(index, r, g, b, w);
    }

    inline void internalSetLedRgb(int index, uint8_t r, uint8_t g, uint8_t b)
    {
        if (_ledsType == LedType::SK6812)
        {            
            if (auto handle = findHandle(index); handle) {
                const ColorRgbw calibrated = rgb2rgbw(r, g, b);
                led_strip_set_pixel_rgbw(handle, index, calibrated.R, calibrated.G, calibrated.B, calibrated.W);
            }
        }
        else if (_ledsType == LedType::WS2812)
        {
            if (auto handle = findHandle(index); handle) {
                led_strip_set_pixel(handle, index, r, g, b);
            }
        }
        else if (_ledsType == LedType::APA102)
        {
            if (index >= _totalLedsNumber || cfgSpi.spi_led_buffer == nullptr) return;

            int offset = 4 + (index * 4);    
            cfgSpi.spi_led_buffer[offset]     = 0xFF;
            cfgSpi.spi_led_buffer[offset + 1] = b; 
            cfgSpi.spi_led_buffer[offset + 2] = g;
            cfgSpi.spi_led_buffer[offset + 3] = r;
        }
    }

    inline void internalSetLedRgbw(int index, uint8_t r, uint8_t g, uint8_t b, uint8_t w)
    {
        if (_ledsType == LedType::SK6812)
        {
            if (auto handle = findHandle(index); handle) {
                led_strip_set_pixel_rgbw(handle, index, r, g, b, w);
            }
        }
        else if (_ledsType == LedType::WS2812)
        {
            if (auto handle = findHandle(index); handle) {
                led_strip_set_pixel(handle, index, r, g, b);
            }
        }
        else if (_ledsType == LedType::APA102)
        {
            if (index >= _totalLedsNumber || cfgSpi.spi_led_buffer == nullptr) return;

            int offset = 4 + (index * 4);
            cfgSpi.spi_led_buffer[offset]     = 0xFF;
            cfgSpi.spi_led_buffer[offset + 1] = b; 
            cfgSpi.spi_led_buffer[offset + 2] = g;
            cfgSpi.spi_led_buffer[offset + 3] = r;
        }
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    struct SegmentCapabilities {
        int rmt;
        bool rmt_has_dma;
        int rmt_mem_block_symbols;
        int rmt_mem_block_symbols_aligment;
        int spi;

        enum Channels {
            RMT, SPI, NONE
        };
        
        SegmentCapabilities() {
            rmt = 0;            
            rmt_has_dma = false;
            rmt_mem_block_symbols = 0;
            rmt_mem_block_symbols_aligment = 64;
            spi = 0;

            #if defined(CONFIG_IDF_TARGET_ESP32S3)
                rmt = 2;
                spi = 2; 
                rmt_has_dma = true;
                rmt_mem_block_symbols = 192;
                rmt_mem_block_symbols_aligment = 48;
            #elif defined(CONFIG_IDF_TARGET_ESP32S2)
                rmt = 2;
                spi = 2;
                rmt_mem_block_symbols = 256;
            #elif defined(CONFIG_IDF_TARGET_ESP32C3)
                rmt = 1;
                spi = 1;
                rmt_mem_block_symbols = 96;
                rmt_mem_block_symbols_aligment = 48;
            #elif defined(CONFIG_IDF_TARGET_ESP32C2)
                rmt = 0;
                spi = 1;
            #elif defined(CONFIG_IDF_TARGET_ESP32) 
                rmt = 4;
                spi = 2;
                rmt_mem_block_symbols = 512;       
            #endif
        };

        int maxSegments() const {
            return rmt + spi;
        };

        size_t getRmtSize(int segments, bool hasDmaLeft) const {
            if (hasDmaLeft) {
                auto result = 1024;
                Log::debug("Consumed ", result," DMA memory for new RMT channel");
                return static_cast<size_t>(result);
            }
            else {
                segments -= spi + ((!rmt_has_dma) ? 0 : 1);

                auto result = ((!rmt_has_dma) ? rmt_mem_block_symbols : (rmt_mem_block_symbols - rmt_mem_block_symbols_aligment)) / std::max(segments, 1);

                result = (result / rmt_mem_block_symbols_aligment) * rmt_mem_block_symbols_aligment;

                Log::debug("Consumed ", result," symbols from RMT memory pool for new RMT channel. RMT has ", rmt_mem_block_symbols, " symbols");
                return static_cast<size_t>(result);
            }
        };

        Channels getFree() {
            if (spi > 0 ) {
                spi--;
                return Channels::SPI;
            }
            else if (rmt > 0 ) {
                rmt--;
                return Channels::RMT;
            }
            return Channels::NONE;
        }

    };

    const SegmentCapabilities hardwareInfo;
};