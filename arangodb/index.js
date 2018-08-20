//'use strict';

// arango objects
const router = require('@arangodb/foxx/router')();
const request = require('@arangodb/request');
const db = require('@arangodb').db;

module.context.use(router);

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
    var edges = db._query("for e in @@collection collect from = e._from with count into numb return {'vertex':from, 'degree':numb}", {'@collection': collection.name()});
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
    edges = db._query("for e in @@collection collect to = e._to with count into numb return {'vertex':to, 'degree':numb}", {'@collection': collection.name()});
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
router.get('/getCommunities/:collection/:resultField', function(req,res) {
    // get collection from param
    const community = req.pathParams.resultField;
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
    const docs = db._query("for d in @@collection collect commy = d.@community with count into size return {'community':commy, 'size':size}", {'@collection': collection.name(),'community':community});
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
