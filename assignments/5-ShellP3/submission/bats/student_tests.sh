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
    rm -f combined.txt
}

# BASIC PIPE TESTS

@test "Basic pipe: ls | grep .c" {
    run bash -c "echo 'ls | grep \".c\"' | $DSH | grep -v 'dsh3>' | grep -v 'cmd loop'"
    [ "$status" -eq 0 ]
    [[ "$output" == *"dsh_cli.c"* ]]
    [[ "$output" == *"dshlib.c"* ]]
}

@test "Multiple pipes: ls | grep .c | wc -l" {
    run bash -c "echo 'ls | grep \".c\" | wc -l' | $DSH | grep -v 'dsh3>' | grep -v 'cmd loop'"
    [ "$status" -eq 0 ]
    # Should be at least 2 .c files
    [[ "$output" =~ [2-9] ]]
}

@test "Four pipe chain: ls | grep .c | sort | wc -l" {
    run bash -c "echo 'ls | grep \".c\" | sort | wc -l' | $DSH | grep -v 'dsh3>' | grep -v 'cmd loop'"
    [ "$status" -eq 0 ]
    # Same number as the regular pipe should come out
    [[ "$output" =~ [2-9] ]]
}

# INPUT REDIRECTION TESTS

@test "Input redirection: cat < test_input.txt" {
    run bash -c "echo 'cat < test_input.txt' | $DSH | grep -v 'dsh3>' | grep -v 'cmd loop'"
    [ "$status" -eq 0 ]
    [[ "$output" == *"line1 test"* ]]
    [[ "$output" == *"line2 data"* ]]
    [[ "$output" == *"line3 test"* ]]
}

@test "Input redirection with command arguments: grep test < test_input.txt" {
    run bash -c "echo 'grep test < test_input.txt' | $DSH | grep -v 'dsh3>' | grep -v 'cmd loop'"
    [ "$status" -eq 0 ]
    [[ "$output" == *"line1 test"* ]]
    [[ "$output" == *"line3 test"* ]]
    [[ "$output" != *"line2 data"* ]]
}

@test "Input redirection with quoted filename: cat < \"test_input.txt\"" {
    run bash -c "echo 'cat < \"test_input.txt\"' | $DSH | grep -v 'dsh3>' | grep -v 'cmd loop'"
    [ "$status" -eq 0 ]
    [[ "$output" == *"line1 test"* ]]
    [[ "$output" == *"line2 data"* ]]
}

# OUTPUT REDIRECTION TESTS

@test "Output redirection: echo hello > test_output.txt" {
    run bash -c "echo 'echo hello > test_output.txt' | $DSH"
    [ "$status" -eq 0 ]
    run cat test_output.txt
    [[ "$output" == "hello" ]]
}

@test "Output redirection with quoted text: echo \"hello world\" > test_output.txt" {
    run bash -c "echo 'echo \"hello world\" > test_output.txt' | $DSH"
    [ "$status" -eq 0 ]
    run cat test_output.txt
    [[ "$output" == "hello world" ]]
}

@test "Output redirection truncates existing file: echo new > test_output.txt" {
    # First, create a file with content
    echo "original content" > test_output.txt
    
    # Then, overwrite it
    run bash -c "echo 'echo new > test_output.txt' | $DSH"
    [ "$status" -eq 0 ]
    
    # Check that the file was truncated
    run cat test_output.txt
    [[ "$output" == "new" ]]
    [[ "$output" != *"original content"* ]]
}

# APPEND REDIRECTION TESTS

@test "Append redirection: echo line1 >> test_append.txt" {
    run bash -c "echo 'echo line1 >> test_append.txt' | $DSH"
    [ "$status" -eq 0 ]
    run cat test_append.txt
    [[ "$output" == "line1" ]]
}

@test "Multiple append operations: create and append to file" {
    run bash -c "echo -e 'echo first > test_append.txt\necho second >> test_append.txt\necho third >> test_append.txt' | $DSH"
    [ "$status" -eq 0 ]
    run cat test_append.txt
    [[ "$output" == *"first"* ]]
    [[ "$output" == *"second"* ]]
    [[ "$output" == *"third"* ]]
    
    # Verify order is correct
    line1=$(head -1 test_append.txt)
    line2=$(head -2 test_append.txt | tail -1)
    line3=$(tail -1 test_append.txt)
    
    [[ "$line1" == "first" ]]
    [[ "$line2" == "second" ]]
    [[ "$line3" == "third" ]]
}

