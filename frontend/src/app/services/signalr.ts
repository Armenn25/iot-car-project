import { Injectable } from '@angular/core';
import * as signalR from '@microsoft/signalr';
import { BehaviorSubject, Observable } from 'rxjs';

// Interfejsi za tipiziranje podataka
export interface TelemetryData {
  batteryLevel: number;
  currentConsumption: number;
  speed: number;
  motorRpm: number;
  timestamp: Date;
}

export interface CarCommand {
  commandType: string;
  value: string;
  duration?: number;
  createdAt: Date;
}

@Injectable({
  providedIn: 'root'
})
export class SignalrService {
  
  // SignalR hub konekcija
  private hubConnection!: signalR.HubConnection;
  
  // Observable za telemetry podatke koje komponente mogu subscribe-ovati
  private telemetrySubject = new BehaviorSubject<TelemetryData | null>(null);
  public telemetry$: Observable<TelemetryData | null> = this.telemetrySubject.asObservable();
  
  // Observable za status konekcije
  private connectionStatusSubject = new BehaviorSubject<boolean>(false);
  public connectionStatus$: Observable<boolean> = this.connectionStatusSubject.asObservable();

  constructor() { }

  /**
   * Pokreće SignalR konekciju sa backend-om
   * @param hubUrl - URL SignalR hub-a (npr. 'https://localhost:7xxx/carhub')
   */
  public startConnection(hubUrl: string): Promise<void> {
    // Kreiraj hub konekciju
    this.hubConnection = new signalR.HubConnectionBuilder()
      .withUrl(hubUrl) // URL backend SignalR hub-a
      .withAutomaticReconnect() // Automatski reconnect ako padne konekcija
      .configureLogging(signalR.LogLevel.Information) // Logging za debugging
      .build();

    // Registruj event listenere prije nego što se konektuješ
    this.registerSignalREvents();

    // Pokušaj se konektovati
    return this.hubConnection
      .start()
      .then(() => {
        console.log('✅ SignalR konekcija uspješna!');
        this.connectionStatusSubject.next(true);
        
        // Registruj frontend klijenta u grupu
        this.hubConnection.invoke('RegisterFrontend', 'angular-user-' + Date.now())
          .catch(err => console.error('❌ Greška pri registraciji frontend-a:', err));
      })
      .catch(err => {
        console.error('❌ SignalR konekcija neuspješna:', err);
        this.connectionStatusSubject.next(false);
      });
  }

  /**
   * Registruje event listenere za primanje podataka sa backend-a
   */
  private registerSignalREvents(): void {
    // Sluša 'ReceiveTelemetry' event koji backend šalje
    this.hubConnection.on('ReceiveTelemetry', (data: TelemetryData) => {
      console.log('📊 Telemetrija primljena:', data);
      this.telemetrySubject.next(data);
    });

    // Sluša 'CommandConfirmed' event (potvrda da je komanda primljena)
    this.hubConnection.on('CommandConfirmed', (confirmed: boolean) => {
      console.log('✅ Komanda potvrđena:', confirmed);
    });

    // Sluša 'ESP32Connected' event (kada se auto konektuje)
    this.hubConnection.on('ESP32Connected', (deviceId: string) => {
      console.log('🚗 ESP32 konektovan:', deviceId);
    });

    // Handler za disconnection
    this.hubConnection.onclose(() => {
      console.log('⚠️ SignalR konekcija zatvorena');
      this.connectionStatusSubject.next(false);
    });

    // Handler za reconnecting
    this.hubConnection.onreconnecting(() => {
      console.log('🔄 Reconnecting...');
      this.connectionStatusSubject.next(false);
    });

    // Handler za reconnected
    this.hubConnection.onreconnected(() => {
      console.log('✅ Reconnected!');
      this.connectionStatusSubject.next(true);
    });
  }

  /**
   * Šalje komandu backendu preko SignalR hub-a
   * @param command - CarCommand objekat sa podacima o komandi
   */
  public sendCommand(command: CarCommand): Promise<void> {
    if (this.hubConnection.state === signalR.HubConnectionState.Connected) {
      return this.hubConnection
        .invoke('SendCommandToESP32', command)
        .catch(err => console.error('❌ Greška pri slanju komande:', err));
    } else {
      console.error('❌ SignalR nije konektovan!');
      return Promise.reject('SignalR not connected');
    }
  }

  /**
   * Zaustavlja SignalR konekciju
   */
  public stopConnection(): Promise<void> {
    if (this.hubConnection) {
      return this.hubConnection.stop();
    }
    return Promise.resolve();
  }
}
