mkdir -p bin/Debug
cmake -DCMAKE_INSTALL_PREFIX:PATH=/usr -DCMAKE_BUILD_TYPE=Debug -DEXECUTABLE_OUTPUT_PATH="bin/Debug" $@ .
cmake --build . --config Debug