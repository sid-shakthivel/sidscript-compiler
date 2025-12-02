#!/bin/bash

# Attempt to build the project

cd build || exit

make || { echo "Build failed."; exit 1; }

# Gather all test files