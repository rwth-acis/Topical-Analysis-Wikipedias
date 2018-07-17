#include <stdio.h>
//#include <iostream>
#include "src/xmlscraper.h"
#include "src/sqlscraper.h"
#include <getopt.h>

// default logfile path
#define XML_LOGFILE "./xmlLogfile"
#define SQL_LOGFILE "./sqlLogfile"
#define PARSING_LOGFILE "./parsingLogfile"
// sql input files
#define SQL_CATEGORYLINKS_PATH "../media/english/rawFiles/enwiki-20180701-categorylinks.sql"
#define SQL_TAXONOMY_PATH "../media/english/rawFiles/wiki.category.taxonomy.tree.sql"
// parsed sql files
#define TXT_CATEGORYLINKS_PATH "../media/english/categorylinks"
#define TXT_TAXONOMY_PATH "../media/english/wiki.category.taxonomy.tree"
#define TXT_EXT ".txt"
#define CSV_EXT ".csv"
// number of attributes
#define NUM_ATTRIBUTES_LINKS 7
#define NUM_ATTRIBUTES_TAXONOMY 2

using namespace std;

/*
 * ERROR CODES::
 * -1: No valid path given
 */
static struct option options[] = {
    {"logfile", no_argument, NULL, 'l'},
    {"articles", no_argument, NULL, 'a'},
    {"categories", no_argument, NULL, 'c'},
    {"filter", required_argument, NULL, 'f'},
    {"missing", required_argument, NULL, 'm'},
    {"range", required_argument, NULL, 'r'},
    {"sql", required_argument, NULL, 's'},
    {"linkmap", no_argument, NULL, 'i'},
    {"hashmap", no_argument, NULL, 'h'},
    {0, 0, 0, 0}
};

