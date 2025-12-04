import { Component, OnInit } from '@angular/core';
import { MatButtonModule } from '@angular/material/button';
import { MatIconModule } from '@angular/material/icon';
import { CommonModule } from '@angular/common';

// Dodaj import servisa
import { SignalrService, TelemetryData } from '../../services/signalr';

@Component({
  selector: 'app-car-control',
  standalone: true,
  imports: [MatButtonModule, MatIconModule, CommonModule],
  templateUrl: './car-control.html',
  styleUrl: './car-control.css'
})
export class CarControlComponent implements OnInit {
  // State za prikaz u UI (preuzima se iz SignalR servisa)
  isConnected = false;
  batteryPercent = 0;
  speed = 0;
  rpm = 0;
  current = 0.0;
  lightsOn = false;
  hornActive = false;

  constructor(private signalR: SignalrService) {}

  ngOnInit(): void {
    // Povezivanje na SignalR hub
    this.signalR.startConnection();

    // Subscribe na telemetry podatke
    this.signalR.telemetry$.subscribe((data: TelemetryData | null) => {
      if (data) {
        this.batteryPercent = data.batteryLevel;
        this.current = data.currentConsumption;
        this.speed = data.speed;
        this.rpm = data.motorRpm;
        // timestamp ti je dostupan u data.timestamp
      }
    });

    // Subscribe na status konekcije
    this.signalR.connectionStatus$.subscribe((status: boolean) => {
      this.isConnected = status;
    });
  }

  onForward() {
    this.sendCarCommand('move', 'forward');
  }

  onBackward() {
    this.sendCarCommand('move', 'backward');
  }

  onLeft() {
    this.sendCarCommand('move', 'left');
  }

  onRight() {
    this.sendCarCommand('move', 'right');
  }

  onStop() {
    this.sendCarCommand('move', 'stop');
  }

  onHorn() {
    this.hornActive = true;
    this.sendCarCommand('horn', 'on');
    setTimeout(() => {
      this.hornActive = false;
      this.sendCarCommand('horn', 'off');
    }, 500);
  }

  onLightsToggle() {
    this.lightsOn = !this.lightsOn;
    this.sendCarCommand('light', this.lightsOn ? 'on' : 'off');
  }

  // Helper za slanje komandi backendu
  private sendCarCommand(commandType: string, value: string) {
    this.signalR.sendCommand({
      commandType,
      value,
      createdAt: new Date()
    });
  }
}
