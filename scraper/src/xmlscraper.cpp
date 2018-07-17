#include "xmlscraper.h"
#include "util/loggingutil.h"
#include <unordered_map>

// path to arangoDB
#define BASE_PATH "http://localhost:8529//_db/_system/wikipics/"
// path to raw files
#define RAW_PATH(x) "../media/english/rawFiles/articles/enwiki-latest-pages-articles" + to_string(x) + ".xml"
#define RAW_TITLES_PATH "../media/english/rawFiles/enwiki-latest-all-titles"
// path to minimized files
#define XML_CATEGORY_PATH(x) "../media/english/xmlFiles/categories" + to_string(x) + ".xml"
#define XML_ARTICLE_PATH(x) "../media/english/xmlFiles/articles" + to_string(x) + ".xml"
// path to json files with categories/articles/category links
#define JSON_CATEGORY_PATH(x) "../media/english/jsonFiles/categories" + to_string(x) + ".json"
#define JSON_ARTICLE_PATH(x) "../media/english/jsonFiles/articles" + to_string(x) + ".json"
#define JSON_CATEGORYLINK_PATH(x) "../media/english/jsonFiles/categoryLinks" + to_string(x) + ".json"
// path to link-/hashmap
#define HASHMAP_PATH "../media/english/jsonFiles/hashmap.json"
#define LINKMAP_PATH "../media/english/jsonFiles/linkmap.json"
// XML TAGS
#define PAGE_END "</page>"
#define BODY_START "<text"
#define BODY_END "</text>"
#define TITLE_START "<title>"
#define TITLE_END "</title>"
#define CATEGORY_START "<title>Category:"
#define NAMESPACE_START "<ns>"
#define ID_START "<id>"
#define REDIRECT "<redirect"
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
// content held in backlog
#define CATEGORY_BACKLOG "<page>\n    <title>Category:"
#define ARTICLE_BACKLOG "<page>\n    <title>"

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

int XMLScraper::createHashmap()
{
    FILE* pFile = fopen(HASHMAP_PATH, "r");
    if (pFile)
    {
        cout << "Overwriting existing hashmap for categories and ids" << endl;
        fclose(pFile);
    }
    else
        cout << "Creating hashmap for categories and ids at " << HASHMAP_PATH << endl;
    unordered_map<string,int>* hashmap = new unordered_map<string,int>();

    for (int i = 1; i <= 27; i++)
    {
        string f_path = XML_CATEGORY_PATH(i);
        cout << "Opening file " << f_path << endl;
        FILE* ipFile = fopen(f_path.c_str(), "r");
        if (!ipFile)
        {
            cerr << "Could not open file" << endl;
            continue;
        }

        int res, count = 0;
        while (!(res = parser.findPattern(ipFile, CATEGORY_START)))
        {
            bool fatal = false;

            // get Title
            DynamicBuffer titleBuffer(TITLE_LENGTH);
            titleBuffer.setMax(PATH_LENGTH);
            fatal |= (parser.writeToBuffer(ipFile, &titleBuffer, stopChars) == -1);

            // get id
            parser.findPattern(ipFile, ID_START);
            int id = stoi(parser.writeToString(ipFile, stopChars));

            // create hashmap entry
            pair<string, int> entry(titleBuffer.print(), id);
            hashmap->insert(entry);
            count++;

            // check if fatal error has occured
            if (fatal)
            {
                cerr << "Fatal error (Buffer overflow): Integrity of file cannot be guaranteed" << endl;
                delete hashmap;
                return -count;
            }
        }

        if (res == -1)
            LoggingUtil::warning("File " + f_path + " ended unexpectedly", logfile);

        cout << "Got " << count << " categories" << endl;
    }

    // save hashmap to file
    int count = 0;
    FILE* opFile = fopen(HASHMAP_PATH, "w");
    fputs("[\n", opFile);
    for (auto it = hashmap->begin(); it != hashmap->end(); it++)
    {
        fputs("{\"title\":\"", opFile);
        fputs(it->first.c_str(), opFile);
        fputs("\",\"id\":\"", opFile);
        fputs(to_string(it->second).c_str(), opFile);
        fputs("\"},\n", opFile);
        count++;
    }
    fputs("]", opFile);
    fclose(opFile);
    return count;
}

