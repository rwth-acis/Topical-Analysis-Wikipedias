//'use strict';

// arango objects
const router = require('@arangodb/foxx/router')();
const request = require('@arangodb/request');
const db = require('@arangodb').db;

// collections
const scoCatLinks = db._collection('scoCategoryLinks');
const scoArts = db._collection('scoArticles');
const scoLinks = db._collection('scoArticleLinks');

const enCats = db._collection('enCategories');
const enCatLinks = db._collection('enCategoryLinks');
const enArts = db._collection('enArticles');

module.context.use(router);

router.post('/addCategory', function(req, res) {

    if (typeof req.body == "undefined" || typeof req.body.title == "undefined" || typeof req.body.categories == "undefined" || typeof req.body.id == "undefined" || typeof req.body.catIds == "undefined")
    {
        res.send("Invalid format of post");
        return -1;
    }

    enCats.save({"title":req.body.title, "categories":req.body.categories});

    for (var i = 0; i < req.body.categories.length; i++)
        enCatLinks.save({"_from":req.body.id,"_to":req.body.catIds[i]});
})

router.get('/averagePathLength', function(req, res) {


    var shortestSum = 0;
    var averageSum = 0;
    var mainSum = 0;
    var counter = 0;

    var cursor = db._query("FOR entry IN scoArticleLinks RETURN entry");
    var entry;
    var shortest = 0;
    var average = 0;
    var main = 0;
    var shortMax = 0;
    var averageMax = 0;
    var mainMax = 0;
    var shortMin = 9999;
    var averageMin = 9999;
    var mainMin = 9999;
    while ((typeof (entry = cursor.next())) !== "undefined")
    {
        if (entry.Shortest == 9999 || entry.Average == 9999 || entry.Main == 9999)
            continue;
        shortest += entry.Shortest;
        average += entry.Average;
        main += entry.Main;
        shortMax = entry.Shortest > shortMax ? entry.Shortest : shortMax;
        averageMax = entry.Average > averageMax ? entry.Average : averageMax;
        mainMax = entry.Main > mainMax ? entry.Main : mainMax;
        shortMin = entry.Shortest < shortMin ? entry.Shortest : shortMin;
        averageMin = entry.Average < averageMin ? entry.Average : averageMin;
        mainMin = entry.Main < mainMin ? entry.Main : mainMin;
        counter++;
    }

    var a = {shortest, shortMax, shortMin};
    var b = {average, averageMax, averageMin};
    var c = {main, mainMax, mainMin};
    var response = {"Shortest Path Sum": a, "Average Path Sum": b, "Main Topic Sum": c, "Counter": counter};
    res.send(response);

})

