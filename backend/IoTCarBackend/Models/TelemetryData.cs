namespace IoTCarBackend.Models
{
    public class TelemetryData
    {
        public double BatteryLevel { get; set; }
        public double CurrentConsumption { get; set; }
        public double Speed { get; set; }
        public int MotorRpm { get; set; }
        public DateTime Timestamp { get; set; } = DateTime.UtcNow;
    }
}
