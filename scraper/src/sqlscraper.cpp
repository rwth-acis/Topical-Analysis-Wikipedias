#include "sqlscraper.h"
#include "util/loggingutil.h"
#include <unordered_map>
// file paths
#define EN_HASHMAP_PATH "../../media/english/index-pages.txt"
#define EN_LINKMAP_PATH "../../media/english/csvFiles/category_links/linkmap"
#define EN_CATEGORYLINKS_PATH "../../media/english/rawFiles/enwiki-20180701-categorylinks.sql"
#define EN_LINKFILE_SIZE 18137842743
#define VI_HASHMAP_PATH "../../media/vietnamese/index-pages.txt"
#define VI_LINKMAP_PATH "../../media/vietnamese/csvFiles/linkmap"
#define VI_CATEGORYLINKS_PATH "../../media/vietnamese/rawFiles/viwiki-20180701-categorylinks.sql"
#define VI_LINKFILE_SIZE 1037325185
#define EN_CATEGORY_TAG "Category:"
#define VI_CATEGORY_TAG "Thể loại:"
#define SQL_TABLE_START "VALUES"
#define TITLE_LENGTH 256
#define USERGROUP_SIZE 2027235
// language codes
#define LNG_EN "en"
#define LNG_VI "vi"

using namespace std;
SQLScraper::SQLScraper(const char* logfile)
{
    // create logfile
    parser = ParsingUtil();
    this->logfile = logfile;
    FILE* pFile = fopen(logfile, "w");
    fclose(pFile);
}

SQLScraper::SQLScraper(const char *ownLogfile, const char* parserLogfile)
{
    // create logfile
    parser = ParsingUtil(parserLogfile);
    this->logfile = ownLogfile;
    FILE* pFile = fopen(ownLogfile, "w");
    fclose(pFile);
}

size_t SQLScraper::createLinkmap(language lng)
{
    // load hashmap
    string fPath, pHashmap, pLinkmap, category_tag;
    size_t file_size = 0;
    switch (lng)
    {
        case EN:
            category_tag = EN_CATEGORY_TAG;
            file_size = EN_LINKFILE_SIZE;
            pHashmap = EN_HASHMAP_PATH;
            pLinkmap = EN_LINKMAP_PATH;
            fPath = EN_CATEGORYLINKS_PATH;
            break;
        case VI:
            category_tag = VI_CATEGORY_TAG;
            file_size = VI_LINKFILE_SIZE;
            pHashmap = VI_HASHMAP_PATH;
            pLinkmap = VI_LINKMAP_PATH;
            fPath = VI_CATEGORYLINKS_PATH;
            break;
        default:
            category_tag = EN_CATEGORY_TAG;
            file_size = EN_LINKFILE_SIZE;
            pHashmap = EN_HASHMAP_PATH;
            pLinkmap = EN_LINKMAP_PATH;
            fPath = EN_CATEGORYLINKS_PATH;
            break;
    }
    cout << "Loading " << pHashmap << endl;
    unordered_map<string,int>* hashmap = parser.parseHashmap(pHashmap.c_str());
    unordered_map<int,string>* reverse_hashmap = parser.parseReverseHashmap(pHashmap.c_str());
    if (!hashmap)
        return 0;

    // try to open file
    FILE* ipFile = LoggingUtil::openFile(fPath, false);
    if (!ipFile)
    {
        delete hashmap;
        return 0;
    }
    // initialize linkmap
    cout << "Reading category links from file " << fPath << endl;
    vector<char> stopChars = { ',', '\'' };
    vector<vector<int>>* linkmap = new vector<vector<int>>();

    // get to beginning of entry
    parser.findPattern(ipFile, SQL_TABLE_START);
    int prog = 0;
    while (!parser.findSQLPattern(ipFile, "("))
    {
        // compute progress indicator
        if (((ftell(ipFile)*20)/file_size) > prog || (file_size - ftell(ipFile) < TITLE_LENGTH && prog < 20))
        {
            prog++;
            cout << "Read " << prog*5 << "% of linkfile\n";
        }
        // get id of page
        int id_from = stoi(parser.writeToString(ipFile, stopChars));
        if (!parser.findPattern(ipFile, stopChars))
        {
            LoggingUtil::warning("File " + fPath + " ended unexpectedly at page " + to_string(id_from), logfile);
            cout << "Got " << linkmap->size() << " links" << endl;
            delete hashmap;
            size_t count = linkmap->size();
            delete linkmap;
            return count;
        }
        // get title of category
        char c;
        bool escape = false;
        DynamicBuffer titleBuffer(TITLE_LENGTH);
        titleBuffer.write(category_tag);

        // title end is identified by ' character
        while (!((c = fgetc(ipFile)) == '\'' && !escape))
        {
            if (c == EOF)
            {
                LoggingUtil::warning("File " + fPath + " ended unexpectedly while reading " + titleBuffer.print(), logfile);
                cout << "Got " << linkmap->size() << " links"  << endl;
                delete hashmap;
                size_t count = linkmap->size();
                delete linkmap;
                return count;
            }

            // replace _ with spaces, dont print '\'
            if (c == '_')
                titleBuffer.write(' ');
            else if (c != '\\' || escape)
                titleBuffer.write(c);

            // adjust escaping
            if (c != '\\')
                escape = false;
            else
                escape = !escape;
        }
        // get id of category
        int id_to = 0;
        unordered_map<string,int>::const_iterator catId = hashmap->find(titleBuffer.print());
        if (catId != hashmap->end())
            id_to = catId->second;
        else
        {
            LoggingUtil::warning("No match for category " + titleBuffer.print(), logfile);
            continue;
        }

        // go to end of entry
        if (parser.findSQLPattern(ipFile, "),") == -1)
        {
            LoggingUtil::warning("File " + fPath + " ended unexpectedly at page " + to_string(id_from) + " after reading category " + titleBuffer.print(), logfile);
            cout << "Got " << linkmap->size() << " links" << endl;
            this->writeLinkmap(linkmap, pLinkmap.c_str(), lng);
            delete hashmap;
            size_t count = linkmap->size();
            delete linkmap;
            return count;
        }

        // get namespace of page
        // only add article/category edges to save RAM
        unordered_map<int,string>::const_iterator pageNs = reverse_hashmap->find(id_from);
        if (pageNs == reverse_hashmap->end())
            continue;
        int ns = ParsingUtil::reasonableStringCompare(pageNs->second, category_tag) ? 14 : 0;

        // save entry to linkmap
        linkmap->push_back({id_from,id_to,ns});
    }
    cout << "Got " << linkmap->size() << " links" << endl;
    this->writeLinkmap(linkmap, pLinkmap.c_str(), lng);
    delete hashmap;
    size_t count = linkmap->size();
    delete linkmap;
    return count;
}