int XMLScraper::scrapeCategories()
{
    cout << "Loading hashmap from " << HASHMAP_PATH << endl;
    unordered_map<string,int>* hashmap = parser.parseHashmap(HASHMAP_PATH);
    if (!hashmap)
        return 0;
    cout << "Loading linkmap from " << LINKMAP_PATH << endl;
    unordered_map<int,string>* linkmap = parser.parseLinkmap(LINKMAP_PATH);
    if (!linkmap)
        return 0;

    int totalCount = 0;
    for (int i = 1; i <= 27; i++)
    {
        string f_path = XML_CATEGORY_PATH(i);
        cout << "Opening file " << f_path << endl;
        FILE* ipFile = fopen(f_path.c_str(), "r");
        if (!ipFile)
        {
            cerr << "Could not open file" << endl;
            continue;
        }

        string outputFile = JSON_CATEGORYLINK_PATH(i);
        FILE* opFile = fopen(outputFile.c_str(), "r");
        if (opFile)
        {
            cout << "Overwriting " << outputFile << endl;
            fclose(opFile);
        }
        else
            cout << "Creating output file for edges" << endl;

        opFile = fopen(outputFile.c_str(), "w");
        fputs("[\n", opFile);

        int res;
        bool first = true;
        while (!(res = parser.findPattern(ipFile, CATEGORY_START)))
        {
            // get ID
            parser.findPattern(ipFile, ID_START);
            int id = stoi(parser.writeToString(ipFile, stopChars));

            // get categories
            int count = 0;
            unordered_map<int,string>::const_iterator linkmapEntry = linkmap->find(id);
            if (linkmapEntry != linkmap->end())
            {
                // get ids of category
                vector<int>* categories = this->writeCategoryToBuffer(linkmapEntry);
                // write to file
                for (int i = 0; i < categories->size(); i++)
                {
                    if (first)
                        fprintf(opFile, "{\"_from\":\"enCategories/%d\",\"_to\":\"enCategories/%d\"}\n", categories->at(i), id);
                    else
                        fprintf(opFile, ",{\"_from\":\"enCategories/%d\",\"_to\":\"enCategories/%d\"}\n", categories->at(i), id);
                    totalCount++;
                    first = false;
                }
                count = categories->size();
            }
            if (count == 0)
                LoggingUtil::warning("No category for page " + to_string(id), logfile);
        }
        if (res == -1)
            LoggingUtil::warning("File " + f_path + " ended unexpectedly", logfile);

        fputc(']', opFile);
        fclose(opFile);
    }

    delete hashmap;
    return totalCount;
}

int XMLScraper::filterCategories(int fileNr)
{
    if (fileNr < 1 || fileNr > 27)
    {
        cerr << "File number " << fileNr << " does not exist" << endl;
        return -1;
    }

    // try to open file
    string f_path = RAW_PATH(fileNr);
    FILE* ipFile = fopen(f_path.c_str(), "r");
    if (!ipFile)
    {
        cerr << "Could not open file " + f_path << endl;
        return -1;
    }

    // create output file
    string f_name = XML_CATEGORY_PATH(fileNr);
    FILE* opFile = fopen(f_name.c_str(), "r");
    if (opFile)
    {
        cout << "Overwriting file " << f_name << endl;
        fclose(opFile);
    }
    else
        cout << "Creating minimized XML file for file " << f_path << endl;
    fclose(opFile);
    opFile = fopen(f_name.c_str(), "w");

    // only write category pages
    int count = 0;
    while (!(parser.findPattern(ipFile, CATEGORY_START)))
    {
        // check if category is hidden or stub
        DynamicBuffer pageBuffer(MAX_BUFFER_SIZE);
        if (this->writeCategory(ipFile, &pageBuffer) < 0)
            continue;

        // write to output file
        fputs(CATEGORY_BACKLOG, opFile);
        fputs(pageBuffer.raw(), opFile);
        count++;
    }
    if (count == 0)
        LoggingUtil::warning("No category in " + f_path, logfile);

    fclose(opFile);
    return count;
}

