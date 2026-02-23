# Universitatea Tehnică a Moldovei
## Facultatea Calculatoare, Informatică și Microelectronică
### Departamentul Ingineria Software și Automatică

---

# RAPORT
## la Lucrarea de laborator Nr. 2.1
### Disciplina: Sisteme Încorporate (Embedded Systems)

**Tema:** Realizarea unei aplicații modulare pentru MCU cu execuție secvențială non-preemptivă a task-urilor

**Efectuat de:** Titerez Vladislav

**Anul implementării:** 2026

---

## Cuprins

1. [Scopul lucrării](#1-scopul-lucrării)
2. [Analiza domeniului](#2-analiza-domeniului)
   - 2.1 [Sisteme secvențiale non-preemptive](#21-sisteme-secvențiale-non-preemptive)
   - 2.2 [Planificarea task-urilor (Scheduling)](#22-planificarea-task-urilor-scheduling)
   - 2.3 [Modelul Provider/Consumer](#23-modelul-providerconsumer)
   - 2.4 [Comunicarea prin semnale globale](#24-comunicarea-prin-semnale-globale)
   - 2.5 [STDIO pe platforme embedded](#25-stdio-pe-platforme-embedded)
3. [Definirea problemei](#3-definirea-problemei)
4. [Arhitectura software](#4-arhitectura-software)
   - 4.1 [Structura modulară a proiectului](#41-structura-modulară-a-proiectului)
   - 4.2 [Diagrama bloc a sistemului](#42-diagrama-bloc-a-sistemului)
   - 4.3 [Diagrama de clase](#43-diagrama-de-clase)
   - 4.4 [Diagrama de secvență a execuției](#44-diagrama-de-secvență-a-execuției)
   - 4.5 [Diagrama de planificare temporală](#45-diagrama-de-planificare-temporală)
   - 4.6 [Diagrama Provider/Consumer](#46-diagrama-providerconsumer)
5. [Schema electrică](#5-schema-electrică)
6. [Descrierea implementării](#6-descrierea-implementării)
   - 6.1 [Modulul Scheduler](#61-modulul-scheduler)
   - 6.2 [Modulul Signals](#62-modulul-signals)
   - 6.3 [Modulul Tasks](#63-modulul-tasks)
   - 6.4 [Modulul SerialCmd (Idle Report)](#64-modulul-serialcmd-idle-report)
   - 6.5 [Modulul UartStdio](#65-modulul-uartstdio)
   - 6.6 [Modulul Keypad4x4](#66-modulul-keypad4x4)
   - 6.7 [Modulul Led](#67-modulul-led)
7. [Rezultate](#7-rezultate)
8. [Concluzii](#8-concluzii)
9. [Notă AI](#9-notă-ai)
10. [Anexe – Codul sursă al modulelor principale](#10-anexe--codul-sursă-al-modulelor-principale)

---

## 1. Scopul lucrării

Scopul acestei lucrări de laborator constă în:

- **Familiarizarea** cu principiile de planificare secvențială și execuție non-preemptivă a task-urilor într-un sistem embedded bazat pe microcontroler Arduino Uno (ATmega328P).
- **Analiza metodelor** de stabilire a recurenței (perioadei) și a offset-urilor între task-uri pentru optimizarea utilizării resurselor procesorului.
- **Înțelegerea și aplicarea** mecanismului de sincronizare între task-uri prin modelul **provider/consumer**, ca metodă de comunicare internă a datelor.
- **Elaborarea unui model** de execuție secvențială, evidențiind avantajele și limitările acesteia din punct de vedere metodologic.
- **Documentarea detaliată** a arhitecturii software și prezentarea schemelor bloc și a schemelor electrice, ca parte integrantă a metodologiei de proiectare.

---

## 2. Analiza domeniului

### 2.1 Sisteme secvențiale non-preemptive

Un **sistem secvențial non-preemptiv** (cooperative scheduling) reprezintă un model de execuție în care task-urile sunt planificate și rulate pe rând, fără a fi întrerupte forțat. Fiecare task rulează până la finalizarea ciclului său curent, apoi cedează controlul planificatorului, care decide care task urmează.

**Caracteristici fundamentale:**

| Proprietate | Descriere |
|---|---|
| **Non-preemptiv** | Un task nu poate fi întrerupt de alt task odată ce a început execuția |
| **Cooperativ** | Task-urile trebuie să returneze voluntar controlul |
| **Determinist** | Ordinea de execuție este previzibilă și repetabilă |
| **Fără overhead de context-switch** | Nu există salvare/restaurare de registre între task-uri |
| **Simplitate** | Nu necesită mecanisme de protecție a resurselor partajate (mutex, semafoare) |

**Avantaje:**
- Simplicitate conceptuală și de implementare
- Consum redus de memorie (o singură stivă)
- Absența condițiilor de cursă (race conditions) între task-uri, deoarece nu există preemptare
- Predictibilitate temporală

**Limitări:**
- Un task cu timp de execuție lung blochează toate celelalte task-uri
- Nu garantează respectarea deadline-urilor stricte (nu este potrivit pentru sisteme hard real-time)
- Necesită ca fiecare task să fie scurt și să returneze rapid

### 2.2 Planificarea task-urilor (Scheduling)

Planificarea task-urilor într-un sistem secvențial se bazează pe două concepte-cheie:

#### Recurența (Recurrence / Period)
Recurența definește intervalul de timp (în milisecunde) la care un task trebuie să fie executat. Alegerea recurenței trebuie să fie **rezonabilă** pentru a:
- **Diminua încărcarea procesorului** — evitarea execuției inutile la intervale prea mici
- **Asigura responsivitatea** — intervale suficient de mici pentru a detecta evenimentele la timp
- **Permite executarea tuturor task-urilor** — suma timpilor de execuție a task-urilor trebuie să fie mai mică decât perioadă minimă

Formula de verificare a fezabilității:

$$U = \sum_{i=1}^{n} \frac{C_i}{T_i} \leq 1$$

unde $C_i$ este timpul de execuție al task-ului $i$, iar $T_i$ este perioada (recurența) sa.

#### Offset-ul (Phase Offset)
Offset-ul definește întârzierea inițială (în milisecunde) înainte de prima execuție a unui task. Acesta permite:
- **Distribuirea uniformă** a execuțiilor în timp
- **Evitarea coliziunilor** — prevenirea situației în care toate task-urile devin ready simultan
- **Activarea task-urilor în ordinea cuvenită** — asigurarea că datele produse de un task sunt disponibile înainte ca task-ul consumator să le citească

**Exemplu din implementare:**

| Task | Funcție | Recurență | Offset | Justificare offset |
|------|---------|-----------|--------|-------------------|
| T0 | `keypadScan` | 50 ms | 0 ms | Rulează primul — citește intrarea |
| T1 | `buttonLed` | 50 ms | 5 ms | Consumă `sig_key` produs de T0 |
| T2 | `blinkLed` | 50 ms | 15 ms | Consumă `sig_led1State` din T1 |
| T3 | `stateVariable` | 50 ms | 25 ms | Consumă `sig_key` (tastele 2/3) |
| Idle | `idleReport` | ~1000 ms | – | Rulează în bucla principală |

### 2.3 Modelul Provider/Consumer

Modelul **Provider/Consumer** este o paradigmă de comunicare inter-task în care:

- **Provider** (producătorul) — task-ul care generează date și le stochează într-o variabilă globală (semnal)
- **Consumer** (consumatorul) — task-ul care citește și utilizează datele din acel semnal

Acest model oferă o separare clară a responsabilităților:
- Producătorul nu trebuie să cunoască identitatea consumatorului
- Consumatorul nu trebuie să cunoască sursa datelor
- Comunicarea este **decuplată** prin intermediul variabilelor globale (semnale)

**Matricea Provider/Consumer din proiect:**

| Semnal | Tip | Provider | Consumer(s) | Descriere |
|--------|-----|----------|-------------|-----------|
| `sig_key` | `volatile char` | Task 0 (keypadScan) | Task 1, Task 3 | Ultima tastă apăsată pe keypad |
| `sig_led1State` | `volatile bool` | Task 1 (buttonLed) | Task 2, Idle | Starea LED-ului 1 (ON/OFF) |
| `sig_led2State` | `volatile bool` | Task 2 (blinkLed) | Idle | Starea LED-ului 2 (ON/OFF) |
| `sig_blinkInterval` | `volatile int16_t` | Task 3 (stateVariable) | Task 2, Idle | Intervalul de clipire (100–2000 ms) |

### 2.4 Comunicarea prin semnale globale

Semnalele globale sunt variabile declarate cu calificatorul `volatile` pentru a:
- Preveni optimizările compilatorului care ar putea cache-ui valoarea într-un registru
- Asigura citiri/scrieri directe din/în memorie
- Permite accesul corect din context ISR și din bucla principală

Deoarece sistemul este **non-preemptiv**, nu există necesitatea mecanismelor de protecție (mutex/semafoare) pentru aceste variabile — task-urile rulează secvențial și nu se pot interfera reciproc. Totuși, calificatorul `volatile` rămâne necesar deoarece ISR-ul Timer2 poate modifica contoarele planificatorului în orice moment.

### 2.5 STDIO pe platforme embedded

Platforma AVR (ATmega328P) nu dispune nativ de funcțiile standard C `printf()` / `scanf()`. Utilizarea lor presupune:

1. **Redirectarea fluxurilor** `stdout` și `stdin` către UART prin intermediul structurii `FILE` și funcțiilor `fdev_setup_stream()`
2. **Implementarea unui putchar/getchar** personalizat care transmite/primește caractere prin `Serial`

**Particularități importante:**
- `printf()` pe AVR utilizează o implementare minimală (fără suport virgulă mobilă, dacă nu este activat prin linkare)
- Funcția `scanf()` este blocantă — așteaptă date pe UART, motiv pentru care task-ul de raportare (Idle) rulează în bucla principală, nu ca task planificat, evitând astfel blocarea întreruperilor
- Utilizarea STDIO permite o interfață familiară pentru debugging și monitorizare

---

## 3. Definirea problemei

Se cere realizarea unei aplicații modulare pentru microcontrolerul Arduino Uno care să ruleze secvențial cel puțin 3 task-uri distincte:

| Task | Denumire | Funcționalitate |
|------|----------|-----------------|
| **Task 0** | Keypad Scan | Scanarea tastaturii matriceale 4×4 și stocarea tastei apăsate în semnalul `sig_key` |
| **Task 1** | Button LED | Schimbarea stării LED-ului verde (toggle) la apăsarea tastei `1` pe keypad |
| **Task 2** | LED Intermitent | Controlul LED-ului galben cu clipire periodică, activ doar când LED-ul din Task 1 este stins |
| **Task 3** | Variabilă de Stare | Incrementarea/decrementarea intervalului de clipire prin tastele `3`/`2` |
| **Idle** | Raport Serial | Afișarea periodică a stării tuturor semnalelor prin STDIO (`printf`) în bucla principală |

**Cerințe suplimentare:**
- Comunicarea între task-uri prin modelul **provider/consumer**
- Utilizarea unui **scheduler** bazat pe Timer2 cu ISR la 1 ms
- Task-ul de raportare rulează în **bucla principală (Idle)** deoarece `printf()` este bazat pe un spin-lock care ar putea bloca întreruperile
- Structura **modulară** — fiecare periferic implementat în fișiere separate

---

## 4. Arhitectura software

### 4.1 Structura modulară a proiectului

```
Lab1_1/
├── platformio.ini          # Configurația PlatformIO
├── diagram.json            # Schema Wokwi (simulator)
├── wokwi.toml              # Configurație simulator
├── src/
│   └── main.cpp            # Punct de intrare, setup() + loop()
└── lib/
    ├── Scheduler/          # Planificator non-preemptiv pe bază de Timer2
    │   ├── Scheduler.h
    │   └── Scheduler.cpp
    ├── Tasks/              # Logica celor 4 task-uri
    │   ├── Tasks.h
    │   └── Tasks.cpp
    ├── Signals/            # Variabilele globale (semnale provider/consumer)
    │   ├── Signals.h
    │   └── Signals.cpp
    ├── SerialCmd/          # Comanda serial + task Idle de raportare
    │   ├── SerialCmd.h
    │   └── SerialCmd.cpp
    ├── UartStdio/          # Redirectarea printf/scanf pe UART
    │   ├── UartStdio.h
    │   └── UartStdio.cpp
    ├── Keypad4x4/          # Driver tastatura matriceală 4×4
    │   ├── Keypad4x4.h
    │   └── Keypad4x4.cpp
    ├── Led/                # Driver LED
    │   ├── Led.h
    │   └── Led.cpp
    └── Button/             # Driver buton cu debounce
        ├── Button.h
        └── Button.cpp
```

### 4.2 Diagrama bloc a sistemului

```
┌──────────────────────────────────────────────────────────────────────┐
│                          MAIN (setup + loop)                        │
│  ┌────────────┐    ┌──────────────┐    ┌─────────────────────────┐  │
│  │  UartStdio  │    │  Scheduler   │    │     SerialCmd           │  │
│  │  init(9600) │    │  init()      │    │  idleReport() [Idle]    │  │
│  └────────────┘    │  addTask()   │    │  poll()      [Serial]   │  │
│                     │  dispatch()  │    └──────────┬──────────────┘  │
│                     └──────┬───────┘               │                │
│                            │                       │                │
│              ┌─────────────┼─────────────┐         │  (consumes)    │
│              ▼             ▼             ▼         ▼                │
│  ┌──────────────┐ ┌──────────────┐ ┌──────────────┐               │
│  │  Task 0      │ │  Task 1      │ │  Task 2      │               │
│  │ keypadScan   │ │ buttonLed    │ │ blinkLed     │               │
│  │ rec=50 off=0 │ │ rec=50 off=5 │ │ rec=50 off=15│               │
│  └──────┬───────┘ └──────┬───────┘ └──────┬───────┘               │
│         │                │                │                         │
│         ▼                ▼                ▼                         │
│  ┌──────────────────────────────────────────────────┐              │
│  │              SIGNALS (Global Variables)           │              │
│  │  sig_key │ sig_led1State │ sig_led2State │        │              │
│  │          │               │  sig_blinkInterval     │              │
│  └──────────────────────────────────────────────────┘              │
│         ▲                                                           │
│         │                                                           │
│  ┌──────────────┐                                                   │
│  │  Task 3      │                                                   │
│  │ stateVariable│                                                   │
│  │ rec=50 off=25│                                                   │
│  └──────────────┘                                                   │
│                                                                      │
│  ┌─────────────────────────────────────────────────────┐            │
│  │                    HARDWARE LAYER                    │            │
│  │  Keypad4x4 (pins 4–11)  │  Led (pins 2, 12, 13)    │            │
│  └─────────────────────────────────────────────────────┘            │
└──────────────────────────────────────────────────────────────────────┘
```

### 4.3 Diagrama de clase

```
┌───────────────────────────┐       ┌───────────────────────────┐
│        Scheduler          │       │          Led              │
├───────────────────────────┤       ├───────────────────────────┤
│ - _tasks[MAX_TASKS]: Task │       │ - _pin: uint8_t           │
│ - _taskCount: uint8_t     │       │ - _state: bool            │
├───────────────────────────┤       ├───────────────────────────┤
│ + init(): void            │       │ + Led(pin: uint8_t)       │
│ + addTask(...): uint8_t   │       │ + init(): void            │
│ + dispatch(): void        │       │ + on(): void              │
│ + tick(): void            │       │ + off(): void             │
└───────────────────────────┘       │ + toggle(): void          │
                                     │ + getState(): bool        │
┌───────────────────────────┐       └───────────────────────────┘
│        Keypad4x4          │
├───────────────────────────┤       ┌───────────────────────────┐
│ - _rowPins[4]: uint8_t    │       │         Button            │
│ - _colPins[4]: uint8_t    │       ├───────────────────────────┤
│ - _lastKey: char          │       │ - _pin: uint8_t           │
│ - _keyMap[4][4]: char     │       │ - _prevPressed: bool      │
├───────────────────────────┤       ├───────────────────────────┤
│ + Keypad4x4(firstPin)     │       │ + Button(pin: uint8_t)    │
│ + init(): void            │       │ + init(): void            │
│ + getKey(): char          │       │ + isPressed(): bool       │
│ - scanRaw(): char         │       │ + readRaw(): bool         │
└───────────────────────────┘       └───────────────────────────┘

┌────────────────────────┐    ┌───────────────────────────┐
│   Task (struct)        │    │    UartStdio (namespace)   │
├────────────────────────┤    ├───────────────────────────┤
│ function: TaskFunction │    │ + init(baud): void        │
│ recurrence: uint16_t   │    └───────────────────────────┘
│ offset: uint16_t       │
│ counter: int16_t       │    ┌───────────────────────────┐
│ ready: bool            │    │  SerialCmd (namespace)     │
│ enabled: bool          │    ├───────────────────────────┤
└────────────────────────┘    │ + poll(): void            │
                              │ + idleReport(): void      │
                              └───────────────────────────┘
```

### 4.4 Diagrama de secvență a execuției

```
Timp (ms) │  ISR (1ms)     │  Dispatcher      │  T0        │  T1       │  T2       │  T3       │  Idle
──────────┼────────────────┼──────────────────┼────────────┼───────────┼───────────┼───────────┼──────────
   0      │  tick()        │                  │            │           │           │           │
   0      │  T0.ready=true │  run T0          │ keypadScan │           │           │           │
          │                │                  │ → sig_key  │           │           │           │
   5      │  T1.ready=true │  run T1          │            │ buttonLed │           │           │
          │                │                  │            │ ← sig_key │           │           │
          │                │                  │            │ → sig_led1│           │           │
  15      │  T2.ready=true │  run T2          │            │           │ blinkLed  │           │
          │                │                  │            │           │ ← sig_led1│           │
          │                │                  │            │           │ → sig_led2│           │
  25      │  T3.ready=true │  run T3          │            │           │           │ stateVar  │
          │                │                  │            │           │           │ ← sig_key │
          │                │                  │            │           │           │ → sig_blnk│
          │                │                  │            │           │           │           │
 ~1000    │                │                  │            │           │           │           │ idleReport()
          │                │                  │            │           │           │           │ ← ALL sigs
          │                │                  │            │           │           │           │ → printf()
  50      │  T0.ready=true │  run T0          │ keypadScan │           │           │           │
          │     ...        │     ...          │   ...      │   ...     │   ...     │   ...     │   ...
```

### 4.5 Diagrama de planificare temporală

```
          0ms    5ms   15ms   25ms                            50ms   55ms  65ms   75ms
           │      │      │      │                               │      │     │      │
    T0 ────█──────┼──────┼──────┼───────────────────────────────█──────┼─────┼──────┼──
           │      │      │      │                               │      │     │      │
    T1 ────┼──────█──────┼──────┼───────────────────────────────┼──────█─────┼──────┼──
           │      │      │      │                               │      │     │      │
    T2 ────┼──────┼──────█──────┼───────────────────────────────┼──────┼─────█──────┼──
           │      │      │      │                               │      │     │      │
    T3 ────┼──────┼──────┼──────█───────────────────────────────┼──────┼─────┼──────█──
           │      │      │      │                               │      │     │      │
  Idle ────░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░
           │      │      │      │                               │      │     │      │
  ISR  ─┤├─┤├─┤├─┤├─┤├─┤├─┤├─┤├─┤├─┤├─┤├─┤├─┤├─┤├─┤├─┤├─┤├─┤├─┤├─┤├─┤├─┤├─┤├─┤├──

    █ = task execution    ░ = idle/report    ┤├ = 1ms timer tick (ISR)
```

### 4.6 Diagrama Provider/Consumer

```
  ┌─────────────────┐          sig_key           ┌─────────────────┐
  │     Task 0      │ ─────────────────────────▶ │     Task 1      │
  │   keypadScan    │     (volatile char)        │    buttonLed    │
  │   [PROVIDER]    │ ──────────────┐            │ [CONSUMER/PROV] │
  └─────────────────┘               │            └────────┬────────┘
                                    │                      │
                                    │ sig_key              │ sig_led1State
                                    │                      │ (volatile bool)
                                    ▼                      ▼
                             ┌─────────────────┐  ┌─────────────────┐
                             │     Task 3      │  │     Task 2      │
                             │  stateVariable  │  │    blinkLed     │
                             │ [CONSUMER/PROV] │  │ [CONSUMER/PROV] │
                             └────────┬────────┘  └────────┬────────┘
                                      │                    │
                          sig_blinkInterval          sig_led2State
                          (volatile int16_t)         (volatile bool)
                                      │                    │
                                      ▼                    ▼
                             ┌─────────────────────────────────────┐
                             │           Idle Task                 │
                             │         idleReport()                │
                             │         [CONSUMER]                  │
                             │                                     │
                             │  printf("[IDLE] LED1=%s | LED2=%s   │
                             │         | Blink=%dms\n", ...)       │
                             └─────────────────────────────────────┘
```

---

## 5. Schema electrică

### Componente hardware utilizate

| Componentă | Cantitate | Specificații |
|------------|-----------|-------------|
| Arduino Uno (ATmega328P) | 1 | MCU 16 MHz, 32 KB Flash, 2 KB SRAM |
| LED roșu | 1 | Indicator debugging (pin D2) |
| LED galben | 1 | LED intermitent – Task 2 (pin D12) |
| LED verde | 1 | LED toggle – Task 1 (pin D13) |
| Rezistor 220 Ω | 3 | Limitare curent LED-uri |
| Tastatura matriceală 4×4 | 1 | Rânduri: D4–D7, Coloane: D8–D11 |
| LCD 1602 I2C | 1 | Adresa I2C, SDA=A4, SCL=A5 |
| Breadboard | 1 | Prototipare |
| Cabluri jumper | multiple | Conexiuni |

### Schema de conexiuni

```
                        Arduino Uno
                    ┌─────────────────┐
                    │                 │
              D13 ──┤ PB5        5V  ├─── LCD VCC, Alimentare
              D12 ──┤ PB4       GND  ├─── LCD GND, LEDs Catod (prin breadboard)
              D11 ──┤ PB3       A5   ├─── LCD SCL (I2C Clock)
              D10 ──┤ PB2       A4   ├─── LCD SDA (I2C Data)
               D9 ──┤ PB1            │
               D8 ──┤ PB0            │
               D7 ──┤ PD7            │
               D6 ──┤ PD6            │
               D5 ──┤ PD5            │
               D4 ──┤ PD4            │
               D2 ──┤ PD2            │
                    │                 │
                    └─────────────────┘

    LED Connections:                     Keypad 4x4 Connections:
    D13 ──[220Ω]──LED(verde)──GND       D4  ── R1 (Row 1)
    D12 ──[220Ω]──LED(galben)──GND      D5  ── R2 (Row 2)
    D2  ──[220Ω]──LED(roșu)──GND        D6  ── R3 (Row 3)
                                          D7  ── R4 (Row 4)
                                          D8  ── C1 (Col 1)
                                          D9  ── C2 (Col 2)
                                          D10 ── C3 (Col 3)
                                          D11 ── C4 (Col 4)
```

### Principiul funcționării LED-urilor

Fiecare LED este conectat în serie cu un rezistor de 220 Ω pentru limitarea curentului:

$$I_{LED} = \frac{V_{CC} - V_{LED}}{R} = \frac{5V - 2V}{220\Omega} \approx 13.6 \text{ mA}$$

### Principiul funcționării tastaturii matriceale

Tastatura 4×4 utilizează scanarea matriceală:
1. Se setează pe rând fiecare linie de rând (R1–R4) pe `OUTPUT LOW`
2. Se citesc coloanele (C1–C4) configurate cu `INPUT_PULLUP`
3. Dacă o coloană citește `LOW`, tasta de la intersecția rând-coloană este apăsată
4. Se restabilește rândul la `INPUT_PULLUP` și se trece la următorul rând

---

## 6. Descrierea implementării

### 6.1 Modulul Scheduler

Planificatorul reprezintă nucleul sistemului secvențial. Acesta utilizează **Timer2** al ATmega328P configurat în mod CTC (Clear Timer on Compare Match) pentru a genera un tick de exact **1 ms**.

**Configurarea Timer2:**
- Prescaler = 64 → Frecvența timer = 16 MHz / 64 = 250 kHz
- OCR2A = 249 → Perioadă = (249 + 1) / 250 kHz = 1 ms
- Mod CTC (WGM21 = 1)
- Întrerupere pe Compare Match A (OCIE2A)

**Algoritmul de funcționare:**

1. **ISR (la fiecare 1 ms):** Decrementează contorul fiecărui task activ. Când un contor ajunge la 0, setează flag-ul `ready` și reîncarcă contorul cu valoarea recurenței.
2. **Dispatch (în bucla principală):** Parcurge lista de task-uri și execută funcția fiecărui task care are flag-ul `ready` setat, resetând flag-ul imediat.

Acest mecanism asigură o separare clară între **detectarea momentului de execuție** (în ISR — context de întrerupere) și **execuția efectivă** (în main loop — context normal), respectând principiul de a menține ISR-urile cât mai scurte posibil.

### 6.2 Modulul Signals

Modulul `Signals` definește **variabilele globale partajate** (semnalele) utilizate în modelul provider/consumer:

- `sig_key` — Stochează ultima tastă apăsată pe keypad. Resetată de consumator după procesare.
- `sig_led1State` — Reflectă starea curentă a LED-ului verde (Task 1).
- `sig_led2State` — Reflectă starea curentă a LED-ului galben (Task 2).
- `sig_blinkInterval` — Intervalul de clipire al LED-ului galben, ajustabil prin tastele 2/3. Valoare implicită: 500 ms. Domeniu: 100–2000 ms, pas: 100 ms.

Toate variabilele sunt declarate `volatile` pentru a garanta coerența datelor între ISR și bucla principală.

### 6.3 Modulul Tasks

Conține logica celor 4 task-uri planificate:

**Task 0 — `keypadScan()`:**
- Apelează `keypad.getKey()` pentru a detecta o apăsare nouă
- Dacă o tastă este detectată, o scrie în semnalul `sig_key`
- Rolul de **provider** pentru `sig_key`

**Task 1 — `buttonLed()`:**
- **Consumer** al `sig_key` — verifică dacă tasta apăsată este `'1'`
- Toggle-ează LED-ul verde și actualizează `sig_led1State`
- **Provider** al `sig_led1State`
- Resetează `sig_key` după consum (evită reprocessarea)

**Task 2 — `blinkLed()`:**
- **Consumer** al `sig_led1State` și `sig_blinkInterval`
- Dacă LED-ul 1 este stins → clipește LED-ul galben la intervalul configurat
- Dacă LED-ul 1 este aprins → oprește LED-ul galben
- **Provider** al `sig_led2State`
- Utilizează un contor intern (`task2BlinkCnt`) incrementat cu recurența task-ului

**Task 3 — `stateVariable()`:**
- **Consumer** al `sig_key` — verifică tastele `'2'` (decrement) și `'3'` (increment)
- **Provider** al `sig_blinkInterval`
- Modifică intervalul cu un pas de 100 ms, între limitele 100–2000 ms

### 6.4 Modulul SerialCmd (Idle Report)

Acest modul implementează două funcționalități care rulează în **bucla principală** (Idle):

**`idleReport()`:**
- Raportează periodic (la fiecare ~1000 ms) starea tuturor semnalelor prin `printf()`
- Format: `[IDLE] LED1=ON/OFF | LED2=ON/OFF | Blink=XXXms`
- Este consumator pur — citește toate semnalele fără a le modifica
- Rulează în main loop (nu ca task planificat) deoarece `printf()` este blocant

**`poll()`:**
- Verifică dacă există date pe portul serial
- Comenzi disponibile: `r` (reset interval la 500ms), `+` (increment), `-` (decrement)
- Permite controlul suplimentar prin interfața serial

### 6.5 Modulul UartStdio

Realizează **redirectarea** fluxurilor standard C (`stdout`, `stdin`) către interfața UART a Arduino, permițând utilizarea funcțiilor familiare `printf()` și `scanf()`:

- `uart_putchar()` — Transmite un caracter prin `Serial.write()`. Adaugă `\r` înaintea `\n` pentru compatibilitate terminale.
- `uart_getchar()` — Citește un caracter de pe `Serial`. **Blocant** — așteaptă până la disponibilitatea datelor.
- `init()` — Inițializează `Serial` la baud-ul specificat și configurează structurile `FILE` pentru stdout/stdin.

### 6.6 Modulul Keypad4x4

Driver pentru tastatura matriceală 4×4:

- **Scanare activă:** Setează pe rând fiecare linie de rând pe LOW, citind coloanele
- **Debounce software:** Returnează o tastă doar la prima detectare (edge detection). Tasta trebuie eliberată înainte de a fi detectată din nou.
- **Mapare configurabilă:** Tabel static `_keyMap[4][4]` ce mapează poziția fizică la caractere

### 6.7 Modulul Led

Clasă simplă de abstractizare a unui LED digital:

- Encapsulează pinul GPIO și starea curentă
- Oferă operații: `on()`, `off()`, `toggle()`, `getState()`
- Menține sincronizarea între starea internă (`_state`) și ieșirea GPIO

---

## 7. Rezultate

<!-- 
TODO: Completați această secțiune cu:
- Capturi de ecran din Serial Monitor cu output-ul idleReport
- Capturi de ecran din simulatorul Wokwi arătând funcționarea LED-urilor
- Descrierea testelor efectuate și a comportamentului observat
- Fotografii ale montajului fizic (dacă este cazul)

Exemple de rezultate așteptate:
1. La pornire: LED1=OFF, LED2 clipește la 500ms
2. Apăsare tasta '1': LED1=ON, LED2 se oprește
3. Apăsare tasta '1' din nou: LED1=OFF, LED2 reia clipirea
4. Apăsare tasta '3': intervalul crește cu 100ms
5. Apăsare tasta '2': intervalul scade cu 100ms
6. În Serial Monitor: raportare periodică la fiecare ~1s
-->

*Secțiunea va fi completată cu rezultatele experimentale.*

---

## 8. Concluzii

<!-- 
TODO: Completați cu concluziile proprii bazate pe rezultatele obținute.

Puncte de discuție sugerate:
- Experiența cu planificarea secvențială non-preemptivă
- Eficacitatea modelului provider/consumer
- Avantajele structurii modulare
- Observații privind comportamentul temporal al sistemului
- Comparație cu alte metode de planificare (preemptivă, bare-metal super-loop)
- Posibile îmbunătățiri
-->

*Secțiunea va fi completată cu concluziile autorului.*

---

## 9. Notă AI

> **Declarație privind utilizarea inteligenței artificiale:**
> Acest raport a fost generat cu asistența **GitHub Copilot** (model Claude), un instrument AI utilizat în cadrul mediului de dezvoltare Visual Studio Code. AI-ul a fost utilizat pentru:
> - Structurarea și redactarea documentului de raport în format Markdown
> - Generarea diagramelor text (bloc, secvență, clase, provider/consumer)
> - Analiza și documentarea codului sursă existent
> - Formularea analizei domeniului pe baza cerințelor furnizate
>
> Codul sursă al proiectului a fost implementat de autor și furnizat ca input AI-ului pentru analiză.
> Secțiunile de **Rezultate** și **Concluzii** sunt lăsate pentru completare manuală de către autor.

---

## 10. Anexe – Codul sursă al modulelor principale

### Anexa A – main.cpp

```cpp
#include <Arduino.h>
#include <stdio.h>
#include "UartStdio.h"
#include "Signals.h"
#include "Tasks.h"
#include "SerialCmd.h"
#include "Scheduler.h"

#define TASK2_REC_MS 50

void setup()
{
    UartStdio::init(9600);
    Tasks::initHardware();

    Scheduler::init();
    Scheduler::addTask(Tasks::keypadScan, 50, 0);
    Scheduler::addTask(Tasks::buttonLed, 50, 5);
    Scheduler::addTask(Tasks::blinkLed, TASK2_REC_MS, 15);
    Scheduler::addTask(Tasks::stateVariable, 50, 25);

    printf("Menu:\n");
    printf("T0: Keypad    rec=50ms  off=0ms\n");
    printf("T1: ButtonLED rec=50ms  off=5ms\n");
    printf("T2: BlinkLED  rec=50ms  off=15ms\n");
    printf("T3: StateVar  rec=50ms  off=25ms\n");
    printf("Idle: Report  (main loop, 1s)\n");
    printf("Keys: 1=toggle  2=dec  3=inc\n");
}

void loop()
{
    Scheduler::dispatch();
    SerialCmd::idleReport();
    SerialCmd::poll();
}
```

### Anexa B – Scheduler (Scheduler.h + Scheduler.cpp)

**Scheduler.h:**

```cpp
#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <Arduino.h>

#define MAX_TASKS 8

typedef void (*TaskFunction)(void);

struct Task
{
    TaskFunction function;
    uint16_t recurrence;
    uint16_t offset;
    volatile int16_t counter;
    volatile bool ready;
    bool enabled;
};

class Scheduler
{
public:
    static void init();
    static uint8_t addTask(TaskFunction func, uint16_t recurrence, uint16_t offset);
    static void dispatch();
    static void tick();

private:
    static Task _tasks[MAX_TASKS];
    static uint8_t _taskCount;
};

#endif
```

**Scheduler.cpp:**

```cpp
#include "Scheduler.h"

Task Scheduler::_tasks[MAX_TASKS];
uint8_t Scheduler::_taskCount = 0;

void Scheduler::init()
{
    _taskCount = 0;
    cli();
    TCCR2A = 0;
    TCCR2B = 0;
    TCCR2A |= (1 << WGM21);  // CTC mode
    TCCR2B |= (1 << CS22);   // Prescaler = 64
    OCR2A = 249;              // Compare match → 1 ms
    TIMSK2 |= (1 << OCIE2A); // Enable Timer2 Compare-A interrupt
    TCNT2 = 0;
    sei();
}

uint8_t Scheduler::addTask(TaskFunction func, uint16_t recurrence, uint16_t offset)
{
    if (_taskCount >= MAX_TASKS)
        return 0xFF;

    Task &t = _tasks[_taskCount];
    t.function = func;
    t.recurrence = recurrence;
    t.offset = offset;
    t.counter = (offset > 0) ? offset : recurrence;
    t.ready = false;
    t.enabled = true;

    return _taskCount++;
}

void Scheduler::dispatch()
{
    for (uint8_t i = 0; i < _taskCount; i++)
    {
        if (_tasks[i].ready && _tasks[i].enabled)
        {
            _tasks[i].ready = false;
            _tasks[i].function();
        }
    }
}

void Scheduler::tick()
{
    for (uint8_t i = 0; i < _taskCount; i++)
    {
        if (_tasks[i].enabled)
        {
            if (--_tasks[i].counter <= 0)
            {
                _tasks[i].ready = true;
                _tasks[i].counter = _tasks[i].recurrence;
            }
        }
    }
}

ISR(TIMER2_COMPA_vect)
{
    Scheduler::tick();
}
```

### Anexa C – Signals (Signals.h + Signals.cpp)

**Signals.h:**

```cpp
/**
 * @file Signals.h
 * @brief Shared global signals for the provider / consumer model.
 *
 *  Signal            │ Provider │ Consumer(s)
 *  ---------------------------------------------
 *  sig_key           │ Task 0   │ Task 1, Task 3
 *  sig_led1State     │ Task 1   │ Task 2, Idle
 *  sig_led2State     │ Task 2   │ Idle
 *  sig_blinkInterval │ Task 3   │ Task 2, Idle
 */
#ifndef SIGNALS_H
#define SIGNALS_H

#include <stdint.h>

extern volatile char sig_key;
extern volatile bool sig_led1State;
extern volatile bool sig_led2State;
extern volatile int16_t sig_blinkInterval;

#endif
```

**Signals.cpp:**

```cpp
#include "Signals.h"

volatile char    sig_key           = '\0';
volatile bool    sig_led1State     = false;
volatile bool    sig_led2State     = false;
volatile int16_t sig_blinkInterval = 500;
```

### Anexa D – Tasks (Tasks.h + Tasks.cpp)

**Tasks.h:**

```cpp
#ifndef TASKS_H
#define TASKS_H

namespace Tasks
{
    void initHardware();
    void keypadScan();
    void buttonLed();
    void blinkLed();
    void stateVariable();
}

#endif
```

**Tasks.cpp:**

```cpp
#include "Tasks.h"
#include "Signals.h"
#include "Led.h"
#include "Keypad4x4.h"

#define LED1_PIN 13
#define LED2_PIN 12
#define LED3_PIN 2
#define KEYPAD_FIRST 4

static Led led1(LED1_PIN);
static Led led2(LED2_PIN);
static Led led3(LED3_PIN);
static Keypad4x4 keypad(KEYPAD_FIRST);

#define TASK2_REC_MS 50
static volatile uint16_t task2BlinkCnt = 0;

#define BLINK_STEP 100
#define BLINK_MIN 100
#define BLINK_MAX 2000

void Tasks::initHardware()
{
    led1.init();
    led2.init();
    led3.init();
    keypad.init();
}

void Tasks::keypadScan()
{
    char k = keypad.getKey();
    if (k != '\0')
        sig_key = k;
}

void Tasks::buttonLed()
{
    if (sig_key == '1')
    {
        sig_key = '\0';
        led1.toggle();
        sig_led1State = led1.getState();
    }
}

void Tasks::blinkLed()
{
    if (!sig_led1State)
    {
        task2BlinkCnt += TASK2_REC_MS;
        if (task2BlinkCnt >= (uint16_t)sig_blinkInterval)
        {
            led2.toggle();
            sig_led2State = led2.getState();
            task2BlinkCnt = 0;
        }
    }
    else
    {
        if (sig_led2State)
        {
            led2.off();
            sig_led2State = false;
        }
        task2BlinkCnt = 0;
    }
}

void Tasks::stateVariable()
{
    if (sig_key == '3')
    {
        sig_key = '\0';
        if (sig_blinkInterval + BLINK_STEP <= BLINK_MAX)
            sig_blinkInterval += BLINK_STEP;
    }
    if (sig_key == '2')
    {
        sig_key = '\0';
        if (sig_blinkInterval - BLINK_STEP >= BLINK_MIN)
            sig_blinkInterval -= BLINK_STEP;
    }
}
```

### Anexa E – SerialCmd (SerialCmd.h + SerialCmd.cpp)

**SerialCmd.cpp:**

```cpp
#include "SerialCmd.h"
#include "Signals.h"
#include <Arduino.h>
#include <stdio.h>

#define BLINK_STEP 100
#define BLINK_MIN 100
#define BLINK_MAX 2000

void SerialCmd::poll()
{
    if (!Serial.available())
        return;

    char cmd;
    scanf(" %c", &cmd);
    printf("[IN] cmd='%c'\n", cmd);

    switch (cmd)
    {
    case 'r':
    case 'R':
        sig_blinkInterval = 500;
        printf("[IN] BlinkInterval reset to 500ms\n");
        break;
    case '+':
        if (sig_blinkInterval + BLINK_STEP <= BLINK_MAX)
            sig_blinkInterval += BLINK_STEP;
        printf("[IN] BlinkInterval = %dms\n", sig_blinkInterval);
        break;
    case '-':
        if (sig_blinkInterval - BLINK_STEP >= BLINK_MIN)
            sig_blinkInterval -= BLINK_STEP;
        printf("[IN] BlinkInterval = %dms\n", sig_blinkInterval);
        break;
    default:
        printf("[IN] Unknown command. Use: r/+/-\n");
        break;
    }
}

void SerialCmd::idleReport()
{
    static unsigned long lastReport = 0;
    unsigned long now = millis();

    if (now - lastReport >= 1000)
    {
        printf("[IDLE] LED1=%s | LED2=%s | Blink=%dms\n",
               sig_led1State ? "ON " : "OFF",
               sig_led2State ? "ON " : "OFF",
               sig_blinkInterval);
        lastReport = now;
    }
}
```

### Anexa F – UartStdio (UartStdio.cpp)

```cpp
#include "UartStdio.h"

static FILE uart_out;
static FILE uart_in;

static int uart_putchar(char c, FILE *)
{
    if (c == '\n')
        Serial.write('\r');
    Serial.write(c);
    return 0;
}

static int uart_getchar(FILE *)
{
    while (!Serial.available())
        ;
    return Serial.read();
}

void UartStdio::init(unsigned long baud)
{
    Serial.begin(baud);
    fdev_setup_stream(&uart_out, uart_putchar, NULL, _FDEV_SETUP_WRITE);
    fdev_setup_stream(&uart_in, NULL, uart_getchar, _FDEV_SETUP_READ);
    stdout = &uart_out;
    stdin = &uart_in;
}
```

### Anexa G – Keypad4x4 (Keypad4x4.cpp)

```cpp
#include "Keypad4x4.h"

const char Keypad4x4::_keyMap[KEYPAD_ROWS][KEYPAD_COLS] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}};

Keypad4x4::Keypad4x4(uint8_t firstPin) : _lastKey('\0')
{
    for (uint8_t i = 0; i < KEYPAD_ROWS; i++)
        _rowPins[i] = firstPin + i;
    for (uint8_t i = 0; i < KEYPAD_COLS; i++)
        _colPins[i] = firstPin + KEYPAD_ROWS + i;
}

void Keypad4x4::init()
{
    for (uint8_t i = 0; i < KEYPAD_ROWS; i++)
        pinMode(_rowPins[i], INPUT_PULLUP);
    for (uint8_t i = 0; i < KEYPAD_COLS; i++)
        pinMode(_colPins[i], INPUT_PULLUP);
}

char Keypad4x4::scanRaw()
{
    for (uint8_t r = 0; r < KEYPAD_ROWS; r++)
    {
        pinMode(_rowPins[r], OUTPUT);
        digitalWrite(_rowPins[r], LOW);
        for (uint8_t c = 0; c < KEYPAD_COLS; c++)
        {
            if (digitalRead(_colPins[c]) == LOW)
            {
                pinMode(_rowPins[r], INPUT_PULLUP);
                return _keyMap[r][c];
            }
        }
        pinMode(_rowPins[r], INPUT_PULLUP);
    }
    return '\0';
}

char Keypad4x4::getKey()
{
    char key = scanRaw();
    if (key != '\0' && key != _lastKey)
    {
        _lastKey = key;
        return key;
    }
    if (key == '\0')
        _lastKey = '\0';
    return '\0';
}
```

### Anexa H – Led (Led.cpp)

```cpp
#include "Led.h"

Led::Led(uint8_t pin) : _pin(pin), _state(false) {}

void Led::init()
{
    pinMode(_pin, OUTPUT);
    digitalWrite(_pin, LOW);
    _state = false;
}

void Led::on()
{
    digitalWrite(_pin, HIGH);
    _state = true;
}

void Led::off()
{
    digitalWrite(_pin, LOW);
    _state = false;
}

void Led::toggle()
{
    _state = !_state;
    digitalWrite(_pin, _state ? HIGH : LOW);
}

bool Led::getState() const
{
    return _state;
}
```

---

*Raport elaborat în 2026 – Titerez Vladislav – UTM FCIM*
