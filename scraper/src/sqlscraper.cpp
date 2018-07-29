#include "sqlscraper.h"
#include "util/loggingutil.h"
#include <unordered_map>

#define HASHMAP_PATH "../../media/english/index-pages.txt"
#define LINKMAP_PATH "../../media/english/jsonFiles/linkmap"
#define BOTSET_PATH "../../media/english/botset.csv"
#define SQL_TABLE_START "VALUES"
#define TITLE_LENGTH 256
#define LINKFILE_SIZE 18137842743
#define USERGROUP_SIZE 2027235

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

size_t SQLScraper::createLinkmap(const char* fPath)
{
    // load hashmap
    cout << "Loading " << HASHMAP_PATH << endl;
    unordered_map<string,int>* hashmap = parser.parseHashmap(HASHMAP_PATH);
    unordered_map<int,string>* reverse_hashmap = parser.parseReverseHashmap(HASHMAP_PATH);
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
        if (((ftell(ipFile)*20)/LINKFILE_SIZE) > prog || (LINKFILE_SIZE - ftell(ipFile) < TITLE_LENGTH && prog < 20))
        {
            prog++;
            cout << "Read " << prog*5 << "% of linkfile\n";
        }
        // get id of page
        int id_from = stoi(parser.writeToString(ipFile, stopChars));
        if (!parser.findPattern(ipFile, stopChars))
        {
            LoggingUtil::warning("File " + string(fPath) + " ended unexpectedly at page " + to_string(id_from), logfile);
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
        titleBuffer.write("Category:");

        // title end is identified by ' character
        while (!((c = fgetc(ipFile)) == '\'' && !escape))
        {
            if (c == EOF)
            {
                LoggingUtil::warning("File " + string(fPath) + " ended unexpectedly while reading " + titleBuffer.print(), logfile);
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
            LoggingUtil::warning("File " + string(fPath) + " ended unexpectedly at page " + to_string(id_from) + " after reading category " + titleBuffer.print(), logfile);
            cout << "Got " << linkmap->size() << " links" << endl;
            this->writeLinkmap(linkmap, LINKMAP_PATH);
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
        int ns = ParsingUtil::reasonableStringCompare(pageNs->second, "Category:") ? 14 : 0;

        // save entry to linkmap
        linkmap->push_back({id_from,id_to,ns});
    }
    cout << "Got " << linkmap->size() << " links" << endl;
    this->writeLinkmap(linkmap, LINKMAP_PATH);
    delete hashmap;
    size_t count = linkmap->size();
    delete linkmap;
    return count;
}

void SQLScraper::writeLinkmap(vector<vector<int>>* linkmap, const char* path)
{
    size_t pageCount = 1;
    size_t count = 0;
    string fPath = path + to_string(pageCount) + ".csv";
    FILE* pFile = LoggingUtil::openFile(path, true);
    fputs("_from,_to,type\n", pFile);
    if (linkmap->begin() == linkmap->end())
        LoggingUtil::error("Empty linkmap", logfile);
    for (size_t i = 0; i < linkmap->size(); i++)
    {
        fprintf(pFile, "\"enPages/%d\",\"enPages/%d\",\"%d\"\n", linkmap->at(i)[0], linkmap->at(i)[1], linkmap->at(i)[2]);
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

size_t SQLScraper::createBotSet(const char *fPath)
{
    FILE* pFile = LoggingUtil::openFile(fPath, false);
    if (!pFile)
        return 0;
    else
        cout << "Reading bots from file " << fPath << endl;
    unordered_set<size_t>* botset = new unordered_set<size_t>();

    // go to beginning of table
    parser.findPattern(pFile, SQL_TABLE_START);
    while (!parser.findSQLPattern(pFile, "("))
    {
        // get user id
        size_t id = stoi(parser.writeToString(pFile, ','));
        char c = fgetc(pFile);
        if (c != '\'')
            LoggingUtil::warning("Unexpected char " + c, logfile);
        // get user group
        DynamicBuffer groupBuffer = DynamicBuffer(TITLE_LENGTH);
        groupBuffer.setMax(TITLE_LENGTH);
        parser.writeToBuffer(pFile, &groupBuffer, '\'');
        if (ParsingUtil::reasonableStringCompare(groupBuffer.print(), "bot"))
            botset->insert(id);
    }
    this->writeBotSet(botset, BOTSET_PATH);
    size_t count = botset->size();
    delete botset;
    return count;
}

void SQLScraper::writeBotSet(std::unordered_set<size_t> *botset, const char *path)
{
    FILE* pFile = LoggingUtil::openFile(path, true);
    pFile = fopen(path, "w");
    if (botset->begin() == botset->end())
        LoggingUtil::error("Empty botset", logfile);
    for (auto it = botset->cbegin(); it != botset->end(); ++it)
        fprintf(pFile, "%zu\n", *it);
    fclose(pFile);
    cout << "Got " << botset->size() << " bots\n";
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
