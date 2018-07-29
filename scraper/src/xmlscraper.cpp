#include "xmlscraper.h"
#include "util/loggingutil.h"
#include <unordered_map>
#include <unordered_set>

// path to arangoDB
#define BASE_PATH "http://localhost:8529//_db/_system/wikipics/"
// path to files
#define INDEX_PATH "../../media/english/index-pages.txt"
#define LINKMAP_PATH "../../media/english/jsonFiles/linkmap"
#define PAGES_PATH "../../media/english/jsonFiles/pages"
#define HISTORY_INPUT(x) "../../media/english/compressedFiles/history/enwiki-20180701-pages-meta-history" + to_string(x) + ".xml"
#define HISTORY_OUTPUT(x) "../../media/english/csvFiles/history" + to_string(x) + ".csv"
#define AUTHOR_OUTPUT(x) "../../media/english/csvFiles/authors" + to_string(x) + ".csv"
#define BOTSET "../../media/english/csvFiles/bots.csv"
// sizes for progress estimation
#define INDEX_SIZE 502047972
#define HISTORY_SIZE 76873861874
// approximate initial sizes for buffers
#define TITLE_LENGTH 128
#define PATH_LENGTH 256
#define SHA1_LENGTH 32
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
#define WIKIPEDIA_TEMPLATES 44084293 // not considered in current version of DB
// xml tags
#define PAGE_START "<page>"
#define PAGE_END "</page>"
#define TITLE "<title>"
#define ID "<id>"
#define REVISION "<revision>"
#define TIMESTAMP "<timestamp>"
#define CONTRIBUTOR_START "<contributor>"
#define CONTRIBUTOR_END "</contributor>"
#define USERNAME "<username>"
#define BODY_START "<text"
#define BODY_END "</text>"
#define SHA_TAG "<sha1>"
#define NAMESPACE "<ns>"
#define MINOR "<minor />"

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

size_t XMLScraper::scrapePages()
{
    // get index file
    FILE* ipFile = LoggingUtil::openFile(INDEX_PATH, false);
    if (!ipFile)
        return 0;

    // load linkmap
    cout << "Load linkmap from " << LINKMAP_PATH << endl;
    unordered_map<int,string>* linkmap = parser.parseLinkmap(LINKMAP_PATH);
    if (linkmap == NULL)
    {
        cerr << "Error loading linkmap" << endl;
        return 0;
    }

    // create output files
    short fileCount = 1;
    string fPath = PAGES_PATH + to_string(fileCount) + ".json";
    FILE* opFile = LoggingUtil::openFile(fPath, true);
    fputc('[', opFile);
    fclose(opFile);

    // buffer line
    DynamicBuffer lineBuffer(LARGE_BACKLOG);
    lineBuffer.setMax(MAX_BUFFER_SIZE);
    size_t count = 0;
    short prog = 0;
    while (parser.writeToBuffer(ipFile, &lineBuffer, '\n'))
    {
        // compute progress indicator
        if (((ftell(ipFile)*10)/INDEX_SIZE) > prog || (INDEX_SIZE - ftell(ipFile) < LARGE_BACKLOG && prog < 10))
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

        // get namespace
        short ns = ParsingUtil::reasonableStringCompare(title, "Category:") ? 14 : 0;

        // get categories
        bool hasCategories = true;
        vector<int>* catBuffer;
        unordered_map<int,string>::const_iterator categories = linkmap->find(id);
        if (categories == linkmap->end())
        {
            hasCategories = false;
            LoggingUtil::warning("No match for page " + to_string(id), logfile);
        }
        if (hasCategories)
            catBuffer = parser.writeCategoryToBuffer(categories);

        // check for "bad" categories
        if (hasCategories&&ns)
        {
            if (!this->isTopical(catBuffer))
            {
                LoggingUtil::warning("Category " + to_string(id) + " not topical", logfile);
                delete catBuffer;
                continue;
            }
        }
        else if (hasCategories)
        {
            if (!this->isArticle(catBuffer, linkmap))
            {
                LoggingUtil::warning("Article " + to_string(id) + " not topical", logfile);
                delete catBuffer;
                continue;
            }
        }
        // print page
        opFile = fopen(fPath.c_str(), "a");
        if (count)
            fputc(',', opFile);
        fprintf(opFile, "\n{\"title\":\"%s\",\"_key\":\"%d\",\"namespace\":\"%d\"}", title.c_str(), id, ns);
        count++;
        if (hasCategories)
            delete catBuffer;
        // after 1000000 pages open new file
        if (count > 1000000)
        {
            count = 0;
            fputs("\n]\n", opFile);
            fclose(opFile);
            fileCount++;
            fPath = PAGES_PATH + to_string(fileCount) + ".json";
            opFile = LoggingUtil::openFile(fPath, true);
            fputc('[', opFile);
            fclose(opFile);
        }
        else
            fclose(opFile);
    }
    opFile = fopen(fPath.c_str(), "a");
    fputs("\n]\n", opFile);
    fclose(opFile);
    cout << "Got " << 1000000*(fileCount-1)+count << " pages\n";
    return 1000000*(fileCount-1)+count;
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
            {
                delete subCatBuffer;
                return false;
            }
        delete subCatBuffer;
    }
    return true;
}

