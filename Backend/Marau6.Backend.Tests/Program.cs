using System.Net;
using System.Net.Http.Json;
using System.Text.Json.Nodes;

// Integration contract against a real API/PostgreSQL instance. Profiles are fixed;
// tests use an isolated database configured by the test launcher, never the play database.
var client = new HttpClient { BaseAddress = new Uri("http://127.0.0.1:5187") };
client.DefaultRequestHeaders.Add("X-Marau6-Key", Environment.GetEnvironmentVariable("MARAU6_API_KEY") ?? "missing");
int passed = 0;
void Check(bool value, string name) { if (!value) throw new Exception(name); passed++; Console.WriteLine("PASS " + name); }
async Task<JsonObject> Get(int p) => (await client.GetFromJsonAsync<JsonObject>($"/state/{p}"))!;
async Task<JsonObject> Post(string path, object body, HttpStatusCode expected = HttpStatusCode.OK)
{
    var r = await client.PostAsJsonAsync(path, body);
    if (r.StatusCode != expected) throw new Exception($"{path}: {r.StatusCode} {await r.Content.ReadAsStringAsync()}");
    return (await r.Content.ReadFromJsonAsync<JsonObject>())!;
}
string Id(JsonNode n) => n["id"]!.GetValue<string>();
int Rev(JsonObject n) => n["revision"]!.GetValue<int>();
JsonArray Items(JsonObject n) => n["items"]!.AsArray();
var a = await Get(1); var b = await Get(2);
Check(Items(a).Count == 5, "initial grant once");
Check(Items(await Get(1)).Count == 5, "read does not grant again");
var rifle = Items(a).First(x => x!["slot"]!.GetValue<int>() == 10)!;
var op = Guid.NewGuid();
var move = new { operationId = op, profile = 1, revision = Rev(a), item = Id(rifle), destination = 105 };
a = await Post("/move", move);
Check(Items(a).Any(x => Id(x!) == Id(rifle) && x!["slot"]!.GetValue<int>() == 105), "move preserves identity");
var duplicate = await Post("/move", move);
Check(Rev(duplicate) == Rev(a), "replay returns original receipt");
await Post("/move", new { operationId = op, profile = 1, revision = Rev(a), item = Id(rifle), destination = 101 }, HttpStatusCode.Conflict);
await Post("/move", new { operationId = Guid.NewGuid(), profile = 1, revision = 0, item = Id(rifle), destination = 10 }, HttpStatusCode.Conflict);
await Post("/move", new { operationId = Guid.NewGuid(), profile = 1, revision = Rev(a), item = Id(rifle), destination = 8 }, HttpStatusCode.Conflict);
Check(true, "changed replay, stale revision and rifle safe rejected");
a = await Post("/move", new { operationId = Guid.NewGuid(), profile = 1, revision = Rev(a), item = Id(rifle), destination = 10 });
var raid = Guid.NewGuid();
await Post("/raids/open", new { operationId = Guid.NewGuid(), raidId = raid });
await Post("/raids/enter", new { operationId = Guid.NewGuid(), raidId = raid, profile = 1, revision = Rev(a) });
a = await Get(1);
Check(Items(a).All(x => x!["slot"]!.GetValue<int>() >= 100), "entry reserves equipment out of base");
await Post("/raids/enter", new { operationId = Guid.NewGuid(), raidId = raid, profile = 1, revision = Rev(a) }, HttpStatusCode.Conflict);
var enteredB = await Post("/raids/enter", new { operationId = Guid.NewGuid(), raidId = raid, profile = 2, revision = Rev(b) });
var enteredA = (await client.GetFromJsonAsync<JsonObject>($"/raids/{raid}"))!;
var worldItems = enteredA["items"]!.AsArray();
var ownerA = a["id"]!.GetValue<string>();
var ownerB = b["id"]!.GetValue<string>();
// A dies; B has picked A's rifle and extracts first. Full ordered server snapshot.
foreach (var n in worldItems)
{
    var item = n!;
    if (Id(item) == Id(rifle)) { item["owner"] = ownerB; item["slot"] = 0; item["magazine"] = 7; item["custody"] = "result"; }
    else if (item["owner"]!.GetValue<string>() == ownerA)
    { item["owner"] = ""; item["slot"] = -1; item["custody"] = "world"; }
    else item["custody"] = "result";
}
var snapshotOp = Guid.NewGuid();
var snap = new { operationId = snapshotOp, raidId = raid, sequence = 1, items = worldItems, results = new[] { new { profile = 2, outcome = "extracted" }, new { profile = 1, outcome = "eliminated" } } };
await Post("/raids/snapshot", snap);
await Post("/raids/snapshot", snap);
b = await Get(2);
Check(Items(b).Count(x => Id(x!) == Id(rifle)) == 1, "new owner extracts exactly one transferred rifle");
Check(Items(b).First(x => Id(x!) == Id(rifle))!["magazine"]!.GetValue<int>() == 7, "ammo preserved");
Check(Items(await Get(1)).All(x => Id(x!) != Id(rifle)), "death cannot restore old owner's transferred rifle");
await Post("/raids/snapshot", new { operationId = Guid.NewGuid(), raidId = raid, sequence = 1, items = worldItems, results = Array.Empty<object>() }, HttpStatusCode.Conflict);
await Post("/raids/close", new { operationId = Guid.NewGuid(), raidId = raid });
// Abandonment conserves the latest safe, not an entry copy.
a = await Get(1);
var supply = Items(a).First(x => x!["kind"]!.GetValue<int>() == 1)!;
a = await Post("/move", new { operationId = Guid.NewGuid(), profile = 1, revision = Rev(a), item = Id(supply), destination = 0 });
var raid2 = Guid.NewGuid();
await Post("/raids/open", new { operationId = Guid.NewGuid(), raidId = raid2 });
await Post("/raids/enter", new { operationId = Guid.NewGuid(), raidId = raid2, profile = 1, revision = Rev(a) });
var state2 = (await client.GetFromJsonAsync<JsonObject>($"/raids/{raid2}"))!;
var safeItems = state2["items"]!.AsArray();
safeItems.Single()!["slot"] = 8;
await Post("/raids/snapshot", new { operationId = Guid.NewGuid(), raidId = raid2, sequence = 1, items = safeItems, results = Array.Empty<object>() });
await Post("/raids/close", new { operationId = Guid.NewGuid(), raidId = raid2 });
a = await Get(1);
Check(Items(a).Any(x => Id(x!) == Id(supply) && x!["slot"]!.GetValue<int>() == 8), "abandonment preserves last durable safe");
Console.WriteLine($"ALL {passed} CONTRACT ASSERTIONS PASSED");
// Result custody must become usable equipment on the next raid.
b = await Get(2);
var raid3 = Guid.NewGuid();
var openOp = Guid.NewGuid();
await Post("/raids/open", new { operationId = openOp, raidId = raid3 });
await Post("/raids/close", new { operationId = openOp, raidId = raid3 }, HttpStatusCode.Conflict);
await Post("/raids/enter", new { operationId = Guid.NewGuid(), raidId = raid3, profile = 2, revision = Rev(b) });
var third = (await client.GetFromJsonAsync<JsonObject>($"/raids/{raid3}"))!;
var thirdItems = third["items"]!.AsArray();
Check(thirdItems.All(x => x!["custody"]!.GetValue<string>() == "inventory"), "returned equipment is active inventory next raid");
thirdItems.First(x => Id(x!) == Id(rifle))!["magazine"] = 6;
await Post("/raids/snapshot", new { operationId = Guid.NewGuid(), raidId = raid3, sequence = 1, items = thirdItems, results = Array.Empty<object>() });
await Post("/raids/close", new { operationId = Guid.NewGuid(), raidId = raid3 });
Console.WriteLine("NEXT RAID AND CROSS-ROUTE RECEIPT PASSED");// Same revision concurrent requests serialize: precisely one winner.
a = await Get(1);
var concurrentItem = Items(a).First()!;
var payload1 = new { operationId=Guid.NewGuid(), profile=1, revision=Rev(a), item=Id(concurrentItem), destination=110 };
var payload2 = new { operationId=Guid.NewGuid(), profile=1, revision=Rev(a), item=Id(concurrentItem), destination=111 };
var races=await Task.WhenAll(client.PostAsJsonAsync("/move",payload1),client.PostAsJsonAsync("/move",payload2));
Check(races.Count(x => x.StatusCode==HttpStatusCode.OK)==1 && races.Count(x => x.StatusCode==HttpStatusCode.Conflict)==1,"concurrent stale moves have one winner");
// A terminal participant cannot acquire a new result from a later snapshot.
a=await Get(1);b=await Get(2);
var raid4=Guid.NewGuid();
await Post("/raids/open",new {operationId=Guid.NewGuid(),raidId=raid4});
await Post("/raids/enter",new {operationId=Guid.NewGuid(),raidId=raid4,profile=1,revision=Rev(a)});
await Post("/raids/enter",new {operationId=Guid.NewGuid(),raidId=raid4,profile=2,revision=Rev(b)});
var fourth=(await client.GetFromJsonAsync<JsonObject>($"/raids/{raid4}"))!;
var fourthItems=fourth["items"]!.AsArray();
foreach(var i in fourthItems.Where(x=>x!["owner"]!.GetValue<string>()==ownerA))i!["custody"]="result";
var extra=JsonSerializerNode(new {id=Guid.NewGuid(),kind=0,slot=-1,magazine=10,reserve=30,owner="",custody="world"});
fourthItems.Add(extra);
await Post("/raids/snapshot",new {operationId=Guid.NewGuid(),raidId=raid4,sequence=1,items=fourthItems,results=new[]{new{profile=1,outcome="extracted"}}});
extra["custody"]="result";extra["owner"]=ownerA;extra["slot"]=1;
await Post("/raids/snapshot",new {operationId=Guid.NewGuid(),raidId=raid4,sequence=2,items=fourthItems,results=Array.Empty<object>()},HttpStatusCode.Conflict);
Check(true,"finished participant cannot silently receive new loot");
extra["custody"]="world";extra["owner"]="";extra["slot"]=-1;
foreach(var i in fourthItems.Where(x=>x!["owner"]!.GetValue<string>()==ownerB)){i!["owner"]="";i["custody"]="world";i["slot"]=-1;}
await Post("/raids/snapshot",new {operationId=Guid.NewGuid(),raidId=raid4,sequence=2,items=fourthItems,results=new[]{new{profile=2,outcome="eliminated"}}});
Check(Items(await Get(1)).Count==Items(a).Count,"separate later elimination preserves earlier extraction");
await Post("/raids/close",new {operationId=Guid.NewGuid(),raidId=raid4});
// Healing consumption is a terminal identity, never an omitted snapshot record.
a=await Get(1);
var kits=Items(a).Where(x=>x!["kind"]!.GetValue<int>()==1).Take(2).Select(x=>Id(x!)).ToArray();
var healingScrap=Id(Items(a).First(x=>x!["kind"]!.GetValue<int>()==2)!);
for(int n=0;n<2;n++)
{
    var kit=Items(a).Single(x=>Id(x!)==kits[n])!;
    if(kit["slot"]!.GetValue<int>()!=8+n)a=await Post("/move",new{operationId=Guid.NewGuid(),profile=1,revision=Rev(a),item=kits[n],destination=8+n});
}
if(Items(a).Single(x=>Id(x!)==healingScrap)!["slot"]!.GetValue<int>()!=0)
    a=await Post("/move",new{operationId=Guid.NewGuid(),profile=1,revision=Rev(a),item=healingScrap,destination=0});
