#pragma once

namespace opustags {

    enum class LogLevel {
        LOG_NORMAL = 0,
        LOG_VERBOSE = 1,
        LOG_DEBUG = 2,
        LOG_DEBUG_EXTRA = 3,
    };

    class Log
    {
    public:
        Log(std::ostream &out);

        LogLevel level;

        Log& operator<<(LogLevel lvl);
        template<class T> Log& operator<<(const T&);

    private:
        std::ostream &out;
    };


    extern Log log;

}
