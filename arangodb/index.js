//'use strict';

// arango objects
const router = require('@arangodb/foxx/router')();
const request = require('@arangodb/request');
const db = require('@arangodb').db;
const time = require('@arangodb').time;

module.context.use(router);
// returns the average path lengths of pages within communities and the main topic classifications
router.get('/getMainCategories/:language/:collection', function(req,res) {
    var lng;
    if (typeof (lng= req.pathParams.language) == "undefined")
    {
        res.send("Need to specify language");
        return;
    }
    var mainCategories = [];
    if (lng == "vi")
    {
        mainCategories.push('viPages/6853');
        mainCategories.push('viPages/1362');
        mainCategories.push('viPages/7506');
        mainCategories.push('viPages/7311058');
        mainCategories.push('viPages/37648');
        mainCategories.push('viPages/3080');
        mainCategories.push('viPages/7508');
        mainCategories.push('viPages/6515');
        mainCategories.push('viPages/7433');
        mainCategories.push('viPages/434002');
        mainCategories.push('viPages/64793');
        mainCategories.push('viPages/15516');
        mainCategories.push('viPages/3208634');
        mainCategories.push('viPages/6834');
        mainCategories.push('viPages/1331');
        mainCategories.push('viPages/4619');
        mainCategories.push('viPages/6845');
        mainCategories.push('viPages/24895');
        mainCategories.push('viPages/4113');
        mainCategories.push('viPages/6286');
        mainCategories.push('viPages/72306');
        mainCategories.push('viPages/34030');
        mainCategories.push('viPages/7494');
        mainCategories.push('viPages/8417');
    }
    const start = time();
    const commys = db._query("for doc in @@collection return doc",{'@collection':req.pathParams.collection});
    var it, result = [];
    while (typeof (it = commys.next()) !== "undefined")
    {
        // get pages for community
        const pages = it.pages;
//        var lookUpTable = [];
        var closestCount = [];
        var pathLengths = [];
        closestCount.fill(0,0,mainCategories.length-1);
        pathLengths.fill(0,0,mainCategories.length-1);
        for (var i = 0; i < pages.length; i++)
        {
            var closestCat = -1;
            var shortestPath = 9999;
            for (var j = 0; j < mainCategories.length; j++)
            {
                var collectionName = lng + "CategoryLinks";
                // check for shortest path in lookup table
//            const idA = (catsA[m]._to).split('/')[1];
//            const idB = (catsB[n]._to).split('/')[1];
//            var lookUpKey = idA + "," + idB;
//            if (typeof (path = lookUpTable[lookUpKey]) !== "undefined")
//            {
//                paths.push(path._countTotal);
//                if (shortestPath._countTotal > path._countTotal)
//                    shortestPath = path;
//           }
                // check for reversed path
//            lookUpKey = idB + "," + idA;
//            if (typeof (path = lookUpTable[lookUpKey]) !== "undefined")
//            {
//                paths.push(path._countTotal);
//                if (shortestPath._countTotal > path._countTotal)
//                    shortestPath = path;
//            }
                var path = db._query("for doc in any shortest_path @pageA to @pageB @@collection return doc", {'pageA': pages[i], 'pageB': mainCategories[j], '@collection':collectionName});
                if (typeof path !== "undefined" && path._countTotal > 0) // path length 1 means common category
                {
                    // add path to lookup table
//                    lookUpTable[lookUpKey] = path;
                    // get shortest path
                    if (shortestPath > path._countTotal)
                    {
                        closestCat = j;
                        shortestPath = path._countTotal;
                    }
                    // add path length to array
                    pathLengths[j] += path._countTotal;
                }
            }
            closestCount[closestCat]++;
        }
        // get overall closest main category
        var closestCat = 0;
        for (var i = 1; i < closestCount.length; i++)
            closestCat = closestCount[closestCat] < closestCount[i] ? i : closestCat;
        collectionName = lng + "Pages";
        const resCat = db._query("for doc in @@collection filter doc._id == @id return doc",{'@collection':collectionName,'id':mainCategories[closestCat]});
        result.push({"community":it.community,"closest_category":resCat,"average_path_length":pathLengths[closestCat]/pages.length});
    }
    res.send({"result":result,"duration":time()-start});
})

