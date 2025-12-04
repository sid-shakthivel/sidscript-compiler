#!/bin/bash

cd build || exit

make DEBUG=1 || { echo "Build failed."; exit 1; }

./ssc ../test.ss || { echo "Compilation failed."; exit 1; }

arch -x86_64 gcc test.s -o test || { echo "Assembly compilation failed."; exit 1; }

arch -x86_64 ./test || { echo "Execution failed."; exit 1; }

return_value=$?

echo "Return value: $return_value"