using System.ComponentModel.DataAnnotations;

namespace IoTCarBackend.Models;

public class TelemetryData
{
    [Range(0, 100, ErrorMessage = "Nivo baterije mora biti između 0 i 100%.")]
    public double BatteryLevel { get; set; }

    [Range(0, double.MaxValue, ErrorMessage = "Potrošnja struje mora biti pozitivna vrijednost.")]
    public double CurrentConsumption { get; set; }

    [Range(0, 200, ErrorMessage = "Brzina mora biti u okviru očekivanog raspona.")]
    public double Speed { get; set; }

    [Range(0, 20000, ErrorMessage = "Motor RPM mora biti pozitivan.")]
    public int MotorRpm { get; set; }

    public DateTime Timestamp { get; set; } = DateTime.UtcNow;
}
