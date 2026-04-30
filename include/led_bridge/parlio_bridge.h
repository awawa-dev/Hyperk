/* parlio_bridge.h
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

#include "esp_log.h"
#include "esp_check.h"
#include "esp_cache.h"
#include "driver/parlio_tx.h"
#include "esp_heap_caps.h"
#include "led_strip.h"
#include "driver/spi_master.h"
#include "led_bridge.h"
#include "double_buffer.h"

template<bool DOUBLEBUFFER_SUPPORT>
struct parlio_bridge : public led_bridge, InternalBuffer<DOUBLEBUFFER_SUPPORT>
{
    struct ExtSegments{
        LedConfig::Segment segment;        
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

    led_strip_handle_t led_strip_handle = nullptr;

    int getLedsNumber() override
    {
        return _totalLedsNumber;
    }

    void clearAll() override
    {
        if (_segments.size() || cfgSpi.spi_handle != nullptr || led_strip_handle != nullptr)
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
        if (led_strip_handle != nullptr) {
            return led_strip_is_rendering_done(led_strip_handle);
        }
        else if (_segments.size())
        {         
            return parlio_neopixel_can_render();
        }
        
        return true;
    }

    int segmentSupported() override
    {
        return 8;
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
        if (_segments.size() || led_strip_handle != nullptr)
        {
            if constexpr (DOUBLEBUFFER_SUPPORT) {
                for(int i = 0; i < _totalLedsNumber; i++) {
                    uint8_t r, g, b, w;
                    this->doubleBuffer.getPixel(i, r, g, b, w);
                    internalSetLedRgbw(i, r, g, b, w);
                }
            }
            if (led_strip_handle != nullptr) {
                led_strip_refresh(led_strip_handle);
            }
            else {
                parlio_neopixel_show();
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

        parlio_neopixel_deinit();

        _segments.clear();

        if (led_strip_handle != nullptr) {
            led_strip_del(led_strip_handle);
            led_strip_handle = nullptr;
        }

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

        if ((_ledsType == LedType::WS2812 || _ledsType == LedType::SK6812) && cfgSegments.size() == 1)
        {
            led_strip_config_t strip_config = {
                .strip_gpio_num = cfgSegments.front().data,
                .max_leds = static_cast<uint32_t>(cfgLedNumLeds),
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

            if (led_strip_new_spi_device(&strip_config, &spi_config, &led_strip_handle) != ESP_OK) {
                led_strip_handle = NULL;
                Log::debug("led_strip_new_spi_device failed for interface:", spi_config.spi_bus);
            }
            else {
                Log::debug("Created Neopixel(SPI) segment for ", strip_config.max_leds, " LEDS, GPIO: ",  strip_config.strip_gpio_num);
            }
        }
        else if (_ledsType == LedType::WS2812 || _ledsType == LedType::SK6812)
        {
            uint16_t max_segment_length = 0;
            uint8_t parlio_data_width = 1;
            int gpio_nums[8] = {-1, -1, -1, -1, -1, -1, -1, -1};

            for (size_t i = 0; i < cfgSegments.size(); ++i) {
                uint16_t current_start = cfgSegments[i].startIndex;
                uint16_t next_start = (i + 1 < cfgSegments.size()) ? cfgSegments[i + 1].startIndex : cfgLedNumLeds;
                uint16_t segment_length = next_start - current_start;
                
                max_segment_length = std::max(max_segment_length, segment_length);
                
                if (i < 8) {
                    gpio_nums[i] = cfgSegments[i].data;
                }
            }

            size_t num_segments = cfgSegments.size();
            if (num_segments == 1) { parlio_data_width = 1; }
            else if (num_segments == 2) { parlio_data_width = 2; }
            else if (num_segments <= 4) { parlio_data_width = 4; }
            else if (num_segments <= 8) { parlio_data_width = 8; }

            if (parlio_neopixel_init(parlio_data_width, gpio_nums, max_segment_length, (_ledsType == LedType::SK6812))) {
                for(int i = 0; i < cfgSegments.size(); i++) {
                    const auto& seg = cfgSegments[i];
                        int nextIndex = (i + 1 < cfgSegments.size()) ? cfgSegments[i + 1].startIndex : cfgLedNumLeds;
                        int max_leds = static_cast<uint32_t>(std::max(nextIndex - seg.startIndex, 0));  
                    
                        _segments.push_back({seg, static_cast<int>(max_leds)});
                        Log::debug("Created PARLIO Neopixel segment for ", max_leds, " LEDS at: ", seg.startIndex, ", GPIO: ",  seg.data);
                }
            }
            else {
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

    inline std::pair<int,int> findHandle(int& index) const {
        if (index >= 0 && index < _totalLedsNumber){
            for(int i = 0; i < _segments.size(); i++){
                const auto& seg = _segments[i];
                if (index < seg.ledCount) {
                    return {i, index};
                }
                else {
                    index -= seg.ledCount;
                }
            }
        }
        return {-1, -1};
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
            const ColorRgbw calibrated = rgb2rgbw(r, g, b);

            if (led_strip_handle != nullptr) {
                led_strip_set_pixel_rgbw(led_strip_handle, index, calibrated.R, calibrated.G, calibrated.B, calibrated.W);
            }
            else if (auto handle = findHandle(index); handle.first >= 0) {                
                parlio_neopixel_setPixel(handle.first, handle.second, calibrated.R, calibrated.G, calibrated.B, calibrated.W);
            }
        }
        else if (_ledsType == LedType::WS2812)
        {
            if (led_strip_handle != nullptr) {
                led_strip_set_pixel(led_strip_handle, index, r, g, b);
            }
            else if (auto handle = findHandle(index); handle.first >= 0) {
                parlio_neopixel_setPixel(handle.first, handle.second, r, g, b, 0);
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
            if (led_strip_handle != nullptr) {
                led_strip_set_pixel_rgbw(led_strip_handle, index, r, g, b, w);
            }
            else if (auto handle = findHandle(index); handle.first >= 0) {
                parlio_neopixel_setPixel(handle.first, handle.second, r, g, b, w);
            }
        }
        else if (_ledsType == LedType::WS2812)
        {
            if (led_strip_handle != nullptr) {
                led_strip_set_pixel(led_strip_handle, index, r, g, b);
            }            
            else if (auto handle = findHandle(index); handle.first >= 0) {
                parlio_neopixel_setPixel(handle.first, handle.second, r, g, b, 0);
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
    
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    typedef struct {
        parlio_tx_unit_handle_t tx_unit;
        uint8_t *dma_buffer;
        size_t   buffer_size;
        uint16_t num_leds;
        uint8_t  data_width;
        bool     is_rgbw;
        uint16_t ticks_per_led;
        volatile bool is_transfering;
        int64_t next_frame_allowed_at;
    } parlio_led_strip_t;

    inline static parlio_led_strip_t strip = {};

    static inline void IRAM_ATTR parlio_neopixel_setPixel(uint8_t pin_index, uint16_t led_index, uint8_t r, uint8_t g, uint8_t b, uint8_t w)
    {
        if (led_index >= strip.num_leds || pin_index >= strip.data_width) return;

        uint32_t color = 0;
        uint8_t bits = strip.is_rgbw ? 32 : 24;

        if (strip.is_rgbw) {
            color = (g << 24) | (r << 16) | (b << 8) | w;
        } else {
            color = (g << 16) | (r << 8) | b;
        }

        uint32_t tick_base = led_index * strip.ticks_per_led;

        for (int i = 0; i < bits; i++) {
            uint8_t bit_val = (color >> (bits - 1 - i)) & 1;

            for (int tick_offset = 0; tick_offset < 3; tick_offset++) {
                uint32_t current_tick = tick_base + (i * 3) + tick_offset;
                uint32_t global_bit_idx = (current_tick * strip.data_width) + pin_index;
                
                uint32_t byte_idx = global_bit_idx / 8;
                uint8_t  bit_mask = 1 << (global_bit_idx % 8);               

                bool pin_state = (tick_offset == 0) || (tick_offset == 1 && bit_val);

                if (pin_state) {
                    strip.dma_buffer[byte_idx] |= bit_mask;
                } else {
                    strip.dma_buffer[byte_idx] &= ~bit_mask;
                }
            }
        }
    }

    static bool IRAM_ATTR parlio_tx_finish_callback(parlio_tx_unit_handle_t tx_unit, const parlio_tx_done_event_data_t *edata, void *user_ctx) {
        strip.next_frame_allowed_at = esp_timer_get_time() + 300;
        strip.is_transfering = false;
        return false;
    }

    bool parlio_neopixel_can_render() {
        if (strip.is_transfering || strip.next_frame_allowed_at > esp_timer_get_time()) 
            return false;

        strip.next_frame_allowed_at = 0;
        return true;
    }

    bool parlio_neopixel_init(uint8_t data_width, const int *gpio_nums, uint16_t num_leds, bool is_rgbw)
    {
        if (data_width != 1 && data_width != 2 && data_width != 4 && data_width != 8) {
            Log::debug("parlio_neopixel_init: unsupported data_width. must be 1, 2, 4 or 8");
            return false;
        }

        strip.num_leds = num_leds;
        strip.is_rgbw = is_rgbw;
        strip.data_width = data_width;

        strip.is_transfering = false;
        strip.next_frame_allowed_at = 0;
        
        strip.ticks_per_led = is_rgbw ? (32 * 3) : (24 * 3);

        uint32_t total_ticks = strip.num_leds * strip.ticks_per_led;

        strip.buffer_size = (total_ticks * data_width + 7) / 8;
        strip.buffer_size = (strip.buffer_size + 31) & ~31;

        parlio_tx_unit_config_t tx_config = {
            .clk_src = PARLIO_CLK_SRC_DEFAULT,
            .clk_in_gpio_num = GPIO_NUM_NC,
            .output_clk_freq_hz = 2500000,
            .data_width = data_width,
            .clk_out_gpio_num = gpio_num_t::GPIO_NUM_NC,
            .valid_gpio_num = gpio_num_t::GPIO_NUM_NC,
            .trans_queue_depth = 4,
            .max_transfer_size = strip.buffer_size,            
            .sample_edge = PARLIO_SAMPLE_EDGE_POS,
            .bit_pack_order = PARLIO_BIT_PACK_ORDER_LSB,
        };

        for (int i = 0; i < PARLIO_TX_UNIT_MAX_DATA_WIDTH; i++) {
            tx_config.data_gpio_nums[i] = (i < data_width) ? (gpio_num_t)gpio_nums[i] : gpio_num_t::GPIO_NUM_NC;
        }

        bool initOk = false;
        if (parlio_new_tx_unit(&tx_config, &strip.tx_unit) != ESP_OK) {
            Log::debug("parlio_neopixel_init: parlio_new_tx_unit failed"); 
        }
        else
        {
            strip.dma_buffer = (uint8_t*)heap_caps_aligned_alloc(32, strip.buffer_size, MALLOC_CAP_DMA | MALLOC_CAP_32BIT);

            if (strip.dma_buffer == NULL) {
                Log::debug("parlio_neopixel_init: failed to allocate DMA buffer!");            
            }
            else {
                memset(strip.dma_buffer, 0, strip.buffer_size);

                parlio_tx_event_callbacks_t cbs = {};
                cbs.on_trans_done = parlio_tx_finish_callback;
                if (parlio_tx_unit_register_event_callbacks(strip.tx_unit, &cbs, NULL) != ESP_OK) {
                    Log::debug("parlio_neopixel_init: parlio_tx_unit_register_event_callbacks failed");
                }
                else {
                    if (parlio_tx_unit_enable(strip.tx_unit) != ESP_OK) {
                        Log::debug("parlio_neopixel_init: parlio_tx_unit_enable failed");
                    }
                    else {
                        initOk = true;
                        Log::debug("PARLIO Neopixel init @ ", tx_config.output_clk_freq_hz,"Hz. Outputs: ", data_width, ", RGBW: ", is_rgbw, ", Buffer: ", strip.buffer_size," bytes");                
                    }
                }
            }
        }

        if (!initOk) {
            parlio_neopixel_deinit();
        }

        return initOk;
    }

    bool parlio_neopixel_show()
    {
         if (strip.tx_unit == NULL) return false;

        parlio_transmit_config_t trans_config = {
            .idle_value = 0x00,
        };

        size_t total_bits = (strip.num_leds * strip.ticks_per_led) * strip.data_width;

        strip.is_transfering = true;
        strip.next_frame_allowed_at = INT64_MAX;

        if (auto res = parlio_tx_unit_transmit(strip.tx_unit, strip.dma_buffer, total_bits, &trans_config); res != ESP_OK) {
            strip.is_transfering = false;
            strip.next_frame_allowed_at = 0;

            Log::debug("parlio_tx_unit_transmit failed: ", res);
            return false;
        }

        return true;
    }

    void parlio_neopixel_deinit()
    {
        if (strip.tx_unit) {
            parlio_tx_unit_disable(strip.tx_unit);

            parlio_del_tx_unit(strip.tx_unit);
            strip.tx_unit = NULL;
            Log::debug("PARLIO deinitialized: tx_unit");
        }
        if (strip.dma_buffer) {
            free(strip.dma_buffer);
            strip.dma_buffer = NULL;
            Log::debug("PARLIO deinitialized: dma_buffer");
        }

        strip = {};
        Log::debug("PARLIO deinitialized: finished");
    }
};