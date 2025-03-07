#!/usr/bin/env bats

# Setup before each test
setup() {
    # Path to the dsh executable
    DSH=./dsh
    # Create test files
    echo "line1 test" > test_input.txt
    echo "line2 data" >> test_input.txt
    echo "line3 test" >> test_input.txt
}

# Teardown after each test
teardown() {
    # Clean up any temporary files
    rm -f test_input.txt
    rm -f test_output.txt
    rm -f test_append.txt
}
