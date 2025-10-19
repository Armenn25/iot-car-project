import { Injectable } from '@angular/core';
import * as signalR from '@microsoft/signalr';
import { BehaviorSubject, Observable } from 'rxjs';
import { environment } from '../environment';

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

@Injectable({ providedIn: 'root' })
export class SignalrService {

  private hubConnection!: signalR.HubConnection;
  private telemetrySubject = new BehaviorSubject<TelemetryData | null>(null);
  public telemetry$: Observable<TelemetryData | null> = this.telemetrySubject.asObservable();
  private connectionStatusSubject = new BehaviorSubject<boolean>(false);
  public connectionStatus$: Observable<boolean> = this.connectionStatusSubject.asObservable();

  constructor() { }

  // Pokreće konekciju prema backendu, koristi URL iz environment-a
  public startConnection(): Promise<void> {
    this.hubConnection = new signalR.HubConnectionBuilder()
      .withUrl(environment.signalRHub)
      .withAutomaticReconnect()
      .configureLogging(signalR.LogLevel.Information)
      .build();

    this.registerSignalREvents();

    return this.hubConnection
      .start()
      .then(() => {
        console.log('✅ SignalR konekcija uspješna!');
        this.connectionStatusSubject.next(true);
        this.hubConnection.invoke('RegisterFrontend', 'angular-user-' + Date.now())
          .catch(err => console.error('❌ Greška pri registraciji frontend-a:', err));
      })
      .catch(err => {
        console.error('❌ SignalR konekcija neuspješna:', err);
        this.connectionStatusSubject.next(false);
      });
  }

  private registerSignalREvents(): void {
    this.hubConnection.on('ReceiveTelemetry', (data: TelemetryData) => {
      console.log('📊 Telemetrija primljena:', data);
      this.telemetrySubject.next(data);
    });

    this.hubConnection.on('CommandConfirmed', (confirmed: boolean) => {
      console.log('✅ Komanda potvrđena:', confirmed);
    });

    this.hubConnection.on('ESP32Connected', (deviceId: string) => {
      console.log('🚗 ESP32 konektovan:', deviceId);
    });

    this.hubConnection.onclose(() => {
      console.log('⚠️ SignalR konekcija zatvorena');
      this.connectionStatusSubject.next(false);
    });

    this.hubConnection.onreconnecting(() => {
      console.log('🔄 Reconnecting...');
      this.connectionStatusSubject.next(false);
    });

    this.hubConnection.onreconnected(() => {
      console.log('✅ Reconnected!');
      this.connectionStatusSubject.next(true);
    });
  }

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

  public stopConnection(): Promise<void> {
    if (this.hubConnection) {
      return this.hubConnection.stop();
    }
    return Promise.resolve();
  }
}
