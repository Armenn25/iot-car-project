<p align="center">
  <!-- Zamijenite 'URL_DO_VAŠEG_LOGA' sa stvarnim URL-om do loga vašeg projekta. -->
  <!-- Možete uploadati logo u vaš repozitorij i koristiti link do njega. -->
  <img src="assets\iot_readme.png" alt="IoT Car Project Logo" width="200"/>
</p>

<h1 align="center">IoT Car Project</h1>

<p align="center">
  Sveobuhvatni IoT projekat automobila sa telemetrijom u realnom vremenu i daljinskim upravljanjem.
</p>

<p align="center">
  <!-- Bedževi za status projekta. Automatski se ažuriraju. -->
<img src="https://img.shields.io/badge/.NET-v8.0-blue" alt=".NET Verzija">
<img src="https://img.shields.io/badge/Angular-v17-red" alt="Angular Verzija">
<img src="https://img.shields.io/badge/License-MIT-yellow" alt="Licenca">
<img src="https://img.shields.io/badge/Backend-.NET_Core-blue" alt="Backend tehnologija">
<img src="https://img.shields.io/badge/Frontend-Angular-red" alt="Frontend tehnologija">

</p>

---

## 📖 O Projektu

Ovaj projekat je **kompletno rješenje za "Internet of Things" (IoT) automobil**, dizajnirano da omogući napredno daljinsko upravljanje i praćenje performansi putem interneta. Osnovna ideja je transformacija jednostavnog modela automobila u pametno vozilo koje šalje telemetrijske podatke na web server i prima komande sa bilo kojeg uređaja sa pristupom internetu.

Projekat demonstrira **full-stack razvoj**, spajajući hardver, backend logiku i frontend interfejs u jedan funkcionalan sistem. Idealan je za učenje i demonstraciju vještina u oblastima embedded sistema, razvoja web API-ja i modernih frontend tehnologija.

## 🏛️ Arhitektura Projekta

Sistem je dizajniran po troslojnoj arhitekturi koja osigurava modularnost, skalabilnost i lako održavanje. Svaka komponenta ima jasno definisanu ulogu.

### 🚗 1. Embedded Sistem (ESP32 / Arduino)

"Srce" automobila je **ESP-WROOM-32 mikrokontroler**, programiran koristeći **Arduino framework (C++)**. Ova komponenta je zadužena za sve fizičke operacije i interakciju sa senzorima.

*   **Upravljanje motorima:** Koristi **L298N drajver** za kontrolu dva DC motora. Brzina i smjer se regulišu pomoću **PWM (Pulse Width Modulation)** signala, što omogućava preciznu kontrolu kretanja.
*   **Prikupljanje telemetrije:** Očitava podatke sa različitih senzora, kao što su:
    *   **Senzori brzine (SS49R - Hall senzori):** Mjere rotaciju točkova za precizno određivanje brzine.
    *   **Senzor napona (ACS712 - 05A):** Prati stanje baterije kako bi se spriječilo prekomjerno pražnjenje.
*   **Komunikacija:** Povezuje se na Wi-Fi mrežu i uspostavlja **WebSocket konekciju** sa .NET backendom. Kroz ovu konekciju, u realnom vremenu šalje JSON objekte sa telemetrijskim podacima i prima komande za upravljanje.

### ☁️ 2. Backend Server (.NET Core)

Backend je izgrađen kao **ASP.NET Core Web API** i služi kao centralni komunikacijski hub. Njegova uloga je da posreduje između hardvera (automobila) i korisničkog interfejsa (web aplikacije).

*   **Real-time komunikacija:** Koristi **SignalR**, biblioteku koja olakšava dodavanje WebSockets funkcionalnosti. Kreiran je `CarHub` koji upravlja konekcijama.
*   **Obrada komandi:** Kada korisnik na frontend aplikaciji pritisne dugme (npr. "naprijed"), komanda se šalje backendu. Backend je prosljeđuje direktno odgovarajućem ESP32 klijentu (automobilu).
*   **Distribucija telemetrije:** Prima telemetrijske podatke od automobila i odmah ih emituje svim povezanim frontend klijentima, osiguravajući da svi korisnici vide ažurirane informacije u realnom vremenu.

