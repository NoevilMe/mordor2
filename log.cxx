// Copyright (c) 2009 - Mozy, Inc.

#include "log.h"
#include "config.h"

#include <chrono>
#include <iostream>
#include <regex>
#include <sys/types.h>
#include <syscall.h>
#include <unistd.h>

#ifndef WINDOWS
#define SYSLOG_NAMES
#include <syslog.h>
#endif

#include "assert.h"

namespace Mordor {

static void enableLoggers();
static void enableStdoutLogging();
static void enableFileLogging();

static ConfigVar<std::string>::ptr g_logError =
    Config::lookup("log.errormask", std::string(".*"),
                   "Regex of loggers to enable error for.");
static ConfigVar<std::string>::ptr g_logWarn =
    Config::lookup("log.warnmask", std::string(".*"),
                   "Regex of loggers to enable warning for.");
static ConfigVar<std::string>::ptr g_logInfo = Config::lookup(
    "log.infomask", std::string(".*"), "Regex of loggers to enable info for.");
static ConfigVar<std::string>::ptr g_logVerbose =
    Config::lookup("log.verbosemask", std::string(""),
                   "Regex of loggers to enable verbose for.");
static ConfigVar<std::string>::ptr g_logDebug =
    Config::lookup("log.debugmask", std::string(""),
                   "Regex of loggers to enable debugging for.");
static ConfigVar<std::string>::ptr g_logTrace = Config::lookup(
    "log.tracemask", std::string(""), "Regex of loggers to enable trace for.");

static ConfigVar<bool>::ptr g_logStdout =
    Config::lookup("log.stdout", false, "Log to stdout");
static ConfigVar<std::string>::ptr g_logFile =
    Config::lookup("log.file", std::string(), "Log to file");

namespace {

static struct LogInitializer {
    LogInitializer() {
        g_logError->monitor(&enableLoggers);
        g_logWarn->monitor(&enableLoggers);
        g_logInfo->monitor(&enableLoggers);
        g_logVerbose->monitor(&enableLoggers);
        g_logDebug->monitor(&enableLoggers);
        g_logTrace->monitor(&enableLoggers);

        g_logFile->monitor(&enableFileLogging);
        g_logStdout->monitor(&enableStdoutLogging);
    }
} g_init;

} // namespace

static void enableLogger(Logger::ptr logger, const std::regex &errorRegex,
                         const std::regex &warnRegex,
                         const std::regex &infoRegex,
                         const std::regex &verboseRegex,
                         const std::regex &debugRegex,
                         const std::regex &traceRegex) {
    Log::Level level = Log::Level::FATAL;
    if (std::regex_match(logger->name(), errorRegex))
        level = Log::Level::ERROR;
    if (std::regex_match(logger->name(), warnRegex))
        level = Log::Level::WARNING;
    if (std::regex_match(logger->name(), infoRegex))
        level = Log::Level::INFO;
    if (std::regex_match(logger->name(), verboseRegex))
        level = Log::Level::VERBOSE;
    if (std::regex_match(logger->name(), debugRegex))
        level = Log::Level::DEBUG;
    if (std::regex_match(logger->name(), traceRegex))
        level = Log::Level::TRACE;

    if (logger->level() != level)
        logger->level(level, false);
}

static std::regex buildLogRegex(const std::string &exp,
                                const std::string &default_exp) {
    try {
        return std::regex(exp);
    } catch (std::regex_error &) {
        return std::regex(default_exp);
    }
}

static void enableLoggers() {
    std::regex errorRegex = buildLogRegex(g_logError->val(), ".*");
    std::regex warnRegex = buildLogRegex(g_logWarn->val(), ".*");
    std::regex infoRegex = buildLogRegex(g_logInfo->val(), ".*");
    std::regex verboseRegex = buildLogRegex(g_logVerbose->val(), "");
    std::regex debugRegex = buildLogRegex(g_logDebug->val(), "");
    std::regex traceRegex = buildLogRegex(g_logTrace->val(), "");
    Log::visit(std::bind(&enableLogger, std::placeholders::_1,
                         std::cref(errorRegex), std::cref(warnRegex),
                         std::cref(infoRegex), std::cref(verboseRegex),
                         std::cref(debugRegex), std::cref(traceRegex)));
}

static void enableStdoutLogging() {
    std::cout << "enableStdoutLogging" << std::endl;
    static LogSink::ptr stdoutSink;
    bool log = g_logStdout->val();
    if (stdoutSink.get() && !log) {
        Log::root()->removeSink(stdoutSink);
        stdoutSink.reset();
    } else if (!stdoutSink.get() && log) {
        stdoutSink.reset(new StdoutLogSink());
        Log::root()->addSink(stdoutSink);
    }
}

static void enableFileLogging() {
    static LogSink::ptr fileSink;
    std::string file = g_logFile->val();
    if (fileSink.get() && file.empty()) {
        Log::root()->removeSink(fileSink);
        fileSink.reset();
    } else if (!file.empty()) {
        if (fileSink.get()) {
            if (static_cast<FileLogSink *>(fileSink.get())->file() == file)
                return;
            Log::root()->removeSink(fileSink);
            fileSink.reset();
        }
        fileSink.reset(new FileLogSink(file));
        Log::root()->addSink(fileSink);
    }
}

void StdoutLogSink::log(const std::string &logger, system_time_point now,
                        tid_t thread, Log::Level level, const std::string &str,
                        const char *file, int line) {
    std::ostringstream os;
     os << now << " "  << level << " " << thread << " "
       << " " << logger << " " << file << ":" << line << " " << str
       << std::endl;
    std::cout << os.str();
    std::cout.flush();
}

FileLogSink::FileLogSink(const std::string &file) {
    // m_stream.reset(new FileStream(file, FileStream::APPEND,
    // FileStream::OPEN_OR_CREATE));
    // m_file = file;
}

void FileLogSink::log(const std::string &logger, system_time_point now,
                      tid_t thread, Log::Level level, const std::string &str,
                      const char *file, int line) {
    std::ostringstream os;
    // os << now << " " << elapsed << " " << level << " " << thread << " "
    os << " " << logger << " " << file << ":" << line << " " << str
       << std::endl;
    std::string logline = os.str();
    // m_stream->write(logline.c_str(), logline.size());
    // m_stream->flush();
}

static void deleteNothing(Logger *l) {}

Logger::ptr Log::root() {
    static Logger::ptr _root(new Logger());
    return _root;
}

Logger::ptr Log::lookup(const std::string &name) {
    Logger::ptr log = root();
    if (name.empty() || name == ":") {
        return log;
    }
    std::set<Logger::ptr, LoggerLess>::iterator it;
    Logger dummy(name, log);
    Logger::ptr dummyPtr(&dummy, &deleteNothing);
    size_t start = 0;
    std::string child_name;
    std::string node_name;
    while (start < name.size()) {
        size_t pos = name.find(':', start);
        if (pos == std::string::npos) {
            child_name = name.substr(start);
            start = name.size();
        } else {
            child_name = name.substr(start, pos - start);
            start = pos + 1;
        }
        if (child_name.empty()) {
            continue;
        }
        if (!node_name.empty()) {
            node_name += ":";
        }
        node_name += child_name;
        dummyPtr->m_name = node_name;
        it = log->m_children.lower_bound(dummyPtr);
        if (it == log->m_children.end() || (*it)->m_name != node_name) {
            Logger::ptr child(new Logger(node_name, log));
            log->m_children.insert(child);
            log = child;
        } else {
            log = *it;
        }
    }
    return log;
}

void Log::visit(std::function<void(std::shared_ptr<Logger>)> dg) {
    std::list<Logger::ptr> toVisit;
    toVisit.push_back(root());
    while (!toVisit.empty()) {
        Logger::ptr cur = toVisit.front();
        toVisit.pop_front();
        dg(cur);
        for (std::set<Logger::ptr, LoggerLess>::iterator it =
                 cur->m_children.begin();
             it != cur->m_children.end(); ++it) {
            toVisit.push_back(*it);
        }
    }
}

bool LoggerLess::operator()(const Logger::ptr &lhs,
                            const Logger::ptr &rhs) const {
    return lhs->m_name < rhs->m_name;
}

Logger::Logger()
    : m_name(":"), m_level(Log::Level::INFO), m_inheritSinks(false) {}

Logger::Logger(const std::string &name, Logger::ptr parent)
    : m_name(name),
      m_parent(parent),
      m_level(Log::Level::INFO),
      m_inheritSinks(true) {}

Logger::~Logger() {
    m_level = Log::Level::NONE;
    clearSinks();
}

bool Logger::enabled(Log::Level level) {
    return level == Log::Level::FATAL || m_level >= level;
}

void Logger::level(Log::Level level, bool propagate) {
    m_level = level;
    if (propagate) {
        for (std::set<Logger::ptr, LoggerLess>::iterator it(m_children.begin());
             it != m_children.end(); ++it) {
            (*it)->level(level);
        }
    }
}

void Logger::removeSink(LogSink::ptr sink) {
    std::list<LogSink::ptr>::iterator it =
        std::find(m_sinks.begin(), m_sinks.end(), sink);
    if (it != m_sinks.end())
        m_sinks.erase(it);
}

void Logger::log(Log::Level level, const std::string &str, const char *file,
                 int line) {
    if (str.empty() || !enabled(level))
        return;

    auto now = std::chrono::system_clock::now();
    Logger::ptr _this = shared_from_this();
    tid_t thread = gettid();
    bool somethingLogged = false;
    while (_this) {
        for (std::list<LogSink::ptr>::iterator it(_this->m_sinks.begin());
             it != _this->m_sinks.end(); ++it) {
            somethingLogged = true;
            (*it)->log(m_name, now, thread, level, str, file, line);
        }
        if (!_this->m_inheritSinks)
            break;
        _this = _this->m_parent.lock();
    }
}

LogEvent::~LogEvent() { m_logger->log(m_level, m_os.str(), m_file, m_line); }

static const char *levelStrs[] = {
    "NONE", "FATAL", "ERROR", "WARNG", "INFOR", "VERBO", "DEBUG", "TRACE",
};

constexpr auto kMicroSecondsPerSecond = 1000000;

std::string toFormattedString(long long microseconds,
                              bool showMicroseconds = true) {
    char buf[64] = {0};
    time_t seconds = static_cast<time_t>(microseconds / kMicroSecondsPerSecond);
    struct tm tm_time;
    gmtime_r(&seconds, &tm_time);

    if (showMicroseconds) {
        int microseconds =
            static_cast<int>(microseconds % kMicroSecondsPerSecond);
        snprintf(buf, sizeof(buf), "%4d%02d%02d %02d:%02d:%02d.%06d",
                 tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
                 tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec, microseconds);
    } else {
        snprintf(buf, sizeof(buf), "%4d%02d%02d %02d:%02d:%02d",
                 tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
                 tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec);
    }
    return buf;
}

std::ostream &operator<<(std::ostream &os, Log::Level level) {
    assert(level >= Log::Level::FATAL && level <= Log::Level::TRACE);
    return os << levelStrs[static_cast<int>(level)];
}

std::ostream &operator<<(std::ostream &os, system_time_point timestamp) {
    auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(
                            timestamp.time_since_epoch())
                            .count();
    return os << toFormattedString(microseconds);
}

tid_t gettid() { return syscall(__NR_gettid); }

} // namespace Mordor
