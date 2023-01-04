#pragma once
#include <cassert>
#include <windows.h>
#include <iostream>
class Coalesce {
private:
	static const size_t PAGE_SIZE = 512 * 100;
	const size_t SHIFT = 8;
	class Header {
	public:
		bool isFree;
		Header* next;
		Header* prev;
		size_t size;
		void makeNotFree() {
			isFree = false;
			next = nullptr;
			prev = nullptr;
		}
	};
	class Footer {
	public:
		Header* header;
	};
	class Page {
	public:
		Page* next;
		Header* freeListHead;
		Header* buffer;
	};
	Page* headPage = nullptr;

	Header* createBlock(Header* h, Footer* f) {
		byte* res = (byte*)h + sizeof(Header);
		if ((size_t)res % SHIFT != 0) {
			res += (SHIFT - (size_t)res % SHIFT);
		}
		if (res >= (byte*)f) return nullptr;
		h = (Header*)(res - sizeof(Header));
		h->isFree = true;
		h->next = nullptr;
		h->prev = nullptr;
		h->size = (byte*)f - ((byte*)h + sizeof(Header));
		f->header = h;
		return h;
	}

	Page* allocNewPage() {
		void* mem = VirtualAlloc(NULL, sizeof(Page) + PAGE_SIZE, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
		Page* newPage = (Page*)mem;
		newPage->next = nullptr;
		Header* header = (Header*)((byte*)newPage + sizeof(Page));
		Footer* footer = (Footer*)((byte*)newPage + sizeof(Page) + PAGE_SIZE);
		header = createBlock(header, footer);
		newPage->freeListHead = header;
		newPage->buffer = header;
		return newPage;
	}

	void removeFromFreeList(Header* curBlock, Page* curPage) {
		if (curBlock == curPage->freeListHead) {
			curPage->freeListHead = nullptr;
			return;
		}
		if (curBlock->prev != nullptr) {
			curBlock->prev->next = curBlock->next;
		}
		if (curBlock->next != nullptr) {
			curBlock->next->prev = curBlock->prev;
		}
	}
	void* trySplitTwoBlocks(Header* curBlock, size_t size, Page* curPage) {
		Footer* rightFooter = (Footer*)((byte*)curBlock + sizeof(Header) + curBlock->size - sizeof(Footer));
		Header* rightHeader = (Header*)((byte*)curBlock + sizeof(Header) + size + sizeof(Footer));
		rightHeader = createBlock(rightHeader, rightFooter);
		if (rightHeader == nullptr) return false;
		removeFromFreeList(curBlock, curPage);
		addToFreeList(curPage, rightHeader);
		Footer* leftFooter = (Footer*)((byte*)rightHeader - sizeof(Footer));
		curBlock = createBlock(curBlock, leftFooter);
		curBlock->makeNotFree();
		byte* res = (byte*)curBlock + sizeof(Header);
		return (void*)res;

	}
	void* tryOneBlock(Header* curBlock, Page* curPage) {
		byte* res = (byte*)curBlock + sizeof(Header);
		removeFromFreeList(curBlock, curPage);
		curBlock->makeNotFree();
		return (void*)res;
	}

	bool pageContainsPointer(Page* curPage, void* p) const {
		return p >= curPage && p < (byte*)curPage + sizeof(Page) + PAGE_SIZE;
	}
	bool isPointerCorrect(Page* curPage, void* p) const {
		return (size_t)p % SHIFT == 0;
	}

	Header* unionBlocks(Header* h1, Header* h2) {
		Footer* f = (Footer*)((byte*)h2 + sizeof(Header) + h2->size);
		f->header = h1;
		h1->size = h1->size + sizeof(Footer) + sizeof(Header) + h2->size;
		return h1;
	}

	void addToFreeList(Page* curPage, Header* block) {
		block->next = curPage->freeListHead;
		if (curPage->freeListHead != nullptr) {
			curPage->freeListHead->prev = block;
		}
		curPage->freeListHead = block;
	}

public:
	Coalesce() {

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
	virtual void* alloc(size_t size) {
		assert(headPage != nullptr && "Allocator was not initialized or already destroyed.");
		Page* curPage = headPage;
		while (true) {
			for (Header* curBlock = curPage->freeListHead; curBlock != nullptr; curBlock = curBlock->next) {
				if (curBlock->size >= size) {
					void* res = trySplitTwoBlocks(curBlock, size, curPage);
					if (res != nullptr) {
						return res;
					}
					res = tryOneBlock(curBlock, curPage);
					if (res != nullptr) {
						return res;
					}
				}
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
			if (!pageContainsPointer(curPage, p)) {
				continue;
			}
			if (!isPointerCorrect(curPage, p)) {
				return false;
			}
			Header* block = (Header*)((byte*)p - sizeof(Header));
			Footer* leftFooter = (Footer*)((byte*)block - sizeof(Footer));
			Header* leftHeader = leftFooter->header;
			Header* rightHeader = (Header*)((byte*)block + sizeof(Header) + block->size + sizeof(Footer));
			if (pageContainsPointer(curPage, leftHeader) && leftHeader->isFree) {
				removeFromFreeList(leftHeader, curPage);
				block = unionBlocks(leftHeader, block);
			}
			if (pageContainsPointer(curPage, rightHeader) && rightHeader->isFree) {
				removeFromFreeList(rightHeader, curPage);
				block = unionBlocks(block, rightHeader);
			}
			block->isFree = true;
			addToFreeList(curPage, block);
			return true;
		}
		return false;
	}

#ifdef _DEBUG
	virtual void dumpStat() const {
		assert(headPage != nullptr && "Allocator was not initialized or already destroyed.");
		std::cout << "Coalesce dumpStat: \n";
		size_t freeCount = 0;
		size_t fillCount = 0;
		for (Page* curPage = headPage; curPage != nullptr; curPage = curPage->next) {
			std::cout << "Page " << curPage << " size " << sizeof(Page) + PAGE_SIZE << "\n";
			for (Header* curBlock = curPage->buffer;
				pageContainsPointer(curPage, curBlock);
				curBlock = (Header*)((byte*)curBlock + sizeof(Header) + curBlock->size + sizeof(Footer))) {
				if (curBlock->isFree) {
					freeCount++;
				}
				else {
					fillCount++;
				}
			}
		}
		std::cout << "Free " << freeCount << " Filled " << fillCount << "\n";
	}
	virtual void dumpBlocks() const {
		assert(headPage != nullptr && "Allocator was not initialized or already destroyed.");
		std::cout << "Coalesce dumpBlocks: \n";
		for (Page* curPage = headPage; curPage != nullptr; curPage = curPage->next) {
			for (Header* curBlock = curPage->buffer;
				pageContainsPointer(curPage, curBlock);
				curBlock = (Header*)((byte*)curBlock + sizeof(Header) + curBlock->size + sizeof(Footer))) {
				if (!curBlock->isFree) {
					byte* res = (byte*)curBlock + sizeof(Header);
					size_t blockSize = curBlock->size;
					std::cout << "Block address " << (void*)res << " size " << blockSize << "\n";
				}
			}
		}
	}
#endif
};