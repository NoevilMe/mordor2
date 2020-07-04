#include <iostream>
#include <thread>

#include "config.h"
#include "log.h"

using namespace Mordor;

static Logger::ptr g_log = Log::lookup("mordor:iombench");


int main() {
    auto g_logStdout = Config::lookup("log.stdout");
    g_logStdout->fromString("1");

    Mordor::Log::setLogLevel(Mordor::Log::Level::TRACE);

    MORDOR_LOG_TRACE(g_log) << "hello " << 2;
    MORDOR_LOG_DEBUG(g_log) << "hello " << 2;
    MORDOR_LOG_VERBOSE(g_log) << "hello " << 2;
    MORDOR_LOG_WARNING(g_log) << "hello " << 2;
    MORDOR_LOG_ERROR(g_log) << "hello " << 2;
    MORDOR_LOG_FATAL(g_log) << "hello " << 2;

    Mordor::Log::setLogLevel(Mordor::Log::Level::VERBOSE);
    MORDOR_LOG_TRACE(g_log) << "hello " << 2;
    MORDOR_LOG_DEBUG(g_log) << "hello " << 2;
    MORDOR_LOG_VERBOSE(g_log) << "hello " << 2;
    MORDOR_LOG_WARNING(g_log) << "hello " << 2;
    MORDOR_LOG_ERROR(g_log) << "hello " << 2;
    MORDOR_LOG_FATAL(g_log) << "hello " << 2;

    return 0;
}