int XMLScraper::scrapeArticles()
{
    // load linkmap for categories
    cout << "Loading linkmap from " << LINKMAP_PATH << endl;
    unordered_map<int,string>* linkmap = parser.parseLinkmap(LINKMAP_PATH);
    if (!linkmap)
        return 0;

    // get articles
    int totalCount = 0, articleCount = 0;
    for (int i = 1; i <= 27; i++)
    {
        string f_path = XML_ARTICLE_PATH(i);
        cout << "Opening file " << f_path << endl;
        FILE* ipFile = fopen(f_path.c_str(), "r");
        if (!ipFile)
        {
            cerr << "Could not open file" << endl;
            continue;
        }

        string outputFile = JSON_ARTICLE_PATH(i);
        FILE* opFile = fopen(outputFile.c_str(), "r");
        if (opFile)
        {
            cout << "Overwriting " << outputFile << endl;
            fclose(opFile);
        }
        else
            cout << "Creating output file for articles" << endl;
        opFile = fopen(outputFile.c_str(), "w");
        fputs("[\n", opFile);

        int res, count = 0;
        while (!(res = parser.findPattern(ipFile, TITLE_START)))
        {
            // print title to output file
            if (count)
                fputs(",\n", opFile);
            fputs("{\"title\":\"", opFile);
            parser.writeToFile(ipFile, opFile, stopChars);
            fputs("\",\"_key\":\"", opFile);

            // get id
            parser.findPattern(ipFile, ID_START);
            int id = stoi(parser.writeToString(ipFile, stopChars));
            fprintf(opFile, "%d\",\"categories\":[", id);

            // get categories from linkmap
            unordered_map<int,string>::const_iterator linkmapEntry = linkmap->find(id);
            if (linkmapEntry != linkmap->end())
            {
                // get ids of categories
                vector<int>* categories = this->writeCategoryToBuffer(linkmapEntry);
                totalCount += categories->size();
                // write to file
                for (int i = 0; i < categories->size(); i++)
                {
                    if (i > 0)
                        fputs(",", opFile);

                    fprintf(opFile, "\"enCategories/%d\"", categories->at(i));
                }
                if (categories->size() == 0)
                    LoggingUtil::warning("No category for article " + to_string(id), logfile);
            }
            else
                LoggingUtil::warning("No category for article " + to_string(id), logfile);
            fputs("]}", opFile);
            count++;
        }
        if (res == -1)
            LoggingUtil::warning("File " + f_path + " ended unexpectedly", logfile);

        fputs("\n]", opFile);
        fclose(opFile);
        articleCount += count;
    }
    return totalCount;
}

int XMLScraper::filterArticles(int fileNr)
{
    if (fileNr < 1 || fileNr > 27)
    {
        cerr << "File number " << fileNr << " does not exist" << endl;
        return -1;
    }

    // try to open file
    string f_path = RAW_PATH(fileNr);
    FILE* ipFile = fopen(f_path.c_str(), "r");
    if (!ipFile)
    {
        cerr << "Could not open file " + f_path << endl;
        return -1;
    }

    // create output file
    string f_name = XML_ARTICLE_PATH(fileNr);
    FILE* opFile = fopen(f_name.c_str(), "r");
    if (opFile)
    {
        cout << "Overwriting file " << f_name << endl;
        fclose(opFile);
    }
    else
        cout << "Creating minimized XML file for file " << f_path << endl;
    opFile = fopen(f_name.c_str(), "w");

    // only write article pages
    int count = 0;
    while (!(parser.findPattern(ipFile, TITLE_START)))
    {
        vector<char> escapeChars = { '\\' };
        bool fatal = false;
        // check namespace and for redirect tag
        DynamicBuffer nsBuffer(BACKLOG_SIZE);
        nsBuffer.setMax(LARGE_BACKLOG);
        DynamicBuffer dynBuffer(LARGE_BACKLOG);
        dynBuffer.setMax(MAX_BUFFER_SIZE);
        fatal |= (parser.findPattern(ipFile, NAMESPACE_START, &nsBuffer) == -1);
        nsBuffer.write("0<");
        int ns = stoi(parser.writeToString(ipFile, stopChars));
        fatal |= (parser.findPattern(ipFile, BODY_START, &dynBuffer, escapeChars) == -1);
        DynamicBuffer bodyBuffer(MAX_BUFFER_SIZE);
        bodyBuffer.setMax(ACTUAL_BUFFER_LIMIT);
        // if page is no article or redirect or disambiguation go to next page
        if (ns)
            continue;
        if (parser.findPattern(&dynBuffer, REDIRECT))
            continue;
        if (this->writeArticle(ipFile, &bodyBuffer) == -2)
        {
            LoggingUtil::error("Buffer overflow:\n" + bodyBuffer.print(), logfile);
            return -1;
        }
        // write to output file
        fputs(ARTICLE_BACKLOG, opFile);
        fputs(nsBuffer.raw(), opFile);
        fputs(dynBuffer.raw(), opFile);
        fputs(bodyBuffer.raw(), opFile);
        count++;

        // check if fatal error has occured
        if (fatal)
        {
            cerr << "Fatal error (Buffer overflow): Integrity of file cannot be guaranteed" << endl;
            fclose(opFile);
            return -count;
        }
    }
    if (count == 0)
        LoggingUtil::warning("No article in " + f_path, logfile);
    fclose(opFile);
    return count;

}

