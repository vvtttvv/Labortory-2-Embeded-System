#ifndef TASKS_H
#define TASKS_H

namespace Tasks
{
    void initHardware();
    void buttonMonitor();  // Task 1 — detect press, measure duration, green/red LED
    void pressStats();     // Task 2 — count presses, yellow LED blink
    void periodicReport(); // Task 3 — STDIO report every 10 s
}

#endif
