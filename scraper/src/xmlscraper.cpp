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
// path to files (vietnamese)
#define VI_INDEX_PATH "../../media/vietnamese/index-pages.txt"
#define VI_LINKMAP_PATH "../../media/vietnamese/csvFiles/linkmap"
#define VI_PAGES_PATH "../../media/vietnamese/jsonFiles/pages"
#define VI_HISTORY_INPUT "../../media/vietnamese/rawFiles/viwiki-20180701-pages-meta-history.xml"
#define VI_HISTORY_OUTPUT(x) "../../media/vietnamese/csvFiles/history" + to_string(x) + ".csv"
#define VI_AUTHOR_OUTPUT "../../media/vietnamese/csvFiles/authors.csv"
#define VI_AUTHORLINK_OUTPUT(x) "../../media/vietnamese/csvFiles/authorLinks" + to_string(x) + ".csv"
#define VI_BOTSET "../../media/vietnamese/csvFiles/bots.csv"
// path to files (hebrew)
#define HE_INDEX_PATH "../../media/hebrew/index-pages.txt"
#define HE_LINKMAP_PATH "../../media/hebrew/csvFiles/linkmap"
#define HE_PAGES_PATH "../../media/hebrew/jsonFiles/pages"
#define HE_HISTORY_INPUT(x) "../../media/hebrew/rawFiles/hewiki-20180801-pages-meta-history" + to_string(x) + ".xml"
#define HE_HISTORY_OUTPUT(x) "../../media/hebrew/csvFiles/history" + to_string(x) + ".csv"
#define HE_AUTHOR_OUTPUT "../../media/hebrew/csvFiles/authors.csv"
#define HE_AUTHORLINK_OUTPUT(x) "../../media/hebrew/csvFiles/authorLinks" + to_string(x) + ".csv"
#define HE_BOTSET "../../media/hebrew/csvFiles/bots.csv"
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
// bad "category" ids (hebrew)
#define HE_HIDDEN 504284
#define HE_TEMPLATE 98862
#define HE_MAINTENANCE 31583
#define HE_STUB 21182
#define HE_DISAMBIGUATION 26446
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
#define HE_CATEGORY_TAG "קטגוריה:"
// language codes
#define LNG_EN "en"
#define LNG_VI "vi"
#define LNG_HE "he"

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
    switch (lng)
    {
        case VI:
            category_tag = VI_CATEGORY_TAG;
            path = VI_LINKMAP_PATH;
            fPath = VI_INDEX_PATH;
            break;
        case HE:
            category_tag = HE_CATEGORY_TAG;
            path = HE_LINKMAP_PATH;
            fPath = HE_INDEX_PATH;
            break;
        default:
            category_tag = EN_CATEGORY_TAG;
            path = EN_LINKMAP_PATH;
            fPath = EN_INDEX_PATH;
            break;
    }
    // get file size
    FILE* ipFile = fopen(fPath.c_str(), "a");
    size_t file_size = ftell(ipFile);
    fclose(ipFile);
    ipFile = LoggingUtil::openFile(fPath, false);
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
        case VI:
            path = VI_PAGES_PATH;
            break;
        case HE:
            path = HE_PAGES_PATH;
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
        int id = stol(parser.writeToString(&lineBuffer, 0, {':'}));

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
            case VI:
                disamb = VI_DISAMBIGUATION;
                redir = VI_REDIRECT;
                break;
            case HE:
                disamb = HE_DISAMBIGUATION;
                redir = -1; // no id known for redirects
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
            case VI:
                for (int i = 0; i < categories->size(); i++)
                    if (categories->at(i) == VI_MAINTENANCE || categories->at(i) == VI_TEMPLATE || categories->at(i) == VI_CONTAINER || categories->at(i) == VI_HIDDEN || categories->at(i)  == VI_TRACKING || categories->at(i) == VI_REDIRECT || categories->at(i) == VI_STUB)
                        return false;
                break;
            case HE:
                for (int i = 0; i < categories->size(); i++)
                    if (categories->at(i) == HE_MAINTENANCE || categories->at(i) == HE_TEMPLATE || categories->at(i) == HE_HIDDEN || categories->at(i) == HE_STUB)
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
    string path, fPath, pBotset, lngCode;
    short fileCount = 1;
    switch (lng)
    {
        case VI:
            path = VI_HISTORY_INPUT;
            lngCode = LNG_VI;
            break;
        case HE:
            path = HE_HISTORY_INPUT(fileNr);
            lngCode = LNG_HE;
            break;
        default:
            path = EN_HISTORY_INPUT(fileNr);
            lngCode = LNG_EN;
            break;
    }
    // get file size
    FILE* ipFile = fopen(path.c_str(), "a");
    size_t file_size = ftell(ipFile);
    fclose(ipFile);
    ipFile = LoggingUtil::openFile(path, false);
    if (!ipFile)
        return 0;
    // create output file
    switch (lng)
    {
        case VI:
            fPath = VI_HISTORY_OUTPUT(fileNr+fileCount);
            pBotset = VI_BOTSET;
            break;
        case HE:
            fPath = HE_HISTORY_OUTPUT(fileNr);
            pBotset = HE_BOTSET;
            break;
        default:
            fPath = EN_HISTORY_OUTPUT(fileNr);
            pBotset = EN_BOTSET;
            break;
    }
    FILE* opFile = LoggingUtil::openFile(fPath, true);
    fputs("rev_id,_from,page_title,_to,user_name,timestamp\n", opFile);
    fclose(opFile);

    // buffer with hashes of revisions + get bots
    unordered_set<string>* revHashes = new unordered_set<string>;
    unordered_set<string>* botset = parser.parseBotSet(pBotset.c_str());

    // go to beginning of page or next revision
    size_t count = 0, pageId;
    short prog = 0, res;
    DynamicBuffer titleBuffer = DynamicBuffer(LARGE_BACKLOG);
    titleBuffer.setMax(MAX_BUFFER_SIZE);
    cout << "Reading history from file " << path << endl;
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
            if (stol(parser.writeToString(ipFile, '<')))
            {
                parser.findPattern(ipFile, PAGE_END);
                continue;
            }
            // get pageId
            parser.findPattern(ipFile, ID);
            pageId = stol(parser.writeToString(ipFile, '<'));
            continue;
        }
        // got next revision
        if (res == 2)
        {
            // get revision id
            parser.findPattern(ipFile, ID);
            size_t revisionId = stol(parser.writeToString(ipFile, '<'));
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
                userId = stol(idString);
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
                    case VI:
                        fPath = VI_HISTORY_OUTPUT(fileNr+fileCount);
                        break;
                    case HE:
                        fPath = HE_HISTORY_OUTPUT(fileNr+fileCount-1);
                        break;
                    default:
                        fPath = EN_HISTORY_OUTPUT(fileNr);
                        break;
                }
                opFile = LoggingUtil::openFile(fPath, true);
                fputs("rev_id,_from,page_title,_to,user_name,timestamp\n", opFile);
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
        case VI:
            path = VI_AUTHOR_OUTPUT;
            fPath = VI_HISTORY_OUTPUT(fileNr);
            break;
        case HE:
            path = HE_AUTHOR_OUTPUT;
            fPath = HE_HISTORY_OUTPUT(fileNr);
            break;
        default:
            path = EN_AUTHOR_OUTPUT;
            fPath = EN_HISTORY_OUTPUT(fileNr);
            break;
    }
    size_t count = 0;
    FILE* opFile = LoggingUtil::openFile(path, true);
    fputs("_key,user_name\n", opFile);
    fclose(opFile);
    FILE* ipFile;
    while ((ipFile = fopen(fPath.c_str(), "r")))
    {
        fileNr++;
        fclose(ipFile);
        switch (lng)
        {
            case VI:
                fPath = VI_HISTORY_OUTPUT(fileNr);
                break;
            case HE:
                fPath = HE_HISTORY_OUTPUT(fileNr);
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
            case VI:
                fPath = VI_HISTORY_OUTPUT(i);
                break;
            case HE:
                fPath = HE_HISTORY_OUTPUT(i);
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
            int id = stol(parser.writeToString(ipFile, '\"'));
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
    fputs("_key,user_name\n", opFile);
    fclose(opFile);
    unordered_map<size_t,string>* authors = new unordered_map<size_t,string>();

    // get authors and save to map
    while (parser.findPattern(ipFile, "Pages/"))
    {
        string pageId = parser.writeToString(ipFile, '\"');
        if (!parser.findPattern(ipFile, "Authors/"))
        {
            LoggingUtil::warning("File " + string(fPath) + " ended unexpectedly");
            return count;
        }
        size_t userId = stol(parser.writeToString(ipFile, '\"'));
        auto it = authors->find(userId);
        if (it == authors->end())
            // add author to set
            authors->insert({userId,pageId});
        else
            // add page to entry
        {
            // check if page is already added
            vector<size_t> pages = ParsingUtil::stringToNumbers(it->second, ',');
            bool add = true;
            for (size_t i = 0; i < pages.size(); i++)
            {
                if (pages[i] == stol(pageId))
                    add = false;
            }
            if (add)
                it->second += ',' + pageId;
        }

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
    size_t count = 0, lineCount = 0;
    short fileNr = 1, outputNr = 1;
    switch (lng)
    {
        case VI:
            path = VI_HISTORY_OUTPUT(fileNr);
            lngCode = LNG_VI;
            break;
        case HE:
            path = HE_HISTORY_OUTPUT(fileNr);
            lngCode = LNG_HE;
            break;
        default:
            path = EN_HISTORY_OUTPUT(fileNr);
            lngCode = LNG_EN;
            break;
    }
    FILE* ipFile;
    while ((ipFile = fopen(path.c_str(), "r")))
    {
        fileNr++;
        fclose(ipFile);
        switch (lng)
        {
            case VI:
                path = VI_HISTORY_OUTPUT(fileNr);
                break;
            case HE:
                path = HE_HISTORY_OUTPUT(fileNr);
                break;
            default:
                path = EN_HISTORY_OUTPUT(fileNr);
                break;
        }
    }
    unordered_map<string,string>* authorLinks = new unordered_map<string,string>();
    for (short i = 1; i < fileNr; i++)
    {
        switch (lng)
        {
            case VI:
                path = VI_HISTORY_OUTPUT(i);
                break;
            case HE:
                path = HE_HISTORY_OUTPUT(i);
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
            size_t newID = stol(parser.writeToString(ipFile, '\"'));
            if (id != newID)
            {
                if (id)
                {
                    for (auto it = authors.begin(); it != authors.end(); it++)
                        for (auto jt = authors.begin(); jt != authors.end(); jt++)
                        {
                            if (it == jt)
                                continue;
                            string authorPair = to_string(*it) + ',' + to_string(*jt);
                            string pageString = lngCode + "Pages/";
                            auto pointer = authorLinks->find(authorPair);
                            if (pointer != authorLinks->end())
                                pointer->second += ',' + pageString + to_string(id);
                            else
                                authorLinks->insert({authorPair,pageString + to_string(id)});
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
            authors.insert(stol(parser.writeToString(ipFile, '\"')));
            count++;
        }
        for (auto it = authors.begin(); it != authors.end(); it++)
            for (auto jt = it; jt != authors.end(); ++jt)
            {
                if (it == jt)
                    continue;
                string authorPair = to_string(*it) + ',' + to_string(*jt);
                string pageString = lngCode + "Pages/";
                auto pointer = authorLinks->find(authorPair);
                if (pointer != authorLinks->end())
                    pointer->second += ',' + pageString + to_string(id);
                else
                    authorLinks->insert({authorPair,pageString + to_string(id)});
            }
    }
    switch (lng)
    {
        case VI:
            fPath = VI_AUTHORLINK_OUTPUT(outputNr);
            break;
        case HE:
            fPath = HE_AUTHORLINK_OUTPUT(outputNr);
            break;
        default:
            fPath = EN_AUTHORLINK_OUTPUT;
            break;
    }
    FILE* opFile = LoggingUtil::openFile(fPath, true);
    fputs("\"_from\",\"_to\",\"pages\"\n", opFile);
    for (auto it = authorLinks->begin(); it != authorLinks->end(); it++)
    {
        vector<size_t> authors = ParsingUtil::stringToNumbers(it->first, ',');
        fprintf(opFile, "\"%sAuthors/%zu\",\"%sAuthors/%zu\",\"[%s]\"\n",lngCode.c_str(),authors[0],lngCode.c_str(),authors[1],it->second.c_str());
        lineCount++;
        if (lineCount >= 1000000)
        {
            fclose(opFile);
            lineCount = 0;
            count = 0;
            outputNr++;
            switch (lng)
                {
                case VI:
                    fPath = VI_AUTHORLINK_OUTPUT(outputNr);
                    break;
                case HE:
                    fPath = HE_AUTHORLINK_OUTPUT(outputNr);
                    break;
                default:
                    fPath = EN_AUTHORLINK_OUTPUT;
                    break;
                }
            opFile = LoggingUtil::openFile(fPath, true);
            fputs("\"_from\",\"_to\",\"pages\"\n", opFile);
        }
    }
    return 1000000*outputNr+count;
}
