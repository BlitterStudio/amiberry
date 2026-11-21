using System.IO.Pipes;
using System.Text;
using System.Text.Json;
using System.Text.Json.Nodes;

namespace Amiberry.Windows;

internal sealed class NativeClient : IAsyncDisposable
{
    private const int MaximumMessage = 4 * 1024 * 1024;
    private readonly NamedPipeClientStream pipe;
    private readonly SemaphoreSlim requests = new(1, 1);
    private readonly CancellationTokenSource lifetime = new();
    private readonly byte[] readBuffer = new byte[8192];
    private int readOffset, readCount;
    private bool disposed;
    public event Action<Exception>? Disconnected;

    public NativeClient(string name)
    {
        if (!name.StartsWith("amiberry-native-", StringComparison.Ordinal) || name.Contains('\\') || name.Contains('/'))
            throw new ArgumentException("Invalid native host pipe name.");
        pipe = new NamedPipeClientStream(".", name, PipeDirection.InOut, PipeOptions.Asynchronous,
            System.Security.Principal.TokenImpersonationLevel.Anonymous);
    }

    public Task ConnectAsync() => pipe.ConnectAsync(15000, lifetime.Token);

    public async Task<JsonNode> CallAsync(string operation, object? fields = null, CancellationToken cancellation = default)
    {
        using var timeout = CancellationTokenSource.CreateLinkedTokenSource(lifetime.Token, cancellation);
        timeout.CancelAfter(TimeSpan.FromSeconds(60));
        // Only the wait for a turn is bounded. A caller that never wrote anything may fail without losing the connection.
        try { await requests.WaitAsync(timeout.Token); }
        catch (OperationCanceledException) when (!lifetime.IsCancellationRequested && !cancellation.IsCancellationRequested)
        {
            throw new NativeQueueTimeoutException();
        }
        // The limit covers only waiting for a turn. Once a request is on the wire the core may legitimately
        // reply late (a modal file picker or confirmation blocks the thread that services the pipe), and a
        // cancelled partial exchange would drop the connection. A dead core still ends the read at once.
        timeout.CancelAfter(Timeout.InfiniteTimeSpan);
        try { timeout.Token.ThrowIfCancellationRequested(); } catch { requests.Release(); throw; }
        bool sent = false;
        try
        {
            ObjectDisposedException.ThrowIf(disposed, this);
            var request = fields is null ? new JsonObject() : JsonSerializer.SerializeToNode(fields)!.AsObject();
            foreach (string key in request.Where(pair => pair.Value is null).Select(pair => pair.Key).ToArray()) request.Remove(key);
            request["op"] = operation;
            byte[] bytes = Encoding.UTF8.GetBytes(request.ToJsonString() + "\n");
            if (bytes.Length > MaximumMessage) throw new InvalidOperationException("The request is too large.");
            sent = true;
            await pipe.WriteAsync(bytes, timeout.Token);
            await pipe.FlushAsync(timeout.Token);
            using var message = new MemoryStream();
            while (true)
            {
                if (readOffset == readCount)
                {
                    readCount = await pipe.ReadAsync(readBuffer, timeout.Token);
                    readOffset = 0;
                    if (readCount == 0) throw new EndOfStreamException("The emulator closed its connection.");
                }
                int newline = Array.IndexOf(readBuffer, (byte)'\n', readOffset, readCount - readOffset);
                int count = (newline < 0 ? readCount : newline) - readOffset;
                if (message.Length + count > MaximumMessage) throw new IOException("The emulator reply is too large.");
                message.Write(readBuffer, readOffset, count);
                readOffset += count;
                if (newline < 0) continue;
                readOffset++;
                var reply = JsonNode.Parse(message.GetBuffer().AsSpan(0, (int)message.Length))
                    ?? throw new IOException("The emulator returned an empty reply.");
                if (!reply.B("ok")) throw new NativeRequestException(reply.S("error"));
                return reply["data"]?.DeepClone() ?? new JsonObject();
            }
        }
        catch (Exception error) when (sent && error is not NativeRequestException)
        {
            // A cancelled partial exchange cannot be reused: its reply would belong to the next request.
            await DisposeAsync();
            Disconnected?.Invoke(error);
            throw;
        }
        finally { requests.Release(); }
    }

    public async Task ActionAsync(string name, string? path = null, int? index = null, int? drive = null, int? apply = null)
    {
        var ticket = await CallAsync("action", new { name, path, index, drive, apply });
        ulong id = ticket["id"]!.GetValue<ulong>();
        using var timeout = CancellationTokenSource.CreateLinkedTokenSource(lifetime.Token);
        timeout.CancelAfter(TimeSpan.FromSeconds(60));
        while (true)
        {
            // The deadline applies between polls; cancelling an exchange in flight would drop the connection.
            var result = await CallAsync("poll", new { id });
            switch (result.S("state"))
            {
                case "done": return;
                case "failed": throw new NativeRequestException(result.S("error"));
                case "unknown": throw new NativeRequestException("The emulator no longer recognizes this action.");
            }
            await Task.Delay(30, timeout.Token);
        }
    }

    public ValueTask DisposeAsync()
    {
        if (!disposed)
        {
            disposed = true;
            lifetime.Cancel();
            pipe.Dispose();
        }
        return ValueTask.CompletedTask;
    }
}

internal sealed class NativeRequestException(string message) : Exception(message);

// An earlier request kept the core busy for the whole queue limit; nothing was sent, so the connection is intact.
internal sealed class NativeQueueTimeoutException() : Exception("The emulator is still busy with an earlier request.");

internal static class JsonValues
{
    public static string S(this JsonNode? node, string key, string fallback = "") => node?[key]?.ToString() ?? fallback;
    public static bool B(this JsonNode? node, string key, bool fallback = false) => bool.TryParse(node.S(key), out bool value) ? value : fallback;
    public static int I(this JsonNode? node, string key, int fallback = 0) => int.TryParse(node.S(key), out int value) ? value : fallback;
    public static double D(this JsonNode? node, string key, double fallback = 0) => double.TryParse(node.S(key), System.Globalization.NumberStyles.Float, System.Globalization.CultureInfo.InvariantCulture, out double value) ? value : fallback;
    public static IEnumerable<JsonNode> Items(this JsonNode? node, string key) => node?[key] is JsonArray array ? array.OfType<JsonNode>() : [];
    public static IEnumerable<JsonNode> Items(this JsonNode? node) => node is JsonArray array ? array.OfType<JsonNode>() : [];
}
