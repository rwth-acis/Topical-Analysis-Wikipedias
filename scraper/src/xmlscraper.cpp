#include "xmlscraper.h"
#include "util/loggingutil.h"
#include <unordered_map>

// path to arangoDB
#define BASE_PATH "http://localhost:8529//_db/_system/wikipics/"
// path to minimized files
#define ARTICLES_PATH(x) "../../media/english/index-articles(" + to_string(x) + ").txt"
#define CATGORIES_PATH "../../media/english/index-categories.txt"
// path to json files with categories/articles/category links
#define JSON_CATEGORY_PATH "../../media/english/jsonFiles/categories.json"
#define JSON_ARTICLE_PATH(x) "../../media/english/jsonFiles/articles(" + to_string(x) + ").json"
#define JSON_CATEGORYLINK_PATH "../../media/english/jsonFiles/categoryLinks.json"
// path to linkmap
#define LINKMAP_PATH "../../media/english/linkmap.json"
// sizes for progress estimation
#define ARTICLES_SIZE 669749173
#define CATEGORIES_SIZE 69860671
// approximate initial sizes for buffers
#define TITLE_LENGTH 128
#define PATH_LENGTH 256
// maximum amount of numbers in an int
#define MAX_INT_LENGTH 10
// holds content from <title> to <ns>
#define BACKLOG_SIZE (TITLE_LENGTH + 17)
// holds conent from </ns> to <text>
#define LARGE_BACKLOG 1024
// maximum size allowed for buffers
#define MAX_BUFFER_SIZE 65536
#define ACTUAL_BUFFER_LIMIT 1048576
// "bad category" ids
#define HIDDEN 15961454
#define CONTAINER 30176254
#define TEMPLATE 49310588
#define MAINTENANCE 738040
#define TRACKING 7361045
#define STUB 869270
#define REDIRECT 30334744
#define DISAMBIGUATION 19204864

using namespace std;
XMLScraper::XMLScraper(const char* logfile)
{
    this->stopChars = {'\"','<','>','|','[',']','{','}'};
    this->logfile = logfile;
    parser = ParsingUtil();

    // flush logfile
    FILE* pFile = fopen(logfile, "w");
    fclose(pFile);
}

XMLScraper::XMLScraper(const char* parserLogging, const char* ownLogging)
{
    this->stopChars = {'\"','<','>','|','[',']','{','}'};
    this->logfile = ownLogging;
    parser = ParsingUtil(logfile);

    // flush logfiles
    FILE* pFile = fopen(parserLogging, "w");
    fclose(pFile);
    pFile = fopen(ownLogging, "w");
    fclose(pFile);
}

void XMLScraper::scrapeCategories()
{
    // get index file
    FILE* ipFile = fopen(CATGORIES_PATH, "r");
    if (!ipFile)
    {
        cerr << "Could not open file " CATGORIES_PATH << endl;
        return;
    }
    fclose(ipFile);

    // load linkmap
    cout << "Load linkmap from " << LINKMAP_PATH << endl;
    unordered_map<int,string>* linkmap = parser.parseLinkmap(LINKMAP_PATH);
    if (linkmap == NULL)
    {
        cerr << "Error loading linkmap" << endl;
    }

    // create output files
    FILE* opFile = fopen(JSON_CATEGORY_PATH, "r");
    if (opFile)
    {
        cout << "Overwriting file " << JSON_CATEGORY_PATH << endl;
        fclose(opFile);
        opFile = fopen(JSON_CATEGORY_PATH, "w");
        fputc('[', opFile);
        fclose(opFile);
    }
    else
    {
        cout << "Creating output file for categories at " << JSON_CATEGORY_PATH << endl;
        opFile = fopen(JSON_CATEGORY_PATH, "w");
        fputc('[', opFile);
        fclose(opFile);
    }

    opFile = fopen(JSON_CATEGORYLINK_PATH, "r");
    if (opFile)
    {
        cout << "Overwriting file " << JSON_CATEGORYLINK_PATH << endl;
        fclose(opFile);
        opFile = fopen(JSON_CATEGORYLINK_PATH, "w");
        fputc('[', opFile);
        fclose(opFile);
    }
    else
    {
        cout << "Creating output file for the edges at " << JSON_CATEGORYLINK_PATH << endl;
        opFile = fopen(JSON_CATEGORYLINK_PATH, "w");
        fputc('[', opFile);
        fclose(opFile);
    }

    ipFile = fopen(CATGORIES_PATH, "r");
    // buffer line
    DynamicBuffer lineBuffer(LARGE_BACKLOG);
    lineBuffer.setMax(MAX_BUFFER_SIZE);
    int res, catCount = 0, linkCount = 0, prog = 0;
    while ((res = parser.writeToBuffer(ipFile, &lineBuffer, '|')))
    {
        // compute progress indicator
        if (((ftell(ipFile)*10)/CATEGORIES_SIZE) > prog || (CATEGORIES_SIZE - ftell(ipFile) < LARGE_BACKLOG && prog < 10))
        {
            prog++;
            cout << "Read " << prog*10 << "% of categories\n";
        }
        // get id
        int id = stoi(parser.writeToString(&lineBuffer, 0, {':'}));

        // get title
        size_t idx = parser.findPattern(&lineBuffer, ":") + 1;
        string title = lineBuffer.print(idx);
        lineBuffer.flush();

        // get categories
        bool hasCategories = true;
        vector<int>* catBuffer;
        unordered_map<int,string>::const_iterator categories = linkmap->find(id);
        if (categories == linkmap->end())
        {
            hasCategories = false;
            LoggingUtil::warning("No match for category " + title, logfile);
        }
        if (hasCategories)
            catBuffer = parser.writeCategoryToBuffer(categories);

        // check for "bad" categories
        if (hasCategories&&!this->isTopical(catBuffer))
        {
            LoggingUtil::warning("Category " + title + " not topical", logfile);
            delete catBuffer;
            continue;
        }

        // print category
        opFile = fopen(JSON_CATEGORY_PATH, "a");
        if (catCount)
            fputc(',', opFile);
        fprintf(opFile, "\n{\"title\":\"%s\",\"_key\":\"%d\"}", title.c_str(), id);
        catCount++;
        fclose(opFile);

        // print category links
        if (hasCategories)
        {
            opFile = fopen(JSON_CATEGORYLINK_PATH, "a");
            for (int i = 0; i < catBuffer->size(); i++)
            {
                if (linkCount)
                    fputc(',', opFile);
                fprintf(opFile, "\n{\"_from\":\"enCategories/%d\",\"to\":\"enCategories/%d\"}", catBuffer->at(i), id);
                linkCount++;
            }
            fclose(opFile);
            delete catBuffer;
        }
    }
    opFile = fopen(JSON_CATEGORY_PATH, "a");
    fputs("\n]\n", opFile);
    fclose(opFile);
    opFile = fopen(JSON_CATEGORYLINK_PATH, "a");
    fputs("\n]\n", opFile);
    fclose(opFile);
    cout << "Got " << catCount << " categories with " << linkCount << " links\n";
    return;
}

