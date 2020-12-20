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
