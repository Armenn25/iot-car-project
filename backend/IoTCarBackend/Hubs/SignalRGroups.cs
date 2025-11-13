namespace IoTCarBackend.Hubs;

/// <summary>
/// Centralized collection of SignalR group names so controllers and hubs stay in sync.
/// </summary>
public static class SignalRGroups
{
    public const string Esp32Devices = "esp32-devices";
    public const string FrontendClients = "frontend-clients";
}
