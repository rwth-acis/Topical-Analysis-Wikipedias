#ifndef LOGGINGUTIL_H
#define LOGGINGUTIL_H

#include <stdio.h>
#include <iostream>

class LoggingUtil
{

public:
    // print error/warning to std::out
    static void warning(std::string str)
    {
        std::cout << "WARNING: " << str << std::endl;
    }
    static void error(std::string str)
    {
        std::cout << "ERROR: " << str << std::endl;
    }

    // print error/warning to file
    static void error(std::string str, const char* path)
    {
        if (!path)
            LoggingUtil::error(str);
        FILE* pFile = fopen(path, "a");
        fputs("ERROR: ", pFile);
        fputs(str.c_str(), pFile);
        fputc('\n', pFile);
        fclose(pFile);
    }
    static void warning(std::string str, const char* path)
    {
        if (!path)
            LoggingUtil::warning(str);
        FILE* pFile = fopen(path, "a");
        fputs("WARNING: ", pFile);
        fputs(str.c_str(), pFile);
        fputc('\n', pFile);
        fclose(pFile);
    }
};

#endif // LOGGINGUTIL_H
