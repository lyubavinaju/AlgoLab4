#include "MemoryAllocator.h"

int main() {
	MemoryAllocator allocator;
	allocator.init();
	int* pi = (int*)allocator.alloc(sizeof(int));
	double* pd = (double*)allocator.alloc(sizeof(double));
	int* pa = (int*)allocator.alloc(10 * sizeof(int));
	allocator.dumpStat();
	allocator.dumpBlocks();
	allocator.free(pa);
	allocator.free(pd);
	allocator.free(pi);
	allocator.destroy();
}