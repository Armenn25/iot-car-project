using Microsoft.AspNetCore.SignalR;
using IoTCarBackend.Models;

namespace IoTCarBackend.Hubs
{
    public class CarHub : Hub
    {
        public async Task SendCommandToESP32(CarCommand command)
        {
            await Clients.Group("esp32-devices").SendAsync("ReceiveCommand", command);
            await Clients.Caller.SendAsync("CommandConfirmed", true);
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