### 💻 3. Frontend Aplikacija (Angular)

Frontend je **Single Page Application (SPA)** napravljena u **Angularu**. Pruža bogat i interaktivan korisnički interfejs za interakciju sa sistemom.

*   **Vizualizacija podataka:** Prikazuje telemetrijske podatke primljene sa servera koristeći komponente kao što su brzinomjeri (gauges), grafikoni i numerički prikazi.
*   **Korisničke kontrole:** Sadrži dugmad, slajdere ili virtualni džojstik koji omogućavaju korisniku da šalje komande za kretanje automobila.
*   **SignalR klijent:** Povezuje se na SignalR hub na backendu kako bi ostvarila dvosmjernu komunikaciju, primala telemetriju i slala komande.

## 🛠️ Korištene Tehnologije

Projekat je izgrađen koristeći moderne i provjerene tehnologije kako bi se osigurale visoke performanse, skalabilnost i pouzdanost sistema [web:4].

| Komponenta      | Tehnologija                               |
| :-------------- | :---------------------------------------- |
| **Embedded**    | C++, Arduino Framework, ESP32             |
| **Backend**     | C#, .NET 8, ASP.NET Core Web API, SignalR |
| **Frontend**    | TypeScript, Angular 17, HTML, SCSS        |
| **Deployment**  | Docker, Nginx, Microsoft Azure            |

## ✨ Funkcionalnosti

*   **🕹️ Daljinsko upravljanje:** Potpuna kontrola kretanja automobila (naprijed, nazad, lijevo, desno) putem web interfejsa.
*   **📊 Telemetrija u realnom vremenu:** Praćenje brzine, temperature i napona baterije sa vizualizacijom uživo.
*   **📶 Wi-Fi Menadžment:** Integrisana funkcionalnost za skeniranje dostupnih Wi-Fi mreža i povezivanje na odabranu mrežu.
*   **🔒 Sigurna komunikacija:** Implementirana enkripcija podataka korištenjem HTTPS i WSS (Secure WebSockets) protokola.

## 🚀 Postavljanje Projekta

Da biste pokrenuli projekat lokalno i započeli sa testiranjem, pratite sljedeće korake.

### Preduvjeti

*   [.NET 8 SDK](https://dotnet.microsoft.com/download/dotnet/8.0)
*   [Node.js i npm](https://nodejs.org/)
*   [Angular CLI](https://angular.io/cli)
*   [Visual Studio 2022](https://visualstudio.microsoft.com/) ili [VS Code](https://code.visualstudio.com/)
*   [PlatformIO](https://platformio.org/) za VS Code (preporučeno za embedded dio)

### 1. Kloniranje Repozitorija

git clone https://github.com/Armenn25/iot-car-project.git
cd iot-car-project

### 2. Backend (.NET)

1.  Otvorite `backend` folder u Visual Studio-u ili VS Code-u.
2.  Vratite sve potrebne `NuGet` pakete:
    ```
    dotnet restore
    ```
3.  Pokrenite aplikaciju (API će biti dostupan na `https://localhost:5001`):
    ```
    dotnet run
    ```

### 3. Frontend (Angular)

1.  U novom terminalu, pozicionirajte se u `frontend` folder.
2.  Instalirajte sve `npm` pakete:
    ```
    npm install
    ```
3.  Pokrenite razvojni server (aplikacija će biti dostupna na `http://localhost:4200/`):
    ```
    ng serve
    ```

### 4. Embedded (ESP32)

1.  Otvorite `arduino` folder koristeći PlatformIO u VS Code-u ili putem Arduino IDE.
2.  Unutar koda, podesite URL backend servera.
3.  Spojite ESP32 na računar i upload-ujte kod.

## 📜 Licenca

Ovaj projekat je licenciran pod **MIT licencom**. Za više detalja, pogledajte [LICENSE](LICENSE) fajl u repozitoriju.

