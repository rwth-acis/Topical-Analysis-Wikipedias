'use strict';

// create arangodb object
const db = require('@arangodb').db;

// create article collection
if (!db._collection('scoArticles'))
{
    db._createDocumentCollection('scoArticles');
}

// create category collection
if (!db._collection('scoCategories'))
{
    db._createDocumentCollection('scoCategories');
}

// create edge collection for inter category links
if (!db._collection('scoCategoryLinks'))
{
    db._createEdgeCollection('scoCategoryLinks');
}