int XMLScraper::missingArticles()
{
    return 0;
}

int XMLScraper::missingCategories()
{
    return 0;
}

vector<int>* XMLScraper::writeCategoryToBuffer(unordered_map<int,string>::const_iterator categoryLinks)
{
    // get category ids from string
    vector<int>* res = new vector<int>();
    string idBuffer = "";
    for (int i = 0; i < categoryLinks->second.size(); i++)
    {
        char c = categoryLinks->second[i];
        if (c == '0' || c == '1' || c == '2' || c == '3' || c == '4' || c == '5' || c == '6' || c == '7' || c == '8' || c == '9')
            idBuffer += c;
        if (c == ',')
        {
            res->push_back(stoi(idBuffer));
            idBuffer = "";
        }
        if (c != '0' && c != '1' && c != '2' && c != '3' && c != '4' && c != '5' && c != '6' && c != '7' && c != '8' && c != '9' && c != ',')
            cout << categoryLinks->second << endl;
    }

    return res;
}

int XMLScraper::writeCategoryToBuffer(FILE* ipFile, DynamicBuffer* buffer, vector<char> stopChars, bool inBody, bool* knownTemplate)
{
    size_t startBody = 0;
    size_t endBody = 0;
    size_t category = 0;
    size_t usesTemplate = 0;
    string startOfBody = BODY_START;
    string endOfBody = BODY_END;
    string categoryLink = "[[Category:";
    buffer->flush();

    // check for end of body and category link at the same time
    // if category link is not within body, disregard it
    // pattern {{ means category uses template
    char c;
    while ((c = fgetc(ipFile)) != EOF)
    {
        if (c == '{' && inBody && !(*knownTemplate))
            usesTemplate++;
        else
            usesTemplate = 0;

        // uses template
        if (usesTemplate == 2)
        {
            LoggingUtil::warning("Page uses template", logfile);
            *knownTemplate = true;
        }

        if (c == (char) toupper(startOfBody[startBody]) || c == (char) tolower(startOfBody[startBody]))
            startBody++;
        else
            startBody = 0;

        if (c == (char) toupper(endOfBody[endBody]) || c == (char) tolower(endOfBody[endBody]))
            endBody++;
        else
            endBody = 0;

        if (c == (char) toupper(categoryLink[category]) || c == (char) tolower(categoryLink[category]))
            category++;
        else
            category = 0;

        // start of body
        if (startBody >= startOfBody.size())
            inBody = true;

        // end of body
        if (endBody >= endOfBody.size())
            return 0;

        // found category
        if (category >= categoryLink.size())
        {
            if (inBody)
            {
                if (parser.writeToBuffer(ipFile, buffer, stopChars) == -1)
                    return -2;
                return 1;
            }
            category = 0;
        }
    }

    return -1;
}

