import { Component } from '@angular/core';
import { CarControlComponent } from './components/car-control/car-control';

@Component({
  selector: 'app-root',
  standalone: true,
  imports: [CarControlComponent],
  templateUrl: './app.html',
  styleUrl: './app.css'
})
export class AppComponent {}
