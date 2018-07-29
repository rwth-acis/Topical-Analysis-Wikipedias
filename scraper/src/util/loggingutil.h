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


    static FILE* openFile(std::string path, bool write)
    {
        // open output file
        FILE* pFile;
        if (write)
        {
            pFile = fopen(path.c_str(), "r");
            if (pFile)
            {
                // overwrite existing file
                cout << "Overwriting file " << path << endl;
                fclose(pFile);
            }
            else
                // create new output file
                cout << "Creating output file " << path << endl;
            pFile = fopen(path.c_str(), "w");
        }
        else
        {
            pFile = fopen(path.c_str(), "r");
            if (!ipFile)
                cerr << "Could not open file " << fPath << endl;
        }
        return pFile;
    }
};

#endif // LOGGINGUTIL_H
