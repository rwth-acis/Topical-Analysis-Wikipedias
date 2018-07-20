#include "sqlscraper.h"
#include "util/loggingutil.h"
#include <vector>
#include <unordered_map>

#define HASHMAP_PATH "../../media/english/index-pages.txt"
#define LINKMAP_PATH "../../media/english/linkmap.json"
#define SQL_TABLE_START "VALUES"
#define TITLE_LENGTH 256

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

unordered_map<int,int>* SQLScraper::createLinkmap(const char* fPath)
{
    // load hashmap
    cout << "Loading " << HASHMAP_PATH << endl;
    unordered_map<string,int>* hashmap = parser.parseHashmap(HASHMAP_PATH);
    if (!hashmap)
        return NULL;

    // try to open file
    FILE* ipFile = fopen(LINKMAP_PATH, "r");
    if (ipFile)
    {
        cout << "Overwriting file " << LINKMAP_PATH << endl;
        fclose(ipFile);
    }
    else
        cout << "Creating file for linkmap at " << LINKMAP_PATH << endl;
    ipFile = fopen(fPath, "r");
    if (!ipFile)
    {
        cerr << "Could not open file " << fPath;
        return NULL;
    }
    // initialize linkmap
    cout << "Reading category links from file " << fPath << endl;
    vector<char> stopChars = { ',', '\'' };
    unordered_map<int,int>* linkmap = new unordered_map<int,int>();

    // get to beginning of entry
    parser.findPattern(ipFile, SQL_TABLE_START);
    long count = 0;
    while (!parser.findSQLPattern(ipFile, "("))
    {
        // get id of page
        int id_from = stoi(parser.writeToString(ipFile, stopChars));
        if (!parser.findPattern(ipFile, stopChars))
        {
            LoggingUtil::warning("File " + string(fPath) + " ended unexpectedly at page " + to_string(id_from), logfile);
            cout << "Got " << count << " links" << endl;
            return linkmap;
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
                cout << "Got " << count << " links"  << endl;
                return linkmap;
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
            cout << "Got " << count << " links" << endl;
            this->writeLinkmap(linkmap, LINKMAP_PATH);
            return linkmap;
        }

        // save entry to linkmap
        pair<int,int> entry(id_from, id_to);
        linkmap->insert(entry);
        count++;
    }
    cout << "Got " << count << " links" << endl;
    this->writeLinkmap(linkmap, LINKMAP_PATH);
    return linkmap;
}

long SQLScraper::writeLinkmap(unordered_map<int,int>* linkmap, const char* path)
{
    short pageCount = 0;
    string fPath = to_string(pageCount) + path;
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
    long count = 0;
    if (linkmap->begin() == linkmap->end())
        LoggingUtil::error("Empty linkmap", logfile);
    for (auto it = linkmap->begin(); it != linkmap->end(); it++)
    {
        if (count)
            fputs(",", pFile);
        fprintf(pFile, "\n{\"_from\":\"enPages/%d\",\"_to\":\"enPages/%d\"}", it->first, it->second);
        count++;

        // after 1million links open new file
        if (count >= 1000000)
        {
            fputs("\n]", pFile);
            fclose(pFile);
            pageCount++;
            fPath = to_string(pageCount) + path;
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
    return count;
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
