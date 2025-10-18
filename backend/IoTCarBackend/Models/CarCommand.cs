namespace IoTCarBackend.Models
{
    public class CarCommand
    {
        public string CommandType { get; set; } = string.Empty;
        public string Value { get; set; } = string.Empty;
        public int? Duration { get; set; }
        public DateTime CreatedAt { get; set; } = DateTime.UtcNow;
    }
}
