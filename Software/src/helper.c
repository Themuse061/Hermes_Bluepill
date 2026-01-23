#include <windows.h>

void delay_ms(int ms) {
    if (ms > 0) {
        Sleep((DWORD)ms);
    }
}
