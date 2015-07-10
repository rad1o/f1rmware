#include <stdint.h>
#include <stddef.h>
/*
 * sbrk -- changes heap size size. Get nbytes more
 *         RAM. We just increment a pointer in what's
 *         left of memory on the board.
 */

/* just in case, most boards have at least some memory */

extern void * _end;
void * _sbrk(ptrdiff_t __incr) {
	static void * heap_ptr = NULL;
	void *        base;

	if (heap_ptr == NULL) {
		heap_ptr = (void *)&_end;
	}

    /* XXX: Maybe check heap size collision with stack ? */
	base = heap_ptr;
	heap_ptr += __incr;
	return (base);
}