void SQLScraper::writeLinkmap(vector<vector<int>>* linkmap, const char* path, language lng)
{
    string lngCode = "";
    switch (lng)
    {
        case EN:
            lngCode = LNG_EN;
            break;
        case VI:
            lngCode = LNG_VI;
            break;
        default:
            lngCode = LNG_EN;
            break;
    }

    size_t pageCount = 1;
    size_t count = 0;
    string fPath = path + to_string(pageCount) + ".csv";
    FILE* pFile = LoggingUtil::openFile(fPath, true);
    fputs("_from,_to,type\n", pFile);
    if (linkmap->begin() == linkmap->end())
        LoggingUtil::error("Empty linkmap", logfile);
    for (size_t i = 0; i < linkmap->size(); i++)
    {
        fprintf(pFile, "\"%sPages/%d\",\"%sPages/%d\",\"%d\"\n", lngCode.c_str(), linkmap->at(i)[0], lngCode.c_str(), linkmap->at(i)[1], linkmap->at(i)[2]);
        count++;

        // after 1million links open new file
        if (count >= 1000000)
        {
            fclose(pFile);
            pageCount++;
            count = 0;
            fPath = path + to_string(pageCount) + ".csv";
            pFile = fopen(fPath.c_str(), "r");
            if (pFile)
            {
                cout << "Overwriting file " << fPath << endl;
                fclose(pFile);
            }
            else
                cout << "Writing file " << fPath << endl;
            pFile = fopen(fPath.c_str(), "w");
            fputs("_from,_to,type\n", pFile);
        }
    }
    fclose(pFile);
    cout << "Got " << to_string(1000000*(pageCount-1)+count) << " links in " << pageCount << " pages\n";
    return;
}

int SQLScraper::sqlToCSV(const char* iPath, const char* oPath)
{
    FILE* ipFile = LoggingUtil::openFile(iPath, false);
    if (!ipFile)
        return -1;
    FILE* opFile = LoggingUtil::openFile(oPath, true);
    char c;
    while ((c = fgetc(ipFile)) != EOF)
        fputc(c, opFile);
    return 322;
    int count = 0;
    while (!parser.findPattern(ipFile,SQL_TABLE_START))
    {
        if (parser.writeSQLToFile(ipFile, opFile, ";") == -1)
            LoggingUtil::warning("File " + string(iPath) + " ended unexpectedly", logfile);
        fputc('\n', opFile);
        count++;
    }
    fclose(opFile);
    return count;
}
