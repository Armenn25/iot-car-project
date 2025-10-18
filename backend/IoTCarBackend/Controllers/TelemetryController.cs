using Microsoft.AspNetCore.Mvc;
using IoTCarBackend.Models;

namespace IoTCarBackend.Controllers
{
    [Route("api/[controller]")]
    [ApiController]
    public class TelemetryController : ControllerBase
    {
        private static TelemetryData? _latestTelemetry;

        [HttpGet("current")]
        public IActionResult GetCurrentTelemetry()
        {
            if (_latestTelemetry == null)
                return NotFound(new { Message = "Nema dostupnih telemetry podataka" });
            return Ok(_latestTelemetry);
        }

        [HttpPost("update")]
        public IActionResult UpdateTelemetry([FromBody] TelemetryData telemetry)
        {
            _latestTelemetry = telemetry;
            return Ok(new { Success = true, Message = "Telemetrija uspješno ažurirana" });
        }
    }
}