bool XMLScraper::isTopical(vector<int>* categories)
{
    // check against "bad categories"
    for (int i = 0; i < categories->size(); i++)
        if (categories->at(i) == MAINTENANCE || categories->at(i) == TEMPLATE || categories->at(i) == CONTAINER || categories->at(i) == HIDDEN || categories->at(i)  == TRACKING || categories->at(i) == REDIRECT || categories->at(i) == STUB || categories->at(i) == WIKIPEDIA_TEMPLATES)
            return false;
    return true;
}

size_t XMLScraper::historyToCSV(short fileNr)
{
    // get index file
    string fPath = HISTORY_INPUT(fileNr);
    FILE* ipFile = LoggingUtil::openFile(fPath, false);
    if (!ipFile)
        return 0;

    // create output file
    fPath = HISTORY_OUTPUT(fileNr);
    FILE* opFile = LoggingUtil::openFile(fPath, true);
    fputs("_key,_to,page_title,_from,user_name,timestamp\n", opFile);
    fclose(opFile);

    // buffer with hashes of revisions + get bots
    unordered_set<string>* revHashes = new unordered_set<string>;
    unordered_set<string>* botset = parser.parseBotSet(BOTSET);

    // go to beginning of page or next revision
    size_t count = 0, pageId;
    short prog = 0, res;
    DynamicBuffer titleBuffer = DynamicBuffer(LARGE_BACKLOG);
    titleBuffer.setMax(MAX_BUFFER_SIZE);
    while ((res = parser.findPattern(ipFile, PAGE_START, REVISION)))
    {
        // compute progress indicator
        if (((ftell(ipFile)*20)/HISTORY_SIZE) > prog || (HISTORY_SIZE - ftell(ipFile) < ACTUAL_BUFFER_LIMIT && prog < 20))
        {
            prog++;
            cout << "Read " << prog*5 << "% of revisions\n";
        }
        // got next page
        if (res == 1)
        {
            // flush hash/title buffer
            revHashes->clear();
            titleBuffer.flush();

            // get title
            parser.findPattern(ipFile, TITLE);
            if (parser.writeEscapedToBuffer(ipFile, &titleBuffer, '<') == -1)
            {
                cerr << "Max buffer size exceeded";
                return count;
            }
            // check namespace (go to next page if no article)
            parser.findPattern(ipFile, NAMESPACE);
            if (stoi(parser.writeToString(ipFile, '<')))
            {
                parser.findPattern(ipFile, PAGE_END);
                continue;
            }
            // get pageId
            parser.findPattern(ipFile, ID);
            pageId = stoi(parser.writeToString(ipFile, '<'));
            continue;
        }
        // got next revision
        if (res == 2)
        {
            // get revision id
            parser.findPattern(ipFile, ID);
            size_t revisionId = stoi(parser.writeToString(ipFile, '<'));
            // get timestamp
            parser.findPattern(ipFile, TIMESTAMP);
            DynamicBuffer timestampBuffer = DynamicBuffer(TITLE_LENGTH);
            timestampBuffer.setMax(PATH_LENGTH);
            if (parser.writeToBuffer(ipFile, &timestampBuffer, '<') == -1)
            {
                cerr << "Max buffer size exceeded";
                return count;
            }
            //cout << "timestamp: " << timestampBuffer.print() << ", ";
            // buffer contributor information
            parser.findPattern(ipFile, CONTRIBUTOR_START);
            DynamicBuffer contBuffer = DynamicBuffer(LARGE_BACKLOG);
            contBuffer.setMax(MAX_BUFFER_SIZE);
            if (parser.writeToBuffer(ipFile, &contBuffer, CONTRIBUTOR_END) == -1)
            {
                cerr << "Max buffer size exceeded";
                return count;
            }
            // check if contributor is user
            if (!parser.searchPattern(&contBuffer, ID))
                continue;
            // get user information
            size_t userId;
            DynamicBuffer username = DynamicBuffer(TITLE_LENGTH);
            username.setMax(LARGE_BACKLOG);
            {
                size_t nameState = 0, idState = 0;
                string name = USERNAME, id = ID, idString = "";
                bool writingName = false, writingId = false;
                for (size_t i = 0; i < contBuffer.length(); i++)
                {
                    char c = contBuffer.read(i);
                    if (c == '<')
                    {
                        writingName = false;
                        writingId = false;
                    }
                    if (writingName)
                    {
                        if (c == '\\' || c == '\"')
                            username.write('\\');
                        username.write(c);
                    }
                    if (writingId)
                        idString += c;
                    if (c == name[nameState])
                        nameState++;
                    else
                        nameState = 0;
                    if (c == id[idState])
                        idState++;
                    else
                        idState = 0;
                    if (nameState >= name.size())
                        writingName = true;
                    if (idState >= id.size())
                        writingId = true;
                }
                userId = stoi(idString);
            }
            // userId should not be 0
            if (!userId)
            {
                LoggingUtil::warning("User ID equals 0 for user " + username.print(), logfile);
                continue;
            }
            // check if contributor is bot
            if (botset->find(username.print()) != botset->end())
                continue;
            // check for minor tag
            if (parser.findPattern(ipFile, BODY_START, MINOR) == 2)
                continue;
            parser.findPattern(ipFile, ">");

            // check for reversion
            parser.findPattern(ipFile, SHA_TAG);
            DynamicBuffer shaBuffer = DynamicBuffer(SHA1_LENGTH);
            shaBuffer.setMax(SHA1_LENGTH);
            if (parser.writeToBuffer(ipFile, &shaBuffer, '<') == -1)
            {
                cerr << "Max buffer size exceeded";
                return count;
            }
            if (revHashes->find(shaBuffer.print()) != revHashes->end())
                continue;
            // print revision
            revHashes->insert(shaBuffer.print());
            fPath = HISTORY_OUTPUT(fileNr);
            opFile = fopen(fPath.c_str(), "a");
            fprintf(opFile, "\"%zu\",\"enPages/%zu\",\"%s\",\"enAuthors/%zu\",\"%s\",\"%s\"\n", revisionId, pageId, titleBuffer.raw(), userId, username.raw(), timestampBuffer.raw());
            fclose(opFile);
            count++;
        }
    }
    cout << "Got " << count << " revisions" << endl;
    delete revHashes;
    delete botset;
    return count;
}

size_t XMLScraper::getAuthors(short fileNr)
{
    string fPath = HISTORY_OUTPUT(fileNr);
    FILE* ipFile = LoggingUtil::openFile(fPath, false);
    if (!ipFile)
        return 0;
    fPath = AUTHOR_OUTPUT(fileNr);
    FILE* opFile = LoggingUtil::openFile(fPath, true);

}
