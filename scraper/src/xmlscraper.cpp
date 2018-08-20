#include "xmlscraper.h"
#include "util/loggingutil.h"
#include <unordered_map>
#include <unordered_set>

// path to files (english)
#define EN_INDEX_PATH "../../media/english/index-pages.txt"
#define EN_LINKMAP_PATH "../../media/english/csvFiles/category_links/linkmap"
#define EN_PAGES_PATH "../../media/english/jsonFiles/pages"
#define EN_HISTORY_INPUT(x) "../../media/english/compressedFiles/history/enwiki-20180701-pages-meta-history" + to_string(x) + ".xml"
#define EN_HISTORY_OUTPUT(x) "../../media/english/csvFiles/history" + to_string(x) + ".csv"
#define EN_AUTHOR_OUTPUT "../../media/english/csvFiles/authors.csv"
#define EN_AUTHORLINK_OUTPUT "../../media/english/csvFiles/authorLinks.csv"
#define EN_BOTSET "../../media/english/csvFiles/bots.csv"
// sizes for progress estimation (english)
#define EN_INDEX_SIZE 502047972
#define EN_HISTORY_SIZE 76873861874
// path to files (vietnamese)
#define VI_INDEX_PATH "../../media/vietnamese/index-pages.txt"
#define VI_LINKMAP_PATH "../../media/vietnamese/csvFiles/linkmap"
#define VI_PAGES_PATH "../../media/vietnamese/jsonFiles/pages"
#define VI_HISTORY_INPUT "../../media/vietnamese/rawFiles/viwiki-20180701-pages-meta-history.xml"
#define VI_HISTORY_OUTPUT(x) "../../media/vietnamese/csvFiles/history" + to_string(x) + ".csv"
#define VI_AUTHOR_OUTPUT "../../media/vietnamese/csvFiles/authors.csv"
#define VI_AUTHORLINK_OUTPUT "../../media/vietnamese/csvFiles/authorLinks.csv"
#define VI_BOTSET "../../media/vietnamese/csvFiles/bots.csv"
// sizes for progress estimatinon (vietnamese)
#define VI_INDEX_SIZE 70675477
#define VI_HISTORY_SIZE 243255279925
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
// "bad category" ids (english)
#define EN_HIDDEN 15961454
#define EN_CONTAINER 30176254
#define EN_TEMPLATE 49310588
#define EN_MAINTENANCE 738040
#define EN_TRACKING 7361045
#define EN_STUB 869270
#define EN_REDIRECT 30334744
#define EN_DISAMBIGUATION 19204864
#define EN_WIKIPEDIA_TEMPLATES 44084293 // not considered in current version of DB
// "bad category" ids (vietnamese)
#define VI_HIDDEN 239282
#define VI_CONTAINER 2417964
#define VI_TEMPLATE 123748
#define VI_MAINTENANCE 3116367
#define VI_TRACKING 3155218
#define VI_STUB 1439626
#define VI_REDIRECT 40889
#define VI_DISAMBIGUATION 2717
// tags
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
#define EN_CATEGORY_TAG "Category:"
#define VI_CATEGORY_TAG "Thể loại:"
// language codes
#define LNG_EN "en"
#define LNG_VI "vi"

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

