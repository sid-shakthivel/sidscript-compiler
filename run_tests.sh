#!/bin/bash

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color
BOLD='\033[1m'

# Symbols
CHECK="✓"
CROSS="✗"
ARROW="→"

echo -e "${BOLD}${CYAN}╔════════════════════════════════════════╗${NC}"
echo -e "${BOLD}${CYAN}║     SidScript Compiler Test Suite     ║${NC}"
echo -e "${BOLD}${CYAN}╚════════════════════════════════════════╝${NC}"
echo

# Attempt to build the project
echo -e "${BLUE}${ARROW} Building project...${NC}"
cd build || exit

make || { echo -e "${RED}Build failed${NC}"; exit 1; }
echo -e "${GREEN}Build successful${NC}\n"

make || { echo "Build failed."; exit 1; }

: '
    For each test file (denoted by the .ss.in extension) in the tests directory
    -   Compile each test file using the ssc compiler
    -   Assemble the generated assembly code using gcc
    -   Execute the compiled binary
'

for filepath in $(find ../tests -name "*.ss"); do
    echo -e "${BLUE}Testing: ${filepath}${NC}"

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
    ./ssc "$filepath" > /dev/null 2>&1 || {
        echo -e "${RED}✗ Compilation failed${NC}\n"
        ((failed++))
        continue
    }

    : '
        Assemble the generated assembly code 
        Note that we use arch -x86_64 as the generated assembly is for x86_64 architecture
    '

    arch -x86_64 gcc "$filename_s" -o "$filename_e" 2>/dev/null || {
        echo -e "${RED}✗ Assembly failed${NC}\n"
        rm -f "$filename_s"
        ((failed++))
        continue
    }

    # Execute the compiled binary and capture output and return value
    output="$(arch -x86_64 "./$filename_e")"
    rtn=$?

    # The full output should include both stdout and the return value
    full_output="$output"
    full_output+=$'\n'"Return value: $rtn"

    # Compare
    if diff -u <(printf "%s" "$full_output") "$filename_out" >/dev/null; then
        echo -e "${GREEN}✓ PASSED${NC}\n"
    else
        echo -e "${RED}✗ FAILED${NC}"
        diff -u "$filename_out" <(printf "%s" "$full_output") | tail -n +3
        echo
    fi

    # Clean up generated files
    rm "$filename_s" "$filename_e"

    echo "-----------------------------------"
    echo

done