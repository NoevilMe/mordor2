#include <iostream>

#include "config.h"
#include "log.h"

using namespace Mordor;


static Logger::ptr g_log = Log::lookup("mordor:iombench");


int main() {
    std::cout << "hello gondor" << std::endl;

    auto  g_logStdout =
    Config::lookup("log.stdout");
    g_logStdout->fromString("1");

    MORDOR_LOG_INFO(g_log) << "hello " << 2 << std::endl;

    return 0;
}
