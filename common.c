#if defined(USE_BAREMETAL) || defined(USE_FREERTOS) || defined(USE_NORTOS)

#include <stdio.h>
#include <stdint.h>

extern uint32_t __HeapStart;
extern uint32_t __HeapEnd;
extern uint32_t RAM_END;
extern uint32_t STACKSIZE;

int print_memory(void)
{
    printf("STACK %X, %X\n", (int)&RAM_END, (int)&STACKSIZE);
    printf("HEAP  %X, %X\n", (int)&__HeapStart, (int)&__HeapEnd);
    return -1;
}

#endif