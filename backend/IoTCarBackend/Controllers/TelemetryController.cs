using IoTCarBackend.Hubs;
using IoTCarBackend.Models;
using IoTCarBackend.Services;
using Microsoft.AspNetCore.Mvc;
using Microsoft.AspNetCore.SignalR;

namespace IoTCarBackend.Controllers;

[Route("api/[controller]")]
[ApiController]
public class TelemetryController : ControllerBase
{
    private readonly ITelemetryCache _telemetryCache;
    private readonly IHubContext<CarHub> _hubContext;
    private readonly ILogger<TelemetryController> _logger;

    public TelemetryController(ITelemetryCache telemetryCache, IHubContext<CarHub> hubContext, ILogger<TelemetryController> logger)
    {
        _telemetryCache = telemetryCache;
        _hubContext = hubContext;
        _logger = logger;
    }

    [HttpGet("current")]
    public IActionResult GetCurrentTelemetry()
    {
        var telemetry = _telemetryCache.GetLatest();
        if (telemetry is null)
        {
            return NotFound(new { Message = "Nema dostupnih telemetry podataka" });
        }

        return Ok(telemetry);
    }

    [HttpPost("update")]
    public async Task<IActionResult> UpdateTelemetry([FromBody] TelemetryData telemetry)
    {
        _telemetryCache.Update(telemetry);
        await _hubContext.Clients.Group(SignalRGroups.FrontendClients).SendAsync("ReceiveTelemetry", telemetry);
        _logger.LogInformation("Telemetrija ažurirana u {Timestamp}", telemetry.Timestamp);
        return Ok(new { Success = true, Message = "Telemetrija uspješno ažurirana" });
    }
}
