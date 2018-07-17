#ifndef PARSINGUTIL_H
#define PARSINGUTIL_H

#include <stdio.h>
#include <vector>
#include <unordered_map>
#include "buffer.h"

class ParsingUtil
{
    // should be logging?
    bool logging;
    const char* logfile;
public:
    ParsingUtil();
    ParsingUtil(const char* logging);

    // function to set file pointer to next apperance of pattern
    int findPattern(FILE* ipFile, const std::string stopStr);
    char findPattern(FILE* ipFile, const std::vector<char> stopChars);
    // augmented function for sql files
    int findSQLPattern(FILE* ipFile, const std::string stopStr);
    // check for pattern in buffer
    bool findPattern(DynamicBuffer* buffer, const std::string stopStr);
    // find pattern but save text to backlog of dynamic size
    int findPattern(FILE *ipFile, const std::string stopStr, DynamicBuffer* backlog);
    int findPattern(FILE *ipFile, const std::string stopStr, DynamicBuffer* backlog, std::vector<char> escapeChars);
    // function to write from input to output file until pattern is encountered
    int writeToFile(FILE* ipFile, FILE* opFile, const std::string stopStr);
    int writeToFile(FILE* ipFile, FILE* opFile, const std::vector<char> stopChars);
    // augmented function for sql files
    int writeSQLToFile(FILE* ipFile, FILE* opFile, const std::string stopStr);
    // function to write from input to buffer until pattern is encountered
    int writeToBuffer(FILE* ipFile, DynamicBuffer* buffer, const std::string stopStr);
    int writeToBuffer(FILE* ipFile, DynamicBuffer* buffer, const std::vector<char> stopChars);
    std::string writeToString(FILE* ipFile, const std::vector<char> stopChars);
    // function for parsing of a link-/hashmap stored in given path
    std::unordered_map<std::string,int>* parseHashmap(const char* path);
    std::unordered_map<int,std::string>* parseLinkmap(const char* path);
};

#endif // PARSINGUTIL_H
