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

# Basic pipe functionality tests
@test "Basic pipe: ls | grep .c" {
    run bash -c "echo 'ls | grep \".c\"' | $DSH | grep -v 'dsh3>' | grep -v 'cmd loop'"
    [ "$status" -eq 0 ]
    [[ "$output" == *"dsh_cli.c"* ]]
    [[ "$output" == *"dshlib.c"* ]]
}

@test "Basic pipe with quoted string: echo 'hello world' | grep world" {
    run bash -c "echo 'echo \"hello world\" | grep world' | $DSH | grep -v 'dsh3>' | grep -v 'cmd loop'"
    [ "$status" -eq 0 ]
    [[ "$output" == *"hello world"* ]]
}

@test "Pipe with multiple spaces between commands: ls   |    grep .c" {
    run bash -c "echo 'ls   |    grep \".c\"' | $DSH | grep -v 'dsh3>' | grep -v 'cmd loop'"
    [ "$status" -eq 0 ]
    [[ "$output" == *"dsh_cli.c"* ]]
    [[ "$output" == *"dshlib.c"* ]]
}

# Multiple pipe tests
@test "Multiple pipes: ls | grep .c | wc -l" {
    run bash -c "echo 'ls | grep \".c\" | wc -l' | $DSH | grep -v 'dsh3>' | grep -v 'cmd loop'"
    [ "$status" -eq 0 ]
    # Should be at least 2 .c files
    [[ "$output" =~ [2-9] ]]
}

@test "Three pipe chain: cat test_input.txt | grep test | wc -l" {
    run bash -c "echo 'cat test_input.txt | grep test | wc -l' | $DSH | grep -v 'dsh3>' | grep -v 'cmd loop'"
    [ "$status" -eq 0 ]
    [[ "$output" =~ 2 ]]  # Should find 2 lines with "test"
}

@test "Four pipe chain: ls | grep .c | sort | wc -l" {
    run bash -c "echo 'ls | grep \".c\" | sort | wc -l' | $DSH | grep -v 'dsh3>' | grep -v 'cmd loop'"
    [ "$status" -eq 0 ]
    # Same number as the regular pipe should come out
    [[ "$output" =~ [2-9] ]]
}

# Edge cases and error handling
@test "Empty command in pipe chain: ls | | grep .c" {
    run bash -c "echo 'ls | | grep \".c\"' | $DSH"
    [[ "$output" == *"warning: no commands provided"* ]] || 
    [[ "$output" == *"error"* ]]  # Either way should indicate an error condition
}

@test "Command not found in pipe chain: ls | nonexistent | grep .c" {
    run bash -c "echo 'ls | nonexistent_cmd | grep \".c\"' | $DSH"
    [ "$status" -eq 0 ]  # Shell should still exit normally
    [[ "$output" == *"not found"* ]] || 
    [[ "$output" == *"No such file"* ]] || 
    [[ "$output" == *"error"* ]]  # Some kind of error message
}

@test "Too many commands in pipe chain" {
    # Create a command with 9+ pipes (exceeding CMD_MAX=8)
    pipe_cmd="ls | grep c | wc -l | cat | cat | cat | cat | cat | cat | cat"
    run bash -c "echo '$pipe_cmd' | $DSH"
    [[ "$output" == *"error: piping limited to"* ]] || 
    [[ "$output" == *"too many"* ]]
}

# File descriptor handling tests
@test "File descriptor properly closed: yes | head -1" {
    # This test verifies that 'yes' process is properly terminated when 'head' closes its input
    # If file descriptors aren't handled correctly, this could hang
    run timeout 2 bash -c "echo 'yes | head -1' | $DSH | grep -v 'dsh3>' | grep -v 'cmd loop'"
    [ "$status" -eq 0 ]
    [[ "$output" == *"y"* ]]  # Output should just be a single "y"
}

