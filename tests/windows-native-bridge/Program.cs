using System.IO.Pipes;
using System.Runtime.InteropServices;
using System.Text;
using System.Text.Json;

internal static class Program
{
    private static readonly List<string> Assertions = [];

    private static void Check(bool condition, string name)
    {
        if (!condition) throw new InvalidOperationException(name);
        Assertions.Add(name);
    }

    // Failure text shows the exact strings, JSON-escaped, so encoding mismatches are visible.
    private static void CheckPath(bool condition, string name, string actual, string expected)
    {
        if (!condition)
            throw new InvalidOperationException($"{name}: actual {JsonSerializer.Serialize(actual)}, expected {JsonSerializer.Serialize(expected)}");
        Assertions.Add(name);
    }

    [DllImport("kernel32.dll")]
    private static extern uint GetACP();

    private static string Value(JsonElement snapshot, string id) => snapshot.GetProperty("values")
        .EnumerateArray().Single(item => item.GetProperty("id").GetString() == id)
        .GetProperty("value").GetString()!;

    private static string Configuration(JsonElement snapshot) => snapshot.GetProperty("currentConfiguration").GetString()!;

    private static bool SamePath(string actual, string expected) => !string.IsNullOrEmpty(actual) &&
        string.Equals(Path.GetFullPath(actual), Path.GetFullPath(expected), StringComparison.OrdinalIgnoreCase);

    private static Dictionary<string, string?> Values(JsonElement snapshot) => snapshot.GetProperty("values")
        .EnumerateArray().ToDictionary(item => item.GetProperty("id").GetString()!, item => item.GetProperty("value").GetString());

