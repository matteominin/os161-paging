/*
 * Focused test for allocating, freeing, and reusing one physical frame.
 */

#include <types.h>
#include <lib.h>
#include <vm.h>
#include <test.h>

#define COREMAP_TEST_ITERATIONS 4096

int
coremaptest(int nargs, char **args)
{
	volatile uint32_t *first_page;
	volatile uint32_t *second_page;
	vaddr_t first;
	vaddr_t second;
	vaddr_t replacement;
	uint32_t first_pattern;
	uint32_t second_pattern;
	unsigned i;

	(void)nargs;
	(void)args;

	kprintf("Starting coremap frame reuse test...\n");

	for (i = 0; i < COREMAP_TEST_ITERATIONS; i++) {
		first = alloc_kpages(1);
		second = alloc_kpages(1);
		if (first == 0 || second == 0) {
			panic("coremaptest: allocation pair %u failed\n", i);
		}

		KASSERT((first & (PAGE_SIZE - 1)) == 0);
		KASSERT((second & (PAGE_SIZE - 1)) == 0);
		KASSERT(first >= MIPS_KSEG0 && first < MIPS_KSEG1);
		KASSERT(second >= MIPS_KSEG0 && second < MIPS_KSEG1);
		KASSERT(first != second);

		first_page = (volatile uint32_t *)first;
		second_page = (volatile uint32_t *)second;
		first_pattern = 0x5a5a0000U ^ i;
		second_pattern = 0xa5a50000U ^ i;
		first_page[0] = first_pattern;
		second_page[PAGE_SIZE / sizeof(*second_page) - 1] =
			second_pattern;

		free_kpages(first);
		replacement = alloc_kpages(1);
		if (replacement == 0) {
			panic("coremaptest: replacement allocation %u failed\n", i);
		}
		/* First-fit must reuse FIRST, while SECOND remains allocated. */
		KASSERT(replacement == first);
		KASSERT(second_page[PAGE_SIZE / sizeof(*second_page) - 1] ==
			second_pattern);

		first_page = (volatile uint32_t *)replacement;
		first_page[PAGE_SIZE / sizeof(*first_page) - 1] = ~first_pattern;
		KASSERT(first_page[PAGE_SIZE / sizeof(*first_page) - 1] ==
			~first_pattern);

		free_kpages(replacement);
		free_kpages(second);
	}

	kprintf("Coremap frame reuse test passed\n");
	return 0;
}
