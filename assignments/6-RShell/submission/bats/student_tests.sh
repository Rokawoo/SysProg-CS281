#!/usr/bin/env bats

# File: student_tests.sh
# 
# Comprehensive test suite for Remote Shell (dsh)

# Helper functions
setup() {
    # Create test files and directories
    mkdir -p test_dir
    echo "test content" > test_file.txt
    echo "another test file" > test_dir/inner_file.txt

    # Kill any lingering dsh server processes
    pkill -f "./dsh -s" || true
    sleep 1
}

teardown() {
    # Clean up test files and directories
    rm -f test_file.txt
    rm -rf test_dir

    # Kill any lingering dsh server processes
    pkill -f "./dsh -s" || true
    sleep 1
}

# Start server in background and return PID
start_server() {
    local port=$1
    ./dsh -s -p $port &
    echo $!
    sleep 1  # Give server time to start
}

# Basic tests for local shell mode
@test "Local mode: Basic command execution" {
    run ./dsh <<EOF
ls
exit
EOF
    [ "$status" -eq 0 ]
    [[ "$output" == *"dsh4>"* ]]
}

@test "Local mode: Command with arguments" {
    run ./dsh <<EOF
echo hello world
exit
EOF
    [ "$status" -eq 0 ]
    [[ "$output" == *"hello world"* ]]
}

@test "Local mode: Built-in exit command" {
    run ./dsh <<EOF
exit
EOF
    [ "$status" -eq 0 ]
    [[ "$output" == *"exiting..."* ]]
}

@test "Local mode: Built-in cd command" {
    run ./dsh <<EOF
mkdir -p testdir
cd testdir
pwd
cd ..
rmdir testdir
exit
EOF
    [ "$status" -eq 0 ]
    [[ "$output" == *"testdir"* ]]
}

@test "Local mode: Pipe commands" {
    run ./dsh <<EOF
echo hello world | grep hello
exit
EOF
    [ "$status" -eq 0 ]
    [[ "$output" == *"hello world"* ]]
}

@test "Local mode: Multiple pipe commands" {
    run ./dsh <<EOF
echo hello world | grep hello | wc -w
exit
EOF
    [ "$status" -eq 0 ]
    [[ "$output" == *"2"* ]]
}

@test "Local mode: Input redirection" {
    echo "test content" > test_input.txt
    run ./dsh <<EOF
cat < test_input.txt
exit
EOF
    rm -f test_input.txt
    [ "$status" -eq 0 ]
    [[ "$output" == *"test content"* ]]
}

@test "Local mode: Output redirection" {
    run ./dsh <<EOF
echo "redirected output" > test_output.txt
cat test_output.txt
exit
EOF
    [ "$status" -eq 0 ]
    [[ "$output" == *"redirected output"* ]]
    rm -f test_output.txt
}

# Server mode tests
@test "Server mode: Start server" {
    # Start server
    SERVER_PID=$(start_server 5000)
    
    # Check if server is running
    ps -p $SERVER_PID
    [ "$?" -eq 0 ]
    
    # Kill server
    kill $SERVER_PID
}

# Client mode tests with server running
@test "Remote shell: Basic command execution" {
    # Start server
    SERVER_PID=$(start_server 5001)
    
    # Run client
    run ./dsh -c -p 5001 <<EOF
ls
exit
EOF
    
    # Kill server
    kill $SERVER_PID
    
    # Verify output
    [ "$status" -eq 0 ]
    [[ "$output" == *"dsh4>"* ]]
}

@test "Remote shell: Command with arguments" {
    # Start server
    SERVER_PID=$(start_server 5002)
    
    # Run client
    run ./dsh -c -p 5002 <<EOF
echo hello remote world
exit
EOF
    
    # Kill server
    kill $SERVER_PID
    
    # Verify output
    [ "$status" -eq 0 ]
    [[ "$output" == *"hello remote world"* ]]
}

@test "Remote shell: Built-in exit command" {
    # Start server
    SERVER_PID=$(start_server 5003)
    
    # Run client
    run ./dsh -c -p 5003 <<EOF
exit
EOF
    
    # Kill server
    kill $SERVER_PID
    
    # Verify output
    [ "$status" -eq 0 ]
    [[ "$output" == *"exiting..."* ]]
}

@test "Remote shell: stop-server command" {
    # Start server
    SERVER_PID=$(start_server 5004)
    
    # Run client
    run ./dsh -c -p 5004 <<EOF
stop-server
EOF
    
    # Check if server is still running (it shouldn't be)
    sleep 2
    ps -p $SERVER_PID
    [ "$?" -ne 0 ]
}

