#!/usr/bin/env bats

# File: student_tests.sh
# Complete test suite for dsh shell implementation

@test "Example: check ls runs without errors" {
    run ./dsh <<EOF                
ls
EOF

    # Assertions
    [ "$status" -eq 0 ]
}

@test "Test nonexistent command handling" {
    run ./dsh <<EOF
nonexistentcommand
EOF

    # The command should fail but the shell should continue
    [ "$status" -eq 0 ]
    
    # Check for common error patterns without relying on specific CMD_ERR_EXECUTE variable
    [[ "$output" == *"failed to execute"* ]] || [[ "$output" == *"not found"* ]] || [[ "$output" == *"No such file"* ]] || [[ "$output" == *"cannot"* ]] || [[ "$output" == *"failed"* ]]
}

@test "Test cd with valid directory" {
    current=$(pwd)
    
    run ./dsh <<EOF
cd /tmp
pwd
EOF

    # Shell should exit cleanly
    [ "$status" -eq 0 ]
    
    # Output should contain /tmp
    [[ "$output" == *"/tmp"* ]]
}

@test "Test cd with invalid directory" {
    run ./dsh <<EOF
cd /nonexistentdirectory123456
EOF

    # Shell should continue despite invalid directory
    [ "$status" -eq 0 ]
    
    # Should show some kind of error (implementation dependent)
    [[ "$output" == *"No such file"* ]] || [[ "$output" == *"cannot"* ]] || [[ "$output" == *"error"* ]] || [[ "$output" == *"failed"* ]]
}

@test "Test cd with no arguments" {
    # Get current directory for comparison
    current_dir=$(pwd)
    
    run ./dsh <<EOF
cd
pwd
EOF

    # Shell should exit cleanly
    [ "$status" -eq 0 ]
    
    # Current directory should not change (but we don't check exact output)
    [ "$status" -eq 0 ]
}

@test "Test exit command" {
    run ./dsh <<EOF
exit
EOF

    # Shell should exit cleanly
    [ "$status" -eq 0 ]
}

@test "Test command with multiple arguments" {
    run ./dsh <<EOF
echo hello world testing
EOF

    # Shell should exit cleanly
    [ "$status" -eq 0 ]
    
    # Output should contain the echoed text
    [[ "$output" == *"hello world testing"* ]]
}

@test "Test complex quoted arguments" {
    run ./dsh <<EOF
echo "hello   spaced   world" regular "more   spaces"
EOF

    # Shell should exit cleanly
    [ "$status" -eq 0 ]
    
    # Output should preserve spaces in quotes
    [[ "$output" == *"hello   spaced   world regular more   spaces"* ]]
}

@test "Test empty command" {
    run ./dsh <<EOF

EOF

    # Shell should exit cleanly
    [ "$status" -eq 0 ]
    
    # Should handle empty command gracefully
    [[ "$output" != *"Segmentation fault"* ]]
}

@test "Test command with many arguments" {
    run ./dsh <<EOF
echo arg1 arg2 arg3 arg4 arg5 arg6 arg7 arg8 arg9 arg10
EOF

    # Just test that the shell doesn't crash with many arguments
    [ "$status" -eq 0 ]
    
    # Check that at least some arguments appear in output
    [[ "$output" == *"arg1"* ]] && [[ "$output" == *"arg10"* ]]
}

@test "Test command with input/output redirection" {
    # This is testing that the shell doesn't crash with redirection
    # Full redirection implementation isn't required for this assignment
    run ./dsh <<EOF
echo test > /dev/null
EOF

    # Shell should continue even if redirection isn't implemented
    [ "$status" -eq 0 ]
}

@test "Test nested quotes handling" {
    run ./dsh <<EOF
echo "outer \"inner\" quotes"
EOF

    # Shell should exit cleanly
    [ "$status" -eq 0 ]
}

@test "Test command execution with environmental variables" {
    run ./dsh <<EOF
echo $HOME
EOF

    # Shell should exit cleanly
    [ "$status" -eq 0 ]
    
    # Should show $HOME literally or the actual home path
    [[ "$output" == *"$HOME"* ]] || [[ "$output" == *"/"* ]]
}