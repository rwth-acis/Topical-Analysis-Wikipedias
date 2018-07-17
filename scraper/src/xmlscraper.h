#ifndef XMLSCRAPER_H
#define XMLSCRAPER_H

#include <stdio.h>
#include <iostream>
#include <vector>
#include <unordered_map>
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
    // returns number of category links
    int scrapeCategories();
    int scrapeArticles();

    // creates hashmap with categories and assosiated ids
    int createHashmap();

    // create xml file with only categories/articles
    // returns number of added elements, *(-1) if file ended unexpectedly
    int filterCategories(int fileNr);
    int filterArticles(int fileNr);

    // create file with missing articles/categories
    int missingArticles();
    int missingCategories();
private:

    // specific function to get category links
    int writeCategoryToBuffer(FILE* ipFile, DynamicBuffer* buffer, const std::vector<char> stopChars, bool inBody, bool* knownTemplate);
    std::vector<int>* writeCategoryToBuffer(std::unordered_map<int,std::string>::const_iterator categoryLinks);
    // specific function to write body of article/category to buffer
    int writeArticle(FILE* ipFile, DynamicBuffer* buffer);
    int writeCategory(FILE* ipFile, DynamicBuffer* buffer);
    // function to print warnings/errors
    void warning(std::string);
    void error(std::string);
};

#endif // XMLSCRAPER_H
