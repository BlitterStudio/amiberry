using Microsoft.UI.Xaml;
using System.Runtime.InteropServices;

namespace Amiberry.Windows;

public partial class App : Application
{
    private MainWindow? window;
    public App() => InitializeComponent();

    protected override void OnLaunched(LaunchActivatedEventArgs args)
    {
        Marshal.ThrowExceptionForHR(SetCurrentProcessExplicitAppUserModelID("BlitterStudio.Amiberry.Native"));
        string[] arguments = Environment.GetCommandLineArgs();
        int position = Array.IndexOf(arguments, "--pipe");
        string? pipe = position >= 0 && position + 1 < arguments.Length ? arguments[position + 1] : null;
        window = new MainWindow(pipe);
        window.Activate();
    }
    [DllImport("shell32.dll", CharSet = CharSet.Unicode)]
    private static extern int SetCurrentProcessExplicitAppUserModelID(string applicationId);
}