router.get('/getPathLengths/:from/:to', function(req, res) {
    // get articles from db
    var startNodeID = parseInt(req.pathParams.from);
    var endNodeID = parseInt(req.pathParams.to);

    var startNode = db._query("FOR node IN scoArticles FILTER node.`id` == @id RETURN node", {'id': startNodeID}).next();
    var endNode = db._query("FOR node IN scoArticles FILTER node.`id` == @id RETURN node", {'id': endNodeID}).next();

    if (typeof startNode == "undefined")
        res.send("No article with ID " + startNodeID + " found.");
    if (typeof endNode == "undefined")
        res.send("No article with ID " + endNodeID + " found.");

    // get array with category IDs
    var catsA = [];
    var catsB = [];
    for (var i = 0; i < startNode.categories.length; i++)
    {
        var cat = db._query("FOR cat IN scoCategories FILTER cat.`title` == @title RETURN cat._id", {'title': startNode.categories[i]}).next();
        if (typeof cat !== "undefined")
            catsA.push(cat);
    }
    for (var i = 0; i < endNode.categories.length; i++)
    {
        var cat = db._query("FOR cat IN scoCategories FILTER cat.`title` == @title RETURN cat._id", {'title': endNode.categories[i]}).next();
        if (typeof cat !== "undefined")
            catsB.push(cat);
    }

    // get array with shortest paths
    var paths = [];
    var shortestPath = {"_countTotal": 9999};
    for (var i = 0; i < catsA.length; i++) {
        for (var j = 0; j < catsB.length; j++) {
            path = db._query("FOR v IN ANY SHORTEST_PATH @catA TO @catB scoCategoryLinks RETURN v", {'catA': catsA[i], 'catB': catsB[j]});
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
    for (var i = 0; i < paths.length; i++)
        avgPath += paths[i];
    avgPath /= paths.length;

    // get array with paths to main topic classfications
    var catsC = ["Airt", "Airts", "Concepts", "Cultur", "Eddication", "Employment", "Environs", "Fowk", "Geografie", "Heal", "History", "Humanities", "Law", "Life", "Mathematics", "Naitur", "Politics", "Science and technology", "Society", "Sports", "Technology", "Tuils", "Warld"];
    for (var i = 0; i < catsC.length; i++)
    {
        var cat = db._query("FOR cat IN scoCategories FILTER cat.`title` == @title RETURN cat._id", {'title': catsC[i]}).next();
        if (typeof cat !== "undefined")
            catsC[i] = cat;
    }
    var mainPathsA = [];
    var mainPathsB = [];
    for (var i = 0; i < catsC.length; i++) {
        var mainPath = [];
        for (var j = 0; j < catsA.length; j++) {
            path = db._query("FOR v IN ANY SHORTEST_PATH @catA TO @catC scoCategoryLinks RETURN v", {'catA': catsA[j], 'catC': catsC[i]});
            if (typeof path !== "undefined" && path._countTotal > 0)
                mainPath.push(path._countTotal);
        }
        mainPathEntry = {"Category": catsC[i], "pathLengths": mainPath};
        mainPathsA.push(mainPathEntry);
    }
    for (var i = 0; i < catsC.length; i++) {
        var mainPath = [];
        for (var j = 0; j < catsB.length; j++) {
            path = db._query("FOR v IN ANY SHORTEST_PATH @catB TO @catC scoCategoryLinks RETURN v", {'catB': catsB[j], 'catC': catsC[i]});
            if (typeof path !== "undefined" && path._countTotal > 0)
                mainPath.push(path._countTotal);
        }
        mainPathEntry = {"category": catsC[i], "pathLengths": mainPath};
        mainPathsB.push(mainPathEntry);
    }

    // get shortest common main category
    var shortestMainPath = 9999;
    var mainPathLengths = [];
    var mainTopic = "";
    for (var i = 0; i < catsC.length; i++)
    {
        for (var j = 0; j < mainPathsA[i].pathLengths.length; j++)
        {
            for (var k = 0; k < mainPathsB[i].pathLengths.length; k++)
            {
                var curPathLength = mainPathsA[i].pathLengths[j] + mainPathsB[i].pathLengths[k];
                if (shortestMainPath > curPathLength)
                {
                    shortestMainPath = curPathLength;
                    mainTopic = catsC[i];
                    mainPathLengths = [mainPathsA[i].pathLengths[j], mainPathsB[i].pathLengths[k]]
                }
            }
        }
    }

    var response = {"Shortest Path": shortestPath, "Average Path Length": avgPath, "Common Main Topic": mainTopic, "Path Length from Article A": mainPathLengths[0], "Path Length from Article B": mainPathLengths[1]};
    res.send(response);
})

router.get('/linkCategories', function(req, res) {
    // probably very slow <-- after testing the answer is:... sort of. Could be worse. Probably not feasable for 30 times the amount of categories
    var date = new Date();
    var startime = date.getTime();
    var cursor = db._query("FOR entry IN scoCategories RETURN entry");
    while ((typeof (entry = cursor.next())) !== "undefined")
    {
        for (var i = 0; i < entry.categories.length; i++)
        {
        var id = db._query("FOR item IN scoCategories FILTER item.`title` == @title RETURN item._id", {'title': entry.categories[i]}).next();
        if (typeof id !== "undefined")
            scoCatLinks.save({"_from": id, "_to": entry._id});
        }
    }

    var runtime = date.getTime() - startime;
    res.send("Finished job after: " + runtime + " milliseconds!");
})
