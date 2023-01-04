#pragma once
#include <cstdint>
#include <windows.h>
#include <cstddef>
#include <cassert>
#include <iostream>

template <uint32_t blockSize, uint32_t blocksCount>
class FSA {
private:
	const uint32_t SHIFT = 8;
	class Page {
	public:
		Page* next;
		uint32_t freeListHeadIndex;
		uint32_t initialized;
		uint32_t filledBlocks;
	};

	Page* headPage = nullptr;

	Page* allocNewPage() {
		void* mem = VirtualAlloc(NULL, SHIFT + sizeof(Page) + blockSize * blocksCount, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
		Page* newPage = (Page*)mem;
		newPage->next = nullptr;
		newPage->freeListHeadIndex = -1;
		newPage->initialized = 0;
		newPage->filledBlocks = 0;
		return newPage;
	}

	byte* getFirstBlock(Page* curPage) const {
		byte* firstBlock = (byte*)curPage + sizeof(Page);
		return firstBlock + (SHIFT - ((size_t)firstBlock % SHIFT));
	}

	void* getIBlock(Page* curPage, uint32_t i) const {
		return (void*)(getFirstBlock(curPage) + blockSize * i);
	}

	bool pageContainsPointer(Page* curPage, void* p) const {
		return p >= getIBlock(curPage, 0) && p <= getIBlock(curPage, blocksCount - 1);
	}
	bool isPointerCorrect(Page* curPage, void* p) const {
		if ((size_t)p % SHIFT != 0) return false;
		std::ptrdiff_t diff = (byte*)p - getFirstBlock(curPage);
		return diff % blockSize == 0;
	}
public:
	FSA() {

	}
	virtual void init() {
		assert(headPage == nullptr && "Already initialized");
		headPage = allocNewPage();
	}
	virtual void destroy() {
		assert(headPage != nullptr && "Allocator was not initialized or already destroyed.");
		Page* cur = headPage;
		while (cur != nullptr) {
			Page* next = cur->next;
			VirtualFree((void*)cur, 0, MEM_RELEASE);
			cur = next;
		}
	}

	virtual void* alloc() {
		assert(headPage != nullptr && "Allocator was not initialized or already destroyed.");

		Page* curPage = headPage;
		while (true) {
			if (curPage->freeListHeadIndex != -1) {
				void* block = getIBlock(curPage, curPage->freeListHeadIndex);
				curPage->freeListHeadIndex = *((uint32_t*)block);
				curPage->filledBlocks++;
				return block;
			}
			if (curPage->initialized < blocksCount) {
				void* block = getIBlock(curPage, curPage->initialized);
				curPage->initialized++;
				curPage->filledBlocks++;
				return (void*)block;
			}
			if (curPage->next == nullptr) {
				curPage->next = allocNewPage();
			}
			curPage = curPage->next;
		}

	}
	virtual bool free(void* p) {
		assert(headPage != nullptr && "Allocator was not initialized or already destroyed.");
		for (Page* curPage = headPage; curPage != nullptr; curPage = curPage->next) {
			if (pageContainsPointer(curPage, p)) {
				if (!isPointerCorrect(curPage, p)) {
					return false;
				}
				*((uint32_t*)p) = curPage->freeListHeadIndex;
				curPage->freeListHeadIndex = (uint32_t)((byte*)p - getFirstBlock(curPage)) / blockSize;
				curPage->filledBlocks--;
				return true;
			}
		}
		return false;
	}
#ifdef _DEBUG
	virtual void dumpStat() const {
		assert(headPage != nullptr && "Allocator was not initialized or already destroyed.");
		std::cout << "FSA dumpStat: \n";
		uint32_t freeCount = 0;
		uint32_t fillCount = 0;
		for (Page* curPage = headPage; curPage != nullptr; curPage = curPage->next) {
			fillCount += curPage->filledBlocks;
			freeCount += blocksCount - curPage->filledBlocks;
		}
		std::cout << "Free " << freeCount << " Filled " << fillCount << "\n";
		for (Page* curPage = headPage; curPage != nullptr; curPage = curPage->next) {
			std::cout << "Page " << curPage << " size " << SHIFT + sizeof(Page) + blockSize * blocksCount << "\n";
		}
	}
	virtual void dumpBlocks() const {
		assert(headPage != nullptr && "Allocator was not initialized or already destroyed.");
		std::cout << "FSA dumpblocks: \n";
		bool* isFree = (bool*)VirtualAlloc(NULL, sizeof(bool) * blocksCount, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
		assert(isFree != NULL);
		Page* curPage = headPage;
		while (curPage != nullptr) {
			uint32_t freeCount = 0;
			for (uint32_t i = 0; i < blocksCount; i++) {
				isFree[i] = false;
			}
			uint32_t i = curPage->freeListHeadIndex;
			while (i != -1) {
				isFree[i] = true;
				freeCount++;
				i = *((uint32_t*)getIBlock(curPage, i));
			}
			assert(freeCount + curPage->filledBlocks == curPage->initialized && "Wrong stat");
			for (uint32_t i = 0; i < curPage->initialized; i++) {
				if (!isFree[i]) {
					std::cout << "Block address " << getIBlock(curPage, i) << " size " << blockSize << "\n";
				}
			}

			curPage = curPage->next;
		}

		VirtualFree((void*)isFree, 0, MEM_RELEASE);
	}
#endif
};