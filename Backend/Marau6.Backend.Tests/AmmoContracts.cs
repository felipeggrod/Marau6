using Marau6;
static class AmmoContracts
{
    public static void Run()
    {
        var s=new State();var p=Profiles.Get(s,1);var rifle=p.Items.Single(x=>x.Kind==0);
        if(rifle.Magazine!=20 || rifle.Reserve!=40)throw new Exception("New rifle must be20/40");
        var box=new Item{Id=Guid.NewGuid(),Kind=3,Slot=100,Owner=p.Id.ToString()};p.Items.RemoveAll(x=>x.Slot==100);p.Items.Add(box);
        Inventory.Move(s,new MoveCommand(Guid.NewGuid(),1,p.Revision,box.Id,10));
        if(rifle.Reserve!=60 || p.Items.Contains(box))throw new Exception("Ammo transfer must consume once and add20");
        try{Inventory.Move(s,new MoveCommand(Guid.NewGuid(),1,p.Revision,box.Id,10));throw new Exception("Consumed ammo reused");}catch(Conflict){}
        rifle.Reserve=110;var extra=new Item{Id=Guid.NewGuid(),Kind=3,Slot=0,Owner=p.Id.ToString()};p.Items.Add(extra);
        try{Inventory.Move(s,new MoveCommand(Guid.NewGuid(),1,p.Revision,extra.Id,10));throw new Exception("Partial box consumed");}catch(Conflict){}
        if(rifle.Reserve!=110 || !p.Items.Contains(extra))throw new Exception("Overflow changed items");
        rifle.Reserve=100;Inventory.Move(s,new MoveCommand(Guid.NewGuid(),1,p.Revision,extra.Id,10));
        if(rifle.Reserve!=120)throw new Exception("Maximum120 unreachable");
        var raid=Guid.NewGuid();RaidService.Open(s,new RaidCommand(Guid.NewGuid(),raid));
        try{RaidService.Snapshot(s,new SnapshotCommand(Guid.NewGuid(),raid,1,[new Item{Id=box.Id,Kind=3,Slot=-1,Custody="world"}],[]));throw new Exception("Base consumed identity resurrected");}catch(Conflict){}
        Console.WriteLine("PASS base ammunition consumption, overflow and historical identity");
    }
}
