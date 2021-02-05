#include <kernel.h>

#define __vdso_dat (&(VVAR_REF(vdso_dat)))

clock_now platform_monotonic_now;
clock_timer platform_timer;
thunk platform_timer_percpu_init;

void init_clock(void)
{
    /* detect rdtscp */
    u32 regs[4];
    cpuid(0x80000001, 0, regs);
    __vdso_dat->clock_src = VDSO_CLOCK_SYSCALL;
    __vdso_dat->platform_has_rdtscp = (regs[3] & U64_FROM_BIT(27)) != 0;
}
