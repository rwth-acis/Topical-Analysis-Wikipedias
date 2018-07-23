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

int SQLScraper::makeReadable(const char* iPath, const char* oPath, int numAttributes)
{
    // open input file
    FILE* ipFile = fopen(iPath, "r");
    if (!ipFile)
    {
        cerr << "Could not open file " << iPath << endl;
        return -1;
    }

    // open output file
    FILE* opFile = fopen(oPath, "r");
    if (opFile)
    {
        cout << "Overwriting file " << oPath << endl;
        fclose(opFile);
    }
    else
        cout << "Creating output file " << oPath << endl;
    opFile = fopen(oPath, "w");

    // set file pointer to beginning of entries
    if (parser.findPattern(ipFile, "VALUES") == -1)
        cout << "File ended unexpectedly" << endl;

    // set line breaks after end of entry
    // end of entry is defined if numEntries
    // attributes have been passed
    int count = 0, commaCount = 0;
    char c, p = '0';
    bool inQuotes = false;
    while ((c = fgetc(ipFile)) != EOF)
    {
        fputc(c, opFile);
        // check for single quotes
        if (c == '\'' && p != '\\')
            inQuotes = !inQuotes;
        // commas only count outside of quotes
        if (c == ',' && !inQuotes)
            commaCount++;
        // brackets outside of quotes reset counter
        if ((c == ')' || c == '(')  && p != '\\' && !inQuotes && commaCount < numAttributes-1)
            commaCount = 0;
        if (commaCount >= numAttributes-1)
        {
            // get to end of entry
            if (parser.writeSQLToFile(ipFile, opFile, "),") == -1)
                cout << "File ended unexpectedly" << endl;
            // insert line break, reset comma counter
            fputc('\n', opFile);
            count++;
            commaCount = 0;
        }
        p = c;
    }
    fclose(opFile);
    return count;
}

int SQLScraper::splitFiles(const char *iPath, const char *oPath)
{
    // open input file
    FILE* ipFile = fopen(iPath, "r");
    if (!ipFile)
    {
        cerr << "Could not open file " << iPath << endl;
        return -1;
    }

    int countFiles = 1;
    while (!parser.findPattern(ipFile, "VALUES"))
    {
        // open output file
        string ofPath = string(oPath) + '(' + to_string(countFiles) + ").txt";
        FILE* opFile = fopen(ofPath.c_str(), "r");
        if (opFile)
        {
            cout << "Overwriting file " << ofPath << endl;
            fclose(opFile);
        }
        else
            cout << "Creating output file " << oPath << endl;
        opFile = fopen(ofPath.c_str(), "w");

        // write until semicolon
        if (parser.writeToFile(ipFile, opFile, ");") == -1)
        {
            cout << "File " << iPath << " ended unexpectedly" << endl;
            fclose(opFile);
            return -countFiles;
        }
        countFiles++;
        fclose(opFile);
    }
    return countFiles-1;
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
    FILE* ipFile = fopen(fPath, "r");
    if (!ipFile)
    {
        cerr << "Could not open file " << fPath;
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
    string fPath = path + to_string(pageCount) + ".json";
    FILE* pFile = fopen(fPath.c_str(), "r");
    if (pFile)
    {
        cout << "Overwriting file " << fPath << endl;
        fclose(pFile);
    }
    else
        cout << "Writing file " << fPath << endl;
    pFile = fopen(fPath.c_str(), "w");
    fputc('[', pFile);
    if (linkmap->begin() == linkmap->end())
        LoggingUtil::error("Empty linkmap", logfile);
    for (size_t i = 0; i < linkmap->size(); i++)
    {
        if (count)
            fputs(",", pFile);
        fprintf(pFile, "\n{\"_from\":\"enPages/%d\",\"_to\":\"enPages/%d\",\"type\":\"%d\"}", linkmap->at(i)[0], linkmap->at(i)[1], linkmap->at(i)[2]);
        count++;

        // after 2million links open new file
        if (count >= 2000000)
        {
            fputs("\n]\n", pFile);
            fclose(pFile);
            pageCount++;
            count = 0;
            fPath = path + to_string(pageCount) + ".json";
            pFile = fopen(fPath.c_str(), "r");
            if (pFile)
            {
                cout << "Overwriting file " << fPath << endl;
                fclose(pFile);
            }
            else
                cout << "Writing file " << fPath << endl;
            pFile = fopen(fPath.c_str(), "w");
            fputc('[', pFile);
        }
    }
    fputs("\n]\n", pFile);
    fclose(pFile);
    cout << "Got " << to_string(2000000*(pageCount-1)+count) << " links in " << pageCount << " pages\n";
    return;
}

size_t SQLScraper::createBotSet(const char *fPath)
{
    FILE* pFile = fopen(fPath, "r");
    if (!pFile)
    {
        cerr << "Could not open file " << fPath << endl;
        return 0;
    }
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
    FILE* pFile = fopen(path, "r");
    if (pFile)
    {
        cout << "Overwriting file " << path << endl;
        fclose(pFile);
    }
    else
        cout << "Writing file " << path << endl;
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
    FILE* ipFile = fopen(iPath, "r");
    FILE* opFile = fopen(oPath, "r");
    if (!ipFile)
    {
        cerr << "Could not open file " << iPath << endl;
        fclose(opFile);
        return -1;
    }
    if (opFile)
    {
        cout << "Overwriting file " << oPath << endl;
        fclose(opFile);
    }
    else
        cout << "Writing to file " << oPath << endl;
    opFile = fopen(oPath, "w");
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
