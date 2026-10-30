using System.Text.Json;
using System.Security.Cryptography;
using Npgsql;

namespace Marau6;

// Prototype's coarse transaction lock is deliberate: two profiles share one raid.
// SQL constraints project all live item custodians and independently catch duplicates.
public sealed class Store(NpgsqlDataSource data)
{
    public static readonly JsonSerializerOptions Json = new(JsonSerializerDefaults.Web);
    public async Task Initialize()
    {
        await using var c = data.CreateCommand("""
            CREATE TABLE IF NOT EXISTS marau6_state (id integer PRIMARY KEY CHECK(id=1), body jsonb NOT NULL);
            INSERT INTO marau6_state VALUES(1,'{"profiles":[],"raids":[]}') ON CONFLICT DO NOTHING;
            CREATE TABLE IF NOT EXISTS marau6_receipts (id uuid PRIMARY KEY, hash text NOT NULL, response jsonb NOT NULL);
            CREATE TABLE IF NOT EXISTS marau6_items (id uuid PRIMARY KEY, location text NOT NULL, slot integer NOT NULL);
            CREATE UNIQUE INDEX IF NOT EXISTS marau6_slot ON marau6_items(location,slot) WHERE slot>=0;
            """);
        await c.ExecuteNonQueryAsync();
    }
    public async Task<string> Execute<T>(T command, Guid? operation, Func<State, object> change)
    {
        var hash = Convert.ToHexString(SHA256.HashData(JsonSerializer.SerializeToUtf8Bytes(command, Json)));
        await using var conn = await data.OpenConnectionAsync();
        await using var tx = await conn.BeginTransactionAsync();
        await using var read = new NpgsqlCommand("SELECT body::text FROM marau6_state WHERE id=1 FOR UPDATE", conn, tx);
        var state = JsonSerializer.Deserialize<State>((string)(await read.ExecuteScalarAsync())!, Json)!;
        if (operation is { } op)
        {
            if (op == Guid.Empty) throw new Conflict("operationId obrigatório");
            await using var receipt = new NpgsqlCommand("SELECT hash,response::text FROM marau6_receipts WHERE id=$1", conn, tx);
            receipt.Parameters.AddWithValue(op);
            await using var rows = await receipt.ExecuteReaderAsync();
            if (await rows.ReadAsync())
            {
                if (rows.GetString(0) != hash) throw new Conflict("operationId reutilizado com outro conteúdo");
                return rows.GetString(1);
            }
        }
        var response = JsonSerializer.Serialize(change(state), Json);
        await using var save = new NpgsqlCommand("UPDATE marau6_state SET body=$1::jsonb WHERE id=1", conn, tx);
        save.Parameters.AddWithValue(JsonSerializer.Serialize(state, Json));
        await save.ExecuteNonQueryAsync();
        await using var clear = new NpgsqlCommand("DELETE FROM marau6_items", conn, tx);
        await clear.ExecuteNonQueryAsync();
        foreach (var p in state.Profiles)
            foreach (var item in p.Items) await Index(item, "profile:" + p.Id, item.Slot, conn, tx);
        foreach (var r in state.Raids.Where(x => !x.Closed))
            foreach (var i in r.Items.Where(x => x.Custody != "result"))
                await Index(i, "raid:" + r.Id + ":" + i.Owner, i.Owner == "" ? -1 : i.Slot, conn, tx);
        if (operation is { } id)
        {
            await using var insert = new NpgsqlCommand("INSERT INTO marau6_receipts VALUES($1,$2,$3::jsonb)", conn, tx);
            insert.Parameters.AddWithValue(id); insert.Parameters.AddWithValue(hash); insert.Parameters.AddWithValue(response);
            await insert.ExecuteNonQueryAsync();
        }
        await tx.CommitAsync();
        return response;
    }
    static async Task Index(Item i, string location, int slot, NpgsqlConnection c, NpgsqlTransaction t)
    {
        await using var insert = new NpgsqlCommand("INSERT INTO marau6_items VALUES($1,$2,$3)", c, t);
        insert.Parameters.AddWithValue(i.Id); insert.Parameters.AddWithValue(location); insert.Parameters.AddWithValue(slot);
        await insert.ExecuteNonQueryAsync();
    }
}
