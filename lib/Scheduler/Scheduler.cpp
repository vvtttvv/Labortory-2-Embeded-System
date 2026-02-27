#include "Scheduler.h"

static Task_t tasks[MAX_TASKS];
static uint8_t taskCount = 0;

// -------- Timer2 setup: CTC, 1 ms tick --------
void scheduler_init(void)
{
    taskCount = 0;

    cli();

    TCCR2A = 0;
    TCCR2B = 0;
    TCCR2A |= (1 << WGM21);  // CTC mode
    TCCR2B |= (1 << CS22);   // Prescaler = 64
    OCR2A   = 249;            // 16 MHz / 64 / 250 = 1 kHz => 1 ms
    TIMSK2 |= (1 << OCIE2A); // Enable compare-match A interrupt
    TCNT2   = 0;

    sei();
}

// -------- Register a new task --------
uint8_t scheduler_addTask(void (*func)(void), int rec, int offset)
{
    if (taskCount >= MAX_TASKS)
        return 0xFF;

    Task_t &t  = tasks[taskCount];
    t.task_func = func;
    t.rec       = rec;
    t.offset    = offset;
    t.rec_cnt   = (offset > 0) ? offset : rec;   // first fire after offset
    t.ready     = false;

    return taskCount++;
}

// -------- Main-loop dispatcher --------
void scheduler_loop(void)
{
    for (uint8_t i = 0; i < taskCount; i++)
    {
        if (tasks[i].ready)
        {
            tasks[i].ready = false;
            tasks[i].task_func();     // execute in main-loop context
        }
    }
}

// -------- Called from Timer2 ISR every 1 ms --------
void scheduler_tick(void)
{
    for (uint8_t i = 0; i < taskCount; i++)
    {
        if (--tasks[i].rec_cnt <= 0)
        {
            tasks[i].ready   = true;
            tasks[i].rec_cnt = tasks[i].rec;    // reload period
        }
    }
}

// -------- Timer2 Compare Match A — 1 ms ISR --------
ISR(TIMER2_COMPA_vect)
{
    scheduler_tick();
}
