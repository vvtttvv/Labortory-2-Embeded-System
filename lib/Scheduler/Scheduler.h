#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <Arduino.h>

#define MAX_TASKS 8

typedef void (*TaskFunction)(void);

struct Task
{
    TaskFunction function;
    uint16_t recurrence;      // Period in ms
    uint16_t offset;          // Initial offset in ms
    volatile int16_t counter; // Countdown (decremented by ISR)
    volatile bool ready;      // Flag set by ISR, cleared by dispatch
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
