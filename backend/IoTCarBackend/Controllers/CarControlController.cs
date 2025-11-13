using System.ComponentModel.DataAnnotations;
using IoTCarBackend.Hubs;
using IoTCarBackend.Models;
using Microsoft.AspNetCore.Mvc;
using Microsoft.AspNetCore.SignalR;

namespace IoTCarBackend.Controllers;

[Route("api/[controller]")]
[ApiController]
public class CarControlController : ControllerBase
{
    private static readonly HashSet<string> AllowedDirections =
        new(StringComparer.OrdinalIgnoreCase) { "forward", "backward", "left", "right", "stop" };

    private readonly IHubContext<CarHub> _hubContext;

    public CarControlController(IHubContext<CarHub> hubContext)
    {
        _hubContext = hubContext;
    }

    [HttpPost("move")]
    public async Task<IActionResult> Move([FromBody] MoveRequest request)
    {
        if (!AllowedDirections.Contains(request.Direction))
        {
            return BadRequest(new { Message = $"Nepoznat smjer: {request.Direction}" });
        }

        var command = new CarCommand { CommandType = "move", Value = request.Direction.ToLowerInvariant() };
        await DispatchCommandAsync(command);
        return Ok(new { Success = true, Message = $"Auto se kreće: {request.Direction}" });
    }

    [HttpPost("lights")]
    public async Task<IActionResult> ControlLights([FromBody] ToggleRequest request)
    {
        var state = request.State.ToLowerInvariant();
        if (state is not ("on" or "off"))
        {
            return BadRequest(new { Message = "Stanje svjetala mora biti 'on' ili 'off'." });
        }

        var command = new CarCommand { CommandType = "lights", Value = state };
        await DispatchCommandAsync(command);
        return Ok(new { Success = true, Message = $"Svjetla: {state}" });
    }

    [HttpPost("horn")]
    public async Task<IActionResult> ActivateHorn([FromBody] HornRequest request)
    {
        var duration = request.Duration is > 0 ? request.Duration.Value : 1000;
        var command = new CarCommand { CommandType = "horn", Value = "on", Duration = duration };
        await DispatchCommandAsync(command);
        return Ok(new { Success = true, Message = "Sirena aktivirana" });
    }

    private Task DispatchCommandAsync(CarCommand command)
    {
        return _hubContext.Clients.Group(SignalRGroups.Esp32Devices).SendAsync("ReceiveCommand", command);
    }
}

public sealed record MoveRequest([property: Required] string Direction);

public sealed record ToggleRequest([property: Required] string State);

public sealed record HornRequest(int? Duration);
