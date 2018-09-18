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
// returns modularity for the given set of communities
router.get('/getModularity/:language/:collection', function(req,res){
    var community, lng;
    if (typeof (community = req.pathParams.collection) === "undefined")
    {
        res.send("Specify community");
        return;
    }
    if (typeof (lng = req.pathParams.language) === "undefined")
    {
        res.send("Specify language");
        return;
    }
    var timestamp = Date(time()).toString();
    var status = "running";
    const logKey = db._collection("serviceLogs").save({"function":{"type":"modularity","language":lng,"community":community},"start":timestamp,"status":status,"communitiesPassed":0});
    var collectionName = lng;
    collectionName += (community[4] == 'H') ? "Pages" : "Authors";
    const commys = db._query("for doc in @@collection collect commy = doc.@commy into docs return {'community':commy,'docs':docs}",{'@collection':collectionName,'commy':community});
    collectionName = (community[4] == 'H') ? lng + "History" : lng + "AuthorLinks";
    const degree = (community[4] == 'H') ? "historyDegree" : "authorsDegree";
    const m = db._collection(collectionName).count();
    var it, commyCount = 0, q = 0;
    while (typeof (it = commys.next()) !== "undefined")
    {
        // update log entry
        db._query("for doc in serviceLogs filter doc._id == @id update doc with {'communitiesPassed':@commyCount} in serviceLogs",{'id':logKey._id,'commyCount':commyCount});
        // if status == canceled break loop
        if (db._query("for doc in serviceLogs filter doc._id == @id return doc.status",{'id':logKey._id}).next() === "canceled")
        {
            status = "canceled";
            break;
        }
        for (var i = 0; i < it.docs.length; i++)
        {
            const A = it.docs[i].doc;
            for (var j = 0; j < it.docs.length; j++)
            {
                if (i == j)
                    continue;
                const B = it.docs[j].doc;
                const a = (db._query("for doc in @@collection filter doc._from == @from && doc._to == @to return doc",{'@collection':collectionName,'from':A._id,'to':B._id}).toArray().length > 0) ? 1 : 0;
                q += a - (A[degree] * B[degree] / (2 * m));
            }
        }
        commyCount++;
    }
    if (status !== "canceled")
        status = "done";
    timestamp = Date(time()).toString();
    q /= 2 * m;
    db._query("for doc in serviceLogs filter doc._id == @id update doc with {'fin':@time,'status':@status,'communitiesPassed':@commyCount,'modularity':@q} in serviceLogs",{'id':logKey._id,'time':timestamp,'status':status,'commyCount':commyCount,'q':q});
    res.send(q);
})
// returns the average path lengths of pages within communities and the main topic classifications
router.get('/getMainCategories/:language/:collection', function(req,res) {
    var lng;
    if (typeof (lng = req.pathParams.language) == "undefined")
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
    var timestamp = Date(start).toString();
    var status = "running";
    var collectionName = lng + "CategoryLinks";
    // create document in service log collection
    const logKey = db._collection("serviceLogs").save({"function":{"type":"getMainCategories","language":lng,"collection":req.pathParams.collection},"start":timestamp,"currentCommunity":{},"communitiesPassed":0,"pathQueries":0,"status":status});
    const commys = db._query("for doc in @@collection filter doc.pageCount > 1 return doc",{'@collection':req.pathParams.collection});
    var it, result = [], communityCount = 0, queryCount = 0;
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
        // get pages for community
        const pages = it.pages;
        var closestCount = {}, pathLengths = [];
        communityCount++;
        for (var i = 0; i < pages.length; i++)
        {
            var closestCat = -1;
            var shortestPath = 9999;
            for (var j = 0; j < mainCategories.length; j++)
            {
                collectionName = lng + "CategoryLinks";
                // check for shortest path in lookup table
                var path = db._query("with viPages for doc in any shortest_path @pageA to @pageB @@collection return doc", {'pageA': pages[i], 'pageB': mainCategories[j], '@collection':collectionName});
                queryCount++;
                if (typeof path !== "undefined" && path._countTotal > 0) // path length 1 means common category
                {
                    // get shortest path
                    if (shortestPath > path._countTotal)
                    {
                        closestCat = j;
                        shortestPath = path._countTotal;
                    }
                    // add path length to array
                    if (typeof pathLengths[j] == "undefined" || pathLengths[j] == NaN)
                        pathLengths[j] = 1;
                    else
                        pathLengths[j] += path._countTotal;
                }
            }
            // get id of closestCat
            const closestCatId = mainCategories[closestCat];
            if (typeof closestCount[closestCatId] == "undefined" || closestCount[closestCatId] == NaN)
                closestCount[closestCatId] = 1;
            else
                closestCount[closestCatId]++;
        }
        // get overall closest main category
        var closestCat = 0;
        for (var i = 1; i < closestCount.length; i++)
            closestCat = closestCount[closestCat] < closestCount[i] ? i : closestCat;
        collectionName = lng + "Pages";
        const resCat = db._query("for doc in @@collection filter doc._id == @id return doc",{'@collection':collectionName,'id':mainCategories[closestCat]}).next();
        // store result
        const communityRes = {"community":it._id,"closest_category":resCat,"path_lengths":pathLengths,"closest_count":closestCount};
        result.push(communityRes);
        db._collection("pathLengths").save({"type":"pathLengthsMain","result":communityRes});
    }
    const fin = time();
    timestamp = Date(fin).toString()
    if (status != "canceled")
        status = "done";
    db._query("for doc in serviceLogs filter doc._id == @id update doc with {'status':@status,'fin':@time} in serviceLogs",{'id':logKey._id,'time':timestamp,'status':status});
    res.send({"result":result,"duration":fin-start});
})
// returns the average path lengths between pages worked on by the given community
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
    const categoryGraphName = lng + "Categories";
    const categoryTreeName = lng + "CategoryTree";
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
            for (var j = i+1; j < pages.length; j++)
            {
                if (db._query("for doc in serviceLogs filter doc._id == @id return doc.status",{'id':logKey._id}).next() === "canceled")
                {
                    status = "canceled";
                    break;
                }
                // get path
                const result = getCategoryPaths(pages[i],pages[j],categoryGraph,categoryTreeName,pathCollection,collection);
                if (typeof result.empty !== "undefined")
                    continue;
                queryCount++;
                shortest = (result.shortestPath < shortest) ? result.shortestPath : shortest;
                if (result.shortestPath < 9999 && result.avgPath > 0)
                {
                    sumShortest += result.shortestPath;
                    sumAvg += result.avgPath;
                    count++;
                }
            }
        // get average path length and store result
        const communityRes = {"shortest_path":shortest,"average_shortest_path":sumShortest/count,"avgavg_shortest_path":sumAvg/count};
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
    // get community
    if (typeof req.pathParams.language == "undefined")
    {
        res.send("Need to specify language");
        return;
    }
    const lng = req.pathParams.language;
    const start = time();
    const amount = parseInt(req.pathParams.amount);
    var timestamp = Date(start).toString();
    var status = "running";
    var collectionName = lng + "CategoryLinks";
    const categoryGraphName = lng + "Categories";
    const categoryTreeName = lng + "CategoryTree";
    const categoryGraph = require("@arangodb/general-graph")._graph(categoryGraphName);
    const categoryTree = require("@arangodb/general-graph")._graph(categoryTreeName);
    const pathCollection = "pathLengths";
    const logKey = db._collection("serviceLogs").save({"function":{"type":"getPathLengths","language":lng,"collection":req.pathParams.collection},"currentCommunity":{},"start":timestamp,"communitiesPassed":0,"pathQueries":0,"status":status});
    const commys = db._query("for doc in @@collection filter doc.pageCount > 1 return doc",{'@collection':req.pathParams.collection}).toArray();
    // get 'amount' numbers of random ids
    var incCommys = [];
    for (var i = 0; i < amount; i++)
        incCommys.push(parseInt(Math.random()*commys.length));
    var commyCount = 0, queryCount = 0;
    for (var k = 0; k < incCommys.length; k++)
    {
        const it = commys[incCommys[k]];
        // update log entry
        db._query("for doc in serviceLogs filter doc._id == @id update doc with {'currentCommunity':@commy,'communitiesPassed':@cc,'pathQueries':@qc} in serviceLogs",{'id':logKey._id,'commy':it,'cc':commyCount,'qc':queryCount});
        // if status == canceled break loop
        if (status == "canceled")
            break;
        // get pages for community
        const pages = it.pages;
        var shortest = 9999, sumShortest = 0, sumAvg = 0, count = 0;
        for (var i = 0; i < pages.length-1; i++)
            for (var j = i+1; j < pages.length; j++)
            {
                if (db._query("for doc in serviceLogs filter doc._id == @id return doc.status",{'id':logKey._id}).next() === "canceled")
                {
                    status = "canceled";
                    break;
                }
                // get path
                empty = false;
                const result = getCategoryPaths(pages[i],pages[j],categoryGraph,categoryTreeName,pathCollection,req.pathParams.collection);
                if (typeof result.empty !== "undefined")
                    continue;
                queryCount++;
                shortest = (result.shortestPath < shortest) ? result.shortestPath : shortest;
                if (result.shortestPath < 9999 && result.avgPath > 0)
                {
                    sumShortest += result.shortestPath;
                    sumAvg += result.avgPath;
                    count++;
                }
            }
        // get average path length and store result
        commyCount++;
        const communityRes = {"shortest_path":shortest,"average_shortest_path":sumShortest/count,"avgavg_shortest_path":sumAvg/count};
        db._query("for doc in @@collection filter doc._id == @id update doc with {'shortestPaths':@result} in @@collection",{'@collection':req.pathParams.collection,'id':it._id,'result':communityRes});
    }
    const fin = time();
    timestamp = Date(fin).toString();
    if (status != "canceled")
        status = "done";
    db._query("for doc in serviceLogs filter doc._id == @id update doc with {'status':@status,'fin':@time,'communitiesPassed':@cc,'pathQueries':@qc} in serviceLogs",{'id':logKey._id,'time':timestamp,'status':status,'cc':commyCount,'qc':queryCount});
    res.send({"duration":fin-start});
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
    var catsA = categoryGraph._neighbors(pageA);
    var catsB = categoryGraph._neighbors(pageB);
    var shortestPath = 9999, sumPath = 0, count = 0;
    for (var i = 0; i < catsA.length; i++)
    {
        for (var j = 0; j < catsB.length; j++)
        {
            var path = db._query("for doc in @@collection filter doc._from == @a && doc._to == @b return doc.distance",{'@collection':collectionName,'a':catsA[i],'b':catsB[j]}).next();
            if (typeof path == "undefined")
                path = db._query("for doc in @@collection filter doc._to == @a && doc._from == @b return doc.distance",{'@collection':collectionName,'a':catsA[i],'b':catsB[j]}).next();
            // path doesn't exist in collection => compute and save
            if (typeof path == "undefined")
            {
                path = db._query("for doc in any shortest_path @a to @b graph @graph return doc",{'a':catsA[i],'b':catsB[j],'graph':categoryTree})._countTotal;
                if (path !== 0)
                    db._collection(collectionName).save({'_from':catsA[i],'_to':catsB[j],'distance':path,'community':community});
            }
            if (path !== 0)
            {
                sumPath += path;
                shortestPath = (shortestPath < path) ? shortestPath : path;
                count++;
            }
        }
    }
    if (count < 1)
        return {'empty':true};
    return {'shortestPath':shortestPath,'avgPath':sumPath/count};
}
