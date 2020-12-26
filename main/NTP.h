/*
 * MIT License
 *
 * Copyright (c) 2020 Christopher B. Liebman
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
*/

#ifndef _NTP_H
#define _NTP_H
#include "PPS.h"

typedef struct ntp_time
{
    uint32_t seconds;
    uint32_t fraction;
} NTPTime;

class NTP
{
public:
    NTP(PPS& pps);
    ~NTP();
    void begin();
    uint32_t getRequests() { return _req_count; }
    uint32_t getResponses() { return _rsp_count; }

private:
    PPS&     _pps;
    uint32_t _req_count;
    uint32_t _rsp_count;
    uint8_t  _precision;

    void getNTPTime(NTPTime *time);
    int8_t computePrecision();
    void handleRequest();
    static void task(void* data);
};

#endif // _NTP_H
