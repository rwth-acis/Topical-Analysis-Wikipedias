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
#define SQL_CATEGORYLINKS_PATH "../../media/english/rawFiles/enwiki-20180701-categorylinks.sql"
#define SQL_TAXONOMY_PATH "../../media/english/rawFiles/wiki.category.taxonomy.tree.sql"
// parsed sql files
#define TXT_CATEGORYLINKS_PATH "../../media/english/categorylinks"
#define TXT_TAXONOMY_PATH "../../media/english/wiki.category.taxonomy.tree"
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
    {"scrape", no_argument, NULL, 's'},
    {"linkmap", no_argument, NULL, 'i'},
    {0, 0, 0, 0}
};

int main(int argc, char* argv[])
{
    // create scraper object with logfile
    XMLScraper* scraper = NULL;
    SQLScraper* otherScraper = NULL;

    // find out what to do
    char opt;
    string input;
    while ((opt = getopt_long(argc, argv, "lsi", options, NULL)) != -1)
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
        case 's':
            cout << "Hello, I am a nervous wreck at this point and hope I don't explode your RAM\n"
                    "If that should happen, please note: ...sorry\n";
            if (!scraper)
                scraper = new XMLScraper(PARSING_LOGFILE, XML_LOGFILE);
            scraper->scrapePages();
            break;
        case 'i':
            if (!otherScraper)
                otherScraper = new SQLScraper(PARSING_LOGFILE, SQL_LOGFILE);
            otherScraper->createLinkmap(SQL_CATEGORYLINKS_PATH);
            break;
    }
    return 0;
}
