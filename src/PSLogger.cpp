
#include <iostream>

#include "PSLogger.hpp"

void PSLogger::log(LogType p_type, std::string p_category, std::string p_msg)
{
    if(p_type < log_verbosity )
    {
        return;
    }

    std::string enum_str = "";
    switch(p_type)
    {
    case LogType::None:
    enum_str = "None";
        break;
    case LogType::VerboseLog:
    enum_str = "VerboseLog";
        break;
    case LogType::Log:
    enum_str = "Log";
        break;
    case LogType::Warning:
    enum_str = "Warning";
        break;
    case LogType::Error:
    enum_str = "Error";
        break;
    case LogType::Critical:
    enum_str = "Critical";
        break;
    default:
    enum_str = "UNKNOWN LOGTYPE";
        break;
    }

    std::cout << enum_str << " : " << p_category << " : " << p_msg << "\n";
}
