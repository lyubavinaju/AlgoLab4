project(AlgoLab4)

cmake_minimum_required(VERSION 3.14)
set(CMAKE_CXX_STANDART 14)


add_executable(
	AlgoLab4
	main.cpp
        FSA.h
        Coalesce.h
        MemoryAllocator.h
	OsStorage.h
)
