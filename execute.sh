#!/bin/bash

cd build || exit

make

if [ $? -ne 0 ]; then
    echo "Build failed."
    exit 1
fi

./ssc ../tests/hey.ss ../tests/test.ss 

arch -x86_64 gcc test.s hey.s -o test

if [ $? -ne 0 ]; then
    echo "Assembly compilation failed."
    exit 1
fi

arch -x86_64 ./test

return_value=$?

echo "Return value: $return_value"