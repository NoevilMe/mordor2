#include "timestamp.h"

#include <stdio.h>
#include <sys/time.h>

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif

#include <chrono>
#include <inttypes.h>

namespace Mordor {

static_assert(sizeof(Timestamp) == sizeof(int64_t),
              "Timestamp is same size as int64_t");

std::string Timestamp::FormattedString(bool showMicroseconds) const {
    char buf[64] = {0};
    time_t seconds =
        static_cast<time_t>(microseconds_since_epoch_ / kMicroSecondsPerSecond);
    struct tm tm_time;
    gmtime_r(&seconds, &tm_time);

    if (showMicroseconds) {
        int microseconds = static_cast<int>(microseconds_since_epoch_ %
                                            kMicroSecondsPerSecond);
        snprintf(buf, sizeof(buf), "%4d%02d%02d %02d:%02d:%02d.%06d",
                 tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
                 tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec, microseconds);
    } else {
        int milliseconds = static_cast<int>(microseconds_since_epoch_ /
                                            kMicroSecondsPerMilliSecond %
                                            kMilliSecondsPerSecond);
        snprintf(buf, sizeof(buf), "%4d%02d%02d %02d:%02d:%02d.%03d",
                 tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
                 tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec, milliseconds);
    }
    return buf;
}

Timestamp Timestamp::Now() {
    return Timestamp(std::chrono::duration_cast<std::chrono::microseconds>(
                         std::chrono::system_clock::now().time_since_epoch())
                         .count());
}

int64_t Timestamp::MicrosecondsNow() {
    return std::chrono::duration_cast<std::chrono::microseconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
}

int64_t Timestamp::MillisecondsNow() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
}

} // namespace Mordor