size_t XMLScraper::scrapePages(language lng)
{
    // get index file
    string path, category_tag, fPath;
    size_t file_size = 0;
    switch (lng)
    {
        case EN:
            category_tag = EN_CATEGORY_TAG;
            path = EN_LINKMAP_PATH;
            fPath = EN_INDEX_PATH;
            file_size = EN_INDEX_SIZE;
            break;
        case VI:
            category_tag = VI_CATEGORY_TAG;
            path = VI_LINKMAP_PATH;
            fPath = VI_INDEX_PATH;
            file_size = VI_INDEX_SIZE;
            break;
        default:
            category_tag = EN_CATEGORY_TAG;
            path = EN_LINKMAP_PATH;
            fPath = EN_INDEX_PATH;
            file_size = EN_INDEX_SIZE;
            break;
    }
    FILE* ipFile = LoggingUtil::openFile(fPath, false);
    if (!ipFile)
        return 0;
    // load linkmap
    cout << "Load linkmap from " << path << endl;
    unordered_map<int,string>* linkmap = parser.parseLinkmap(path.c_str());
    if (linkmap == NULL)
    {
        cerr << "Error loading linkmap" << endl;
        return 0;
    }

    // create output files
    short fileCount = 1;
    switch (lng)
    {
        case EN:
            path = EN_PAGES_PATH;
            break;
        case VI:
            path = VI_PAGES_PATH;
            break;
        default:
            path = EN_PAGES_PATH;
            break;
    }
    fPath = path + to_string(fileCount) + ".json";
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
        if (((ftell(ipFile)*10)/file_size) > prog || (file_size - ftell(ipFile) < LARGE_BACKLOG && prog < 10))
        {
            prog++;
            cout << "Read " << prog*10 << "% of pages\n";
        }
        // get id
        int id = stoi(parser.writeToString(&lineBuffer, 0, {':'}));

        // get title
        size_t idx = parser.findPattern(&lineBuffer, ":") + 1;
        string title = lineBuffer.print(idx);
        lineBuffer.flush();

        // get namespace
        short ns = ParsingUtil::reasonableStringCompare(title, category_tag) ? 14 : 0;

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
            if (!this->isTopical(catBuffer, lng))
            {
                LoggingUtil::warning("Category " + to_string(id) + " not topical", logfile);
                delete catBuffer;
                continue;
            }
        }
        else if (hasCategories)
        {
            if (!this->isArticle(catBuffer, linkmap, lng))
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
            fPath = path + to_string(fileCount) + ".json";
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

bool XMLScraper::isArticle(vector<int>* categories, unordered_map<int,string>* linkmap, language lng)
{
    int disamb, redir;
    switch (lng)
        {
            case EN:
                disamb = EN_DISAMBIGUATION;
                redir = EN_REDIRECT;
                break;
            case VI:
                disamb = VI_DISAMBIGUATION;
                redir = VI_REDIRECT;
                break;
            default:
                disamb = EN_DISAMBIGUATION;
                redir = EN_REDIRECT;
                break;
        }
    for (int i = 0; i < categories->size(); i++)
    {
        // check for disambiguation/redirect
        if (categories->at(i) == disamb || categories->at(i) == redir)
            return false;
        // get subcategories
        unordered_map<int,string>::const_iterator subCats = linkmap->find(categories->at(i));
        if (subCats == linkmap->end())
            continue;
        vector<int>* subCatBuffer = parser.writeCategoryToBuffer(subCats);
        // check for redirect
        for (int j = 0; j < subCatBuffer->size(); j++)
            if (subCatBuffer->at(j) == redir)
            {
                delete subCatBuffer;
                return false;
            }
        delete subCatBuffer;
    }
    return true;
}

bool XMLScraper::isTopical(vector<int>* categories, language lng)
{
    // check against "bad categories"
    switch (lng)
        {
            case EN:
                for (int i = 0; i < categories->size(); i++)
                    if (categories->at(i) == EN_MAINTENANCE || categories->at(i) == EN_TEMPLATE || categories->at(i) == EN_CONTAINER || categories->at(i) == EN_HIDDEN || categories->at(i)  == EN_TRACKING || categories->at(i) == EN_REDIRECT || categories->at(i) == EN_STUB || categories->at(i) == EN_WIKIPEDIA_TEMPLATES)
                        return false;
                break;
            case VI:
                for (int i = 0; i < categories->size(); i++)
                    if (categories->at(i) == VI_MAINTENANCE || categories->at(i) == VI_TEMPLATE || categories->at(i) == VI_CONTAINER || categories->at(i) == VI_HIDDEN || categories->at(i)  == VI_TRACKING || categories->at(i) == VI_REDIRECT || categories->at(i) == VI_STUB)
                        return false;
                break;
            default:
                for (int i = 0; i < categories->size(); i++)
                    if (categories->at(i) == EN_MAINTENANCE || categories->at(i) == EN_TEMPLATE || categories->at(i) == EN_CONTAINER || categories->at(i) == EN_HIDDEN || categories->at(i)  == EN_TRACKING || categories->at(i) == EN_REDIRECT || categories->at(i) == EN_STUB || categories->at(i) == EN_WIKIPEDIA_TEMPLATES)
                        return false;
                break;
        }
    return true;
}

size_t XMLScraper::historyToCSV(short fileNr, language lng)
{
    // get index file
    string fPath, pBotset, lngCode;
    size_t file_size = 0;
    short fileCount = 1;
    switch (lng)
    {
        case EN:
            fPath = EN_HISTORY_INPUT(fileNr);
            lngCode = LNG_EN;
            break;
        case VI:
            fPath = VI_HISTORY_INPUT;
            lngCode = LNG_VI;
            break;
        default:
            fPath = EN_HISTORY_INPUT(fileNr);
            lngCode = LNG_EN;
            break;
    }
    FILE* ipFile = LoggingUtil::openFile(fPath, false);
    if (!ipFile)
        return 0;
    // create output file
    switch (lng)
    {
        case EN:
            fPath = EN_HISTORY_OUTPUT(fileNr);
            pBotset = EN_BOTSET;
            file_size = EN_HISTORY_SIZE;
            break;
        case VI:
            fPath = VI_HISTORY_OUTPUT(fileNr+fileCount);
            pBotset = VI_BOTSET;
            file_size = VI_HISTORY_SIZE;
            break;
        default:
            fPath = EN_HISTORY_OUTPUT(fileNr);
            pBotset = EN_BOTSET;
            file_size = EN_HISTORY_SIZE;
            break;
    }
    FILE* opFile = LoggingUtil::openFile(fPath, true);
    fputs("rev_id,_to,page_title,_from,user_name,timestamp\n", opFile);
    fclose(opFile);

    // buffer with hashes of revisions + get bots
    unordered_set<string>* revHashes = new unordered_set<string>;
    unordered_set<string>* botset = parser.parseBotSet(pBotset.c_str());

    // go to beginning of page or next revision
    size_t count = 0, pageId;
    short prog = 0, res;
    DynamicBuffer titleBuffer = DynamicBuffer(LARGE_BACKLOG);
    titleBuffer.setMax(MAX_BUFFER_SIZE);
    while ((res = parser.findPattern(ipFile, PAGE_START, REVISION)))
    {
        // compute progress indicator
        if (((ftell(ipFile)*20)/file_size) > prog || (file_size - ftell(ipFile) < ACTUAL_BUFFER_LIMIT && prog < 20))
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
            opFile = fopen(fPath.c_str(), "a");
            fprintf(opFile, "\"%zu\",\"%sPages/%zu\",\"%s\",\"%sAuthors/%zu\",\"%s\",\"%s\"\n", revisionId, lngCode.c_str(), pageId, titleBuffer.raw(), lngCode.c_str(), userId, username.raw(), timestampBuffer.raw());
            count++;
            // after 1000000 pages open new file
            if (count > 1000000)
            {
                count = 0;
                fclose(opFile);
                fileCount++;
                switch (lng)
                {
                    case EN:
                        fPath = EN_HISTORY_OUTPUT(fileNr);
                        break;
                    case VI:
                        fPath = VI_HISTORY_OUTPUT(fileNr+fileCount);
                        break;
                    default:
                        fPath = EN_HISTORY_OUTPUT(fileNr);
                        break;
                }
                opFile = LoggingUtil::openFile(fPath, true);
                fputs("_key,_to,page_title,_from,user_name,timestamp\n", opFile);
                fclose(opFile);
            }
            else
                fclose(opFile);
        }
    }
    cout << "Got " << (fileCount-1)*1000000 + count << " revisions" << endl;
    delete revHashes;
    delete botset;
    return count;
}

