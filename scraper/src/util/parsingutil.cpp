#include "parsingutil.h"
#include "loggingutil.h"
// JSON TAGS
#define TITLE "\"title\":\""
#define ID "\"id\":\""
#define CATEGORIES "\"categories\":["
// approximate initial sizes for buffers
#define TITLE_LENGTH 128
#define PATH_LENGTH 256
#define CATEGORY_BUFFER 4096

using namespace std;
ParsingUtil::ParsingUtil()
{
    logging = false;
}

ParsingUtil::ParsingUtil(const char* logfile)
{
    logging = true;
    this->logfile = logfile;
}

int ParsingUtil::findPattern(FILE *ipFile, const string stopStr)
{
    char c;
    size_t state = 0;
    while ((c = fgetc(ipFile)) != EOF)
    {
        if (c == (char) toupper(stopStr[state]) || c == (char) tolower(stopStr[state]))
            state++;
        else
            state = 0;

        if (state >= stopStr.size())
            return 0;
    }

    return -1;
}

bool ParsingUtil::findPattern(DynamicBuffer* buffer, const string stopStr)
{
    char c;
    size_t pos = 0;
    size_t state = 0;
    while (pos < buffer->length())
    {
        c = buffer->read(pos);
        pos++;
        if (c == (char) toupper(stopStr[state]) || c == (char) tolower(stopStr[state]))
            state++;
        else
            state = 0;

        if (state >= stopStr.size())
            return true;
    }

    return false;
}

char ParsingUtil::findPattern(FILE *ipFile, const vector<char> stopChars)
{
    char c;
    while ((c = fgetc(ipFile)) != EOF)
    {
        for (size_t i = 0; i < stopChars.size(); i++)
            if (c == stopChars[i])
                return c;
    }

    return 0;
}

int ParsingUtil::findSQLPattern(FILE *ipFile,  const string stopStr)
{
    char c, p = '0';
    size_t state = 0;
    bool inQuotes = false;
    while (!((c = fgetc(ipFile)) == EOF && p == EOF)) // apparently sometimes EOF chars just hang out in SQL files... who knew
    {
        if (c == '\'' && p != '\\')
            inQuotes = !inQuotes;
        if (!inQuotes && (c == (char) toupper(stopStr[state]) || c == (char) tolower(stopStr[state])))
            state++;
        else
            state = 0;
        if (state >= stopStr.size())
            return 0;
        if (p == c && c == '\\')
            p = 0;
        else
            p = c;
    }
    return -1;
}

int ParsingUtil::findPattern(FILE *ipFile, const string stopStr, DynamicBuffer* backlog)
{
    backlog->flush();
    char c;
    size_t state = 0;
    size_t initSize = backlog->size();
    int endSize = 0;
    while ((c = fgetc(ipFile)) != EOF)
    {
        if ((endSize = backlog->write(c)) < 0)
        {
            // print error to logfile
            if (logging)
                LoggingUtil::error("Buffer exceeded maximum size! Stopped parsing\n" + backlog->print(), logfile);
            return -1;
        }

        if (c == (char) toupper(stopStr[state]) || c == (char) tolower(stopStr[state]))
            state++;
        else
            state = 0;

        // print warning if title is larger than expected
        if (state >= stopStr.size())
        {
            if (endSize > initSize && logging)
                LoggingUtil::warning("Buffer had to be extended for content:" + backlog->print(), logfile);

            return backlog->length();
        }
    }

    return 1;
}

int ParsingUtil::findPattern(FILE *ipFile, const string stopStr, DynamicBuffer* backlog, vector<char> escapeChars)
{
    backlog->flush();
    char c, p = 0;
    size_t state = 0;
    size_t initSize = backlog->size();
    int endSize = 0;
    while ((c = fgetc(ipFile)) != EOF)
    {
        // escape chars
        for (int i = 0; i < escapeChars.size(); i++)
            if (c == escapeChars[i] && p != '\\')
                if (backlog->write('\\') < 0)
                {
                    if (logging)
                        LoggingUtil::error("Buffer exceeded maximum size! Stopped parsing\n" + backlog->print(), logfile);
                    return -1;
                }

        if ((endSize = backlog->write(c)) < 0)
        {
            // print error to logfile
            if (logging)
                LoggingUtil::error("Buffer exceeded maximum size! Stopped parsing\n" + backlog->print(), logfile);
            return -1;
        }

        if (c == (char) toupper(stopStr[state]) || c == (char) tolower(stopStr[state]))
            state++;
        else
            state = 0;

        // print warning if title is larger than expected
        if (state >= stopStr.size())
        {
            if (endSize > initSize && logging)
                LoggingUtil::warning("Buffer had to be extended for content:" + backlog->print(), logfile);

            return backlog->length();
        }
        p = c;
    }

    return 1;
}

int ParsingUtil::writeToFile(FILE *ipFile, FILE *opFile, const string stopStr)
{
    char c;
    size_t state = 0;
    while ((c = fgetc(ipFile)) != EOF)
    {
        fputc(c, opFile);

        if (c == (char) toupper(stopStr[state]) || c == (char) tolower(stopStr[state]))
            state++;
        else
            state = 0;

        if (state >= stopStr.size())
        {
            fputc('\n', opFile);
            return state;
        }
    }

    return -1;
}

int ParsingUtil::writeToFile(FILE *ipFile, FILE *opFile, const vector<char> stopChars)
{
    char c;
    while ((c = fgetc(ipFile)) != EOF)
    {
        for (size_t i = 0; i < stopChars.size(); i++)
            if (c == stopChars[i])
                return (int) c;

        fputc(c, opFile);
    }

    return -1;
}

