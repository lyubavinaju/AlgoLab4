#pragma once

#include "FSA.h"
#include "Coalesce.h"
#include "OsStorage.h"
#include <windows.h>

class MemoryAllocator {
private:
	static const uint32_t blocksCount = 100;
	static const uint32_t BIG_THRESHOLD = 10485760;
	FSA<16, blocksCount> fsa16;
	FSA<32, blocksCount> fsa32;
	FSA<64, blocksCount> fsa64;
	FSA<128, blocksCount> fsa128;
	FSA<256, blocksCount> fsa256;
	FSA<512, blocksCount> fsa512;
	Coalesce coalesce;
	OsStorage osStorage;
public:
	MemoryAllocator() {

	}
	~MemoryAllocator() {

	}
	virtual void init() {
		fsa16.init();
		fsa32.init();
		fsa64.init();
		fsa128.init();
		fsa256.init();
		fsa512.init();
		coalesce.init();
		osStorage.init();
	}
	virtual void destroy() {
		fsa16.destroy();
		fsa32.destroy();
		fsa64.destroy();
		fsa128.destroy();
		fsa256.destroy();
		fsa512.destroy();
		coalesce.destroy();
		osStorage.destroy();
	}
	virtual void* alloc(size_t size) {
		void* res = nullptr;
		if (size <= 16) {
			res = fsa16.alloc();
		}
		else if (size <= 32) {
			res = fsa32.alloc();
		}
		else if (size <= 64) {
			res = fsa64.alloc();
		}
		else if (size <= 128) {
			res = fsa128.alloc();
		}
		else if (size <= 256) {
			res = fsa256.alloc();
		}
		else if (size <= 512) {
			res = fsa512.alloc();
		}
		else {
			if (size < BIG_THRESHOLD) {
				res = coalesce.alloc(size);
			}
			else {
				res = osStorage.alloc(size);
			}
		}
		return res;
	}
	virtual void free(void* p) {
		if (fsa16.free(p))return;
		if (fsa32.free(p))return;
		if (fsa64.free(p))return;
		if (fsa128.free(p))return;
		if (fsa256.free(p))return;
		if (fsa512.free(p))return;
		if (coalesce.free(p)) return;
		osStorage.free(p);
	}

#ifdef _DEBUG
	virtual void dumpStat() const {
		fsa16.dumpStat();
		fsa32.dumpStat();
		fsa64.dumpStat();
		fsa128.dumpStat();
		fsa256.dumpStat();
		fsa512.dumpStat();
		coalesce.dumpStat();
		osStorage.dumpStat();
	}
	virtual void dumpBlocks() const {
		fsa16.dumpBlocks();
		fsa32.dumpBlocks();
		fsa64.dumpBlocks();
		fsa128.dumpBlocks();
		fsa256.dumpBlocks();
		fsa512.dumpBlocks();
		coalesce.dumpBlocks();
		osStorage.dumpBlocks();
	}
#endif
};