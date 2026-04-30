/* double_buffer.h
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
#include <Arduino.h>
#include "logger.h"

class DoubleBuffer {
public:
    typedef uint32_t color_cell_t;

    virtual ~DoubleBuffer() {
        releaseMemory();
    }

    void releaseMemory()
    {
        if (_backBuffer) {
            heap_caps_free(_backBuffer);
            _backBuffer = nullptr;
        }
        _ledsNumber = 0;
    }

    bool init(int ledsNumber)
    {
        releaseMemory();

        _ledsNumber = ledsNumber;
        _backBuffer = (color_cell_t*) heap_caps_malloc(_ledsNumber * sizeof(color_cell_t), MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL);

        bool result = (_backBuffer != nullptr);

        if (!result) {
            Log::debug("Cannot alloc double buffer memory");
        }

        return result;
    }

    inline void getPixel(int index, uint8_t& r, uint8_t& g, uint8_t& b, uint8_t& w) const {
        uint32_t val = _backBuffer[index];
        
        b = val & 0xFF;
        g = (val >> 8) & 0xFF;
        r = (val >> 16) & 0xFF;
        w = (val >> 24) & 0xFF;
    }

    inline void setPixel(int index, uint8_t r, uint8_t g, uint8_t b, uint8_t w) {
        _backBuffer[index] = ((uint32_t)w << 24) | ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
    }

private:
    int _ledsNumber = 0;
    color_cell_t* _backBuffer = nullptr;
};

template<bool DOUBLEBUFFER_SUPPORT>
struct InternalBuffer {    
};

template<>
struct InternalBuffer<true> {
    DoubleBuffer doubleBuffer;
};
