#pragma once

#include <string>


class PSLogger
{
public:

    enum class LogType
    {
        None,
        VerboseLog,
        Log,
        Warning,
        Error,
        Critical,
    };

    virtual ~PSLogger(){};

    bool only_log_errors = false;

    virtual void log(LogType p_type, std::string p_category, std::string p_msg);
};