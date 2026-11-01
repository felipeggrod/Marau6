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
        Require(i.Slot != c.Destination, "Destino incompatível");
        var other = p.Items.SingleOrDefault(x => x.Slot == c.Destination);
        if(i.Kind==3 && other?.Kind==0)
        {
            Require(other.Reserve<=100, "Reserva cheia: caixa de 20 deve caber inteira (limite 120)");
            Require(!s.ConsumedItems.Contains(i.Id), "Munição já consumida");
            other.Reserve+=20;p.Items.Remove(i);s.ConsumedItems.Add(i.Id);p.Revision++;return p;
        }
        Require(Accepts(c.Destination, i.Kind), "Destino incompatível");
        Require(other == null || Accepts(i.Slot, other.Kind), "Troca incompatível");
        if (other != null) other.Slot = i.Slot;
        i.Slot = c.Destination; p.Revision++; return p;
    }
    static void Require(bool valid,string message){if(!valid)throw new Conflict(message);}
}