    public static async Task<int> Main(string[] args)
    {
        string? reportPath = Environment.GetEnvironmentVariable("AMIBERRY_NATIVE_TEST_REPORT");
        string? failure = null;
        PipeClient? client = null;
        using var lifetime = new CancellationTokenSource(TimeSpan.FromSeconds(90));
        try
        {
            if (string.IsNullOrWhiteSpace(reportPath))
                throw new InvalidOperationException("AMIBERRY_NATIVE_TEST_REPORT must name the report file.");
            if (args.Length != 2 || args[0] != "--pipe" || string.IsNullOrWhiteSpace(args[1]))
                throw new ArgumentException("Expected --pipe NAME from the native core launcher.");

            // The launcher puts this client in the isolated portable install's native-ui directory.
            uint acp = GetACP();
            string path = Path.GetFullPath(Path.Combine(AppContext.BaseDirectory, "..", "Configurations", "bridge-\u6a4b-\u00e4.uae"));
            Directory.CreateDirectory(Path.GetDirectoryName(path)!);
            client = new PipeClient(args[1], lifetime.Token);
            await client.ConnectAsync();

            var snapshot = await client.CallAsync(new { op = "open", preset = "a500", config = 0, compatibility = 1 });
            Check(Value(snapshot, "cpu_model") == "68000", "open A500 uses CPU 68000");
            Check(Configuration(snapshot) == "", "new preset has no saved configuration association");

            await client.CallAsync(new { op = "save", path });
            snapshot = await client.CallAsync(new { op = "snapshot" });
            string saved = Configuration(snapshot);
            Check(SamePath(saved, path), "save associates the Unicode configuration path");
            Check(File.Exists(path), "save creates the Unicode .uae file");
            Check(Value(snapshot, "cpu_model") == "68000", "saved A500 retains CPU 68000");

            snapshot = await client.CallAsync(new { op = "preset", id = "a1200", config = 0, compatibility = 1 });
            Check(Configuration(snapshot) == saved, "model preset preserves the saved association");
            Check(Value(snapshot, "cpu_model") == "68020", "A1200 preset changes CPU to 68020");

            snapshot = await client.CallAsync(new { op = "revert" });
            Check(Configuration(snapshot) == saved, "Revert preserves the saved association");
            Check(Value(snapshot, "cpu_model") == "68000", "Revert restores saved CPU 68000");

            snapshot = await client.CallAsync(new { op = "open", path = saved });
            Check(Configuration(snapshot) == saved, "open reloads the Unicode saved association");
            Check(Value(snapshot, "cpu_model") == "68000", "open reloads saved CPU 68000");

            // The layer preserves an existing FPU when upgrading the CPU;
            // selecting a CPU alone intentionally also permits EC models.
            await client.CallAsync(new { op = "set", id = "cycle_exact", value = "false" });
            await client.CallAsync(new { op = "set", id = "cpu_model", value = "68030" });
            await client.CallAsync(new { op = "set", id = "fpu_model", value = "68882" });
            snapshot = await client.CallAsync(new { op = "set", id = "cpu_model", value = "68040" });
            Check(Value(snapshot, "cpu_model") == "68040", "CPU setter crosses the native pipe");
            Check(Value(snapshot, "fpu_model") == "68040", "CPU setter applies the dependent internal FPU rule");

            // Rename is file metadata only: the pending 68040 edit must survive, and nothing may reload prefs.
            string renamed = Path.Combine(Path.GetDirectoryName(path)!, "bridge-renamed-\u00e9-\u6539.uae");
            File.Delete(renamed);
            var library = await client.CallAsync(new { op = "configuration_rename", path = saved, newPath = renamed });
            Check(SamePath(library.GetProperty("currentConfiguration").GetString()!, renamed), "rename reply reports the renamed Unicode association");
            Check(!File.Exists(path), "rename removes the old Unicode .uae file");
            Check(File.Exists(renamed), "rename creates the new Unicode .uae file");
            snapshot = await client.CallAsync(new { op = "snapshot" });
            saved = Configuration(snapshot);
            Check(SamePath(saved, renamed), "rename keeps the active association on the new path");
            Check(Value(snapshot, "cpu_model") == "68040", "rename preserves the private CPU 68040 edit");
            Check(Value(snapshot, "fpu_model") == "68040", "rename preserves the dependent internal FPU 68040");

            await client.CallAsync(new { op = "save", path = saved });
            Check(!File.Exists(path), "save after rename does not recreate the old file");
            Check(File.Exists(renamed), "save after rename writes the new file");
            Check(File.ReadAllLines(renamed).Any(line => line.Replace(" ", "") == "cpu_model=68040"), "save after rename stores the private CPU 68040 edit");

            string sentinel = Path.Combine(Path.GetDirectoryName(path)!, "bridge-existing-\u00fc-\u4e2d.uae");
            const string sentinelText = "; bridge sentinel must remain unchanged\ncpu_model=68000\n";
            await File.WriteAllTextAsync(sentinel, sentinelText, new UTF8Encoding(false));
            string sourceText = await File.ReadAllTextAsync(renamed);
            var refused = await client.ExchangeAsync(new { op = "configuration_rename", path = saved, newPath = sentinel });
            Check(!refused.GetProperty("ok").GetBoolean(), "rename to an existing configuration returns an error envelope");
            Check(!string.IsNullOrWhiteSpace(refused.GetProperty("error").GetString()), "rename refusal includes an error message");
            Check(await File.ReadAllTextAsync(sentinel) == sentinelText, "rename refusal leaves the existing target unchanged");
            Check(File.Exists(renamed) && await File.ReadAllTextAsync(renamed) == sourceText, "rename refusal leaves the source file unchanged");
            snapshot = await client.CallAsync(new { op = "snapshot" });
            Check(Configuration(snapshot) == saved, "rename refusal keeps the association on the new path");
            Check(Value(snapshot, "cpu_model") == "68040", "rename refusal keeps the private CPU 68040 edit");
            var beforeInvalid = Values(snapshot);

            var rejected = await client.ExchangeAsync(new { op = "set", id = "cpu_model", value = "68099" });
            Check(!rejected.GetProperty("ok").GetBoolean(), "invalid CPU setter returns an error envelope");
            Check(!string.IsNullOrWhiteSpace(rejected.GetProperty("error").GetString()), "invalid CPU setter includes an error message");
            snapshot = await client.CallAsync(new { op = "snapshot" });
            Check(Value(snapshot, "cpu_model") == "68040", "invalid CPU setter retains CPU 68040");
            Check(Value(snapshot, "fpu_model") == "68040", "invalid CPU setter retains internal FPU 68040");
            var afterInvalid = Values(snapshot);
            Check(beforeInvalid.Count == afterInvalid.Count && beforeInvalid.All(item =>
                afterInvalid.TryGetValue(item.Key, out var value) && value == item.Value), "invalid CPU setter leaves all setting values unchanged");
            Check(Configuration(snapshot) == saved, "invalid CPU setter preserves the saved association");

            // A path the UI types is Unicode whatever the process code page is: it must survive save and reopen.
            string configurations = Path.GetDirectoryName(path)!;
            string unicodeDisk = Path.Combine(configurations, "disk-\u6a4b-\u00e4.adf");
            await File.WriteAllBytesAsync(unicodeDisk, new byte[901120]);
            snapshot = await client.CallAsync(new { op = "set", id = "floppy0", value = unicodeDisk });
            Check(SamePath(Value(snapshot, "floppy0"), unicodeDisk), "Unicode media path is stored exactly");
            string unicodeSaved = Path.Combine(configurations, "media-\u6a4b-\u00e4.uae");
            await client.CallAsync(new { op = "save", path = unicodeSaved });
            snapshot = await client.CallAsync(new { op = "open", path = unicodeSaved });
            Check(SamePath(Configuration(snapshot), unicodeSaved), "reopening the saved Unicode configuration keeps its association");
            Check(SamePath(Value(snapshot, "floppy0"), unicodeDisk), "Unicode media path survives save and reopen");

            // Old builds saved ANSI paths; the UI is UTF-8. They must display exactly and survive save and reopen.
            Encoding.RegisterProvider(CodePagesEncodingProvider.Instance);
            Encoding ansi = Encoding.GetEncoding((int)acp, EncoderFallback.ExceptionFallback, DecoderFallback.ExceptionFallback);
            bool Representable(string text)
            {
                try { ansi.GetBytes(text); return true; }
                catch (EncoderFallbackException) { return false; }
            }
            string? accent = new[] { "caf\u00e9", "m\u00fcnchen", "na\u00efve", "\u0436", "\u03b1", "\u6a4b" }.FirstOrDefault(Representable);
            if (acp != 65001 && accent is not null)
            {
                string folder = Path.Combine(configurations, "legacy-" + accent);
                Directory.CreateDirectory(folder);
                string disk = Path.Combine(folder, "disk-" + accent + ".adf");
                await File.WriteAllBytesAsync(disk, new byte[901120]);
                string legacy = Path.Combine(configurations, "legacy-" + accent + ".uae");
                await File.WriteAllBytesAsync(legacy, ansi.GetBytes("cpu_model=68000\nfloppy0=" + disk + "\n"));

                snapshot = await client.CallAsync(new { op = "open", path = legacy });
                Check(SamePath(Configuration(snapshot), legacy), "open reads an ANSI-named legacy configuration");
                string shown = Value(snapshot, "floppy0");
                CheckPath(!shown.Contains('\uFFFD') && SamePath(shown, disk), "ANSI media path in an old configuration reaches the UI as exact UTF-8", shown, disk);
                string raw = await client.ExchangeRawAsync(new { op = "snapshot" });
                Check(!raw.Contains('\uFFFD'), "snapshot of an ANSI configuration has no replacement characters");

                string legacySaved = Path.Combine(configurations, "legacy-saved-\u6a4b-" + accent + ".uae");
                await client.CallAsync(new { op = "save", path = legacySaved });
                snapshot = await client.CallAsync(new { op = "open", path = legacySaved });
                Check(SamePath(Configuration(snapshot), legacySaved), "legacy configuration saves to a Unicode path and reopens");
                CheckPath(!Value(snapshot, "floppy0").Contains('\uFFFD') && SamePath(Value(snapshot, "floppy0"), disk),
                    "ANSI media path survives save and reopen as the same file", Value(snapshot, "floppy0"), disk);
            }
        }
        catch (Exception error)
        {
            failure = error.ToString();
        }

        string report = (failure is null ? "PASS" : "FAIL") + $": {Assertions.Count} assertions" + Environment.NewLine +
            string.Join(Environment.NewLine, Assertions.Select(name => "OK: " + name)) + Environment.NewLine;
        if (failure is not null) report += failure + Environment.NewLine;
        Console.Write(report);
        try
        {
            if (string.IsNullOrWhiteSpace(reportPath)) return 1;
            await File.WriteAllTextAsync(reportPath, report, new UTF8Encoding(false));
        }
        catch (Exception error)
        {
            Console.Error.WriteLine($"Cannot write native bridge report: {error}");
            return 1;
        }
        finally
        {
            // Publish the report before closing the pipe causes the core to quit.
            if (client is not null) await client.DisposeAsync();
        }
        return failure is null ? 0 : 1;
    }
}

