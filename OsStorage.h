#pragma once
#include <cstdint>
#include <cassert>
#include <windows.h>
#include <iostream>

class OsStorage {
private:
	class BufferElement {
	public:
		void* pointer;
		size_t size;
		BufferElement(void* pointer, size_t size) {
			this->pointer = pointer;
			this->size = size;
		}
	};
	BufferElement* buffer = nullptr;
	size_t capacity;
	size_t size;
	const size_t INIT_CAPACITY = 256;

public:
	OsStorage() {
	}
	virtual void init() {
		assert(buffer == nullptr && "Already initialized");
		size = 0;
		capacity = INIT_CAPACITY;
		buffer = (BufferElement*)VirtualAlloc(NULL, capacity * sizeof(BufferElement), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	}
	virtual void destroy() {
		assert(buffer != nullptr && "Allocator was not initialized or already destroyed.");
		for (size_t i = 0; i < size; i++) {
			VirtualFree(buffer[i].pointer, 0, MEM_RELEASE);
		}
		VirtualFree(buffer, 0, MEM_RELEASE);
		buffer = nullptr;
	}
	virtual void* alloc(size_t _size) {
		assert(buffer != nullptr && "Allocator was not initialized or already destroyed.");
		void* res = VirtualAlloc(NULL, _size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
		if (size == capacity) {
			capacity *= 2;
			BufferElement* newBuffer = (BufferElement*)VirtualAlloc(NULL, sizeof(BufferElement) * capacity, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
			for (size_t i = 0; i < size; i++) {
				new (newBuffer + i) BufferElement(buffer[i].pointer, buffer[i].size);
			}
			VirtualFree((void*)buffer, 0, MEM_RELEASE);
			buffer = newBuffer;
		}
		new (buffer + size) BufferElement(res, _size);
		size++;
		return res;
	}
	virtual bool free(void* p) {
		assert(buffer != nullptr && "Allocator was not initialized or already destroyed.");
		for (size_t i = 0; i < size; i++) {
			if (buffer[i].pointer == p) {
				VirtualFree(p, 0, MEM_RELEASE);
				buffer[i] = buffer[size - 1];
				size--;
				return true;
			}
		}
		return false;
	}

#ifdef _DEBUG
	virtual void dumpStat() const {
		assert(buffer != nullptr && "Allocator was not initialized or already destroyed.");
		std::cout << "OsStorage dumpStat: \n";
		size_t fillCount = 0;
		for (size_t i = 0; i < size; i++) {
			std::cout << "Address " << buffer[i].pointer << " size " << buffer[i].size << "\n";
			fillCount++;
		}
		std::cout << "Filled: " << fillCount << "\n";
	}
	virtual void dumpBlocks() const {
		assert(buffer != nullptr && "Allocator was not initialized or already destroyed.");
		std::cout << "OsStorage dumpBlocks: \n";
		for (size_t i = 0; i < size; i++) {
			std::cout << "Address " << buffer[i].pointer << " size " << buffer[i].size << "\n";
		}
	}
#endif
};