# COMBINED REDIRECTION TESTS

@test "Input and output redirection: grep test < test_input.txt > test_output.txt" {
    run bash -c "echo 'grep test < test_input.txt > test_output.txt' | $DSH"
    [ "$status" -eq 0 ]
    run cat test_output.txt
    [[ "$output" == *"line1 test"* ]]
    [[ "$output" == *"line3 test"* ]]
    [[ "$output" != *"line2 data"* ]]
}

@test "Redirection with multiple arguments: cat test_input.txt > test_output.txt" {
    run bash -c "echo 'cat test_input.txt > test_output.txt' | $DSH"
    [ "$status" -eq 0 ]
    run cat test_output.txt
    [[ "$output" == *"line1 test"* ]]
    [[ "$output" == *"line2 data"* ]]
    [[ "$output" == *"line3 test"* ]]
}

# PIPE WITH REDIRECTION TESTS

@test "Pipe with input redirection: cat < test_input.txt | grep test" {
    run bash -c "echo 'cat < test_input.txt | grep test' | $DSH | grep -v 'dsh3>' | grep -v 'cmd loop'"
    [ "$status" -eq 0 ]
    [[ "$output" == *"line1 test"* ]]
    [[ "$output" == *"line3 test"* ]]
    [[ "$output" != *"line2 data"* ]]
}

@test "Pipe with output redirection: ls | grep .c > test_output.txt" {
    run bash -c "echo 'ls | grep \".c\" > test_output.txt' | $DSH"
    [ "$status" -eq 0 ]
    run cat test_output.txt
    [[ "$output" == *"dsh_cli.c"* ]]
    [[ "$output" == *"dshlib.c"* ]]
}

@test "Pipe with append redirection: ls | grep .c >> test_append.txt" {
    # First create a file
    echo "Original content" > test_append.txt
    
    # Run the pipe with append
    run bash -c "echo 'ls | grep \".c\" >> test_append.txt' | $DSH"
    [ "$status" -eq 0 ]
    
    # Verify original content is preserved and new content is appended
    run cat test_append.txt
    [[ "$output" == *"Original content"* ]]
    [[ "$output" == *"dsh_cli.c"* ]]
    [[ "$output" == *"dshlib.c"* ]]
}

@test "Complex pipe with redirection: cat < test_input.txt | grep test | wc -l > test_output.txt" {
    run bash -c "echo 'cat < test_input.txt | grep test | wc -l > test_output.txt' | $DSH"
    [ "$status" -eq 0 ]
    run cat test_output.txt
    [[ "$output" =~ 2 ]]  # Should find 2 lines with "test"
}

# ERROR HANDLING TESTS

@test "Error handling: redirection with nonexistent input file" {
    run bash -c "echo 'cat < nonexistent_file.txt' | $DSH"
    [ "$status" -eq 0 ]
    [[ "$output" == *"No such file"* ]] || [[ "$output" == *"cannot open"* ]]
}

@test "Error handling: redirection to invalid output path" {
    run bash -c "echo 'echo hello > /invalid/path/file.txt' | $DSH"
    [ "$status" -eq 0 ]
    [[ "$output" == *"No such file"* ]] || [[ "$output" == *"Permission denied"* ]]
}

# EDGE CASES

@test "Multiple redirections in one command: sort < test_input.txt > test_output.txt" {
    run bash -c "echo 'sort < test_input.txt > test_output.txt' | $DSH"
    [ "$status" -eq 0 ]
    run cat test_output.txt
    [[ "$output" == *"line1 test"* ]]
    [[ "$output" == *"line2 data"* ]]
    [[ "$output" == *"line3 test"* ]]
    
    # Verify sort worked
    first_line=$(head -1 test_output.txt)
    [[ "$first_line" == "line1 test" ]]
}

@test "Spaces around redirection operators: echo hello  >  test_output.txt" {
    run bash -c "echo 'echo hello  >  test_output.txt' | $DSH"
    [ "$status" -eq 0 ]
    run cat test_output.txt
    [[ "$output" == "hello" ]]
}

@test "Built-in command with redirection: echo hello > test_output.txt && cat test_output.txt" {
    run bash -c "echo -e 'echo hello > test_output.txt\ncat test_output.txt' | $DSH | grep -v 'dsh3>' | grep -v 'cmd loop'"
    [ "$status" -eq 0 ]
    [[ "$output" == *"hello"* ]]
}