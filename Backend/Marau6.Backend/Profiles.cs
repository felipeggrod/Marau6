namespace Marau6;

public static class Profiles
{
    public static Profile Get(State s, int number)
    {
        if (number is not (1 or 2)) throw new Conflict("Perfil de teste inválido");
        var p = s.Profiles.SingleOrDefault(x => x.Number == number);
        if (p != null) return p;
        p = new Profile { Id = Guid.Parse(number == 1 ? "11111111-1111-1111-1111-111111111111" : "22222222-2222-2222-2222-222222222222"), Number = number };
        for (int n = 0; n < 5; n++) p.Items.Add(new Item { Id = Guid.NewGuid(), Kind = n == 0 ? 0 : n < 3 ? 1 : 2, Slot = n == 0 ? 10 : 99 + n, Magazine = n == 0 ? 10 : 0, Reserve = n == 0 ? 40 : 0, Durability = n == 0 ? 10000 : null, Owner = p.Id.ToString() });
        s.Profiles.Add(p); return p;
    }
}
