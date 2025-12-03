#!/bin/bash

# Attempt to build the project

cd build || exit

make || { echo "Build failed."; exit 1; }

: '
    For each test file (denoted by the .ss.in extension) in the tests directory
    -   Compile each test file using the ssc compiler
    -   Assemble the generated assembly code using gcc
    -   Execute the compiled binary
'

for filepath in $(find ../tests -name "*.ss"); do
    echo "Running test: $filepath"

    : '
        First determine the appropriate filenames to use
        Names required include:
        -   base_filename: the base name of the test file without path or extension
        -   filename_s: the name of the generated assembly file
        -   filename_e: the name of the compiled executable
        -   filename_out: the name of the output file (to compare against stdout))
    '

    : '
        To get the base filename (from ../tests/test_arrays.ss.in to test_arrays):
        -   Remove the path using parameter expansion i.e. ../tests/test_arrays.ss.in -> test_arrays.ss.in
        -   Remove the .ss.in extension using parameter expansion i.e test_arrays.ss.in -> test_arrays
    '
    test_directory="${filepath%/*}"

    base_filename="${filepath##*/}"      
    base_filename="${base_filename%.ss}"   

    filename_s="${base_filename}.s"
    filename_e="${base_filename}"
    filename_out="${test_directory}/${base_filename}.out"

    # Compile the test file using the ssc compiler
    ./ssc "$filepath" || { echo "Compilation failed for $filepath"; exit 1; }

    : '
        Assemble the generated assembly code 
        Note that we use arch -x86_64 as the generated assembly is for x86_64 architecture
    '

    arch -x86_64 gcc "$filename_s" -o "$filename_e" || { echo "Assembly compilation failed for $filename_s"; exit 1; }

    # Execute the compiled binary and capture output and return value
    output="$(arch -x86_64 "./$filename_e")"
    rtn=$?

    # The full output should include both stdout and the return value
    full_output="$output"
    full_output+=$'\n'"Return value: $rtn"

    # Compare
    if diff -u <(printf "%s" "$full_output") "$filename_out" >/dev/null; then
        echo "PASS"
    else
        echo "FAIL"
        echo "Differences:"
        diff -u <(printf "%s" "$full_output") "$filename_out"
    fi

    echo "-----------------------------------"
    echo

done