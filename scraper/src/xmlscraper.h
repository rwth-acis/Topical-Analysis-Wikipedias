#ifndef XMLSCRAPER_H
#define XMLSCRAPER_H

#include <stdio.h>
#include <iostream>
#include <vector>
#include "util/parsingutil.h"
#include "util/buffer.h"

class XMLScraper
{
    // list of typical function chars in xml/json files
    std::vector<char> stopChars;
    // parser/logger
    ParsingUtil parser;
    const char* logfile;
public:
    XMLScraper(const char* logfile);
    XMLScraper(const char* parserLogging, const char* ownLogging);

    // creates a JSON file containing all categories/articles with category tags
    // tracking/hidden/container/template/maintenance categories are excluded
    // redirect pages are excluded
    size_t scrapePages();
    // creates a CSV file with entries for each revision (reversions excluded)
    // revisionId,pageId,title,userId,username,size,timestamp
    // users without username/id and bots are excluded
    size_t historyToCSV(short fileNr);
    // creates a CSV file with entries for each author within the history files
    // each author is only added once (identified by contributor ID)
    size_t getAuthors();

private:

    // determine if article/category should be added
    bool isTopical(std::vector<int>* categories);
    bool isArticle(std::vector<int>* categories, std::unordered_map<int, std::string>* linkmap);
};

#endif // XMLSCRAPER_H
