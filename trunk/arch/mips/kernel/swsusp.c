#include <asm/page.h>
#include <linux/preempt.h>

extern const void __nosave_begin, __nosave_end;
void __save_processor_state(void)
{
}

void save_processor_state(void)
{
       __save_processor_state();
}

void __restore_processor_state(void)
{
}

void restore_processor_state(void)
{
       __restore_processor_state();
}

int pfn_is_nosave(unsigned long pfn)
{
	unsigned long nosave_begin_pfn = __pa(&__nosave_begin) >> PAGE_SHIFT;
	unsigned long nosave_end_pfn   = PAGE_ALIGN(__pa(&__nosave_end)) >> PAGE_SHIFT;

	return (pfn >= nosave_begin_pfn) && (pfn < nosave_end_pfn);
}