size_t XMLScraper::getAuthors(language lng)
{
    string path, fPath;
    short fileNr = 1;
    switch (lng)
    {
        case EN:
            path = EN_AUTHOR_OUTPUT;
            fPath = EN_HISTORY_OUTPUT(fileNr);
            break;
        case VI:
            path = VI_AUTHOR_OUTPUT;
            fPath = VI_HISTORY_OUTPUT(fileNr);
            break;
        default:
            path = EN_AUTHOR_OUTPUT;
            fPath = EN_HISTORY_OUTPUT(fileNr);
            break;
    }
    size_t count = 0;
    FILE* opFile = LoggingUtil::openFile(path, true);
    fputs("_key,username\n", opFile);
    fclose(opFile);
    FILE* ipFile;
    while ((ipFile = fopen(fPath.c_str(), "r")))
    {
        fileNr++;
        fclose(ipFile);
        switch (lng)
        {
            case EN:
                fPath = EN_HISTORY_OUTPUT(fileNr);
                break;
            case VI:
                fPath = VI_HISTORY_OUTPUT(fileNr);
                break;
            default:
                fPath = EN_HISTORY_OUTPUT(fileNr);
                break;
        }
    }
    fileNr--;
    unordered_map<int,string>* authors = new unordered_map<int, string>();
    for (short i = 1; i <= fileNr; i++)
    {
        switch (lng)
        {
            case EN:
                fPath = EN_HISTORY_OUTPUT(i);
                break;
            case VI:
                fPath = VI_HISTORY_OUTPUT(i);
                break;
            default:
                fPath = EN_HISTORY_OUTPUT(i);
                break;
        }
        ipFile = LoggingUtil::openFile(fPath, false);
        if (!ipFile)
            return 0;
        cout << "Reading input file " << i << "/" << fileNr << endl;
        while (parser.findPattern(ipFile, "Authors/"))
        {
            int id = stoi(parser.writeToString(ipFile, '\"'));
            if (authors->find(id) != authors->end())
                continue;
            if (!parser.findPattern(ipFile, "\""))
            {
                LoggingUtil::warning("File " + fPath + " ended unexpectedly");
                return count;
            }

            string name = parser.writeToString(ipFile, '\"');
            authors->insert({id,name});
            count++;
        }
    }
    // print map
    for (auto it = authors->begin(); it != authors->end(); it++)
    {
        opFile = fopen(path.c_str(), "a");
        fprintf(opFile, "\"%d\",\"%s\"\n", it->first, it->second.c_str());
        fclose(opFile);
    }
    return count;
}