int ParsingUtil::writeSQLToFile(FILE *ipFile, FILE *opFile,  const string stopStr)
{
    char c, p = '0';
    size_t state = 0;
    bool inQuotes = false;
    while (!((c = fgetc(ipFile)) == EOF && p == EOF)) // apparently sometimes EOF chars just hang out in SQL files... who knew
    {
        if (c == EOF)
        {
            p = c;
            continue;
        }
        fputc(c, opFile);
        if (c == '\'' && p != '\\')
            inQuotes = !inQuotes;
        if (!inQuotes && (c == (char) toupper(stopStr[state]) || c == (char) tolower(stopStr[state])))
            state++;
        else
            state = 0;
        if (state >= stopStr.size())
        {
            fputc('\n', opFile);
            return state;
        }
        if (p == c && c == '\\')
            p = 0;
        else
            p = c;
    }
    return -1;
}

int ParsingUtil::writeToBuffer(FILE *ipFile, DynamicBuffer* buffer, const string stopStr)
{
    char c;
    size_t state;
    size_t initSize = buffer->size();
    int endSize = 0;
    while ((c = fgetc(ipFile)) != EOF)
    {
        if ((endSize = buffer->write(c)) < 0)
        {
            // print error to logfile
            if (logging)
                LoggingUtil::error("Buffer exceeded maximum size! Stopped parsing\n" + buffer->print(), logfile);
            return -1;
        }

        if (c == (char) toupper(stopStr[state]) || c == (char) tolower(stopStr[state]))
            state++;
        else
            state = 0;

        if (state >= stopStr.size())
        {
            // print warning if title is larger than expected
            if (endSize > initSize && logging)
                LoggingUtil::warning("Buffer had to be extended for content: " + buffer->print(), logfile);

            return state;
        }
    }

    return 0;
}

int ParsingUtil::writeToBuffer(FILE *ipFile, DynamicBuffer* buffer, const vector<char> stopChars)
{
    char c;
    size_t initSize = buffer->size();
    int endSize = 0;
    while ((c = fgetc(ipFile)) != EOF)
    {
        for (size_t i = 0; i < stopChars.size(); i++)
            if (c == stopChars[i])
            {
                // print warning if buffer had to be extended
                if (endSize > initSize && logging)
                    LoggingUtil::warning("Buffer had to be extended for content: " + buffer->print(), logfile);
                return c;
            }

        if ((endSize = buffer->write(c)) < 0)
        {
            // print error to logfile
            if (logging)
                LoggingUtil::error("Buffer exceeded maximum size! Stopped parsing\n" + buffer->print(), logfile);
            return -1;
        }
    }

    return 0;
}

string ParsingUtil::writeToString(FILE *ipFile, const vector<char> stopChars)
{
    char c;
    string res = "";
    while ((c = fgetc(ipFile)) != EOF)
    {
        for (size_t i = 0; i < stopChars.size(); i++)
            if (c == stopChars[i])
                return res;

        res += c;
    }

    return 0;
}

unordered_map<string,int>* ParsingUtil::parseHashmap(const char* path)
{
    // open file
    vector<char> stopChars = { '\"' };
    FILE* ipFile = fopen(path, "r");
    if (!ipFile)
    {
        cerr << "Could not open file " << path << endl;
        return NULL;
    }

    unordered_map<string,int>* hashmap = new unordered_map<string,int>();
    int res = 0;
    while (!(res = this->findPattern(ipFile, TITLE)))
    {
        // get entry from file
        bool fatal = false;
        DynamicBuffer titleBuffer(TITLE_LENGTH);
        titleBuffer.setMax(PATH_LENGTH);
        fatal |= (this->writeToBuffer(ipFile, &titleBuffer, stopChars) == -1);
        if (this->findPattern(ipFile, ID))
            LoggingUtil::warning("Hashmap file ended unexpectedly", logfile);
        int id = stoi(this->writeToString(ipFile, stopChars));

        // add entry to map
        pair<string,int> entry(titleBuffer.print(), id);
        hashmap->insert(entry);

        // check if fatal error has occured
        if (fatal)
        {
            LoggingUtil::error("Fatal error (Buffer overflow): Integrity of file cannot be guaranteed", logfile);
            fclose(ipFile);
            delete hashmap;
            return NULL;
        }
    }
    if (hashmap->size() == 0)
        LoggingUtil::warning("Empty hashmap", logfile);
    return hashmap;
}

unordered_map<int,string>* ParsingUtil::parseLinkmap(const char* path)
{
    // open file
    vector<char> stopChars = { '\"', ']' };
    FILE* ipFile = fopen(path, "r");
    if (!ipFile)
    {
        cerr << "Could not open file " << path << endl;
        return NULL;
    }

    unordered_map<int,string>* linkmap = new unordered_map<int,string>();
    int res = 0;
    while (!(res = this->findPattern(ipFile, ID)))
    {
        // get entry from file
        bool fatal = false;
        int id = stoi(this->writeToString(ipFile, stopChars));
        DynamicBuffer catBuffer(PATH_LENGTH);
        catBuffer.setMax(CATEGORY_BUFFER);
        if (this->findPattern(ipFile, CATEGORIES) == -1)
            LoggingUtil::warning("Linkmap file ended unexpectedly", logfile);
        fatal |= (this->writeToBuffer(ipFile, &catBuffer, stopChars) == -1);
        // add entry to map
        pair<int,string> entry(id, catBuffer.print());
        linkmap->insert(entry);

        // check if fatal error has occured
        if (fatal)
        {
            LoggingUtil::error("Fatal error (Buffer overflow): Integrity of file cannot be guaranteed", logfile);
            fclose(ipFile);
            delete linkmap;
            return NULL;
        }
    }
    if (linkmap->size() == 0)
        LoggingUtil::warning("Empty linkmap", logfile);
    return linkmap;
}
