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
   - 6.6 [Modulul Button](#66-modulul-button)
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
- **Permite executarea tuturor task-urilor** — suma timpilor de execuție a task-urilor trebuie să fie mai mică decât perioada minimă

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
| T0 | `buttonScan` | 50 ms | 0 ms | Rulează primul — citește butoanele |
| T1 | `buttonLed` | 50 ms | 5 ms | Consumă `sig_btnToggle` produs de T0 |
| T2 | `blinkLed` | 50 ms | 15 ms | Consumă `sig_led1State` din T1 |
| T3 | `stateVariable` | 50 ms | 25 ms | Consumă `sig_btnDec` / `sig_btnInc` din T0 |
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
| `sig_btnToggle` | `volatile bool` | Task 0 (buttonScan) | Task 1 | Butonul de toggle LED1 a fost apăsat |
| `sig_btnDec` | `volatile bool` | Task 0 (buttonScan) | Task 3 | Butonul de decrementare apăsat |
| `sig_btnInc` | `volatile bool` | Task 0 (buttonScan) | Task 3 | Butonul de incrementare apăsat |
| `sig_led1State` | `volatile bool` | Task 1 (buttonLed) | Task 2, Idle | Starea LED-ului verde (ON/OFF) |
| `sig_led2State` | `volatile bool` | Task 2 (blinkLed) | Idle | Starea LED-ului galben (ON/OFF) |
| `sig_blinkInterval` | `volatile int16_t` | Task 3 (stateVariable) | Task 2, Idle | Intervalul de clipire (100–2000 ms) |

### 2.4 Comunicarea prin semnale globale

Semnalele globale sunt variabile declarate cu calificatorul `volatile` pentru a:
- Preveni optimizările compilatorului care ar putea cache-ui valoarea într-un registru
- Asigura citiri/scrieri directe din/în memorie
- Permite accesul corect din context ISR și din bucla principală

Deoarece sistemul este **non-preemptiv**, nu există necesitatea mecanismelor de protecție (mutex/semafoare) pentru aceste variabile — task-urile rulează secvențial și nu se pot interfera reciproc. Totuși, calificatorul `volatile` rămâne necesar deoarece ISR-ul Timer2 poate modifica contoarele planificatorului în orice moment.

Utilizarea semnalelor booleane (`sig_btnToggle`, `sig_btnDec`, `sig_btnInc`) în loc de un singur caracter (`sig_key`) permite consumarea independentă a fiecărui eveniment de buton, eliminând situația în care un task consumă un eveniment destinat altui task.

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
| **Task 0** | Button Scan | Scanarea celor 3 butoane fizice și generarea semnalelor booleane `sig_btnToggle`, `sig_btnDec`, `sig_btnInc` |
| **Task 1** | Button LED | Schimbarea stării LED-ului verde (toggle) la apăsarea butonului de pe pin D8 |
| **Task 2** | LED Intermitent | Controlul LED-ului galben cu clipire periodică, activ doar când LED-ul din Task 1 este stins |
| **Task 3** | Variabilă de Stare | Incrementarea (buton D2) / decrementarea (buton D7) intervalului de clipire |
| **Idle** | Raport Serial | Afișarea periodică a stării tuturor semnalelor prin STDIO (`printf`) în bucla principală |

**Cerințe suplimentare:**
- Comunicarea între task-uri prin modelul **provider/consumer** cu semnale booleane
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
    ├── Button/             # Driver buton cu detecție front (edge detection)
    │   ├── Button.h
    │   └── Button.cpp
    └── Led/                # Driver LED
        ├── Led.h
        └── Led.cpp
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
│                            │                       │  (consumes     │
│              ┌─────────────┼─────────────┐         │   all signals) │
│              ▼             ▼             ▼         ▼                │
│  ┌──────────────┐ ┌──────────────┐ ┌──────────────┐               │
│  │  Task 0      │ │  Task 1      │ │  Task 2      │               │
│  │ buttonScan   │ │ buttonLed    │ │ blinkLed     │               │
│  │ rec=50 off=0 │ │ rec=50 off=5 │ │ rec=50 off=15│               │
│  └──────┬───────┘ └──────┬───────┘ └──────┬───────┘               │
│         │                │                │                         │
│         ▼                ▼                ▼                         │
│  ┌──────────────────────────────────────────────────┐              │
│  │              SIGNALS (Global Variables)           │              │
│  │  sig_btnToggle │ sig_btnDec │ sig_btnInc         │              │
│  │  sig_led1State │ sig_led2State │ sig_blinkInterval│              │
│  └──────────────────────────────────────────────────┘              │
│         ▲                                                           │
│  ┌──────────────┐                                                   │
│  │  Task 3      │                                                   │
│  │ stateVariable│                                                   │
│  │ rec=50 off=25│                                                   │
│  └──────────────┘                                                   │
│                                                                      │
│  ┌─────────────────────────────────────────────────────┐            │
│  │                    HARDWARE LAYER                    │            │
│  │  Button×3 (D8, D7, D2)  │  Led×2 (D12, D13)        │            │
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
│         Button            │
├───────────────────────────┤       ┌───────────────────────────┐
│ - _pin: uint8_t           │       │   Task (struct)           │
│ - _prevPressed: bool      │       ├───────────────────────────┤
├───────────────────────────┤       │ function: TaskFunction    │
│ + Button(pin: uint8_t)    │       │ recurrence: uint16_t      │
│ + init(): void            │       │ offset: uint16_t          │
│ + isPressed(): bool       │       │ counter: int16_t          │
│ + readRaw(): bool         │       │ ready: bool               │
└───────────────────────────┘       │ enabled: bool             │
                                     └───────────────────────────┘
┌────────────────────────┐
│  UartStdio (namespace) │    ┌───────────────────────────┐
├────────────────────────┤    │  SerialCmd (namespace)     │
│ + init(baud): void     │    ├───────────────────────────┤
└────────────────────────┘    │ + poll(): void            │
                              │ + idleReport(): void      │
                              └───────────────────────────┘
```

### 4.4 Diagrama de secvență a execuției

```
Timp (ms) │  ISR (1ms)     │  Dispatcher      │  T0          │  T1       │  T2       │  T3       │  Idle
──────────┼────────────────┼──────────────────┼──────────────┼───────────┼───────────┼───────────┼──────────
   0      │  tick()        │                  │              │           │           │           │
   0      │  T0.ready=true │  run T0          │ buttonScan   │           │           │           │
          │                │                  │ → sig_btnX   │           │           │           │
   5      │  T1.ready=true │  run T1          │              │ buttonLed │           │           │
          │                │                  │              │ ← btnTgl  │           │           │
          │                │                  │              │ → led1St  │           │           │
  15      │  T2.ready=true │  run T2          │              │           │ blinkLed  │           │
          │                │                  │              │           │ ← led1St  │           │
          │                │                  │              │           │ ← blnkInt │           │
          │                │                  │              │           │ → led2St  │           │
  25      │  T3.ready=true │  run T3          │              │           │           │ stateVar  │
          │                │                  │              │           │           │ ← btnDec  │
          │                │                  │              │           │           │ ← btnInc  │
          │                │                  │              │           │           │ → blnkInt │
          │                │                  │              │           │           │           │
 ~1000    │                │                  │              │           │           │           │ idleReport
          │                │                  │              │           │           │           │ ← ALL sigs
          │                │                  │              │           │           │           │ → printf()
  50      │  T0.ready=true │  run T0          │ buttonScan   │           │           │           │
          │     ...        │     ...          │   ...        │   ...     │   ...     │   ...     │   ...
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
                        ┌──────────────────┐
                        │     Task 0       │
                        │   buttonScan     │
                        │   [PROVIDER]     │
                        └──┬─────┬─────┬───┘
                           │     │     │
              sig_btnToggle│     │     │sig_btnInc
              (bool)       │     │     │(bool)
                           │     │     │
                           ▼     │     ▼
               ┌────────────┐    │    ┌────────────┐
               │   Task 1   │    │    │   Task 3   │
               │  buttonLed │    │    │ stateVar   │
               │ [CONS/PROV]│    │    │ [CONS/PROV]│
               └─────┬──────┘    │    └─────┬──────┘
                     │           │          │
          sig_led1State     sig_btnDec   sig_blinkInterval
          (bool)        │   (bool)      (int16_t)
                     │           │          │
                     ▼           │          │
               ┌────────────┐   │          │
               │   Task 2   │◀──┘          │
               │  blinkLed  │◀─────────────┘
               │ [CONS/PROV]│
               └─────┬──────┘
                     │
              sig_led2State
              (bool)
                     │
                     ▼
          ┌────────────────────────────────┐
          │          Idle Task             │
          │        idleReport()            │
          │        [CONSUMER]              │
          │                                │
          │  ← sig_led1State               │
          │  ← sig_led2State               │
          │  ← sig_blinkInterval           │
          │                                │
          │  printf("[IDLE] LED1=%s |       │
          │    LED2=%s | Blink=%dms")       │
          └────────────────────────────────┘
```

---

## 5. Schema electrică

### Componente hardware utilizate

| Componentă | Cantitate | Specificații |
|------------|-----------|-------------|
| Arduino Uno (ATmega328P) | 1 | MCU 16 MHz, 32 KB Flash, 2 KB SRAM |
| LED galben | 1 | LED intermitent – Task 2 (pin D12) |
| LED verde | 1 | LED toggle – Task 1 (pin D13) |
| Rezistor 220 Ω | 2 | Limitare curent LED-uri |
| Buton push (momentary) | 3 | Toggle (D8), Decrement (D7), Increment (D2) |
| Breadboard | 1 | Prototipare |
| Cabluri jumper | multiple | Conexiuni |

### Schema de conexiuni

```
                        Arduino Uno
                    ┌─────────────────┐
                    │                 │
              D13 ──┤ PB5        5V  ├───
              D12 ──┤ PB4       GND  ├─── GND comun (breadboard rail)
              D11 ──┤ PB3            │
              D10 ──┤ PB2            │
               D9 ──┤ PB1            │
               D8 ──┤ PB0            │    ← Btn Toggle (LED1)
               D7 ──┤ PD7            │    ← Btn Decrement
               D6 ──┤ PD6            │
               D5 ──┤ PD5            │
               D4 ──┤ PD4            │
               D2 ──┤ PD2            │    ← Btn Increment
                    │                 │
                    └─────────────────┘

    LED Connections:                       Button Connections:
    D13 ──[220Ω]──LED(verde)──GND         D8 ──[BTN]──GND  (toggle, INPUT_PULLUP)
    D12 ──[220Ω]──LED(galben)──GND        D7 ──[BTN]──GND  (decrement, INPUT_PULLUP)
                                           D2 ──[BTN]──GND  (increment, INPUT_PULLUP)
```

### Principiul funcționării LED-urilor

Fiecare LED este conectat în serie cu un rezistor de 220 Ω pentru limitarea curentului:

$$I_{LED} = \frac{V_{CC} - V_{LED}}{R} = \frac{5V - 2V}{220\Omega} \approx 13.6 \text{ mA}$$

### Principiul funcționării butoanelor

Butoanele sunt conectate între pinul Arduino și GND. Pinii sunt configurați cu `INPUT_PULLUP` (rezistor intern de circa 20–50 kΩ la VCC):
- **Buton neapăsat** → pinul citește `HIGH` (pull-up intern la 5V)
- **Buton apăsat** → pinul citește `LOW` (conectat la GND prin buton)

Detecția apăsării se realizează prin **edge detection** (detecția frontului descendent): semnalul este emis doar la tranziția de la neapăsat la apăsat, evitând generarea repetată a evenimentelor.

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

Modulul `Signals` definește **variabilele globale partajate** (semnalele) utilizate în modelul provider/consumer. Spre deosebire de o implementare cu tastatură matriceală (unde un singur `char` ar servi mai multor consumatori), utilizarea butoanelor fizice permite semnale booleane dedicate:

- `sig_btnToggle` — Setat de Task 0 la apăsarea butonului de pe D8. Consumat și resetat de Task 1.
- `sig_btnDec` — Setat de Task 0 la apăsarea butonului de pe D7. Consumat și resetat de Task 3.
- `sig_btnInc` — Setat de Task 0 la apăsarea butonului de pe D2. Consumat și resetat de Task 3.
- `sig_led1State` — Reflectă starea curentă a LED-ului verde (Task 1).
- `sig_led2State` — Reflectă starea curentă a LED-ului galben (Task 2).
- `sig_blinkInterval` — Intervalul de clipire al LED-ului galben. Valoare implicită: 500 ms. Domeniu: 100–2000 ms, pas: 100 ms.

Toate variabilele sunt declarate `volatile` pentru a garanta coerența datelor între ISR și bucla principală.

**Avantajul semnalelor booleane separate:** Fiecare consumer are propriul semnal dedicat, eliminând conflictul în care un task consumă evenimentul altui task (problemă posibilă la utilizarea unui singur `sig_key` partajat).

### 6.3 Modulul Tasks

Conține logica celor 4 task-uri planificate:

**Task 0 — `buttonScan()`:**
- Scanează cele 3 butoane fizice folosind `Button::isPressed()` (edge detection)
- Setează semnalele booleane corespunzătoare: `sig_btnToggle`, `sig_btnDec`, `sig_btnInc`
- Rolul de **provider** pentru toate cele 3 semnale de buton

**Task 1 — `buttonLed()`:**
- **Consumer** al `sig_btnToggle`
- Toggle-ează LED-ul verde și actualizează `sig_led1State`
- **Provider** al `sig_led1State`
- Resetează `sig_btnToggle = false` după consum

**Task 2 — `blinkLed()`:**
- **Consumer** al `sig_led1State` și `sig_blinkInterval`
- Dacă LED-ul 1 este stins → clipește LED-ul galben la intervalul configurat
- Dacă LED-ul 1 este aprins → oprește LED-ul galben
- **Provider** al `sig_led2State`
- Utilizează un contor intern (`task2BlinkCnt`) incrementat cu recurența task-ului (50 ms)

**Task 3 — `stateVariable()`:**
- **Consumer** al `sig_btnInc` și `sig_btnDec`
- **Provider** al `sig_blinkInterval`
- Modifică intervalul cu un pas de 100 ms, între limitele 100–2000 ms
- Resetează fiecare semnal boolean după consum

### 6.4 Modulul SerialCmd (Idle Report)

Acest modul implementează două funcționalități care rulează în **bucla principală** (Idle):

**`idleReport()`:**
- Raportează periodic (la fiecare ~1000 ms) starea tuturor semnalelor prin `printf()`
- Format: `[IDLE] LED1=ON/OFF | LED2=ON/OFF | Blink=XXXms`
- Este consumator pur — citește semnalele fără a le modifica
- Rulează în main loop (nu ca task planificat) deoarece `printf()` este blocant

**`poll()`:**
- Verifică dacă există date pe portul serial
- Comenzi disponibile: `r` (reset interval la 500ms), `+` (increment), `-` (decrement)
- Permite controlul suplimentar prin interfața serial (debugging)

### 6.5 Modulul UartStdio

Realizează **redirectarea** fluxurilor standard C (`stdout`, `stdin`) către interfața UART a Arduino:

- `uart_putchar()` — Transmite un caracter prin `Serial.write()`. Adaugă `\r` înaintea `\n` pentru compatibilitate terminale.
- `uart_getchar()` — Citește un caracter de pe `Serial`. **Blocant** — așteaptă până la disponibilitatea datelor.
- `init()` — Inițializează `Serial` la baud-ul specificat și configurează structurile `FILE` pentru stdout/stdin.

### 6.6 Modulul Button

Driver pentru buton push cu detecție front (edge detection):

- **Configurare:** Pinul configurat cu `INPUT_PULLUP` (pull-up intern ~ 20–50 kΩ)
- **Active LOW:** Butonul apăsat → `digitalRead() == LOW`
- **Edge detection:** `isPressed()` returnează `true` doar o singură dată per apăsare (la tranziția neapăsat→apăsat). Butonul trebuie eliberat înainte de a fi detectat din nou.
- **readRaw():** Returnează starea brută a butonului (pentru scenarii care necesită citire continuă)

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
2. Apăsare buton D8: LED1=ON, LED2 se oprește
3. Apăsare buton D8 din nou: LED1=OFF, LED2 reia clipirea
4. Apăsare buton D2: intervalul crește cu 100ms
5. Apăsare buton D7: intervalul scade cu 100ms
6. În Serial Monitor: raportare periodică la fiecare ~1s
-->

*Secțiunea va fi completată cu rezultatele experimentale.*

---

## 8. Concluzii

<!-- 
TODO: Completați cu concluziile proprii bazate pe rezultatele obținute.

Puncte de discuție sugerate:
- Experiența cu planificarea secvențială non-preemptivă
- Eficacitatea modelului provider/consumer cu semnale booleane
- Avantajele structurii modulare
- Comparație cu alte metode de planificare
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
> - Refactorizarea codului de la tastatură matriceală la butoane fizice
>
> Codul sursă al proiectului a fost dezvoltat de autor și furnizat ca input AI-ului pentru analiză și optimizare.
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
    Scheduler::addTask(Tasks::buttonScan,    50, 0);
    Scheduler::addTask(Tasks::buttonLed,     50, 5);
    Scheduler::addTask(Tasks::blinkLed,      TASK2_REC_MS, 15);
    Scheduler::addTask(Tasks::stateVariable, 50, 25);

    printf("System ready\n");
    printf("T0: BtnScan   rec=50ms  off=0ms\n");
    printf("T1: ButtonLED rec=50ms  off=5ms\n");
    printf("T2: BlinkLED  rec=50ms  off=15ms\n");
    printf("T3: StateVar  rec=50ms  off=25ms\n");
    printf("Idle: Report  (main loop, 1s)\n");
    printf("Btn: D8=toggle D7=dec D2=inc\n");
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
 *  -----------------------------------------------
 *  sig_btnToggle     │ Task 0   │ Task 1
 *  sig_btnDec        │ Task 0   │ Task 3
 *  sig_btnInc        │ Task 0   │ Task 3
 *  sig_led1State     │ Task 1   │ Task 2, Idle
 *  sig_led2State     │ Task 2   │ Idle
 *  sig_blinkInterval │ Task 3   │ Task 2, Idle
 */
#ifndef SIGNALS_H
#define SIGNALS_H

#include <stdint.h>

extern volatile bool    sig_btnToggle;     // Button 1 pressed (toggle LED1)
extern volatile bool    sig_btnDec;        // Button 2 pressed (decrement interval)
extern volatile bool    sig_btnInc;        // Button 3 pressed (increment interval)
extern volatile bool    sig_led1State;     // LED1 state (green)
extern volatile bool    sig_led2State;     // LED2 state (yellow, blink)
extern volatile int16_t sig_blinkInterval; // Blink period in ms

#endif
```

**Signals.cpp:**

```cpp
#include "Signals.h"

volatile bool    sig_btnToggle     = false;
volatile bool    sig_btnDec        = false;
volatile bool    sig_btnInc        = false;
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
    void buttonScan();     // Task 0 — scan 3 buttons (provider)
    void buttonLed();      // Task 1 — toggle LED1 on button press
    void blinkLed();       // Task 2 — blink LED2 when LED1 is OFF
    void stateVariable();  // Task 3 — adjust blink interval
}

#endif
```

**Tasks.cpp:**

```cpp
#include "Tasks.h"
#include "Signals.h"
#include "Led.h"
#include "Button.h"

/* ── Pin assignments (from diagram.json / Wokwi) ──────────── */
#define LED1_PIN       13  // Task 1 — toggle LED  (green)
#define LED2_PIN       12  // Task 2 — blinking LED (yellow)
#define BTN_TOGGLE_PIN  8  // Button: toggle LED1
#define BTN_DEC_PIN     7  // Button: decrement blink interval
#define BTN_INC_PIN     2  // Button: increment blink interval

static Led    led1(LED1_PIN);
static Led    led2(LED2_PIN);
static Button btnToggle(BTN_TOGGLE_PIN);
static Button btnDec(BTN_DEC_PIN);
static Button btnInc(BTN_INC_PIN);

#define TASK2_REC_MS 50
static volatile uint16_t task2BlinkCnt = 0;

#define BLINK_STEP 100
#define BLINK_MIN  100
#define BLINK_MAX  2000

void Tasks::initHardware()
{
    led1.init();
    led2.init();
    btnToggle.init();
    btnDec.init();
    btnInc.init();
}

/* ── Task 0: Button scan (PROVIDER) ──────────────────────── */
void Tasks::buttonScan()
{
    if (btnToggle.isPressed())
        sig_btnToggle = true;

    if (btnDec.isPressed())
        sig_btnDec = true;

    if (btnInc.isPressed())
        sig_btnInc = true;
}

/* ── Task 1: Toggle LED1 on button press ─────────────────── */
void Tasks::buttonLed()
{
    if (sig_btnToggle)
    {
        sig_btnToggle = false;   // consume signal
        led1.toggle();
        sig_led1State = led1.getState();
    }
}

/* ── Task 2: Blink LED2 when LED1 is OFF ─────────────────── */
void Tasks::blinkLed()
{
    if (!sig_led1State)
    {
        // LED1 is OFF → blink LED2 at the interval set by Task 3
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
        // LED1 is ON → keep LED2 OFF
        if (sig_led2State)
        {
            led2.off();
            sig_led2State = false;
        }
        task2BlinkCnt = 0;
    }
}

/* ── Task 3: Adjust blink interval via buttons ───────────── */
void Tasks::stateVariable()
{
    if (sig_btnInc)
    {
        sig_btnInc = false;      // consume signal
        if (sig_blinkInterval + BLINK_STEP <= BLINK_MAX)
            sig_blinkInterval += BLINK_STEP;
    }
    if (sig_btnDec)
    {
        sig_btnDec = false;      // consume signal
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

### Anexa G – Button (Button.h + Button.cpp)

**Button.h:**

```cpp
#ifndef BUTTON_H
#define BUTTON_H

#include <Arduino.h>

class Button
{
public:
    Button(uint8_t pin);
    void init();
    bool isPressed();
    bool readRaw() const;

private:
    uint8_t _pin;
    bool _prevPressed;
};

#endif
```

**Button.cpp:**

```cpp
#include "Button.h"

Button::Button(uint8_t pin) : _pin(pin), _prevPressed(false) {}

void Button::init()
{
    pinMode(_pin, INPUT_PULLUP);
    _prevPressed = false;
}

bool Button::isPressed()
{
    bool pressed = (digitalRead(_pin) == LOW); // Active LOW (pull-up)

    if (pressed && !_prevPressed)
    {
        _prevPressed = pressed;
        return true; // New press detected
    }

    _prevPressed = pressed;
    return false;
}

bool Button::readRaw() const
{
    return (digitalRead(_pin) == LOW);
}
```

### Anexa H – Led (Led.h + Led.cpp)

**Led.h:**

```cpp
#ifndef LED_H
#define LED_H

#include <Arduino.h>

class Led
{
public:
    Led(uint8_t pin);
    void init();
    void on();
    void off();
    void toggle();
    bool getState() const;

private:
    uint8_t _pin;
    bool _state;
};

#endif
```

**Led.cpp:**

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