size_t XMLScraper::getAuthors(const char* fPath, const char* path)
{
    size_t count = 0;
    FILE* ipFile;
    ipFile = LoggingUtil::openFile(fPath, false);
    if (!ipFile)
    {
        cerr << "Could not open file " << fPath << endl;
        return 0;
    }
    cout << "Reading input file " << fPath << endl;
    FILE* opFile = LoggingUtil::openFile(path, true);
    if (!opFile)
    {
        cerr << "Could not create file " << path << endl;
        return 0;
    }
    fputs("_key,pages\n", opFile);
    fclose(opFile);
    unordered_map<size_t,string>* authors = new unordered_map<size_t, string>();

    // get authors and save to map
    while (parser.findPattern(ipFile, "Test/"))
    {
        size_t userId = stoi(parser.writeToString(ipFile, '\"'));
        if (!parser.findPattern(ipFile, "Pages/"))
        {
            LoggingUtil::warning("File " + string(fPath) + " ended unexpectedly");
            return count;
        }
        string pageId = parser.writeToString(ipFile, '\"');
        auto it = authors->find(userId);
        if (it == authors->end())
            authors->insert({userId,pageId});
        else if (it->second != pageId)
            it->second += ',' + pageId;
        count++;
    }
    // print map
    for (auto it = authors->begin(); it != authors->end(); it++)
    {
        opFile = fopen(path, "a");
        fprintf(opFile, "\"%zu\",\"%s\"\n", it->first, it->second.c_str());
        fclose(opFile);
    }
    return count;
}

