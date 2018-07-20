#ifndef SQLSCRAPER_H
#define SQLSCRAPER_H

#include <stdio.h>
#include "util/parsingutil.h"

class SQLScraper
{
    const char* logfile;
    ParsingUtil parser;
    int writeLinkmap(std::unordered_map<int,int>* linkmap, const char* path);
public:
    SQLScraper(const char *logfile);
    SQLScraper(const char *ownLogfile, const char* parserLogfile);

    // adding line break after each entry
    int makeReadable(const char* iPath, const char* oPath, int numAttributes);
    int splitFiles(const char* iPath, const char* oPath);

    // does what it says
    int sqlToCSV(const char* iPath, const char* oPath);

    // parse file with category links to hashmap
    std::unordered_map<int,int>* createLinkmap(const char* fPath);
};

#endif // SQLSCRAPER_H
