namespace Marau6;

public static class Inventory
{
    public static bool Accepts(int slot, int kind) => slot is >= 100 and < 132 || slot is >= 0 and < 8 || slot is 8 or 9 && kind != 0 || slot == 10 && kind == 0;
    public static object Move(State s, MoveCommand c)
    {
        var p = Profiles.Get(s, c.Profile);
        Require(p.Revision == c.Revision, "Versão antiga: atualize a base");
        Require(!s.Raids.Any(r => !r.Closed && r.Participants.Any(x => x.Profile == c.Profile && x.Outcome == "active")), "Equipamento reservado na partida");
        var i = p.Items.SingleOrDefault(x => x.Id == c.Item) ?? throw new Conflict("Item não pertence ao perfil");
        Require(Accepts(c.Destination, i.Kind) && i.Slot != c.Destination, "Destino incompatível");
        var other = p.Items.SingleOrDefault(x => x.Slot == c.Destination);
        Require(other == null || Accepts(i.Slot, other.Kind), "Troca incompatível");
        if (other != null) other.Slot = i.Slot;
        i.Slot = c.Destination; p.Revision++; return p;
    }
    static void Require(bool valid,string message){if(!valid)throw new Conflict(message);}
}