size_t XMLScraper::getAuthorLinks(language lng)
{
    string path, fPath, lngCode;
    size_t count = 0;
    short fileNr = 1;
    switch (lng)
    {
        case EN:
            path = EN_HISTORY_OUTPUT(fileNr);
            fPath = EN_AUTHORLINK_OUTPUT;
            lngCode = LNG_EN;
            break;
        case VI:
            path = VI_HISTORY_OUTPUT(fileNr);
            fPath = VI_AUTHORLINK_OUTPUT;
            lngCode = LNG_VI;
            break;
        default:
            fPath = EN_AUTHORLINK_OUTPUT;
            lngCode = LNG_EN;
            break;
    }
    FILE* opFile = LoggingUtil::openFile(fPath, true);
    fputs("\"_from\",\"_to\",\"page\"\n", opFile);
    fclose(opFile);
    FILE* ipFile;
    while ((ipFile = fopen(path.c_str(), "r")))
    {
        fileNr++;
        fclose(ipFile);
        switch (lng)
        {
            case EN:
                path = EN_HISTORY_OUTPUT(fileNr);
                break;
            case VI:
                path = VI_HISTORY_OUTPUT(fileNr);
                break;
            default:
                path = EN_HISTORY_OUTPUT(fileNr);
                break;
        }
    }
    for (short i = 1; i < fileNr; i++)
    {
        switch (lng)
        {
            case EN:
                path = EN_HISTORY_OUTPUT(i);
                break;
            case VI:
                path = VI_HISTORY_OUTPUT(i);
                break;
            default:
                path = EN_HISTORY_OUTPUT(i);
                break;
        }
        ipFile = LoggingUtil::openFile(path, false);
        cout << "Reading input file " << i << "/" << fileNr-1 << endl;
        size_t id = 0;
        unordered_set<size_t> authors;
        while (parser.findPattern(ipFile, "Pages/"))
        {
            size_t newID = stoi(parser.writeToString(ipFile, '\"'));
            if (id != newID)
            {
                if (id)
                {
                    for (auto it = authors.begin(); it != authors.end(); it++)
                        for (auto jt = it; jt != authors.end(); ++jt)
                        {
                            if (it == jt)
                                continue;
                            opFile = fopen(fPath.c_str(), "a");
                            fprintf(opFile,"\"%sAuthors/%zu\",\"%sAuthors/%zu\",\"%sPages/%zu\"\n", lngCode.c_str(),*it,lngCode.c_str(),*jt,lngCode.c_str(),id);
                            fclose(opFile);
                        }
                    authors.clear();
                }
                id = newID;
            }
            if (!parser.findPattern(ipFile, "Authors/"))
            {
                cout << "File ended unexpectedly\n";
                return count;
            }
            authors.insert(stoi(parser.writeToString(ipFile, '\"')));
            count++;
        }
        for (auto it = authors.begin(); it != authors.end(); it++)
            for (auto jt = it; jt != authors.end(); ++jt)
            {
                if (it == jt)
                    continue;
                opFile = fopen(fPath.c_str(), "a");
                fprintf(opFile,"\"%sAuthors/%zu\",\"%sAuthors/%zu\",\"%sPages/%zu\"\n", lngCode.c_str(),*it,lngCode.c_str(),*jt,lngCode.c_str(),id);
                fclose(opFile);
            }
    }
    return count;
}