int main(int argc, char* argv[])
{
    if (argc < 2)
        cout << "Hello, I am a cool parsing program and can do the following things:\n"
            "--filter/-f parses files in ../media/english/rawFiles and saves them to "
            "../media/english/xmlFiles. Please specify either articles, categories or both "
            "so I know what to look for.\n"
            "--articles/-a and --categories/-c parses files in ../media/english/xmlFiles "
            "for articles and categories and writes the result to ../media/english/jsonFiles\n"
            "--logfile/-l changes the path of the logfile (default: ./Logfile) but if you think "
            "that this is a good idea, please make this your first flag. Otherwise I won't know "
            "until it's already too late :(\n"
            "--filter/-f only filters either articles or categories out of ../media/english/rawFile "
            "and writes the result to ../media/english/xmlFiles\n"
            "--range/-r filter either articles or categories only out of certain file in "
            "../media/english/rawFiles and writes the result to ../media/english/xmlFiles\n"
            "--sql/-s writes sql files located at ../media/english/rawFiles either to readable txt "
            "files or CSV files" << endl;

    // create scraper object with logfile
    XMLScraper* scraper = NULL;
    SQLScraper* otherScraper = NULL;

    // find out what to do
    char opt;
    string input;
    string path;
    bool filterA = false;
    bool filterC = false;
    int total = 0;
    int totalA = 0;
    int totalC = 0;
    int count = 0;
    while ((opt = getopt_long(argc, argv, "lacihf;r;s;", options, NULL)) != -1)
    switch(opt)
    {
        // change path to logfile
        case 'l':
            input = "";
            if (argc < 3)
            {
                cout << "Haha! I see that you want me to change the path of the logfile without even doing "
                        "anything. That's very funny :D If you need me, use one of the flags that actually "
                        "do something" << endl;
                return 0;
            }
            while (input.size() < 1)
            {
                cout << "Hello, I am a cool parsing program and am exited that you decided to change the old "
                        "stinky default path of the logfile! Please let me know where I'm suppose to put my "
                        "warnings" << endl;
                cin >> input;
            }
           scraper = new XMLScraper(PARSING_LOGFILE, input.c_str());
            break;
        // parse articles
        case 'a':
            //path = string(TXT_CATEGORYLINKS_PATH) + string(TXT_EXT);
            if (!scraper)
                scraper = new XMLScraper(PARSING_LOGFILE, XML_LOGFILE);
            if (!otherScraper)
                otherScraper = new SQLScraper(PARSING_LOGFILE, SQL_LOGFILE);
            input = "";
            while (tolower(input[0]) != 'y' && tolower(input[0]) != 'n' && !filterA)
            {
                cout << "Hello, I am a cool parsing program and really want to parse your articles! "
                        "However, you need to tell me if I first have to filter the article information "
                        "from the files in ../media/english/rawFiles\n"
                        "Say y(es) for yes and n(o) for no" << endl;
                cin >> input;
            }
            // minimize before parsing
            if (input[0] == 'y')
            {
                filterA = true;
                total = 0;
                count = 0;
                for (int i = 1 ; i < 28; i++)
                {
                    count = scraper->filterArticles(i);
                    cout << "Got " << count << " articles from file " << i << endl;
                    total += count;
                }
                cout << "Got " << total << " articles total" << endl;
            }
            // get article categories as json files
            cout << scraper->scrapeArticles() << endl;
            break;
        // parse categories
        case 'c':
            //path = string(TXT_CATEGORYLINKS_PATH) + string(TXT_EXT);
            if (!scraper)
                scraper = new XMLScraper(PARSING_LOGFILE, XML_LOGFILE);
            if (!otherScraper)
                otherScraper = new SQLScraper(SQL_LOGFILE);
            input = "";
            while (tolower(input[0]) != 'y' && tolower(input[0]) != 'n' && !filterC)
            {
                cout << "Hello, I am a cool parsing program and really want to parse your categories! "
                        "However, you need to tell me if I first have to filter the category information "
                        "from the files in ../media/english/rawFiles\n"
                        "Say y(es) for yes and n(o) for no" << endl;
                cin >> input;
            }
            // minimize before parsing
            if (tolower(input[0]) == 'y')
            {
                filterC = true;
                total = 0;
                count = 0;
                for (int i = 1 ; i < 28; i++)
                {
                    count = scraper->filterCategories(i);
                    cout << "Got " << count << " categories from file " << i << endl;
                    total += count;
                }
                cout << "Got " << total << " categories total" << endl;
            }
            // get category network data as json files
            //scraper->createHashmap();

            cout << scraper->scrapeCategories() << endl;
            break;
        // get filtered xml files
        case 'f':
            if (!scraper)
                scraper = new XMLScraper(PARSING_LOGFILE, XML_LOGFILE);
            total = 0;
            count = 0;
            if (!optarg)
            {
                cout << "Hello, I am a cool parsing program and would love to filter your files "
                        "but you need to tell me what to look for:\n"
                        "articles, categories or both" << endl;
                return -1;
            }
            if (tolower(optarg[0]) == 'a')
            {
                if (filterA)
                {
                    cout << "Hello, I am a cool parsing program and I do like parsing articles but I "
                            "can't help but feel like I just did that, so I am going to pass on "
                            "parsing your articles" << endl;
                    return 0;
                }
                for (int i = 1; i < 28; i++)
                {
                    count = scraper->filterArticles(i);
                    cout << "Got " << count << " articles from file " << i << endl;
                    total += count;
                }
                cout << "Got " << total << " articles total" << endl;
            }
            if (tolower(optarg[0]) == 'c')
            {
                if (filterC)
                {
                    cout << "Hello, I am a cool parsing program and I do like parsing categories but I "
                            "can't help but feel like I just did that, so I am going to pass on "
                            "parsing your categories" << endl;
                    return 0;
                }
                for (int i = 1; i < 28; i++)
                {
                    count = scraper->filterCategories(i);
                    cout << "Got " << count << " categories from file " << i << endl;
                    total += count;
                }
                cout << "Got " << total << " categories total" << endl;
            }
            if (tolower(optarg[0]) == 'b')
            {
                totalA = 0;
                totalC = 0;
                for (int i = 1; i < 28; i++)
                {
                    count = scraper->filterArticles(i);
                    cout << "Got " << count << " articles from file " << i << endl;
                    totalA += count;
                    count = scraper->filterCategories(i);
                    cout << "Got " << count << " categories from file " << i << endl;
                    totalC += count;
                }
                cout << "Got " << totalA << " articles total" << endl;
                cout << "Got " << totalC << " categories total" << endl;
            }
            break;
        case 'm':
            if (!scraper)
                scraper = new XMLScraper(PARSING_LOGFILE, XML_LOGFILE);
            totalA = 0;
            totalC = 0;
            if (!optarg)
                {
                    cout << "Hello, I am a cool parsing program and would love to find missing pages "
                            "but you need to tell me what to look for:\n"
                            "articles, categories or both" << endl;
                    return -1;
                }
            if (tolower(optarg[0]) == 'a')
            {
                totalA = scraper->missingArticles();
                cout << "Got " << totalA << " missing articles " << total << endl;
            }
            if (tolower(optarg[0]) == 'c')
            {
                totalC = scraper->missingCategories();
                cout << "Got " << totalC << " missing categories " << total << endl;
            }
            if (tolower(optarg[0]) == 'b')
            {
                totalA = scraper->missingArticles();
                totalC = scraper->missingCategories();
                cout << "Got " << totalA << " missing articles " << total << endl;
                cout << "Got " << totalC << " missing Categories " << total << endl;
            }
            break;
        case 'r':
            totalA = 0;
            totalC = 0;
            if (!scraper)
                scraper = new XMLScraper(PARSING_LOGFILE, XML_LOGFILE);
            count = 0;
            if (!optarg)
                {
                    cout << "Hello, I am a cool parsing program and parsing stuff is my favourite "
                            "thing in the world. However, you need to tell me whether you want me "
                            "to parse articles, categories or both" << endl;
                    return -1;
                }
            if (tolower(optarg[0]) == 'a')
            {
                input = "";
                int start, range;
                cout << "Hello, I am a cool parsing program and am happy to parse your articles! "
                        "At which article file should I start?" << endl;
                cin >> input;
                start = stoi(input);
                cout << "Sounds great! And how many files do you want me to parse?" << endl;
                cin >> input;
                range = stoi(input);
                for (int i = start; i < start+range; i++)
                    totalA += scraper->filterArticles(i);
                cout << "Got " << totalA << " articles" << endl;
            }
            if (tolower(optarg[0]) == 'c')
            {
                input = "";
                int start, range;
                cout << "Hello, I am a cool parsing program and am happy to parse your categories! "
                        "At which category file should I start?" << endl;
                cin >> input;
                start = stoi(input);
                cout << "Sounds great! And how many files do you want me to parse?" << endl;
                cin >> input;
                range = stoi(input);
                for (int i = start; i < start+range; i++)
                    totalC += scraper->filterCategories(i);
                cout << "Got " << totalC << " categories" << endl;
            }
            if (tolower(optarg[0]) == 'b')
            {
                input = "";
                int start, range;
                cout << "Hello, I am a cool parsing program and am happy to please all your "
                        "article and category parsing needs! "
                        "At which file should I start?" << endl;
                cin >> input;
                start = stoi(input);
                cout << "Sounds great! And how many files do you want me to parse?" << endl;
                cin >> input;
                range = stoi(input);
                for (int i = start; i < start+range; i++)
                {
                    totalA = scraper->filterArticles(i);
                    totalC = scraper->filterCategories(i);
                }
                cout << "Got " << totalA << " articles" << endl;
                cout << "Got " << totalC << " categories" << endl;
            }
            break;
        case 's':
            if (!optarg)
            {
                cout << "Hello, I am a cool parsing program and parsing SQL is not something I do very well :( "
                        "I will try 'though :D\nHowever, you need to tell me whether you want me to print "
                        "the result as text or csv files" << endl;
                return -1;
            }
            if (!otherScraper)
                otherScraper = new SQLScraper(SQL_LOGFILE);
            cout << "Hello, I am a cool parsing program and will try my best to parse your SQL" << endl;
            if (tolower(optarg[0]) == 't')
            {
                path = string(TXT_CATEGORYLINKS_PATH)+string(TXT_EXT);
                count = otherScraper->makeReadable(SQL_CATEGORYLINKS_PATH, path.c_str(), NUM_ATTRIBUTES_LINKS);
                cout << "Got " << count << " entries from file " << SQL_CATEGORYLINKS_PATH << endl;
                path = string(TXT_TAXONOMY_PATH)+string(TXT_EXT);
                count = otherScraper->makeReadable(SQL_TAXONOMY_PATH, path.c_str(), NUM_ATTRIBUTES_TAXONOMY);
                cout << "Got " << count << " entries from file " << SQL_TAXONOMY_PATH << endl;
                count = otherScraper->splitFiles(path.c_str(), TXT_TAXONOMY_PATH);
                cout << "Created " << count << " output files for file " << TXT_TAXONOMY_PATH << endl;
                break;
            }
            else
            {
                path = string(TXT_CATEGORYLINKS_PATH)+string(CSV_EXT);
                count = otherScraper->sqlToCSV(SQL_CATEGORYLINKS_PATH, path.c_str());
                cout << "Got " << count << " entries from file " << SQL_CATEGORYLINKS_PATH << endl;
            }
        case 'i':
            otherScraper->createLinkmap(SQL_CATEGORYLINKS_PATH);
            break;
        case 'h':
            scraper->createHashmap();
            break;
    }
    return 0;
}
