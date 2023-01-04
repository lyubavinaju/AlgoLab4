set BUILD_DIR=build_x64_VS2022
rmdir /S /Q %BUILD_DIR%
mkdir %BUILD_DIR%
cd %BUILD_DIR%
cmake ../ -DCMAKE_BUILD_TYPE=Debug -G "Visual Studio 17 2022"
cmake --build . --config=Debug -j
cmake ../ -DCMAKE_BUILD_TYPE=Release -G "Visual Studio 17 2022"
cmake --build . --config=Release -j
cd ..