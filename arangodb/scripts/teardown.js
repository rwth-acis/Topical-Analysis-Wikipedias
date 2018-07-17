'use strict';

// create arangodb object
const db = require('@arangodb').db;

// drop article collection
if (!db._collection('scoArticles'))
{
    db._drop('scoArticles');
}

// drop category collection
if (!db._collection('scoCategories'))
{
    db._drop('scoCategories');
}

// drop edge collection for inter category links
if (!db._collection('scoCategoryLinks'))
{
    db._drop('scoCategoryLinks');
}
