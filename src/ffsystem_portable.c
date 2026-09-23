#include "ff.h"
#include <stdlib.h>

DWORD get_fattime(void)
{
    /* The application may provide a stronger definition when RTC is available. */
    return ((DWORD)(2026U - 1980U) << 25) | ((DWORD)1U << 21) | ((DWORD)1U << 16);
}

void *ff_memalloc(UINT size) { return malloc((size_t)size); }
void ff_memfree(void *memory) { free(memory); }
