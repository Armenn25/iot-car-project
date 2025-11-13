using IoTCarBackend.Models;
using IoTCarBackend.Services;
using Microsoft.AspNetCore.SignalR;

namespace IoTCarBackend.Hubs;

public class CarHub : Hub
{
    private readonly ILogger<CarHub> _logger;
    private readonly ITelemetryCache _telemetryCache;

    public CarHub(ILogger<CarHub> logger, ITelemetryCache telemetryCache)
    {
        _logger = logger;
        _telemetryCache = telemetryCache;
    }

    public override async Task OnConnectedAsync()
    {
        _logger.LogInformation("Client {ConnectionId} connected to CarHub", Context.ConnectionId);
        await base.OnConnectedAsync();
    }

    public override async Task OnDisconnectedAsync(Exception? exception)
    {
        _logger.LogInformation("Client {ConnectionId} disconnected from CarHub", Context.ConnectionId);
        await base.OnDisconnectedAsync(exception);
    }

    public async Task SendCommandToESP32(CarCommand command)
    {
        ArgumentNullException.ThrowIfNull(command);

        await Clients.Group(SignalRGroups.Esp32Devices).SendAsync("ReceiveCommand", command);
        await Clients.Caller.SendAsync("CommandConfirmed", true);
        _logger.LogInformation("Command dispatched: {CommandType} - {Value}", command.CommandType, command.Value);
    }

    public async Task SendTelemetryData(TelemetryData telemetry)
    {
        ArgumentNullException.ThrowIfNull(telemetry);

        _telemetryCache.Update(telemetry);
        await Clients.Group(SignalRGroups.FrontendClients).SendAsync("ReceiveTelemetry", telemetry);
        _logger.LogDebug("Telemetry propagated at {Timestamp}", telemetry.Timestamp);
    }

    public async Task RegisterESP32(string deviceId)
    {
        await Groups.AddToGroupAsync(Context.ConnectionId, SignalRGroups.Esp32Devices);
        await Clients.Caller.SendAsync("ESP32Connected", deviceId);
        _logger.LogInformation("ESP32 registered: {DeviceId}", deviceId);
    }

    public async Task RegisterFrontend(string userId)
    {
        await Groups.AddToGroupAsync(Context.ConnectionId, SignalRGroups.FrontendClients);
        _logger.LogInformation("Frontend client registered: {UserId}", userId);

        var snapshot = _telemetryCache.GetLatest();
        if (snapshot is not null)
        {
            await Clients.Caller.SendAsync("ReceiveTelemetry", snapshot);
        }
    }
}
