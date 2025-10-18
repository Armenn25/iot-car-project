using Microsoft.AspNetCore.Mvc;
using Microsoft.AspNetCore.SignalR;
using IoTCarBackend.Hubs;
using IoTCarBackend.Models;

namespace IoTCarBackend.Controllers
{
    [Route("api/[controller]")]
    [ApiController]
    public class CarControlController : ControllerBase
    {
        private readonly IHubContext<CarHub> _hubContext;

        public CarControlController(IHubContext<CarHub> hubContext)
        {
            _hubContext = hubContext;
        }

        [HttpPost("move")]
        public async Task<IActionResult> Move([FromBody] string direction)
        {
            var command = new CarCommand { CommandType = "move", Value = direction.ToLower() };
            await _hubContext.Clients.Group("esp32-devices").SendAsync("ReceiveCommand", command);
            return Ok(new { Success = true, Message = $"Auto se kreće: {direction}" });
        }

        [HttpPost("lights")]
        public async Task<IActionResult> ControlLights([FromBody] string state)
        {
            var command = new CarCommand { CommandType = "lights", Value = state.ToLower() };
            await _hubContext.Clients.Group("esp32-devices").SendAsync("ReceiveCommand", command);
            return Ok(new { Success = true, Message = $"Svjetla: {state}" });
        }

        [HttpPost("horn")]
        public async Task<IActionResult> ActivateHorn([FromBody] int? duration)
        {
            var command = new CarCommand { CommandType = "horn", Value = "on", Duration = duration ?? 1000 };
            await _hubContext.Clients.Group("esp32-devices").SendAsync("ReceiveCommand", command);
            return Ok(new { Success = true, Message = "Svirena aktivirana" });
        }
    }
}