internal sealed class PipeClient : IAsyncDisposable
{
    private readonly NamedPipeClientStream pipe;
    private readonly StreamReader reader;
    private readonly StreamWriter writer;
    private readonly CancellationToken lifetime;

    public PipeClient(string name, CancellationToken lifetime)
    {
        this.lifetime = lifetime;
        pipe = new NamedPipeClientStream(".", name, PipeDirection.InOut, PipeOptions.Asynchronous,
            System.Security.Principal.TokenImpersonationLevel.Anonymous);
        var utf8 = new UTF8Encoding(false, true);
        reader = new StreamReader(pipe, utf8, false, 4096, leaveOpen: true);
        writer = new StreamWriter(pipe, utf8, 4096, leaveOpen: true) { NewLine = "\n" };
    }

    public async Task ConnectAsync()
    {
        await pipe.ConnectAsync(30000, lifetime);
        writer.AutoFlush = true;
    }

    public async Task<string> ExchangeRawAsync(object request)
    {
        using var timeout = CancellationTokenSource.CreateLinkedTokenSource(lifetime);
        timeout.CancelAfter(TimeSpan.FromSeconds(15));
        await writer.WriteLineAsync(JsonSerializer.Serialize(request).AsMemory(), timeout.Token);
        return await reader.ReadLineAsync(timeout.Token) ?? throw new EndOfStreamException("Native core closed the pipe before replying.");
    }

    public async Task<JsonElement> ExchangeAsync(object request)
    {
        using var response = JsonDocument.Parse(await ExchangeRawAsync(request));
        return response.RootElement.Clone();
    }

    public async Task<JsonElement> CallAsync(object request)
    {
        var response = await ExchangeAsync(request);
        if (!response.GetProperty("ok").GetBoolean())
            throw new InvalidOperationException($"Native request {JsonSerializer.Serialize(request)} failed: {response.GetProperty("error").GetString()}");
        return response.GetProperty("data");
    }

    public async ValueTask DisposeAsync()
    {
        try
        {
            await writer.DisposeAsync();
            reader.Dispose();
        }
        finally
        {
            await pipe.DisposeAsync();
        }
    }
}