@test "Large data transfer through pipe: seq 1000 | wc -l" {
    run bash -c "echo 'seq 1000 | wc -l' | $DSH | grep -v 'dsh3>' | grep -v 'cmd loop'"
    [ "$status" -eq 0 ]
    [[ "$output" =~ 1000 ]]  # Should show 1000 lines
}

# Input/output redirection cases
@test "File input to pipe: cat test_input.txt | grep test" {
    run bash -c "echo 'cat test_input.txt | grep test' | $DSH | grep -v 'dsh3>' | grep -v 'cmd loop'"
    [ "$status" -eq 0 ]
    [[ "$output" == *"line1 test"* ]]
    [[ "$output" == *"line3 test"* ]]
    [[ "$output" != *"line2 data"* ]]  # Should not show this line
}

# Built-in command interaction with pipes
@test "Built-in command at start of pipe: exit | cat" {
    # This tests that the shell handles built-in commands in pipes appropriately
    run bash -c "echo 'exit | cat' | $DSH"
    # This might either show an error about piping built-ins or might execute
    # as long as the shell exits gracefully, it's handled correctly
    [ "$status" -eq 0 ]
}

@test "Exit command after pipe execution" {
    run bash -c "echo -e 'ls | grep \".c\"\nexit' | $DSH"
    [ "$status" -eq 0 ]
    [[ "$output" == *"exiting"* ]]
}

# Multi-command sequence tests
@test "Multiple separate commands with pipes" {
    run bash -c "echo -e 'ls | grep \".c\"\necho \"test\" | grep test' | $DSH | grep -v 'dsh3>' | grep -v 'cmd loop'"
    [ "$status" -eq 0 ]
    [[ "$output" == *"dshlib.c"* ]]
    [[ "$output" == *"test"* ]]
}

# Argument handling in pipe chains
@test "Commands with multiple arguments: ls -la | grep dsh" {
    run bash -c "echo 'ls -la | grep dsh' | $DSH | grep -v 'dsh3>' | grep -v 'cmd loop'"
    [ "$status" -eq 0 ]
    [[ "$output" == *"dsh"* ]]
}

@test "Commands with quoted arguments: echo \"hello    world\" | grep \"hello    world\"" {
    run bash -c "echo 'echo \"hello    world\" | grep \"hello    world\"' | $DSH | grep -v 'dsh3>' | grep -v 'cmd loop'"
    [ "$status" -eq 0 ]
    [[ "$output" == *"hello    world"* ]]
}

# Memory management and resource cleanup tests
@test "Repeated pipe commands don't cause resource exhaustion" {
    # Run multiple pipe commands in succession to test for descriptor leaks
    command="for i in {1..10}; do echo 'ls -la | grep dsh | wc -l'; done"
    run bash -c "$command | $DSH | grep -v 'dsh3>' | grep -v 'cmd loop'"
    [ "$status" -eq 0 ]
    # All 10 commands should produce output
    count=$(echo "$output" | wc -l)
    [ "$count" -ge 10 ]
}

# Edge case: Empty output through the pipe
@test "Empty output through pipe: grep nonexistent test_input.txt | wc -l" {
    run bash -c "echo 'grep nonexistent test_input.txt | wc -l' | $DSH | grep -v 'dsh3>' | grep -v 'cmd loop'"
    [ "$status" -eq 0 ]
    [[ "$output" =~ 0 ]]  # Should show 0 lines
}

# Standard error handling
@test "Standard error propagation: ls /nonexistent 2>&1 | grep 'No such file'" {
    # This test requires extra credit redirection implementation
    # If not implemented, it will be skipped
    if ! echo "ls > /dev/null" | $DSH 2>/dev/null; then
        skip "Redirection not implemented"
    fi
    
    run bash -c "echo 'ls /nonexistent 2>&1 | grep \"No such file\"' | $DSH | grep -v 'dsh3>' | grep -v 'cmd loop'"
    [ "$status" -eq 0 ]
    [[ "$output" == *"No such file"* ]] || [[ "$output" == *"cannot access"* ]]
}