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

    private static bool SameFolder(string actual, string expected) => SamePath(actual.TrimEnd('\\', '/'), expected);

    private static Dictionary<string, string?> Values(JsonElement snapshot) => snapshot.GetProperty("values")
        .EnumerateArray().ToDictionary(item => item.GetProperty("id").GetString()!, item => item.GetProperty("value").GetString());

    public static async Task<int> Main(string[] args)
    {
        string? reportPath = Environment.GetEnvironmentVariable("AMIBERRY_NATIVE_TEST_REPORT");
        string? failure = null;
        PipeClient? client = null;
        using var lifetime = new CancellationTokenSource(TimeSpan.FromSeconds(240));
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

            // The core already started with a saved WHDBoot folder named outside the ANSI code page inside its root:
            // startup must survive the folder scans, seed the bundled files into that folder and nowhere else.
            string startupWhd = Environment.GetEnvironmentVariable("AMIBERRY_NATIVE_TEST_STARTUP_WHD")
                ?? throw new InvalidOperationException("AMIBERRY_NATIVE_TEST_STARTUP_WHD must name the startup WHDBoot folder.");
            var startupHost = await client.CallAsync(new { op = "host" });
            CheckPath(SameFolder(Value(startupHost, "whdboot_path"), startupWhd), "the saved Unicode WHDBoot folder is loaded at startup",
                Value(startupHost, "whdboot_path"), startupWhd);
            var startupStatus = await client.CallAsync(new { op = "whdboot_status" });
            Check(startupStatus.GetProperty("hostQuit").GetBoolean() && startupStatus.GetProperty("bootData").GetBoolean() &&
                startupStatus.GetProperty("database").GetBoolean(), "startup seeded the bundled WHDBoot files into the Unicode folder");
            Check(File.Exists(Path.Combine(startupWhd, "host-quit")), "the seeded host-quit is in the Unicode folder");
            string[] startupEntries = Directory.GetDirectories(Path.GetFullPath(Path.Combine(AppContext.BaseDirectory, "..")), "startup-*")
                .Select(Path.GetFileName).OfType<string>().ToArray();
            CheckPath(startupEntries.Length == 1, "startup created no misread-name sibling of the Unicode WHDBoot folder",
                string.Join(", ", startupEntries), "only the Unicode startup- folder");

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

            // A reset-class edit raises the apply class; restoring the original value drops it again, so a running
            // machine's editor does not demand a reset for an edit that no longer differs from the saved state.
            string slowOriginal = Value(snapshot, "bogomem_size");
            string? slowOther = snapshot.GetProperty("values").EnumerateArray()
                .Single(item => item.GetProperty("id").GetString() == "bogomem_size").GetProperty("choices").EnumerateArray()
                .Where(choice => choice.GetProperty("available").GetBoolean())
                .Select(choice => choice.GetProperty("value").GetString()).FirstOrDefault(choice => choice != slowOriginal);
            Check(slowOther is not null, "Slow RAM offers a second available value");
            Check(snapshot.GetProperty("apply").GetInt32() == 0 && !snapshot.GetProperty("changed").GetBoolean(), "reopened configuration starts clean");
            snapshot = await client.CallAsync(new { op = "set", id = "bogomem_size", value = slowOther });
            Check(snapshot.GetProperty("changed").GetBoolean() && snapshot.GetProperty("apply").GetInt32() >= 1, "changing Slow RAM requires a reset");
            snapshot = await client.CallAsync(new { op = "set", id = "bogomem_size", value = slowOriginal });
            Check(!snapshot.GetProperty("changed").GetBoolean() && snapshot.GetProperty("apply").GetInt32() == 0, "restoring Slow RAM drops the apply class");

            // One configuration the core cannot read (here: held open exclusively by another process) must not make
            // the whole Library request fail; the machine is listed without metadata and the others stay available.
            string locked = Path.Combine(configurations, "locked-\u00e9.uae");
            await File.WriteAllTextAsync(locked, "config_description=locked\ncpu_model=68000\n", new UTF8Encoding(false));
            using (var hold = new FileStream(locked, FileMode.Open, FileAccess.ReadWrite, FileShare.None))
            {
                var listing = await client.ExchangeAsync(new { op = "library" });
                Check(listing.GetProperty("ok").GetBoolean(), "Library request survives a locked configuration");
                var machines = listing.GetProperty("data").GetProperty("machines").EnumerateArray().ToArray();
                Check(machines.Any(machine => SamePath(machine.GetProperty("path").GetString()!, locked) &&
                    machine.GetProperty("description").GetString() == ""), "locked configuration is listed without metadata");
                Check(machines.Any(machine => SamePath(machine.GetProperty("path").GetString()!, unicodeSaved)),
                    "other machines stay listed beside a locked configuration");
            }
            File.Delete(locked);

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

            // The WHDBoot downloader into a folder whose name is outside the ANSI code page, through the real
            // downloader, network and archive reader. Narrow CRT calls would read the UTF-8 folder name as ANSI and
            // install nothing, or somewhere else.
            string whdFolder = Path.Combine(configurations, "whd-\u6a4b-\u00e4");
            Directory.CreateDirectory(whdFolder);
            string[] entriesBefore = Directory.GetFileSystemEntries(configurations).Select(Path.GetFileName).OfType<string>().ToArray();
            var hostSnapshot = await client.CallAsync(new { op = "host_set", id = "whdboot_path", value = whdFolder });
            CheckPath(SameFolder(Value(hostSnapshot, "whdboot_path"), whdFolder), "Unicode WHDBoot folder is stored exactly",
                Value(hostSnapshot, "whdboot_path"), whdFolder);
            var idle = await client.CallAsync(new { op = "whdboot_status" });
            Check(!idle.GetProperty("whdload").GetBoolean() && !idle.GetProperty("hostQuit").GetBoolean() &&
                !idle.GetProperty("libraries").GetBoolean(), "status reads the empty Unicode WHDBoot folder as having no tools");

            // Stale or corrupt copies of files the download replaces must end up holding the downloaded bytes.
            byte[] stale = Encoding.ASCII.GetBytes("stale copy, not the downloaded file");
            byte[] staleDatabase = Encoding.ASCII.GetBytes("{\"stale\":true}");
            Directory.CreateDirectory(Path.Combine(whdFolder, "game-data"));
            foreach (var name in new[] { "host-quit", "boot-data.zip", "WHDLoad" })
                File.WriteAllBytes(Path.Combine(whdFolder, name), stale);
            File.WriteAllBytes(Path.Combine(whdFolder, "game-data", "whdload_db.json"), staleDatabase);

            async Task<JsonElement> WaitForDownload()
            {
                var deadline = DateTime.UtcNow.AddSeconds(150);
                while (true)
                {
                    var status = await client.CallAsync(new { op = "whdboot_status" });
                    if (!status.GetProperty("downloading").GetBoolean()) return status;
                    if (DateTime.UtcNow > deadline) throw new TimeoutException("The WHDBoot download did not finish within 150 seconds.");
                    await Task.Delay(250);
                }
            }

            var started = await client.CallAsync(new { op = "whdboot_download" });
            Check(started.GetProperty("downloading").GetBoolean(), "WHDBoot download into the Unicode folder starts");

            // A running download reads these settings throughout, so changing them is refused until it ends.
            string baseFolder = Path.Combine(configurations, "base-\u6a4b-\u00e4");
            Directory.CreateDirectory(baseFolder);
            var refusedBase = await client.ExchangeAsync(new { op = "host_set", id = "base_content_path", value = baseFolder });
            var whileRunning = await client.CallAsync(new { op = "whdboot_status" });
            Check(whileRunning.GetProperty("downloading").GetBoolean(), "the download was still running when the base content change was sent");
            Check(!refusedBase.GetProperty("ok").GetBoolean(), "base content change is refused while a WHDBoot download runs");
            var refusedWhd = await client.ExchangeAsync(new { op = "host_set", id = "whdboot_path", value = baseFolder });
            Check(!refusedWhd.GetProperty("ok").GetBoolean(), "WHDBoot folder change is refused while a WHDBoot download runs");

            JsonElement finished = await WaitForDownload();
            Check(finished.GetProperty("result").GetInt32() == 0, "WHDBoot download into the Unicode folder succeeds without failures");
            foreach (var flag in new[] { "whdload", "hostQuit", "bootData", "database", "libraries" })
                Check(finished.GetProperty(flag).GetBoolean(), $"status reads the downloaded {flag} from the Unicode folder");
            Check(!finished.GetProperty("jst").GetBoolean(), "JST is not installed unless selected");

            // Expected bytes: members of the pinned archives, which do not change between downloads.
            string Sha256(string relative) => Convert.ToHexString(System.Security.Cryptography.SHA256.HashData(
                File.ReadAllBytes(Path.Combine(whdFolder, relative.Replace('/', Path.DirectorySeparatorChar)))));
            var expectedHashes = new Dictionary<string, string>
            {
                ["save-data/Kickstarts/kick33180.A500.RTB"] = "703637372F6CFEC06DC802759BF8907C9A01BA259E0FAE5F347D13D4265B0216",
                ["save-data/Kickstarts/kick33192.A500.RTB"] = "703637372F6CFEC06DC802759BF8907C9A01BA259E0FAE5F347D13D4265B0216",
                ["save-data/Kickstarts/kick40068.A4000.RTB"] = "7CEDF931E8B30620BA69954A30F037260325FF57282833FA557B4B0F2A2689F9",
                ["Libs/explode.library"] = "A681BA94E29A791AE280FC8EB4592D704DDFE414CAFC414BEBE87D529559CD9A",
                ["Libs/medplayer.library"] = "F80334D84D9915C4CD8DFFA3AC870807CBFBDB5FD866CFFE0B1F9604C391079E",
                ["Libs/xpkmaster.library"] = "3E709AFEB8506A88FC8A43C05FD056825E0F06BD95CF48F81338733E7D521A39",
                ["Libs/Compressors/xpkSQSH.library"] = "6F399A668B7D136A29F834B425B1C226732A87F28BD35BF0F3A7C6EDCEFA64DA",
            };
            foreach (var (relative, hash) in expectedHashes)
                CheckPath(File.Exists(Path.Combine(whdFolder, relative.Replace('/', Path.DirectorySeparatorChar))) && Sha256(relative) == hash,
                    $"{relative} in the Unicode folder has the expected bytes", relative, hash);
            byte[] whdload = File.ReadAllBytes(Path.Combine(whdFolder, "WHDLoad"));
            Check(whdload.Length > 100000 && whdload[0] == 0 && whdload[1] == 0 && whdload[2] == 3 && whdload[3] == 0xF3,
                "WHDLoad in the Unicode folder is an extracted AmigaOS executable");
            byte[] bootData = File.ReadAllBytes(Path.Combine(whdFolder, "boot-data.zip"));
            Check(bootData.Length > 1000 && bootData[0] == (byte)'P' && bootData[1] == (byte)'K', "boot-data.zip in the Unicode folder is a zip archive");
            byte[] hostQuit = File.ReadAllBytes(Path.Combine(whdFolder, "host-quit"));
            Check(hostQuit.Length > 0, "host-quit in the Unicode folder is not empty");
            Check(!hostQuit.SequenceEqual(stale), "a stale host-quit is replaced by the download");
            // host-quit, boot-data.zip and the database come from mutable upstream URLs: they are checked for being
            // replaced and well-formed, not for exact bytes.
            using (var database = JsonDocument.Parse(File.ReadAllBytes(Path.Combine(whdFolder, "game-data", "whdload_db.json"))))
            {
                Check(database.RootElement.ValueKind == JsonValueKind.Object, "the game database in the Unicode folder is valid JSON");
                Check(!database.RootElement.TryGetProperty("stale", out _), "a stale game database is replaced by the download");
            }
            Check(File.ReadAllBytes(Path.Combine(whdFolder, "game-data", "whdload_db.bak")).SequenceEqual(staleDatabase),
                "the replaced game database is kept as the backup");

            // Nothing is left half-installed, and no folder named by misreading the UTF-8 bytes appeared beside it.
            string[] leftovers = Directory.GetFiles(whdFolder, "*", SearchOption.AllDirectories)
                .Where(file => file.EndsWith(".tmp") || file.EndsWith(".new") || file.EndsWith(".download")).ToArray();
            Check(leftovers.Length == 0, "the download leaves no temporary or scratch files in the Unicode folder");
            string[] entriesAfter = Directory.GetFileSystemEntries(configurations).Select(Path.GetFileName).OfType<string>().ToArray();
            string[] strayEntries = entriesAfter.Except(entriesBefore).Where(name => name != "base-\u6a4b-\u00e4").ToArray();
            CheckPath(strayEntries.Length == 0, "no misread-name sibling of the Unicode folder was created", string.Join(", ", strayEntries), "no new entries");

            // A destination that cannot be replaced (a folder stands in for boot-data.zip) fails the download and keeps
            // what is there; the failed install removes only its own temporary file.
            string blockedZip = Path.Combine(whdFolder, "boot-data.zip");
            File.Delete(blockedZip);
            Directory.CreateDirectory(blockedZip);
            string blockedMarker = Path.Combine(blockedZip, "keep.txt");
            File.WriteAllText(blockedMarker, "keep");
            await client.CallAsync(new { op = "whdboot_download" });
            var blocked = await WaitForDownload();
            Check(blocked.GetProperty("result").GetInt32() == 1, "a download whose destination cannot be replaced reports a partial failure");
            Check(Directory.Exists(blockedZip) && File.ReadAllText(blockedMarker) == "keep", "the blocked destination is left as it was");
            Check(!File.Exists(blockedZip + ".tmp"), "the failed install removes its own temporary file");
            Check(File.ReadAllBytes(Path.Combine(whdFolder, "host-quit")).Length > 0 && File.Exists(Path.Combine(whdFolder, "WHDLoad")),
                "the other files are still installed after the blocked destination failed");

            // Once the download ended, the base content folder can change; the installed tools stay where they were put.
            var changed = await client.CallAsync(new { op = "host_set", id = "base_content_path", value = baseFolder });
            CheckPath(SameFolder(Value(changed, "base_content_path"), baseFolder), "base content change succeeds after the download ended",
                Value(changed, "base_content_path"), baseFolder);
            CheckPath(SameFolder(Value(changed, "whdboot_path"), whdFolder), "the WHDBoot folder keeps the downloader's destination after a base content change",
                Value(changed, "whdboot_path"), whdFolder);
            var kept = await client.CallAsync(new { op = "whdboot_status" });
            Check(kept.GetProperty("whdload").GetBoolean() && kept.GetProperty("libraries").GetBoolean(),
                "status still reads the Unicode folder's tools after the base content change");
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
