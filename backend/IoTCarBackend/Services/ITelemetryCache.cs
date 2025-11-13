using IoTCarBackend.Models;

namespace IoTCarBackend.Services;

/// <summary>
/// Provides a thread-safe cache for the most recent telemetry payload sent by the car.
/// </summary>
public interface ITelemetryCache
{
    /// <summary>
    /// Returns the last telemetry snapshot or <c>null</c> if no telemetry was received yet.
    /// </summary>
    TelemetryData? GetLatest();

    /// <summary>
    /// Persists a telemetry snapshot for later retrieval.
    /// </summary>
    /// <param name="telemetry">Telemetry payload received from the vehicle.</param>
    void Update(TelemetryData telemetry);
}
