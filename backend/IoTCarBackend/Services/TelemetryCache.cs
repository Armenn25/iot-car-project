using System.Threading;
using IoTCarBackend.Models;

namespace IoTCarBackend.Services;

/// <summary>
/// Simple in-memory implementation that keeps the most recent telemetry payload.
/// </summary>
public sealed class TelemetryCache : ITelemetryCache
{
    private TelemetryData? _latest;

    public TelemetryData? GetLatest()
    {
        return Volatile.Read(ref _latest);
    }

    public void Update(TelemetryData telemetry)
    {
        ArgumentNullException.ThrowIfNull(telemetry);
        Volatile.Write(ref _latest, telemetry);
    }
}