void XMLScraper::scrapeArticles(int fileNr)
{
    // get index file
    string iPath = ARTICLES_PATH(fileNr);
    FILE* ipFile = fopen(iPath.c_str(), "r");
    if (!ipFile)
    {
        cerr << "Could not open file " << iPath << endl;
        return;
    }
    fclose(ipFile);

    // load linkmap
    unordered_map<int,string>* linkmap = parser.parseLinkmap(LINKMAP_PATH);

    // create output files
    string oPath = JSON_ARTICLE_PATH(fileNr);
    FILE* opFile = fopen(oPath.c_str(), "r");
    if (opFile)
    {
        cout << "Overwriting file " << oPath << endl;
        fclose(opFile);
        opFile = fopen(oPath.c_str(), "w");
        fputc('[', opFile);
        fclose(opFile);
    }
    else
    {
        cout << "Creating output file for articles at " << oPath << endl;
        opFile = fopen(oPath.c_str(), "w");
        fputc('[', opFile);
        fclose(opFile);
    }

    ipFile = fopen(iPath.c_str(), "r");
    // buffer line
    DynamicBuffer lineBuffer(LARGE_BACKLOG);
    int res, count = 0, prog = 0;
    while ((res = parser.writeToBuffer(ipFile, &lineBuffer, '|')))
    {
        // compute progress indicator
        if (((ftell(ipFile)*10)/(ARTICLES_SIZE/3)) > prog || ((ARTICLES_SIZE/3) - ftell(ipFile) < LARGE_BACKLOG && prog < 10))
        {
            prog++;
            cout << "Read " << prog*10 << "% of categories\n";
        }
        // get id
        size_t idx = parser.findPattern(&lineBuffer, ":") + 1;
        int id = stoi(parser.writeToString(&lineBuffer, idx, {':'}));

        // get title
        idx = parser.findPattern(&lineBuffer, idx, ":") + 1;
        string title = lineBuffer.print(idx);
        lineBuffer.flush();

        // get categories
        bool hasCategories = true;
        vector<int>* catBuffer;
        unordered_map<int,string>::const_iterator categories = linkmap->find(id);
        if (categories == linkmap->end())
        {
            hasCategories = false;
            LoggingUtil::warning("No match for article " + title, logfile);
        }

        if (hasCategories)
            catBuffer = parser.writeCategoryToBuffer(categories);

        // check for "bad" categories
        if (hasCategories && !this->isArticle(catBuffer, linkmap))
        {
            LoggingUtil::warning("Article " + title + " not topical", logfile);
            delete catBuffer;
            continue;
        }
        // print article
        opFile = fopen(oPath.c_str(), "a");
        if (count)
            fputc(',', opFile);
        fprintf(opFile, "\n{\"title\":\"%s\",\"_key\":\"%d\",\"categories\":[", title.c_str(), id);
        for (int i = 0; (hasCategories && i < catBuffer->size()); i++)
        {
            if (i)
                fputc(',', opFile);
            fprintf(opFile, "\"enCategories/%d\"", catBuffer->at(i));
        }
        fputs("]}", opFile);
        fclose(opFile);
        count++;
        delete catBuffer;
    }
    cout << "Got " << count << " articles\n";
    return;
}

bool XMLScraper::isArticle(vector<int>* categories, unordered_map<int,string>* linkmap)
{
    for (int i = 0; i < categories->size(); i++)
    {
        // check for disambiguation/redirect
        if (categories->at(i) == DISAMBIGUATION || categories->at(i) == REDIRECT)
            return false;
        // get subcategories
        unordered_map<int,string>::const_iterator subCats = linkmap->find(categories->at(i));
        if (subCats == linkmap->end())
            continue;
        vector<int>* subCatBuffer = parser.writeCategoryToBuffer(subCats);
        // check for redirect
        for (int j = 0; j < subCatBuffer->size(); j++)
            if (subCatBuffer->at(j) == REDIRECT)
                return false;
    }
    return true;
}

bool XMLScraper::isTopical(vector<int>* categories)
{
    // check against "bad categories"
    for (int i = 0; i < categories->size(); i++)
        if (categories->at(i) == MAINTENANCE || categories->at(i) == TEMPLATE || categories->at(i) == CONTAINER || categories->at(i) == HIDDEN || categories->at(i)  == TRACKING || categories->at(i) == REDIRECT || categories->at(i) == STUB)
            return false;
    return true;
}