int XMLScraper::writeArticle(FILE* ipFile, DynamicBuffer* buffer)
{
    // if {{disambiguation}} appears, disregard article
    char c;
    string stopStr = "{{disambiguation}}";
    string endOfPage = PAGE_END;
    size_t stopState = 0;
    size_t pageEnd = 0;
    buffer->flush();
    while ((c = fgetc(ipFile)) != EOF)
    {
        if (buffer->write(c) == -1)
            return -2;


        if (c == (char) toupper(stopStr[stopState]) || c == (char) tolower(stopStr[stopState]))
            stopState++;
        else
            stopState = 0;
        if (c == (char) toupper(endOfPage[pageEnd]) || c == (char) tolower(endOfPage[pageEnd]))
            pageEnd++;
        else
            pageEnd = 0;

        // disamiguation article
        if (stopState >= stopStr.size())
            return -1;

        // regular article
        if (pageEnd >= endOfPage.size())
            return 1;
    }

    return 0;
}

int XMLScraper::writeCategory(FILE* ipFile, DynamicBuffer* buffer)
{
    // if body contains {{Stub Category or {{hidden category}},
    // if tracking = yes, container = yes or hidden = yes
    // appears in {{}} this category is disregarded

    char c;
    string stubCat = "{{Stub Category";
    string hiddenCat = "{{hidden category}}";
    string trackingCat = "{{tracking category}}";
    string tracking = "tracking = yes";
    string container = "container = yes";
    string hidden = "hidden = yes";
    string endOfPage = PAGE_END;
    int stubCount = 0;
    int hiddenCatCount = 0;
    int trackingCatCount = 0;
    int trackingCount = 0;
    int containerCount = 0;
    int hiddenCount = 0;
    int bracketCount = 0;
    int pageEnd = 0;
    bool inBrackets = false;
    size_t bufferSize = buffer->size();
    buffer->flush();
    while ((c = fgetc(ipFile)) != EOF)
    {
        int bufferState;
        // dont continue if buffer is maxed
        if ((bufferState = buffer->write(c)) == -1)
            return -2;
        // double buffer size if size has to be changed
        if (bufferState > bufferSize)
            // dont continue if bufffer is maxed
            if (!(bufferSize = buffer->enlarge(bufferSize*2)))

        if (bracketCount >= 2)
            inBrackets = true;
        if (bracketCount <= 0)
            inBrackets = false;
        if (c == '{' && !inBrackets)
            bracketCount++;
        if (c == '}' && inBrackets)
            bracketCount--;
        if (c != '{' && !inBrackets)
            bracketCount = 0;
        if (c != '}' && inBrackets)
            bracketCount = 2;

        if (c == (char) toupper(endOfPage[pageEnd]) || c == (char) tolower(endOfPage[pageEnd]))
            pageEnd++;
        else
            pageEnd = 0;
        if (c == (char) toupper(stubCat[stubCount]) || c == (char) tolower(stubCat[stubCount]))
            stubCount++;
        else
            stubCount = 0;
        if (c == (char) toupper(hiddenCat[hiddenCatCount]) || c == (char) tolower(hiddenCat[hiddenCatCount]))
            hiddenCatCount++;
        else
            hiddenCatCount = 0;
        if (c == (char) toupper(trackingCat[trackingCatCount]) || c == (char) tolower(trackingCat[trackingCatCount]))
            trackingCatCount++;
        else
            trackingCount = 0;
        if (c == (char) toupper(tracking[trackingCount]) || c == (char) tolower(tracking[trackingCount]))
            trackingCount++;
        else
            trackingCount = 0;
        if (c == (char) toupper(container[containerCount]) || c == (char) tolower(container[containerCount]))
            containerCount++;
        else
            containerCount = 0;
        if (c == (char) toupper(hidden[hiddenCount]) || c == (char) tolower(hidden[hiddenCount]))
            hiddenCount++;
        else
            hiddenCount = 0;

        // stub category
        if (stubCount >= stubCat.size())
            return -1;
        // hidden category
        if (hiddenCatCount >= hiddenCat.size())
            return -2;
        // tracking category
        if (trackingCount >= tracking.size() && inBrackets)
            return -3;
        // container category
        if (containerCount >= container.size() && inBrackets)
            return -4;
        // hidden category
        if (hiddenCount >= hidden.size() && inBrackets)
            return -5;
        if (trackingCatCount >= trackingCat.size())
            return -6;

        // regular category
        if (pageEnd >= endOfPage.size())
            return 1;
    }
    return 0;
}