@test "Remote shell: Simple pipes" {
    # Start server
    SERVER_PID=$(start_server 5005)
    
    # Run client
    run ./dsh -c -p 5005 <<EOF
echo hello remote world | grep remote
exit
EOF
    
    # Kill server
    kill $SERVER_PID
    
    # Verify output
    [ "$status" -eq 0 ]
    [[ "$output" == *"hello remote world"* ]]
}

@test "Remote shell: Complex pipes" {
    # Start server
    SERVER_PID=$(start_server 5006)
    
    # Run client
    run ./dsh -c -p 5006 <<EOF
echo -e "line1\nline2\nline3" | grep line | wc -l
exit
EOF
    
    # Kill server
    kill $SERVER_PID
    
    # Verify output
    [ "$status" -eq 0 ]
    [[ "$output" == *"3"* ]]
}

@test "Remote shell: Directory navigation with cd" {
    # Start server
    SERVER_PID=$(start_server 5007)
    
    # Run client
    run ./dsh -c -p 5007 <<EOF
cd test_dir
ls
cd ..
exit
EOF
    
    # Kill server
    kill $SERVER_PID
    
    # Verify output
    [ "$status" -eq 0 ]
    [[ "$output" == *"inner_file.txt"* ]]
}

@test "Remote shell: Command execution with complex command" {
    # Start server
    SERVER_PID=$(start_server 5008)
    
    # Run client
    run ./dsh -c -p 5008 <<EOF
find . -name "*.txt" | grep test | sort
exit
EOF
    
    # Kill server
    kill $SERVER_PID
    
    # Verify output
    [ "$status" -eq 0 ]
    [[ "$output" == *"test_file.txt"* ]]
}

@test "Remote shell: Multiple command execution" {
    # Start server
    SERVER_PID=$(start_server 5009)
    
    # Run client
    run ./dsh -c -p 5009 <<EOF
echo "Command 1"
echo "Command 2"
echo "Command 3"
exit
EOF
    
    # Kill server
    kill $SERVER_PID
    
    # Verify output
    [ "$status" -eq 0 ]
    [[ "$output" == *"Command 1"* ]]
    [[ "$output" == *"Command 2"* ]]
    [[ "$output" == *"Command 3"* ]]
}

@test "Remote shell: Background processes" {
    # Start server
    SERVER_PID=$(start_server 5010)
    
    # Run client (with a sleep command and then another command to verify the shell doesn't block)
    run timeout 5s ./dsh -c -p 5010 <<EOF
sleep 2 &
echo "Should run immediately"
exit
EOF
    
    # Kill server
    kill $SERVER_PID
    
    # Verify output
    [ "$status" -eq 0 ]
    [[ "$output" == *"Should run immediately"* ]]
}

@test "Remote shell: Command with large output" {
    # Start server
    SERVER_PID=$(start_server 5011)
    
    # Run client
    run ./dsh -c -p 5011 <<EOF
yes "large output test" | head -n 1000
exit
EOF
    
    # Kill server
    kill $SERVER_PID
    
    # Verify output - we just check that there's a lot of output
    [ "$status" -eq 0 ]
    [ "${#output}" -gt 5000 ]
}

@test "Remote shell: Multiple clients (requires threaded mode)" {
    # Skip if not testing threaded mode
    if [ -z "$TEST_THREADED" ]; then
        skip "Skipping threaded test - set TEST_THREADED=1 to run"
    fi
    
    # Start threaded server
    ./dsh -s -p 5012 -x &
    SERVER_PID=$!
    sleep 1
    
    # Run first client in background
    ./dsh -c -p 5012 <<EOF > client1_output.txt &
echo "Client 1"
sleep 3
echo "Client 1 again"
exit
EOF
    CLIENT1_PID=$!
    
    # Run second client
    ./dsh -c -p 5012 <<EOF > client2_output.txt &
echo "Client 2"
sleep 1
echo "Client 2 again"
exit
EOF
    CLIENT2_PID=$!
    
    # Wait for clients to finish
    wait $CLIENT1_PID
    wait $CLIENT2_PID
    
    # Kill server
    kill $SERVER_PID
    
    # Verify outputs
    [[ "$(cat client1_output.txt)" == *"Client 1"* ]]
    [[ "$(cat client1_output.txt)" == *"Client 1 again"* ]]
    [[ "$(cat client2_output.txt)" == *"Client 2"* ]]
    [[ "$(cat client2_output.txt)" == *"Client 2 again"* ]]
    
    # Clean up
    rm -f client1_output.txt client2_output.txt
}

