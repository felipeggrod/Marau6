using Marau6;
using System.Security.Cryptography;
using System.Text.Json;
static class KitContracts
{
    static void Check(bool value,string name){if(!value)throw new Exception(name);}
    static string Hash(object command)=>Convert.ToHexString(SHA256.HashData(JsonSerializer.SerializeToUtf8Bytes(command,Store.Json)));
    static List<(Guid,int)> Layout(Profile p)=>p.Items.Select(x=>(x.Id,x.Slot)).OrderBy(x=>x.Id).ToList();
    static bool Unique(Profile p)=>p.Items.Select(x=>x.Slot).Distinct().Count()==p.Items.Count;
    static (State s,Profile p,Guid raid,Item kit) Basic(Action<State,Profile>? before=null)
    {
        var s=new State();var p=Profiles.Get(s,1);before?.Invoke(s,p);
        var raid=Guid.NewGuid();RaidService.Open(s,new RaidCommand(Guid.NewGuid(),raid));
        var entered=RaidService.Enter(s,new EnterCommand(Guid.NewGuid(),raid,1,p.Revision,"basic"));
        var kit=((List<Item>)entered.GetType().GetProperty("items")!.GetValue(entered)!).Single();
        return (s,p,raid,kit);
    }
    static Item Supply(State s,Profile p)
    {
        var supply=p.Items.First(x=>x.Kind==1);Inventory.Move(s,new MoveCommand(Guid.NewGuid(),1,p.Revision,supply.Id,8));return supply;
    }
    public static void Run()
    {
        // BASIC entry leaves the base untouched and mints one fresh rifle.
        var s=new State();var p=Profiles.Get(s,1);var before=Layout(p);var revision=p.Revision;
        var raid=Guid.NewGuid();RaidService.Open(s,new RaidCommand(Guid.NewGuid(),raid));
        var entered=RaidService.Enter(s,new EnterCommand(Guid.NewGuid(),raid,1,p.Revision,"basic"));
        var items=(List<Item>)entered.GetType().GetProperty("items")!.GetValue(entered)!;
        var r=s.Raids.Single();var kit=items.Single();
        Check(Layout(p).SequenceEqual(before) && p.Revision==revision+1,"Basic entry must keep profile items and slots");
        Check(kit.Kind==0 && kit.Slot==10 && kit.Magazine==20 && kit.Reserve==40 && kit.Durability==10000 && kit.Owner==p.Id.ToString() && kit.Custody=="inventory","Kit rifle must be a fresh 20/40 rifle at slot 10");
        Check(kit.Id!=Guid.Empty && !p.Items.Any(x=>x.Id==kit.Id) && r.Items.Single().Id==kit.Id,"Kit rifle must be a new reserved identity");
        Check(r.Participants.Single().Kit=="basic","Participant must record basic kit");
        // Invalid kit is rejected without side effects; null kit is today's entry.
        var p2=Profiles.Get(s,2);
        try{RaidService.Enter(s,new EnterCommand(Guid.NewGuid(),raid,2,p2.Revision,"deluxe"));throw new Exception("Invalid kit accepted");}catch(Conflict){}
        Check(r.Participants.Count==1 && p2.Revision==0,"Invalid kit must not change state");
        var equipment=p2.Items.Where(x=>x.Slot<100).Select(x=>x.Id).ToList();
        RaidService.Enter(s,new EnterCommand(Guid.NewGuid(),raid,2,p2.Revision));
        Check(p2.Items.All(x=>x.Slot>=100) && equipment.All(id=>r.Items.Any(x=>x.Id==id)) && r.Participants.Single(x=>x.Profile==2).Kit=="custom","Null kit must reserve profile equipment");
        Check(JsonSerializer.Deserialize<Participation>("{\"profile\":1,\"outcome\":\"active\"}",Store.Json)!.Kit=="custom","Legacy participant must read as custom");
        // Extraction while the profile still owns a slot-10 rifle remaps the kit rifle to the stash.
        (s,p,raid,kit)=Basic();var own=p.Items.Single(x=>x.Slot==10);
        var result=kit.Copy();result.Custody="result";
        RaidService.Snapshot(s,new SnapshotCommand(Guid.NewGuid(),raid,1,[result],[new Result(1,"extracted")]));
        var back=p.Items.Single(x=>x.Id==kit.Id);
        Check(own.Slot==10 && back.Slot==104 && back.Custody=="inventory" && Unique(p),"Extracted kit rifle must take the first free stash slot");
        // Death with looted safe item while the profile's slot 8 is occupied.
        Item supply=null!;(s,p,raid,kit)=Basic((st,pr)=>supply=Supply(st,pr));
        var loot=new Item{Id=Guid.NewGuid(),Kind=1,Slot=-1,Custody="world"};var rifle=kit.Copy();
        RaidService.Snapshot(s,new SnapshotCommand(Guid.NewGuid(),raid,1,[rifle,loot],[]));
        var held=loot.Copy();held.Owner=p.Id.ToString();held.Slot=8;held.Custody="result";rifle.Owner="";rifle.Slot=-1;rifle.Custody="world";
        RaidService.Snapshot(s,new SnapshotCommand(Guid.NewGuid(),raid,2,[rifle,held],[new Result(1,"eliminated")]));
        Check(supply.Slot==8 && p.Items.Single(x=>x.Id==loot.Id).Slot is >=100 and <132 && !p.Items.Any(x=>x.Id==kit.Id) && Unique(p),"Death safe loot must remap to stash");
        // Abandonment of a basic participant remaps the same way.
        (s,p,raid,kit)=Basic((st,pr)=>supply=Supply(st,pr));
        loot=new Item{Id=Guid.NewGuid(),Kind=1,Slot=-1,Custody="world"};
        RaidService.Snapshot(s,new SnapshotCommand(Guid.NewGuid(),raid,1,[kit.Copy(),loot],[]));
        held=loot.Copy();held.Owner=p.Id.ToString();held.Slot=8;held.Custody="inventory";
        RaidService.Snapshot(s,new SnapshotCommand(Guid.NewGuid(),raid,2,[kit.Copy(),held],[]));
        RaidService.Close(s,new RaidCommand(Guid.NewGuid(),raid));
        Check(supply.Slot==8 && p.Items.Single(x=>x.Id==loot.Id).Slot is >=100 and <132 && s.Raids.Single().Participants.Single().Outcome=="abandoned" && Unique(p),"Abandoned safe loot must remap to stash");
        // A full stash loses the colliding item instead of failing the raid path.
        (s,p,raid,kit)=Basic((st,pr)=>{foreach(var slot in Enumerable.Range(100,32).Where(x=>!pr.Items.Any(i=>i.Slot==x)).ToList())pr.Items.Add(new Item{Id=Guid.NewGuid(),Kind=2,Slot=slot,Owner=pr.Id.ToString()});});
        var count=p.Items.Count;revision=p.Revision;result=kit.Copy();result.Custody="result";
        RaidService.Snapshot(s,new SnapshotCommand(Guid.NewGuid(),raid,1,[result],[new Result(1,"extracted")]));
        Check(p.Items.Count==count && p.Revision==revision+1 && Unique(p) && s.Raids.Single().Participants.Single().Outcome=="extracted","Full stash must drop the item without throwing");
        // Legacy enter body keeps its receipt hash; an explicit kit is part of the content.
        var legacy=new{operationId=Guid.NewGuid(),raidId=Guid.NewGuid(),profile=1,revision=3};
        var parsed=JsonSerializer.Deserialize<EnterCommand>(JsonSerializer.Serialize(legacy,Store.Json),Store.Json)!;
        Check(parsed.Kit==null && Hash(new{action="enter",data=legacy})==Hash(new{action="enter",data=parsed}),"Legacy enter hash must be unchanged");
        var withKit=JsonSerializer.Deserialize<EnterCommand>(JsonSerializer.Serialize(new{legacy.operationId,legacy.raidId,legacy.profile,legacy.revision,kit="basic"},Store.Json),Store.Json)!;
        Check(withKit.Kit=="basic" && Hash(new{action="enter",data=withKit})!=Hash(new{action="enter",data=parsed}),"Kit must change the receipt hash");
        Console.WriteLine("PASS basic kit entry, return remapping, full stash and legacy enter hash");
    }
}
