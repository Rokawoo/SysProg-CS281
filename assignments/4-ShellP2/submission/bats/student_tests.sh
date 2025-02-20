#!/usr/bin/env bats

# File: student_tests.sh

@test "Basic external command execution" {
    run ./dsh <<EOF                
ls
exit
EOF
    [ "$status" -eq 0 ]
    [[ "$output" == *"dsh"* ]] || [[ "$output" == *"bats"* ]]
}

@test "Command with arguments" {
    run ./dsh <<EOF
echo testing multiple arguments
exit
EOF
    [ "$status" -eq 0 ]
    [[ "$output" == *"testing multiple arguments"* ]]
}

@test "Empty command handling" {
    run ./dsh <<EOF

exit
EOF
    [ "$status" -eq 0 ]
    [[ "$output" == *"warning: no commands provided"* ]]
}

@test "Quoted arguments preservation" {
    run ./dsh <<EOF
echo "hello   spaced   world"
exit
EOF
    [ "$status" -eq 0 ]
    [[ "$output" == *"hello   spaced   world"* ]]
}

@test "Multiple quoted arguments" {
    run ./dsh <<EOF
echo "first quoted" "second quoted"
exit
EOF
    [ "$status" -eq 0 ]
    [[ "$output" == *"first quoted second quoted"* ]]
}

@test "Mixed quoted and unquoted arguments" {
    run ./dsh <<EOF
echo regular "quoted stuff" more_regular
exit
EOF
    [ "$status" -eq 0 ]
    [[ "$output" == *"regular quoted stuff more_regular"* ]]
}

@test "Leading and trailing spaces" {
    run ./dsh <<EOF
   echo trim spaces   
exit
EOF
    [ "$status" -eq 0 ]
    [[ "$output" == *"trim spaces"* ]]
    [[ "$output" != *"   echo"* ]]
}

@test "cd with valid directory" {
    run ./dsh <<EOF
cd /tmp
pwd
exit
EOF
    [ "$status" -eq 0 ]
    [[ "$output" == *"/tmp"* ]]
}

@test "cd with no arguments" {
    starting_dir=$(pwd)
    run ./dsh <<EOF
cd
pwd
exit
EOF
    [ "$status" -eq 0 ]
    # Verify pwd shows we're still in the same directory
    [[ "$output" == *"$starting_dir"* ]] || [[ "$output" != *"/tmp"* ]]
}

@test "cd with invalid directory" {
    run ./dsh <<EOF
cd /nonexistent_directory_12345
exit
EOF
    [ "$status" -eq 0 ]
    [[ "$output" == *"No such file"* ]] || [[ "$output" == *"cannot"* ]] || [[ "$output" == *"cd:"* ]]
}

@test "exit command works" {
    run ./dsh <<EOF
exit
echo "This should not execute"
EOF
    [ "$status" -eq 0 ]
    [[ "$output" != *"This should not execute"* ]]
}

# Extra credit tests
@test "Command not found error handling" {
    run ./dsh <<EOF
nonexistentcommand123
rc
exit
EOF
    [ "$status" -eq 0 ]
    [[ "$output" == *"Command not found"* ]] || [[ "$output" == *"not found"* ]]
    [[ "$output" == *"2"* ]] # ENOENT is typically 2
}

@test "rc command shows successful execution" {
    run ./dsh <<EOF
echo success
rc
exit
EOF
    [ "$status" -eq 0 ]
    [[ "$output" == *"success"* ]]
    [[ "$output" == *"0"* ]]
}

@test "rc command persists between commands" {
    run ./dsh <<EOF
nonexistentcommand123
rc
echo test
rc
exit
EOF
    [ "$status" -eq 0 ]
    [[ "$output" == *"2"* ]] # First rc after failed command
    [[ "$output" == *"0"* ]] # Second rc after successful command
}

@test "Permission denied error handling" {
    # Create a non-executable file
    touch /tmp/non_executable_test_file
    chmod -x /tmp/non_executable_test_file
    
    run ./dsh <<EOF
/tmp/non_executable_test_file
rc
exit
EOF
    # Clean up
    rm -f /tmp/non_executable_test_file
    
    [ "$status" -eq 0 ]
    [[ "$output" == *"Permission denied"* ]] || [[ "$output" == *"cannot"* ]]
    [[ "$output" == *"13"* ]] # EACCES is typically 13
}

@test "Complex error code sequence" {
    run ./dsh <<EOF
echo first_success
rc
nonexistentcommand
rc
echo second_success
rc
exit
EOF
    [ "$status" -eq 0 ]
    [[ "$output" == *"0"* ]] # After first echo
    [[ "$output" == *"2"* ]] # After nonexistent command (ENOENT)
    [[ "$output" == *"0"* ]] # After second echo
}