@test "Remote shell: Server stability with rapid client connections" {
    # Start server
    SERVER_PID=$(start_server 5013)
    
    # Connect and disconnect rapidly 5 times
    for i in {1..5}; do
        ./dsh -c -p 5013 <<EOF
echo "Quick client $i"
exit
EOF
    done
    
    # Verify server is still running
    ps -p $SERVER_PID
    [ "$?" -eq 0 ]
    
    # Connect once more to test functionality
    run ./dsh -c -p 5013 <<EOF
echo "Final test"
exit
EOF
    
    # Kill server
    kill $SERVER_PID
    
    # Verify output
    [ "$status" -eq 0 ]
    [[ "$output" == *"Final test"* ]]
}

@test "Remote shell: Handle malformed input" {
    # Start server
    SERVER_PID=$(start_server 5014)
    
    # Run client with incomplete pipe
    run ./dsh -c -p 5014 <<EOF
echo hello |
exit
EOF
    
    # Kill server
    kill $SERVER_PID
    
    # Server should handle this gracefully
    [ "$status" -eq 0 ]
}

@test "Remote shell: Dragon command" {
    # Start server
    SERVER_PID=$(start_server 5015)
    
    # Run client
    run ./dsh -c -p 5015 <<EOF
dragon
exit
EOF
    
    # Kill server
    kill $SERVER_PID
    
    # Verify output contains dragon ASCII art
    [ "$status" -eq 0 ]
    [[ "$output" == *"<>"* ]]  # Part of the dragon ASCII art
}

@test "Remote shell: Return code command" {
    # Start server
    SERVER_PID=$(start_server 5016)
    
    # Run client
    run ./dsh -c -p 5016 <<EOF
false
rc
exit
EOF
    
    # Kill server
    kill $SERVER_PID
    
    # Verify output shows non-zero return code
    [ "$status" -eq 0 ]
    [[ "$output" == *"1"* ]]
}

@test "Remote shell: Input redirection" {
    # Create test file
    echo "remote test content" > remote_test_input.txt
    
    # Start server
    SERVER_PID=$(start_server 5017)
    
    # Run client
    run ./dsh -c -p 5017 <<EOF
cat < remote_test_input.txt
exit
EOF
    
    # Kill server
    kill $SERVER_PID
    
    # Clean up
    rm -f remote_test_input.txt
    
    # Verify output
    [ "$status" -eq 0 ]
    [[ "$output" == *"remote test content"* ]]
}

@test "Remote shell: Output redirection" {
    # Start server
    SERVER_PID=$(start_server 5018)
    
    # Run client
    run ./dsh -c -p 5018 <<EOF
echo "remote redirected output" > remote_test_output.txt
cat remote_test_output.txt
exit
EOF
    
    # Kill server
    kill $SERVER_PID
    
    # Verify output
    [ "$status" -eq 0 ]
    [[ "$output" == *"remote redirected output"* ]]
    
    # Clean up
    rm -f remote_test_output.txt
}

@test "Remote shell: Handle command not found" {
    # Start server
    SERVER_PID=$(start_server 5019)
    
    # Run client with a non-existent command
    run ./dsh -c -p 5019 <<EOF
nonexistentcommand
exit
EOF
    
    # Kill server
    kill $SERVER_PID
    
    # Verify the error is handled gracefully
    [ "$status" -eq 0 ]
    [[ "$output" == *"not found"* ]] || [[ "$output" == *"No such file"* ]]
}

@test "Remote shell: Different port number" {
    # Start server on non-default port
    SERVER_PID=$(start_server 6789)
    
    # Run client connecting to that port
    run ./dsh -c -p 6789 <<EOF
echo "Custom port test"
exit
EOF
    
    # Kill server
    kill $SERVER_PID
    
    # Verify output
    [ "$status" -eq 0 ]
    [[ "$output" == *"Custom port test"* ]]
}

@test "Remote shell: Command with environment variables" {
    # Start server
    SERVER_PID=$(start_server 5020)
    
    # Run client
    run ./dsh -c -p 5020 <<EOF
export TESTVAR="environment variable test"
echo \$TESTVAR
exit
EOF
    
    # Kill server
    kill $SERVER_PID
    
    # Verify output
    [ "$status" -eq 0 ]
    [[ "$output" == *"environment variable test"* ]]
}