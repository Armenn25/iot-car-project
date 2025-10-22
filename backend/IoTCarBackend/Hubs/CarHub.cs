using Microsoft.AspNetCore.SignalR;
using IoTCarBackend.Models;
using System.Data;
using Microsoft.Extensions.Logging;

namespace IoTCarBackend.Hubs
{
    public class CarHub : Hub
    {
        private readonly ILogger<CarHub> _logger;

        // Konstruktor DI za ILogger
        public CarHub(ILogger<CarHub> logger)
        {
            _logger = logger;
        }

        public async Task SendCommandToESP32(CarCommand command)
        {
            await Clients.Group("esp32-devices").SendAsync("ReceiveCommand", command);
            await Clients.Caller.SendAsync("CommandConfirmed", true);
            _logger.LogInformation($"Komanda primljena: {command.CommandType}, vrijednost: {command.Value}");
        }

        public async Task SendTelemetryData(TelemetryData telemetry)
        {
            await Clients.Group("frontend-clients").SendAsync("ReceiveTelemetry", telemetry);
        }

        public async Task RegisterESP32(string deviceId)
        {
            await Groups.AddToGroupAsync(Context.ConnectionId, "esp32-devices");
        }

        public async Task RegisterFrontend(string userId)
        {
            await Groups.AddToGroupAsync(Context.ConnectionId, "frontend-clients");
        }
    }
}
