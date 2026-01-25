#include <windows.h>
#include <stdio.h>

void delay_ms(int ms)
{
    if (ms > 0)
    {
        Sleep((DWORD)ms);
    }
}

static LARGE_INTEGER g_starting_tick;
static LARGE_INTEGER g_frequency;

void helper_start_timer()
{
    // 1. Get ticks per second (Frequency)
    QueryPerformanceFrequency(&g_frequency);

    // 2. Save current tick count
    QueryPerformanceCounter(&g_starting_tick);
}

void helper_end_timer()
{
    LARGE_INTEGER ending_tick;

    // 1. Get current tick count
    QueryPerformanceCounter(&ending_tick);

    // 2. Calculate the difference in ticks
    long long elapsed_ticks = ending_tick.QuadPart - g_starting_tick.QuadPart;

    // 3. Convert ticks to Seconds
    //    (ticks / ticks_per_second)
    double elapsed_seconds = (double)elapsed_ticks / (double)g_frequency.QuadPart;

    // 4. Print to 2 decimal places (e.g. "1.25 seconds")
    printf(">> Time elapsed: %.2f seconds\n", elapsed_seconds);
}