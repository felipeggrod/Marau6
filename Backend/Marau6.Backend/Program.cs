using Marau6;
using Npgsql;
using System.Security.Cryptography;
using System.Text;

var builder = WebApplication.CreateBuilder(args);
var key = Environment.GetEnvironmentVariable("MARAU6_API_KEY") ?? throw new InvalidOperationException("MARAU6_API_KEY ausente");
if (key.Length < 32) throw new InvalidOperationException("Use uma chave local aleatória de pelo menos 32 caracteres");
var connection = Environment.GetEnvironmentVariable("MARAU6_DB") ?? throw new InvalidOperationException("MARAU6_DB ausente");
builder.Services.AddSingleton(NpgsqlDataSource.Create(connection));
builder.Services.AddSingleton<Store>();
var app = builder.Build();
await app.Services.GetRequiredService<Store>().Initialize();
app.Use(async (context, next) =>
{
    var provided = context.Request.Headers["X-Marau6-Key"].ToString();
    if (!CryptographicOperations.FixedTimeEquals(Encoding.UTF8.GetBytes(provided), Encoding.UTF8.GetBytes(key)))
    { context.Response.StatusCode = 401; return; }
    try { await next(context); }
    catch (Conflict e) { context.Response.StatusCode = 409; await context.Response.WriteAsJsonAsync(new { error = e.Message }); }
    catch (NpgsqlException e) { app.Logger.LogError("Database operation failed: {Message}", e.Message); context.Response.StatusCode = 503; await context.Response.WriteAsJsonAsync(new { error = "Banco temporariamente indisponível" }); }
});
IResult Json(string text) => Results.Content(text, "application/json");
app.MapGet("/health", () => new { status = "ok" });
app.MapGet("/state/{profile:int}", async (int profile, Store store) => Json(await store.Execute(new { profile }, null, s => Profiles.Get(s,profile))));
app.MapGet("/raids/{id:guid}", async (Guid id, Store store) => Json(await store.Execute(new { id }, null, s => RaidService.ReadRaid(s,id))));
app.MapPost("/move", async (MoveCommand c, Store store) => Json(await store.Execute(new { action = "move", data = c },c.OperationId,s => Inventory.Move(s,c))));
app.MapPost("/raids/open", async (RaidCommand c, Store store) => Json(await store.Execute(new { action = "open", data = c },c.OperationId,s => RaidService.Open(s,c))));
app.MapPost("/raids/enter", async (EnterCommand c, Store store) => Json(await store.Execute(new { action = "enter", data = c },c.OperationId,s => RaidService.Enter(s,c))));
app.MapPost("/raids/snapshot", async (SnapshotCommand c, Store store) => Json(await store.Execute(new { action = "snapshot", data = c },c.OperationId,s => RaidService.Snapshot(s,c))));
app.MapPost("/raids/close", async (RaidCommand c, Store store) => Json(await store.Execute(new { action = "close", data = c },c.OperationId,s => RaidService.Close(s,c))));
app.Run();