var healingRaid=Guid.NewGuid();
await Post("/raids/open",new{operationId=Guid.NewGuid(),raidId=healingRaid});
await Post("/raids/enter",new{operationId=Guid.NewGuid(),raidId=healingRaid,profile=1,revision=Rev(a)});
var healingState=(await client.GetFromJsonAsync<JsonObject>($"/raids/{healingRaid}"))!;
var healingItems=healingState["items"]!.AsArray();
var groundKit=Guid.NewGuid();
healingItems.Add(JsonSerializerNode(new{id=groundKit,kind=1,slot=-1,magazine=0,reserve=0,owner="",custody="world"}));
await Post("/raids/snapshot",new{operationId=Guid.NewGuid(),raidId=healingRaid,sequence=1,items=healingItems,results=Array.Empty<object>()});
JsonArray CopyHealing()=>healingItems.DeepClone().AsArray();
void Consumed(JsonNode item){item["custody"]="consumed";item["owner"]="";item["slot"]=-1;}
var badHeal=CopyHealing();Consumed(badHeal.Single(x=>Id(x!)==healingScrap)!);
await Post("/raids/snapshot",new{operationId=Guid.NewGuid(),raidId=healingRaid,sequence=2,items=badHeal,results=Array.Empty<object>()},HttpStatusCode.Conflict);
badHeal=CopyHealing();Consumed(badHeal.Single(x=>Id(x!)==groundKit.ToString())!);
await Post("/raids/snapshot",new{operationId=Guid.NewGuid(),raidId=healingRaid,sequence=2,items=badHeal,results=Array.Empty<object>()},HttpStatusCode.Conflict);
badHeal=CopyHealing();badHeal.Add(JsonSerializerNode(new{id=Guid.NewGuid(),kind=1,slot=-1,magazine=0,reserve=0,owner="",custody="consumed"}));
await Post("/raids/snapshot",new{operationId=Guid.NewGuid(),raidId=healingRaid,sequence=2,items=badHeal,results=Array.Empty<object>()},HttpStatusCode.Conflict);
Check(true,"consumption rejects scrap, world item and new consumed identity");
var consumedKit=healingItems.Single(x=>Id(x!)==kits[0])!;Consumed(consumedKit);
var consumeOp=Guid.NewGuid();
var consumeCommand=new{operationId=consumeOp,raidId=healingRaid,sequence=2,items=healingItems,results=Array.Empty<object>()};
await Post("/raids/snapshot",consumeCommand);
await Post("/raids/snapshot",consumeCommand);
Check(true,"owned safe supply consumption accepted once with idempotent receipt");
badHeal=CopyHealing();var restored=badHeal.Single(x=>Id(x!)==kits[0])!;restored["custody"]="inventory";restored["owner"]=ownerA;restored["slot"]=8;
await Post("/raids/snapshot",new{operationId=Guid.NewGuid(),raidId=healingRaid,sequence=3,items=badHeal,results=Array.Empty<object>()},HttpStatusCode.Conflict);
badHeal=CopyHealing();badHeal.Remove(badHeal.Single(x=>Id(x!)==kits[0]));
await Post("/raids/snapshot",new{operationId=Guid.NewGuid(),raidId=healingRaid,sequence=3,items=badHeal,results=Array.Empty<object>()},HttpStatusCode.Conflict);
Check(true,"consumed identity cannot be resurrected or omitted");
foreach(var node in healingItems.Where(x=>x!["custody"]!.GetValue<string>()=="inventory"))
{
    if(node!["slot"]!.GetValue<int>()==9)node["custody"]="result";
    else{node["custody"]="world";node["owner"]="";node["slot"]=-1;}
}
await Post("/raids/snapshot",new{operationId=Guid.NewGuid(),raidId=healingRaid,sequence=3,items=healingItems,results=new[]{new{profile=1,outcome="eliminated"}}});
a=await Get(1);
Check(!Items(a).Any(x=>Id(x!)==kits[0]) && Items(a).Any(x=>Id(x!)==kits[1]),"death preserves unused safe supply without restoring consumed kit");
badHeal=CopyHealing();Consumed(badHeal.Single(x=>Id(x!)==kits[1])!);
await Post("/raids/snapshot",new{operationId=Guid.NewGuid(),raidId=healingRaid,sequence=4,items=badHeal,results=Array.Empty<object>()},HttpStatusCode.Conflict);
Check(true,"confirmed result cannot be consumed afterward");
await Post("/raids/close",new{operationId=Guid.NewGuid(),raidId=healingRaid});
var afterHealing=Guid.NewGuid();await Post("/raids/open",new{operationId=Guid.NewGuid(),raidId=afterHealing});
var resurrected=JsonSerializerNode(new{id=kits[0],kind=1,slot=-1,magazine=0,reserve=0,owner="",custody="world"});
await Post("/raids/snapshot",new{operationId=Guid.NewGuid(),raidId=afterHealing,sequence=1,items=new[]{resurrected},results=Array.Empty<object>()},HttpStatusCode.Conflict);
Check(true,"new raid cannot reuse consumed historical identity");
await Post("/raids/close",new{operationId=Guid.NewGuid(),raidId=afterHealing});
var healingReport=Environment.GetEnvironmentVariable("MARAU6_HEALING_REPORT");
if(healingReport!=null)await File.WriteAllTextAsync(healingReport,System.Text.Json.JsonSerializer.Serialize(new{raid=healingRaid,item=kits[0],profile=1}));

// Save a replay contract for the launcher to verify after restarting the backend.
a=await Get(1);
var restartBody=new {operationId=Guid.NewGuid(),profile=1,revision=Rev(a),item=Id(Items(a).First()!),destination=120};
var restartResult=await Post("/move",restartBody);
var replayFile=Environment.GetEnvironmentVariable("MARAU6_REPLAY_FILE");
if(replayFile!=null)await File.WriteAllTextAsync(replayFile,System.Text.Json.JsonSerializer.Serialize(new {body=restartBody,response=restartResult}));
Console.WriteLine($"ALL {passed} CONTRACT ASSERTIONS PASSED");
static JsonObject JsonSerializerNode(object value)=>System.Text.Json.JsonSerializer.SerializeToNode(value)!.AsObject();