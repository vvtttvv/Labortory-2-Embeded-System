#include "Scheduler.h"

Task Scheduler::_tasks[MAX_TASKS];
uint8_t Scheduler::_taskCount = 0;

void Scheduler::init()
{
    _taskCount = 0;

    cli();

    TCCR2A = 0;
    TCCR2B = 0;
    TCCR2A |= (1 << WGM21);  // CTC mode  (clear on compare match)
    TCCR2B |= (1 << CS22);   // Prescaler = 64
    OCR2A = 249;             // Compare match value
    TIMSK2 |= (1 << OCIE2A); // Enable Timer2 Compare-A interrupt
    TCNT2 = 0;               // Reset counter

    sei(); // Re-enable interrupts
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
            _tasks[i].function(); // Execute task in main-loop context
        }
    }
}

// It's called from Timer2 ISR every 1 ms
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

//  Timer2 Compare Match A — 1 ms ISR
ISR(TIMER2_COMPA_vect)
{
    Scheduler::tick();
}
