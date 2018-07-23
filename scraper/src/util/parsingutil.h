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
    short findPattern(FILE* ipFile, const std::string stopStrOne, const std::string stopStrTwo);
    char findPattern(FILE* ipFile, const std::vector<char> stopChars);
    size_t findPattern(DynamicBuffer* buffer, const std::string stopStr);
    size_t findPattern(DynamicBuffer* buffer, size_t offset, const std::string stopStr);
    // augmented function for sql files
    int findSQLPattern(FILE* ipFile, const std::string stopStr);
    // check for pattern in buffer
    bool searchPattern(DynamicBuffer* buffer, const std::string stopStr);
    // find pattern but save text to backlog of dynamic size
    int findPattern(FILE *ipFile, const std::string stopStr, DynamicBuffer* backlog);
    int findPattern(FILE *ipFile, const std::string stopStr, DynamicBuffer* backlog, char flushChar);
    int findPattern(FILE *ipFile, const std::string stopStr, DynamicBuffer* backlog, std::vector<char> escapeChars);
    // function to count characters until pattern is encountered
    long countChars(FILE *ipFile, const std::string stopStr);
    // function to write from input to output file until pattern is encountered
    int writeToFile(FILE* ipFile, FILE* opFile, const std::string stopStr);
    int writeToFile(FILE* ipFile, FILE* opFile, const std::vector<char> stopChars);
    // augmented function for sql files
    int writeSQLToFile(FILE* ipFile, FILE* opFile, const std::string stopStr);
    // function to write from input to buffer until pattern is encountered
    int writeToBuffer(FILE* ipFile, DynamicBuffer* buffer, const std::string stopStr);
    int writeToBuffer(FILE* ipFile, DynamicBuffer* buffer, const std::vector<char> stopChars);
    int writeToBuffer(FILE* ipFile, DynamicBuffer* buffer, const char stopChar);
    std::string writeToString(FILE* ipFile, const char stopChar);
    std::string writeToString(FILE* ipFile, const std::vector<char> stopChars);
    std::string writeToString(DynamicBuffer* buffer, size_t offset, const std::vector<char> stopChars);
    // function for parsing of a link-/hashmap stored in given path
    std::unordered_map<std::string,int>* parseHashmap(const char* path);
    std::unordered_map<int,std::string>* parseReverseHashmap(const char* path);
    std::unordered_map<int,std::string>* parseLinkmap(const char* path);
    // function for reading categories to list from string
    std::vector<int>* writeCategoryToBuffer(std::unordered_map<int,std::string>::const_iterator categoryLinks);

    // static string compare function
    static bool reasonableStringCompare(std::string one, std::string two)
    {
        char c;
        size_t pos = 0;
        size_t state = 0;
        while (pos < one.length())
        {
            c = one[pos];
            pos++;
            if (c == (char) toupper(two[state]) || c == (char) tolower(two[state]))
                state++;
            else
                state = 0;
            if (state >= two.size())
                return true;
        }
        return false;
    }
};

#endif // PARSINGUTIL_H