// returns the average path lengths between pages worked on by the given set of communities
router.get('/getPathLengths/:language/:collection/:community', function(req,res) {
    // get community
    if (typeof req.pathParams.language == "undefined")
    {
        res.send("Need to specify language");
        return;
    }
    const start = time();
    const commys = db._query("for doc in @@collection filter doc.community == @community return doc",{'@collection':req.pathParams.collection,'community':parseInt(req.pathParams.community)});
    var it, result = [];
    while (typeof (it = commys.next()) !== "undefined")
    {
        // get pages for community
        const pages = it.pages;
        var shortestPath = {"_countTotal":9999}, paths = [];
        for (var i = 0; i < pages.length-1; i++)
            for (var j = i+1; j < pages.length; j++)
            {
                // get categories
                var collectionName = req.pathParams.language + "CategoryLinks", catA = [], catB = [];
                const catsA = db._query("for doc in @@collection filter doc._from == @page return doc",{'@collection':collectionName,'page':pages[i]}).toArray();
                const catsB = db._query("for doc in @@collection filter doc._from == @page return doc",{'@collection':collectionName,'page':pages[j]}).toArray();
                // get array with shortest paths
                for (var m = 0; m < catsA.length; m++)
                    for (var n = 0; n < catsB.length; n++)
                    {
                        var path = db._query("for doc in any shortest_path @catA to @catB @@collection return doc", {'catA': catsA[m]._to, 'catB': catsB[n]._to, '@collection':collectionName});
                        if (typeof path !== "undefined" && path._countTotal > 0) // path length 1 means common category
                        {
                            paths.push(path._countTotal);
                            // get shortest path
                            if (shortestPath._countTotal > path._countTotal)
                                shortestPath = path;
                        }
                    }
            }
        // get average path length
        var avgPath = 0;
        for (var x = 0; x < paths.length; x++)
            avgPath += paths[x];
        avgPath /= paths.length;
        result.push({"community":it.community,"shortest_path":shortestPath,"average_path":avgPath,"duration":time()-start});
    }
    res.send(result);
})
router.get('/getPathLengths/:language/:collection', function(req,res) {
    // get community
    if (typeof req.pathParams.language == "undefined")
    {
        res.send("Need to specify language");
        return;
    }
    const start = time();
    const commys = db._query("for doc in @@collection return doc",{'@collection':req.pathParams.collection});
    var it, result = [];
    while (typeof (it = commys.next()) !== "undefined")
    {
        // get pages for community
        const pages = it.pages;
        var lookUpTable = [];
        var shortestPath = {"_countTotal":9999}, paths = [];
        for (var i = 0; i < pages.length-1; i++)
            for (var j = i+1; j < pages.length; j++)
            {
                // get categories
                var collectionName = req.pathParams.language + "CategoryLinks", catA = [], catB = [];
                const catsA = db._query("for doc in @@collection filter doc._from == @page return doc",{'@collection':collectionName,'page':pages[i]}).toArray();
                const catsB = db._query("for doc in @@collection filter doc._from == @page return doc",{'@collection':collectionName,'page':pages[j]}).toArray();
                // get array with shortest paths
                for (var m = 0; m < catsA.length; m++)
                    for (var n = 0; n < catsB.length; n++)
                    {
                        // check for shortest path in lookup table
                        const idA = (catsA[m]._to).split('/')[1];
                        const idB = (catsB[n]._to).split('/')[1];
                        var lookUpKey = idA + "," + idB;
                        if (typeof (path = lookUpTable[lookUpKey]) !== "undefined")
                        {
                            paths.push(path._countTotal);
                            if (shortestPath._countTotal > path._countTotal)
                                shortestPath = path;
                        }
                        // check for reversed path
                        lookUpKey = idB + "," + idA;
                        if (typeof (path = lookUpTable[lookUpKey]) !== "undefined")
                        {
                            paths.push(path._countTotal);
                            if (shortestPath._countTotal > path._countTotal)
                                shortestPath = path;
                        }
                        var path = db._query("for doc in any shortest_path @catA to @catB @@collection return doc", {'catA': catsA[m]._to, 'catB': catsB[n]._to, '@collection':collectionName});
                        if (typeof path !== "undefined" && path._countTotal > 0) // path length 1 means common category
                        {
                            // add path to lookup table
                            lookUpTable[lookUpKey] = path;
                            paths.push(path._countTotal);
                            // get shortest path
                            if (shortestPath._countTotal > path._countTotal)
                                shortestPath = path;
                        }
                    }
            }
        // get average path length
        var avgPath = 0;
        for (var x = 0; x < paths.length; x++)
            avgPath += paths[x];
        avgPath /= paths.length;
        result.push({"community":it.community,"shortest_path":shortestPath,"average_path":avgPath});
    }
    res.send({"result":result,"duration":time()-start});
})
// returns the pages edited by authors within the communities specified by the given tag
router.get('/getPages/:language/:community', function(req,res) {
    // get communites of authors
    if (typeof req.pathParams.language == "undefined")
    {
        res.send("Need to specify language");
        return
    }
    var collectionName = req.pathParams.language + "Authors";
    var commys = db._query("for doc in @@collection collect commy = doc.@community into auths return {'community':commy,'authors':auths}",{'@collection':collectionName,'community':req.pathParams.community});
    // get pages within each community
    var it, result = [];
    while (typeof (it = commys.next()) !== "undefined")
    {
        // get pages for each author
        var size = 0, pages_c = [];
        for (size = 0; typeof it.authors[size] !== "undefined"; size++)
        {
            const author = it.authors[size];
            collectionName = req.pathParams.language + "History";
            var pages_a = db._query("for doc in @@collection filter doc._to == @auth return doc._from",{'@collection':collectionName,'auth':author.doc._id}).toArray();
            pages_a.forEach(entry => pages_c.push(entry));
        }
        // delete duplicates
        var nullCount = 0;
        for (var i = 0; i < pages_c.length-1; i++)
            for (var j = i+1; j < pages_c.length; j++)
                if (pages_c[i] == pages_c[j] && typeof pages_c[i] !== "undefined")
                {
                    delete pages_c[j];
                    nullCount++;
                }
        result.push({"community":it.community,"size":size,"pages":pages_c,"pageCount":pages_c.length-nullCount});
    }
    res.send(result);
})
// returns the degree of the graph constructed based on the given collection
router.get('/getDegree/:collection', function(req,res) {
    // get collection from get param
    const collection = db._collection(req.pathParams.collection);
    if (typeof collection == "null" || typeof collection == "undefined")
    {
        res.send("No such collection");
        return;
    }
    if (collection.type() != 3)
    {
        res.send("Given collection is no edge collection");
        return;
    }
    // get outbound edges
    var minDegreeFrom = 99, maxDegreeFrom = 0, sumDegreeFrom = 0;
    var minDegreeFrom_v, maxDegreeFrom_v;
    var edges = db._query("for doc in @@collection collect from = doc._from with count into numb return {'vertex':from, 'degree':numb}", {'@collection': collection.name()});
    // get degrees
    var entry;
    while (typeof (entry = edges.next()) !== "undefined")
    {
        if (entry.degree < minDegreeFrom)
        {
            minDegreeFrom = entry.degree;
            minDegreeFrom_v = entry.vertex;
        }
        if (maxDegreeFrom < entry.degree)
        {
            maxDegreeFrom = entry.degree;
            maxDegreeFrom_v = entry.vertex;
        }
        sumDegreeFrom += entry.degree;
    }
    // get inbound edges
    var minDegreeTo = 99, maxDegreeTo = 0, sumDegreeTo = 0;
    var minDegreeTo_v, maxDegreeTo_v;
    edges = db._query("for doc in @@collection collect to = doc._to with count into numb return {'vertex':to, 'degree':numb}", {'@collection': collection.name()});
    while (typeof (entry = edges.next()) !== "undefined")
    {
        if (entry.degree < minDegreeTo)
        {
            minDegreeTo = entry.degree;
            minDegreeTo_v = entry.vertex;
        }
        if (maxDegreeTo < entry.degree)
        {
            maxDegreeTo = entry.degree;
            maxDegreeTo_v = entry.vertex;
        }
        sumDegreeTo += entry.degree;
    }
    res.send({"minDegreeFrom":minDegreeFrom,"minDegreeFrom_v":minDegreeFrom_v,"maxDegreeFrom":maxDegreeFrom,"maxDegreeFrom_v":maxDegreeFrom_v,"avgDegreeFrom":(sumDegreeFrom/edges.count()),
             "minDegreeTo":minDegreeTo,"minDegreeTo_v":minDegreeTo_v,"maxDegreeTo":maxDegreeTo,"maxDegreeTo_v":maxDegreeTo_v,"avgDegreeTo":(sumDegreeTo/edges.count())});
})
// returns minimal, maximal and average community size
router.get('/getCommunities/:collection/:community', function(req,res) {
    // get collection from param
    const community = req.pathParams.community;
    if (typeof community == "undefined")
    {
        res.send("Need to specify name of attribute field regarding community");
        return;
    }
    const collection = db._collection(req.pathParams.collection);
    if (typeof collection == "null" || typeof collection == "undefined")
    {
        res.send("No such collection");
        return;
    }
    if (collection.type() != 2)
    {
        res.send("Given collection is no document collection");
        return;
    }
    // get community stats
    var minCommunitySize = 99, maxCommunitySize = 0, sumCommunity = 0;
    var minCommunity, maxCommunity;
    const docs = db._query("for doc in @@collection collect commy = doc.@community with count into size return {'community':commy, 'size':size}", {'@collection': collection.name(),'community':community});
    var entry;
    while (typeof (entry = docs.next()) !== "undefined")
    {
        if (entry.size < minCommunitySize)
        {
            minCommunitySize = entry.size;
            minCommunity = entry.community;
        }
        if (maxCommunitySize < entry.size)
        {
            maxCommunitySize = entry.size;
            maxCommunity = entry.community;
        }
        sumCommunity += entry.size;
    }
    res.send({"minCommunitySize":minCommunitySize,"minCommunity":minCommunity,"maxCommunitySize":maxCommunitySize,"maxCommunity":maxCommunity,"avgCommunitySize":(sumCommunity/docs.count()),"communityCount":docs.count()});
})
