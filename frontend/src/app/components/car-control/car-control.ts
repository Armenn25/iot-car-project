import { Component } from '@angular/core';
import { MatButtonModule } from '@angular/material/button';
import { MatIconModule } from '@angular/material/icon';
import { CommonModule } from '@angular/common';


@Component({
  selector: 'app-car-control',
  standalone: true, // standlone
  imports: [MatButtonModule, MatIconModule,CommonModule],
  templateUrl: './car-control.html',
  styleUrl: './car-control.css'
})
export class CarControlComponent {
  isConnected = false;
  batteryPercent = 85;
  speed = 0;
  rpm = 0;
  temperature = 28;
  current = 0.0;
  lightsOn = false;
  hornActive = false;

  onForward()      { console.log('Forward'); }
  onBackward()     { console.log('Backward'); }
  onLeft()         { console.log('Left'); }
  onRight()        { console.log('Right'); }
  onStop()         { console.log('Stop'); }
  onHorn()         {
    this.hornActive = true;
    setTimeout(() => this.hornActive = false, 500);
  }
  onLightsToggle() { this.lightsOn = !this.lightsOn; }
}
