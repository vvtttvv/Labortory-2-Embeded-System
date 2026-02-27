#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <Arduino.h>

#define MAX_TASKS 8

typedef struct {
    void (*task_func)(void);
    int rec; // period (recurrence) in ms
    int offset; // initial offset in ms
    int rec_cnt; // countdown counter (decremented by ISR)
    volatile bool ready; // flag: ISR sets, dispatch clears
} Task_t;

void scheduler_init(void);
uint8_t scheduler_addTask(void (*func)(void), int rec, int offset);
void scheduler_loop(void);
void scheduler_tick(void);     // called from Timer2 ISR every 1 ms

#endif
