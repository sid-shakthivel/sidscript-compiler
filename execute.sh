#!/bin/bash

cd build || exit

make

if [ $? -ne 0 ]; then
    echo "Build failed."
    exit 1
fi

./ssc

arch -x86_64 gcc test.s -o test

if [ $? -ne 0 ]; then
    echo "Assembly compilation failed."
    exit 1
fi

arch -x86_64 ./test

return_value=$?

echo "Return value: $return_value"