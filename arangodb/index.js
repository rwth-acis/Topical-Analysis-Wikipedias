//'use strict';

// arango objects
const router = require('@arangodb/foxx/router')();
const request = require('@arangodb/request');
const db = require('@arangodb').db;
const time = require('@arangodb').time;

module.context.use(router);
// canceles the service with given id
router.get('/cancelService/:id', function(req,res) {
    if (typeof req.pathParams.id === "undefined")
    {
        res.send("Specify service to cancel");
        return;
    }
    db._query("for doc in serviceLogs filter doc._id == @id update doc with {'status':'canceled'} in serviceLogs", {'id':req.pathParams.id});
    res.send("Canceled service " + req.pathParams.id);
})
router.get('/getPathLengths/:language/:community', function(req,res) {
    // get community
    var community;
    if (typeof (community = req.pathParams.community) == "undefined")
    {
        res.send("Need to specify community");
        return;
    }
    const collection = community.split('/')[0];
    const commys = db._query("for doc in @@collection filter doc._id == @id && doc.pageCount > 1 return doc",{'@collection':collection,'id':community});
    if (typeof req.pathParams.language == "undefined")
    {
        res.send("Need to specify language");
        return;
    }
    const lng = req.pathParams.language;
    const start = time();
    var timestamp = Date(start).toString();
    var status = "running";
    var collectionName = lng + "CategoryLinks";
    const categoryGraphName = "Pages";
    const categoryTreeName = "Categories";
    const categoryGraph = require("@arangodb/general-graph")._graph(categoryGraphName);
    const categoryTree = require("@arangodb/general-graph")._graph(categoryTreeName);
    const pathCollection = "pathLengths";
    const logKey = db._collection("serviceLogs").save({"function":{"type":"getPathLengths","language":lng,"collection":collection,"community":community},"start":timestamp,"pathQueries":0,"status":status});
    var it, queryCount = 0;
    while (typeof (it = commys.next()) !== "undefined")
    {
        // update log entry
        db._query("for doc in serviceLogs filter doc._id == @id update doc with {'pathQueries':@qc} in serviceLogs",{'id':logKey._id,'qc':queryCount});
        // if status == canceled break loop
        if (status == "canceled")
            break;
        // get pages for community
        const pages = it.pages;
        var shortest = 9999, sumShortest = 0, sumAvg = 0, count = 0;
        for (var i = 0; i < pages.length-1; i++)
        {
            if (status == "canceled")
                break;
            for (var j = i+1; j < pages.length; j++)
            {
                if (db._query("for doc in serviceLogs filter doc._id == @id return doc.status",{'id':logKey._id}).next() === "canceled")
                {
                    status = "canceled";
                    break;
                }
                // get path
                let result = getCategoryPaths(pages[i],pages[j],categoryGraph,categoryTreeName,pathCollection,collection);
                if (result.count == 0)
                    continue;
                queryCount += result.count;
                shortest = (result.shortestPath < shortest) ? result.shortestPath : shortest;
                if (result.shortestPath < 9999 && result.avgPath > 0)
                {
                    sumShortest += result.shortestPath;
                    sumAvg += result.avgPath;
                    count++;
                }
            }
        }
        // get average path length and store result
        let communityRes = {"shortest_path":shortest,"average_shortest_path":sumShortest/count,"avgavg_shortest_path":sumAvg/count};
        db._query("for doc in @@collection filter doc._id == @id update doc with {'shortestPaths':@result} in @@collection",{'@collection':collection,'id':it._id,'result':communityRes});
    }
    const fin = time();
    timestamp = Date(fin).toString();
    if (status != "canceled")
        status = "done";
    db._query("for doc in serviceLogs filter doc._id == @id update doc with {'status':@status,'fin':@time,'pathQueries':@qc} in serviceLogs",{'id':logKey._id,'time':timestamp,'status':status,'qc':queryCount});
    res.send({"duration":fin-start});
})
// returns the average path lengths between pages worked on by the given set of communities
router.get('/getPathLengtsRandomly/:language/:collection/:amount', function(req,res) {
    // get language
    var lng, collection;
    if (typeof (lng = req.pathParams.language) == "undefined")
    {
        res.send("Need to specify language");
        return;
    }
    // get community collection
    if (typeof (collection = req.pathParams.collection) == "undefined")
        collection = "none";
    const start = time();
    const amount = parseInt(req.pathParams.amount);
    var timestamp = Date(start).toString();
    var status = "running";
    const categoryGraphName = "Pages";
    const categoryTreeName = "Categories";
    const categoryGraph = require("@arangodb/general-graph")._graph(categoryGraphName);
    const categoryTree = require("@arangodb/general-graph")._graph(categoryTreeName);
    const pathCollection = "pathLengths";
    const logKey = db._collection("serviceLogs").save({"function":{"type":"getPathLengths","language":lng,"collection":collection,"amount":amount},"start":timestamp,"pairsPassed":0,"status":status});
    var pages = [], numPages = 0;
    // arbitrary article pairs include all articles
    if (collection == "none")
    {
        pages = db._query("for doc in @@pages filter doc.namespace == '0' return doc",{'@pages':lng + "Pages"}).toArray();
        numPages = pages.length;
    }
    else
    {
        pages = db._query("for doc in @@collection filter doc.pageCount > 1 return doc.pages",{'@collection':collection}).toArray();
        pages.forEach(entry => numPages += entry.length);
    }
    var shortest = 9999, sumShortest = 0, sumAvg = 0, count = 0;
    for (let i = 0; i < amount; i++)
    {
        // update log entry
        db._query("for doc in serviceLogs filter doc._id == @id update doc with {'pairsPassed':@count} in serviceLogs",{'id':logKey._id,'count':count});
        // if status == canceled break loop
        if (db._query("for doc in serviceLogs filter doc._id == @id return doc.status",{'id':logKey._id}).next() === "canceled")
        {
            status = "canceled";
            break;
        }
        // get random pair of articles
        let idA = parseInt(Math.random()*numPages);
        var community = 0, commySize = 0;
        if (collection != "none")
        {
            // get community of page
            var pageCount = 0;
            while (pageCount <= idA)
                pageCount += pages[community++].length;
            // get community size
            commySize = pages[--community].length;
            idA = commySize + (idA - pageCount);
        }
        var idB = idA;
        var pageA, pageB;
        while (idA == idB)
        {
            // get any other article for arbitrary pairs
            if (collection == "none")
            {
                idB = parseInt(Math.random()*pages.length);
                pageA = pages[idA];
                pageB = pages[idB];
            }
            else
            {
                // if community size == 2 => pageB == other article
                if (numPages == 2)
                    idB = 1 - idA;
                else
                    idB = parseInt(Math.random()*commySize);
                pageA = pages[community][idA];
                pageB = pages[community][idB];
            }
        }
        // get path length
        let result = getCategoryPaths(pageA,pageB,categoryGraph,categoryTreeName,pathCollection,collection);
        // if a path exists => save
        if (result.count > 0)
        {
            count++;
            shortest = (shortest < result.shortestPath) ? shortest : result.shortestPath;
            sumShortest += result.shortestPath;
            sumAvg += result.avgPath;
            // add document with current timestamp and number of lookups vs. new path computations
            db._collection("pathLengthLookups").save({'timestamp':Date(time()).toString(),'queriesTotal':result.count,'added':result.added});
        }
    }
    if (status !== "canceled");
        status = "done";
    let result = {'shortest_path':shortest,'average_shortest_path':sumShortest/count,'avgavg_shortestPath':sumAvg/count};
    const fin = time();
    timestamp = Date(fin).toString();
    db._query("for doc in serviceLogs filter doc._id == @id update doc with {'shortestPaths':@result,'status':@status,'fin':@fin} in serviceLogs",{'id':logKey._id,'result':result,'status':status,'fin':timestamp});
    res.send({"result":result,"duration":fin-start});
})
// returns the average path lengths between pages worked on by the given set of communities
router.get('/getOverlappingPathLengths/:language/:amount/:iterations/', function(req,res) {
    // get language
    var lng, attr;
    if (typeof (lng = req.pathParams.language) == "undefined")
    {
        res.send("Need to specify language");
        return;
    }
    if (typeof req.pathParams.iterations == "undefined")
    {
        res.send("Need to specify number of iterations (10 or 100)");
        return;
    }
    attr = "overlappingSLPA" + req.pathParams.iterations;
    const start = time();
    const amount = parseInt(req.pathParams.amount);
    var timestamp = Date(start).toString();
    var status = "running";
    const collection = "overlappingSLPA" + req.pathParams.iterations;
    const gm = require("@arangodb/general-graph");
    var categoryGraphName = lng + "Categories";
    var categoryTreeName = lng + "CategoryTree";
    if (lng != "sh")
    {
        categoryGraphName = "Pages";
        categoryTreeName = "Categories";
    }
    var categoryGraph = gm._graph(categoryGraphName);
    var categoryTree = gm._graph(categoryTreeName);
    const pathCollection = "pathLengths";
    const logKey = db._collection("serviceLogs").save({"function":{"type":"getPathLengths","language":lng,"amount":amount},"start":timestamp,"pairsPassed":0,"status":status});
    var pages = [];
    pages = db._query("for doc in @@collection filter doc.namespace == '0' return doc",{'@collection':lng + "Pages"}).toArray();
    // prepare community map
    var commyMap = {};
    for (var i = 0; i < pages.length; i++)
    {
        // parse communities
        var commys = pages[i][attr];
        if (typeof commys == "object")
        {
            for (var j = 0; j < commys.length; j++)
            {
                var commyId = commys[j][0];
                if (typeof commyMap[commyId] == "undefined")
                    commyMap[commyId] = [];
                commyMap[commyId].push(pages[i]._id);
            }
        }
        else
        {
            if (typeof commyMap[commys] == "undefined")
                commyMap[commys] = [];
            commyMap[commys].push(pages[i]._id);
        }
    }
    var keys = Object.keys(commyMap);
    pages = null;
    // get communities with more than one page
    var bigCommys = [];
    for (var i = 0; i < keys.length; i++)
        if (commyMap[keys[i]].length > 1)
            bigCommys.push(keys[i]);
    var shortest = 9999, sumShortest = 0, sumAvg = 0, count = 0;
    for (let i = 0; i < amount; i++)
    {
        // update log entry
        db._query("for doc in serviceLogs filter doc._id == @id update doc with {'pairsPassed':@count} in serviceLogs",{'id':logKey._id,'count':count});
        // if status == canceled break loop
        if (db._query("for doc in serviceLogs filter doc._id == @id return doc.status",{'id':logKey._id}).next() === "canceled")
        {
            status = "canceled";
            break;
        }
        // get random community
        let commyId = parseInt(Math.random()*bigCommys.length);
        var community = commyMap[bigCommys[commyId]];
        var commySize = community.length;
        if (commySize < 2)
        {
            i--;
            continue;
        }
        // get random pair of articles
        let idA = parseInt(Math.random()*commySize);
        var pageA = community[idA];
        var idB = idA;
        while (idA == idB)
        {
            // get any other article for arbitrary pairs
            // if community size == 2 => pageB == other article
            if (commySize == 2)
                idB = 1 - idA;
            else
                idB = parseInt(Math.random()*commySize);
            pageB = community[idB];
        }
        // get path length
        let result = getCategoryPaths(pageA,pageB,categoryGraph,categoryTreeName,pathCollection,collection);
        // if a path exists => save
        if (result.count > 0)
        {
            count++;
            shortest = (shortest < result.shortestPath) ? shortest : result.shortestPath;
            sumShortest += result.shortestPath;
            sumAvg += result.avgPath;
        }
    }
    if (status !== "canceled");
        status = "done";
    let result = {'shortest_path':shortest,'average_shortest_path':sumShortest/count,'avgavg_shortestPath':sumAvg/count};
    const fin = time();
    timestamp = Date(fin).toString();
    db._query("for doc in serviceLogs filter doc._id == @id update doc with {'shortestPaths':@result,'status':@status,'fin':@fin} in serviceLogs",{'id':logKey._id,'result':result,'status':status,'fin':timestamp});
    res.send({"result":result,"duration":fin-start});
})
// returns the average path lengths between pages worked on by the given set of communities
router.get('/getPathLengthsWithOffset/:language/:collection/:offset', function(req,res) {
    // get community
    if (typeof req.pathParams.language == "undefined")
    {
        res.send("Need to specify language");
        return;
    }
    const lng = req.pathParams.language;
    const offset = parseInt(req.pathParams.offset);
    const start = time();
    var timestamp = Date(start).toString();
    var status = "running";
    const logKey = db._collection("serviceLogs").save({"function":{"type":"getPathLengths","language":lng,"collection":req.pathParams.collection},"currentCommunity":{},"start":timestamp,"communitiesPassed":0,"pathQueries":0,"status":status});
    const commys = db._query("for doc in @@collection filter doc.pageCount > 1 return doc",{'@collection':req.pathParams.collection});
    var it, result = [], lookUpTable = [], lookups = 0, communityCount = 0, queryCount = 0;
    while (typeof (it = commys.next()) !== "undefined")
    {
        // update log entry
        db._query("for doc in serviceLogs filter doc._id == @id update doc with {'currentCommunity':@commy,'communitiesPassed':@cc,'pathQueries':@qc} in serviceLogs",{'id':logKey._id,'commy':it,'cc':communityCount,'qc':queryCount});
        // if status == canceled break loop
        if (db._query("for doc in serviceLogs filter doc._id == @id return doc.status",{'id':logKey._id}).next() === "canceled")
        {
            status = "canceled";
            break;
        }
        // skip to start
        if (communityCount < offset)
        {
            communityCount++;
            continue;
        }
        // get pages for community
        const pages = it.pages;
        communityCount++;
        var shortestPath = {"_countTotal":9999}, paths = [];
        for (var i = 0; i < pages.length-1; i++)
            for (var j = i+1; j < pages.length; j++)
            {
                // get path
                var path = db._query("for doc in any shortest_path @pageA to @pageB @@collection return doc", {'pageA': pages[i], 'pageB': pages[j], '@collection':collectionName});
                queryCount++;
                if (typeof path !== "undefined" && path._countTotal > 0) // path length 1 means common category
                {
                    paths.push(path._countTotal);
                    // get shortest path
                    if (shortestPath._countTotal > path._countTotal)
                        shortestPath = path;
                }
            }
        // get average path length
        var avgPath = 0;
        for (var x = 0; x < paths.length; x++)
            avgPath += paths[x];
        avgPath /= paths.length;
        // store result
        const communityRes = {"community":it._id,"shortest_path":shortestPath,"average_path":avgPath};
        result.push(communityRes);
        db._query("for doc in @@collection filter doc._key == @community update doc with {'avgShortestPath':@result} in @@collection",{'@collection':req.pathParams.collection,'community':req.pathParams.community,'result':avgPath});
    }
    const fin = time();
    timestamp = Date(fin).toString();
    if (status != "canceled")
        status = "done";
    db._query("for doc in serviceLogs filter doc._id == @id update doc with {'status':@status,'fin':@time} in serviceLogs",{'id':logKey._id,'time':timestamp,'status':status});
    res.send({"result":result,"duration":fin-start});
})
// returns article pages
router.get('/getArticles/:language', function(req,res) {
    var lng;
    // get author community IDs
    if (typeof (lng = req.pathParams.language) == "undefined")
    {
        res.send("Need to specify language");
        return
    }
    // get pages
    var result = db._query("for doc in @@collection filter doc.namespace == '0' return doc",{'@collection':lng+'Pages'}).toArray();
    res.send(result);
})
// returns the pages edited by authors within the communities specified by the given tag
router.get('/getPages/:language/:community', function(req,res) {
    // get communites of authors
    if (typeof req.pathParams.language == "undefined")
    {
        res.send("Need to specify language");
        return
    }
    const lng = req.pathParams.language;
    var timestamp = Date(time()).toString();
    var status = "running";
    const logKey = db._collection("serviceLogs").save({"function":{"type":"getPages","language":lng,"collection":req.pathParams.community},"currentCommunity":{},"start":timestamp,"communitiesPassed":0,"status":status});
    var collectionName = lng + "Authors";
    var commys = db._query("for doc in @@collection collect commy = doc.@community into auths return {'community':commy,'authors':auths}",{'@collection':collectionName,'community':req.pathParams.community});
    // get pages within each community
    var it, result = [], commyCount = 0;
    while (typeof (it = commys.next()) !== "undefined")
    {
        // update log entry
        db._query("for doc in serviceLogs filter doc._id == @id update doc with {'currentCommunity':@commy,'communitiesPassed':@cc} in serviceLogs",{'id':logKey._id,'commy':it,'cc':commyCount});
        // if status == canceled break loop
        if (db._query("for doc in serviceLogs filter doc._id == @id return doc.status",{'id':logKey._id}).next() === "canceled")
        {
            status = "canceled";
            break;
        }
        // get pages for each author
        var size = 0, pages_c = {};
        for (size = 0; typeof it.authors[size] !== "undefined"; size++)
        {
            const author = it.authors[size];
            collectionName = lng + "History";
            var pages_a = db._query("for doc in @@collection filter doc._to == @auth return doc._from",{'@collection':collectionName,'auth':author.doc._id}).toArray();
            // add pages as keys to object
            pages_a.forEach(entry => pages_c[entry] = 0);
        }
        // write pages to array
        var pageArray = [];
        Object.keys(pages_c).forEach(key => pageArray.push(key));
        result.push({"_key":it.community,"size":size,"pages":pageArray,"pageCount":pageArray.length});
        commyCount++;
    }
    timestamp = Date(time()).toString();
    if (status != "canceled")
        status = "done";
    db._query("for doc in serviceLogs filter doc._id == @id update doc with {'status':@status,'communitiesPassed':@cc,'fin':@time} in serviceLogs",{'id':logKey._id,'cc':commyCount,'time':timestamp,'status':status});
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
// returns some statistics regarding the communities in the given collection
router.get('/getCommunityDetails/:collection', function(req,res) {
    // get collections
    if (typeof req.pathParams.collection == "undefined")
    {
        res.send("Need to specify name of collection");
        return;
    }
    const commy = db._query("for doc in @@collection return {'community':doc._key,'size':doc.size,'pageCount':doc.pageCount}",{'@collection':req.pathParams.collection});
    var it, result = {"communities":0,"single_page_communities":0,"single_user_communities":0,"avg_page_count":0,"avg_size":0,"max_pages_community":{"pageCount":0},"max_size_community":{"size":0},"empty_community_count":0,"empty_communities":[],"zero_page_community_count":0,"zero_page_communities":[]};
    while (typeof (it = commy.next()) !== "undefined")
    {
        result.avg_page_count += it.pageCount;
        result.avg_size += it.size;
        result.communities++;
        // count communities of size 1
        if (it.pageCount < 2)
        {
            if (it.pageCount < 1)
                result.zero_page_communities.push(it);
            else
                result.single_page_communities++;
        }
        if (it.size < 2)
        {
            if (it.size < 1)
                result.empty_communities.push(it);
            else
                result.single_user_communities++;
        }
        // check for max communities
        if (result.max_pages_community.pageCount < it.pageCount)
            result.max_pages_community = it;
        if (result.max_size_community.size < it.size)
            result.max_size_community = it;
    }
    result.avg_page_count /= result.communities;
    result.avg_size /= result.communities;
    result.empty_community_count = result.empty_communities.length;
    result.zero_page_community_count = result.zero_page_communities.length;
    res.send(result);
})
router.get('/getOverlappingDetails/:language/:iterations', function(req,res) {
    var lng, attr;
    if (typeof (lng = req.pathParams.language) == "undefined")
    {
        res.send("Need to specify language");
        return;
    }
    if (typeof req.pathParams.iterations == "undefined")
    {
        res.send("Need to specify number of iterations (10 or 100)");
        return;
    }
    attr = "overlappingSLPA" + req.pathParams.iterations;
    // make Commnity Map
    var pages = db._query("for doc in @@collection filter doc.namespace == '0' return doc",{'@collection':lng + "Pages"}).toArray();
    // prepare community map
    var commyMap_articles = {};
    for (var i = 0; i < pages.length; i++)
    {
        // parse communities
        var commys = pages[i][attr];
        if (typeof commys == "object")
        {
            for (var j = 0; j < commys.length; j++)
            {
                var commyId = commys[j][0];
                if (typeof commyMap_articles[commyId] == "undefined")
                    commyMap_articles[commyId] = [];
                commyMap_articles[commyId].push(pages[i]._id);
            }
        }
        else
        {
            if (typeof commyMap_articles[commys] == "undefined")
                commyMap_articles[commys] = [];
            commyMap_articles[commys].push(pages[i]);
        }
    }
    var articleKeys = Object.keys(commyMap_articles);
    var commyMap_authors = {};
    var authors = db._query("for doc in @@collection return doc",{'@collection':lng + "Authors"}).toArray();
    for (var i = 0; i < authors.length; i++)
    {
        // parse communities
        var commys = authors[i][attr];
        if (typeof commys == "object")
        {
            for (var j = 0; j < commys.length; j++)
            {
                var commyId = commys[j][0];
                if (typeof commyMap_authors[commyId] == "undefined")
                    commyMap_authors[commyId] = [];
                commyMap_authors[commyId].push(authors[i]._id);
            }
        }
        else
        {
            if (typeof commyMap_authors[commys] == "undefined")
                commyMap_authors[commys] = [];
            commyMap_authors[commys].push(authors[i]._id);
        }
    }
    var authorKeys = Object.keys(commyMap_authors);
    var details = {"article_communities":articleKeys.length};
    var count = 0, sum = 0, max = [], amount = articleKeys.length, sizeMap = {}, authorCount = 0;
    for (var i = 0; i < amount; i++)
    {
        let key = articleKeys[i];
        let size = commyMap_articles[key].length;
        sum += size;
        if (size < 2)
            count++;
        if (typeof sizeMap[size] == "undefined")
            sizeMap[size] = [];
        sizeMap[size].push(articleKeys[i]);
        let authors = commyMap_authors[key];
        if (typeof authors != "undefined")
            authorCount += authors.length;
    }
    details["single_article_communities"] = count;
    details["average_article_communities"] = sum/amount;
    details["average_author_count"] = authorCount/amount;
    var sizeKeys = Object.keys(sizeMap);
    for (var i = 0; i < 10; i++)
    {
        var commySize = sizeKeys[sizeKeys.length-(i+1)];
        var commyIds = sizeMap[commySize];
        var authors = [];
        for (var j = 0; j < commyIds.length; j++)
            if (typeof commyMap_authors[commyIds[j]] != "undefined")
                authors.push(commyMap_authors[commyIds[j]].length);
        max[i] = {"id":commyIds,"articles":commySize,"authors":authors};
    }
    details["max_article_communities"] = max;
    amount = authorKeys.length;
    count = 0;
    max = [];
    sizeMap = {};
    sum = 0;
    var articleCount = 0;
    for (var i = 0; i < amount; i++)
    {
        let key = authorKeys[i];
        var size = commyMap_authors[key].length;
        sum += size;
        if (size < 2)
            count++;
        if (typeof sizeMap[size] == "undefined")
            sizeMap[size] = [];
        sizeMap[size].push(authorKeys[i]);
        let articles = commyMap_articles[key];
        if (typeof articles != "undefined")
            articleCount += articles.length;
    }
    details["author_communities"] = authorKeys.length;
    details["single_authors_communities"] = count;
    details["average_authors_communities"] = sum/amount;
    details["average_article_count"] = articleCount/amount;
    sizeKeys = Object.keys(sizeMap);
    for (var i = 0; i < 10; i++)
    {
        var commySize = sizeKeys[sizeKeys.length-(i+1)];
        var commyIds = sizeMap[commySize];
        var articles = [];
        for (var j = 0; j < commyIds.length; j++)
            if (typeof commyMap_articles[commyIds[j]] != "undefined")
                articles.push(commyMap_articles[commyIds[j]].length);
        max[i] = {"id":commyIds,"authors":commySize,"articles":articles};
    }
    details["max_author_communities"] = max;
    res.send(details);
})
router.get('/getCentralCategories/:language/:communityId/:communityKey', function(req,res) {
    var lng, commyId, attr;
    if (typeof (lng = req.pathParams.language) == "undefined")
    {
        res.send("Specify language");
        return;
    }
    if (typeof (commyId = req.pathParams.communityId) == "undefined")
    {
        res.send("Specify community");
        return;
    }
    if (typeof (attr = req.pathParams.communityKey) == "undefined")
    {
        res.send("Specify community key");
        return;
    }
    var graphName = "Pages";
    if (lng == "sh")
        graphName = "shCategories";
    const graph = require("@arangodb/general-graph")._graph(graphName);
    // get community
    var community = [];
    let pages = db._query("for doc in @@collection filter doc.namespace == '0' return doc",{'@collection':lng + "Pages"}).toArray();
    for (var i = 0; i < pages.length; i++)
    {
        let commys = pages[i][attr];
        if (typeof commys == "object")
        {
            for (var j = 0; j < commys.length; j++)
                if (commys[j][0] == commyId)
                    community.push(pages[i]._id);
        }
        else
            if (commys == commyId)
                community.push(pages[i]._id);
        commys = null;
    }
    pages = null;
    categories = {};
    for (var i = 0; i < community.length; i++)
    {
        let cats = graph._neighbors(community[i]);
        for (var j = 0; j < cats.length; j++)
        {
            if (cats[j] != null)
            {
                if (typeof categories[cats[j]] == "undefined")
                    categories[cats[j]] = 1;
                else
                    categories[cats[j]] += 1;
            }
        }
        cats = null;
    }
    catKeys = Object.keys(categories);
    first = catKeys[0];
    second = catKeys[1];
    third = catKeys[2];
    for (var i = 3; i < catKeys.length; i++)
    {
        if (typeof catKeys[i] != "string")
            continue;
        if (categories[catKeys[i]] > categories[first])
        {
            third = second;
            second = first;
            first = catKeys[i];
        }
        else if (categories[catKeys[i]] > categories[second])
        {
            third = second;
            second = catKeys[i];
        }
        else if (categories[catKeys[i]] > categories[third])
            third = catKeys[i];
    }
    maxCats = {};
    maxCats[first] = categories[first];
    maxCats[second] = categories[second];
    maxCats[third] = categories[third];
    res.send(maxCats);
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
function getCategoryPaths(pageA,pageB,categoryGraph,categoryTree,collectionName,community)
{
    let catsA = categoryGraph._neighbors(pageA);
    let catsB = categoryGraph._neighbors(pageB);
    var shortestPath = 9999, sumPath = 0, count = 0, numAdded = 0;
    for (var i = 0; i < catsA.length; i++)
    {
        for (var j = 0; j < catsB.length; j++)
        {
            var queryRes = db._query("for doc in @@collection filter doc._from == @a && doc._to == @b return doc",{'@collection':collectionName,'a':catsA[i],'b':catsB[j]}).next();
            if (typeof queryRes == "undefined")
                queryRes = db._query("for doc in @@collection filter doc._to == @a && doc._from == @b return doc",{'@collection':collectionName,'a':catsA[i],'b':catsB[j]}).next();
            // update community
            var path = 0;
            if (typeof queryRes !== "undefined")
            {
                if (queryRes.community !== community)
                {
                    db._query("for doc in @@collection filter doc._id == @id update doc with {'community':@community} in @@collection",{'@collection':collectionName,'id':queryRes._id,'community':community});
                    numAdded++;
                }
                path = queryRes.distance;
            }
            // path doesn't exist in collection => compute and save
            else
            {
                // check for faulty result
                var queryRes;
                try { queryRes = db._query("for doc in any shortest_path @a to @b graph @graph return doc",{'a':catsA[i],'b':catsB[j],'graph':categoryTree}); }
                catch(err) { path = 0; }
                if (typeof queryRes == "undefined")
                    continue;
                var it, countTotal = queryRes._countTotal;
                while (typeof (it = queryRes.next()) !== "undefined")
                    if (it == null)
                        countTotal = 0;
                path = countTotal;
                // add to lookup table
                if (typeof path == "number" && path !== 0)
                {
                    let entry = {'_from':catsA[i],'_to':catsB[j],'distance':path,'community':community};
                    db._collection(collectionName).save(entry);
                    numAdded++;
                }
            }
            if (typeof path == "number" && path !== 0)
            {
                sumPath += path;
                shortestPath = (shortestPath < path) ? shortestPath : path;
                count++;
            }
        }
    }
    return {'shortestPath':shortestPath,'avgPath':sumPath/count,'count':count,'added':numAdded};
}
