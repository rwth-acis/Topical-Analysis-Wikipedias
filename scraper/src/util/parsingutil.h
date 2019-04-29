#ifndef PARSINGUTIL_H
#define PARSINGUTIL_H

#include <stdio.h>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include "buffer.h"
#include "loggingutil.h"

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
    // looks for pattern in line
    int findPatternInLine(FILE* ipFile, const std::string stopStr);
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
    int writeEscapedToBuffer(FILE* ipFile, DynamicBuffer* buffer, const char stopChar);
    std::string writeToString(FILE* ipFile, const char stopChar);
    std::string writeToString(FILE* ipFile, const std::vector<char> stopChars);
    std::string writeToString(DynamicBuffer* buffer, size_t offset, const std::vector<char> stopChars);
    // function for parsing of a link-/hashmap/botset stored in given path
    std::unordered_map<std::string,int>* parseHashmap(const char* path);
    std::unordered_map<int,std::string>* parseReverseHashmap(const char* path);
    std::unordered_map<int,std::string>* parseLinkmap(const char* path);
    std::unordered_set<std::string>* parseBotSet(const char* path);
    // function for reading categories to list from string
    std::vector<int>* writeCategoryToBuffer(std::unordered_map<int,std::string>::const_iterator categoryLinks);
    // function listing the amount of appearences of pageIds in the given file
    std::unordered_map<size_t,size_t>* countPages(const char* path);

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
            if (state >= two.length())
                return true;
        }
        return false;
    }
    static std::string csv2json(const char* path)
    {
        FILE* ipFile = LoggingUtil::openFile(path, false);
        if (!ipFile)
            return "Invalid file path";
        std::vector<std::string> attributes;
        char c;
        std::string attr = "";
        // get attributes
        while ((c = fgetc(ipFile)) != '\n')
        {
            if (c == EOF)
            {
                LoggingUtil::error("Nice file, jackass!");
                return "Invalid file";
            }
            if (c == ',')
            {
                attributes.push_back(attr);
                attr = "";
            }
            else
                attr += c;
        }
        attributes.push_back(attr);
        attr = "";
        // write to output file
        FILE* opFile = LoggingUtil::openFile("output.json", true);
        fprintf(opFile, "[\n{\"%s\":", attributes[0].c_str());
        size_t pos = 0;
        size_t count = 0;
        bool newLine = true;
        while ((c = fgetc(ipFile)) != EOF)
        {
            if (c == '\n')
                newLine = true;
            if (newLine)
            {
                if (pos+1 < attributes.size())
                    LoggingUtil::warning(std::to_string(count) + ": Expected " + std::to_string(attributes.size()) + " columns, got " + std::to_string(pos+1));
                fprintf(opFile, "},\n{\"%s\":", attributes[0].c_str());
                pos = 0;
                count++;
                newLine = false;
            }
            else if (c == ',')
            {
                pos++;
                fprintf(opFile, ",\"%s\":", attributes[pos].c_str());
            }
            else
                fputc(c, opFile);
        }
        fputs("}\n]", opFile);
        fclose(opFile);
        return "output.json";
    }
    // return vector of numbers extracted from given string seperated by given delimiter
    static std::vector<size_t> stringToNumbers(std::string str, char del)
    {
        char c;
        std::string item;
        std::vector<std::size_t> res;
        for (size_t i = 0; i < str.length(); i++)
        {
            c = str[i];
            // add item if c == delimiter char
            if (c == del)
            {
                size_t entry = 0;
                try{entry = std::stol(item);}
                catch(const std::invalid_argument){std::cerr << "Invalid entry " << item << std::endl;}
                res.push_back(entry);
                item = "";
                continue;
            }
            item += c;
        }
        size_t entry = 0;
        try{entry = std::stol(item);}
        catch(const std::invalid_argument){std::cerr << "Invalid entry " << item << std::endl;}
        res.push_back(entry);
        return res;
    }
    // return vector of strings extracted from given string seperated by given delimiter
    static std::vector<std::string> splitString(std::string str, char del)
    {
        char c;
        std::string item = "";
        std::vector<std::string> res;
        for (size_t i = 0; i < str.length(); i++)
        {
            c = str[i];
            // add item if c == delimiter char
            if (c == del)
            {
                res.push_back(item);
                item = "";
                continue;
            }
            item += c;
        }
        res.push_back(item);
        return res;
    }
    // return given string with all upper case letters
    static std::string capitalize(std::string str)
    {
        std::string res = "";
        for (unsigned short i = 0; i < str.size(); i++)
            res += std::toupper(str[i]);
        return res;
    }
    // return given string with all lower case letters
    static std::string decapitalize(std::string str)
    {
        std::string res = "";
        for (unsigned short i = 0; i < str.size(); i++)
            res += std::tolower(str[i]);
        return res;
    }
};

#endif // PARSINGUTIL_H
