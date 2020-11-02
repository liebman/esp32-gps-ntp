#include "NTP.h"
#include "esp_log.h"
#include "math.h"

static const char* TAG = "NTP";

#ifndef NTP_TASK_PRI
#define NTP_TASK_PRI configMAX_PRIORITIES-3
#endif

#ifndef NTP_TASK_CORE
#define NTP_TASK_CORE 1
#endif

//#define NTP_PACKET_DEBUG

#define NTP_PORT               123
#define PRECISION_COUNT        10000

typedef struct ntp_packet
{
    uint8_t  flags;
    uint8_t  stratum;
    uint8_t  poll;
    int8_t   precision;
    uint32_t delay;
    uint32_t dispersion;
    uint8_t  ref_id[4];
    NTPTime  ref_time;
    NTPTime  orig_time;
    NTPTime  recv_time;
    NTPTime  xmit_time;
} NTPPacket;

#define LI_NONE         0
#define LI_SIXTY_ONE    1
#define LI_FIFTY_NINE   2
#define LI_NOSYNC       3

#define MODE_RESERVED   0
#define MODE_ACTIVE     1
#define MODE_PASSIVE    2
#define MODE_CLIENT     3
#define MODE_SERVER     4
#define MODE_BROADCAST  5
#define MODE_CONTROL    6
#define MODE_PRIVATE    7

#define NTP_VERSION     4

#define REF_ID          "PPS "  // "GPS " when we have one!

#define setLI(value)    ((value&0x03)<<6)
#define setVERS(value)  ((value&0x07)<<3)
#define setMODE(value)  ((value&0x07))

#define getLI(value)    ((value>>6)&0x03)
#define getVERS(value)  ((value>>3)&0x07)
#define getMODE(value)  (value&0x07)

#define SEVENTY_YEARS   2208988800L
#define toEPOCH(t)      ((uint32_t)t-SEVENTY_YEARS)
#define toNTP(t)        ((uint32_t)t+SEVENTY_YEARS)

#ifdef NTP_PACKET_DEBUG
#include <time.h>
char* timestr(long int t)
{
    t = toEPOCH(t);
    return ctime(&t);
}

void dumpNTPPacket(NTPPacket* ntp)
{
    ESP_LOGI(TAG, "size:       %u\n", sizeof(*ntp));
    ESP_LOGI(TAG, "firstbyte:  0x%02x\n", *(uint8_t*)ntp);
    ESP_LOGI(TAG, "li:         %u\n", getLI(ntp->flags));
    ESP_LOGI(TAG, "version:    %u\n", getVERS(ntp->flags));
    ESP_LOGI(TAG, ("mode:       %u\n", getMODE(ntp->flags));
    ESP_LOGI(TAG, "stratum:    %u\n", ntp->stratum);
    ESP_LOGI(TAG, "poll:       %u\n", ntp->poll);
    ESP_LOGI(TAG, "precision:  %d\n", ntp->precision);
    ESP_LOGI(TAG, "delay:      %u\n", ntp->delay);
    ESP_LOGI(TAG, "dispersion: %u\n", ntp->dispersion);
    ESP_LOGI(TAG, "ref_id:     %02x:%02x:%02x:%02x\n", ntp->ref_id[0], ntp->ref_id[1], ntp->ref_id[2], ntp->ref_id[3]);
    ESP_LOGI(TAG, "ref_time:   %08x:%08x\n", ntp->ref_time.seconds, ntp->ref_time.fraction);
    ESP_LOGI(TAG, "orig_time:  %08x:%08x\n", ntp->orig_time.seconds, ntp->orig_time.fraction);
    ESP_LOGI(TAG, "recv_time:  %08x:%08x\n", ntp->recv_time.seconds, ntp->recv_time.fraction);
    ESP_LOGI(TAG, "xmit_time:  %08x:%08x\n", ntp->xmit_time.seconds, ntp->xmit_time.fraction);
}
#else
#define dumpNTPPacket(x)
#endif


NTP::NTP(PPS& pps)
: _pps(pps)
{
}

NTP::~NTP()
{
}

void NTP::begin()
{
    _precision = computePrecision();
    xTaskCreatePinnedToCore(&NTP::task, "NTP", 4096, this, NTP_TASK_PRI, nullptr, NTP_TASK_CORE);
}

int8_t NTP::computePrecision()
{
    NTPTime t;
    uint64_t start = esp_timer_get_time();
    for (int i = 0; i < PRECISION_COUNT; ++i)
    {
        getNTPTime(&t);
    }
    uint64_t end = esp_timer_get_time();
    double        total = (double)(end - start) / 1000000.0;
    double        time  = total / PRECISION_COUNT;
    double        prec  = log2(time);
    ESP_LOGI(TAG, "computePrecision: total:%f time:%f prec:%f (%d)", total, time, prec, (int8_t)prec);
    return (int8_t)prec;
}

#define us2s(x) (((double)x)/(double)1000000) // microseconds to seconds

void NTP::getNTPTime(NTPTime* time)
{
    struct timeval tv;
    _pps.getTime(&tv);
    time->seconds = toNTP(tv.tv_sec);
    double percent = us2s(tv.tv_usec);
    time->fraction = (uint32_t)(percent * (double)4294967296L);
}

void NTP::task(void* data)
{
    ESP_LOGI(TAG, "::task started with priority %d core %d", uxTaskPriorityGet(nullptr), xPortGetCoreID());

    while(true)
    {
        vTaskDelay(pdMS_TO_TICKS(100)); // for now do nothing!
    }
}
