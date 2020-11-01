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

    LogType log_verbosity = LogType::Warning;

    virtual void log(LogType p_type, std::string p_category, std::string p_msg);
};
