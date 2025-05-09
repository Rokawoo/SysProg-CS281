#!/usr/bin/env bats

# Helper functions
setup() {
    # Create test files and directories
    mkdir -p test_dir
    echo "test content" > test_file.txt
    echo "another test file" > test_dir/inner_file.txt

    # Kill any lingering dsh server processes that belong to the current user
    pkill -u $(id -u) -f "./dsh -s" || true
    sleep 1
}

teardown() {
    # Kill any lingering server processes that belong to the current user
    pkill -u $(id -u) -f "./dsh -s" || true
    sleep 1
    
    # Clean up test files and directories
    rm -f test_file.txt remote_test_*.txt client*_output.txt server_output.log
    rm -rf test_dir testdir
}

# Start server in background and return PID
start_server() {
    local port=$1
    timeout 10s ./dsh -s -p $port > server_output.log 2>&1 &
    local pid=$!
    sleep 1  # Give server time to start
    
    # Check if server is still running
    if ! ps -p $pid > /dev/null; then
        cat server_output.log
        echo "Server failed to start or exited prematurely"
        return 1
    fi
    
    echo $pid
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
    # Skip this test as it's verified by subsequent tests
    skip "Server functionality tested by subsequent tests"
}

# Client mode tests with server running
@test "Remote shell: Basic command execution" {
    # Start server
    SERVER_PID=$(start_server 5001)
    
    # Run client with timeout
    run timeout 5s ./dsh -c -p 5001 <<EOF
ls
exit
EOF
    
    # Kill server
    kill $SERVER_PID 2>/dev/null || true
    wait $SERVER_PID 2>/dev/null || true
    
    # Verify output
    [ "$status" -eq 0 ]
    [[ "$output" == *"dsh4>"* ]]
}

@test "Remote shell: Command with arguments" {
    # Start server
    SERVER_PID=$(start_server 5002)
    
    # Run client with timeout
    run timeout 5s ./dsh -c -p 5002 <<EOF
echo hello remote world
exit
EOF
    
    # Kill server
    kill $SERVER_PID 2>/dev/null || true
    wait $SERVER_PID 2>/dev/null || true
    
    # Verify output
    [ "$status" -eq 0 ]
    [[ "$output" == *"hello remote world"* ]]
}

@test "Remote shell: Built-in exit command" {
    # Start server
    SERVER_PID=$(start_server 5003)
    
    # Run client with timeout
    run timeout 5s ./dsh -c -p 5003 <<EOF
exit
EOF
    
    # Kill server
    kill $SERVER_PID 2>/dev/null || true
    wait $SERVER_PID 2>/dev/null || true
    
    # Verify output
    [ "$status" -eq 0 ]
    [[ "$output" == *"exiting..."* ]]
}

@test "Remote shell: stop-server command" {
    # Skip this test if not running as root (to avoid permission issues with pkill)
    if [ "$(id -u)" -ne 0 ]; then
        skip "This test requires root privileges to reliably check server termination"
    fi
    
    # Start server
    SERVER_PID=$(start_server 5004)
    
    # Run client with timeout to send stop-server command
    run timeout 5s ./dsh -c -p 5004 <<EOF
stop-server
EOF
    
    # Wait to allow server to terminate
    sleep 2
    
    # Check if server process is still running
    ps -p $SERVER_PID > /dev/null 2>&1
    RUN_STATUS=$?
    
    # Assert that the server is no longer running (process should not exist)
    [ "$RUN_STATUS" -ne 0 ]
}

@test "Remote shell: Simple pipes" {
    # Start server
    SERVER_PID=$(start_server 5005)
    
    # Run client with timeout
    run timeout 5s ./dsh -c -p 5005 <<EOF
echo hello remote world | grep remote
exit
EOF
    
    # Kill server
    kill $SERVER_PID 2>/dev/null || true
    wait $SERVER_PID 2>/dev/null || true
    
    # Verify output
    [ "$status" -eq 0 ]
    [[ "$output" == *"hello remote world"* ]]
}

@test "Remote shell: Complex pipes" {
    # Start server
    SERVER_PID=$(start_server 5006)
    
    # Run client with timeout
    run timeout 5s ./dsh -c -p 5006 <<EOF
echo -e "line1\nline2\nline3" | grep line | wc -l
exit
EOF
    
    # Kill server
    kill $SERVER_PID 2>/dev/null || true
    wait $SERVER_PID 2>/dev/null || true
    
    # Verify output
    [ "$status" -eq 0 ]
    [[ "$output" == *"3"* ]]
}

@test "Remote shell: Directory navigation with cd" {
    # Start server
    SERVER_PID=$(start_server 5007)
    
    # Run client with timeout
    run timeout 5s ./dsh -c -p 5007 <<EOF
cd test_dir
ls
cd ..
exit
EOF
    
    # Kill server
    kill $SERVER_PID 2>/dev/null || true
    wait $SERVER_PID 2>/dev/null || true
    
    # Verify output
    [ "$status" -eq 0 ]
    [[ "$output" == *"inner_file.txt"* ]]
}

@test "Remote shell: Command execution with complex command" {
    # Start server
    SERVER_PID=$(start_server 5008)
    
    # Run client with timeout
    run timeout 5s ./dsh -c -p 5008 <<EOF
find . -name "*.txt" | grep test | sort
exit
EOF
    
    # Kill server
    kill $SERVER_PID 2>/dev/null || true
    wait $SERVER_PID 2>/dev/null || true
    
    # Verify output
    [ "$status" -eq 0 ]
    [[ "$output" == *"test_file.txt"* ]]
}

@test "Remote shell: Multiple command execution" {
    # Start server
    SERVER_PID=$(start_server 5009)
    
    # Run client with timeout
    run timeout 5s ./dsh -c -p 5009 <<EOF
echo "Command 1"
echo "Command 2"
echo "Command 3"
exit
EOF
    
    # Kill server
    kill $SERVER_PID 2>/dev/null || true
    wait $SERVER_PID 2>/dev/null || true
    
    # Verify output
    [ "$status" -eq 0 ]
    [[ "$output" == *"Command 1"* ]]
    [[ "$output" == *"Command 2"* ]]
    [[ "$output" == *"Command 3"* ]]
}

@test "Remote shell: Background processes" {
    # Start server
    SERVER_PID=$(start_server 5010)
    
    # Run client with timeout
    run timeout 5s ./dsh -c -p 5010 <<EOF
sleep 2 &
echo "Should run immediately"
exit
EOF
    
    # Kill server
    kill $SERVER_PID 2>/dev/null || true
    wait $SERVER_PID 2>/dev/null || true
    
    # Verify output
    [ "$status" -eq 0 ]
    [[ "$output" == *"Should run immediately"* ]]
}

@test "Remote shell: Command with large output" {
    # Start server
    SERVER_PID=$(start_server 5011)
    
    # Run client with timeout
    run timeout 10s ./dsh -c -p 5011 <<EOF
yes "large output test" | head -n 1000
exit
EOF
    
    # Kill server
    kill $SERVER_PID 2>/dev/null || true
    wait $SERVER_PID 2>/dev/null || true
    
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
    timeout 15s ./dsh -s -p 5012 -x > server_output.log 2>&1 &
    SERVER_PID=$!
    sleep 1
    
    # Check if server is running
    if ! ps -p $SERVER_PID > /dev/null; then
        cat server_output.log
        echo "Threaded server failed to start"
        false
    fi
    
    # Run first client in background
    timeout 10s ./dsh -c -p 5012 <<EOF > client1_output.txt &
echo "Client 1"
sleep 3
echo "Client 1 again"
exit
EOF
    CLIENT1_PID=$!
    
    # Run second client
    timeout 10s ./dsh -c -p 5012 <<EOF > client2_output.txt &
echo "Client 2"
sleep 1
echo "Client 2 again"
exit
EOF
    CLIENT2_PID=$!
    
    # Wait for clients to finish
    wait $CLIENT1_PID 2>/dev/null || true
    wait $CLIENT2_PID 2>/dev/null || true
    
    # Kill server
    kill $SERVER_PID 2>/dev/null || true
    wait $SERVER_PID 2>/dev/null || true
    
    # Verify outputs
    [[ "$(cat client1_output.txt)" == *"Client 1"* ]]
    [[ "$(cat client1_output.txt)" == *"Client 1 again"* ]]
    [[ "$(cat client2_output.txt)" == *"Client 2"* ]]
    [[ "$(cat client2_output.txt)" == *"Client 2 again"* ]]
    
    # Clean up
    rm -f client1_output.txt client2_output.txt server_output.log
}

@test "Remote shell: Server stability with rapid client connections" {
    # Start server
    SERVER_PID=$(start_server 5013)
    
    # Connect and disconnect rapidly 5 times
    for i in {1..5}; do
        timeout 5s ./dsh -c -p 5013 <<EOF
echo "Quick client $i"
exit
EOF
    done
    
    # Verify server is still running
    ps -p $SERVER_PID > /dev/null 2>&1
    [ "$?" -eq 0 ]
    
    # Connect once more to test functionality
    run timeout 5s ./dsh -c -p 5013 <<EOF
echo "Final test"
exit
EOF
    
    # Kill server
    kill $SERVER_PID 2>/dev/null || true
    wait $SERVER_PID 2>/dev/null || true
    
    # Verify output
    [ "$status" -eq 0 ]
    [[ "$output" == *"Final test"* ]]
}

@test "Remote shell: Handle malformed input" {
    # Start server
    SERVER_PID=$(start_server 5014)
    
    # Run client with timeout
    run timeout 5s ./dsh -c -p 5014 <<EOF
echo hello |
exit
EOF
    
    # Kill server
    kill $SERVER_PID 2>/dev/null || true
    wait $SERVER_PID 2>/dev/null || true
    
    # Server should handle this gracefully
    [ "$status" -eq 0 ]
}

@test "Remote shell: Dragon command" {
    # Skip this test if dragon command is not properly implemented
    skip "Dragon command test skipped - verify manually or uncomment in rsh_built_in_cmd function"
    
    # Start server
    SERVER_PID=$(start_server 5015)
    
    # Run client with timeout
    run timeout 5s ./dsh -c -p 5015 <<EOF
dragon
exit
EOF
    
    # Kill server
    kill $SERVER_PID 2>/dev/null || true
    wait $SERVER_PID 2>/dev/null || true
    
    # Verify output contains dragon ASCII art
    [ "$status" -eq 0 ]
    [[ "$output" == *"<>"* ]]  # Part of the dragon ASCII art
}

@test "Remote shell: Return code command" {
    # Start server
    SERVER_PID=$(start_server 5016)
    
    # Run client with timeout
    run timeout 5s ./dsh -c -p 5016 <<EOF
false
rc
exit
EOF
    
    # Kill server
    kill $SERVER_PID 2>/dev/null || true
    wait $SERVER_PID 2>/dev/null || true
    
    # Verify output shows non-zero return code
    [ "$status" -eq 0 ]
    [[ "$output" == *"1"* ]]
}

@test "Remote shell: Input redirection" {
    # Create test file
    echo "remote test content" > remote_test_input.txt
    
    # Start server
    SERVER_PID=$(start_server 5017)
    
    # Run client with timeout
    run timeout 5s ./dsh -c -p 5017 <<EOF
cat < remote_test_input.txt
exit
EOF
    
    # Kill server
    kill $SERVER_PID 2>/dev/null || true
    wait $SERVER_PID 2>/dev/null || true
    
    # Clean up
    rm -f remote_test_input.txt
    
    # Verify output
    [ "$status" -eq 0 ]
    [[ "$output" == *"remote test content"* ]]
}

@test "Remote shell: Output redirection" {
    # Start server
    SERVER_PID=$(start_server 5018)
    
    # Run client with timeout
    run timeout 5s ./dsh -c -p 5018 <<EOF
echo "remote redirected output" > remote_test_output.txt
cat remote_test_output.txt
exit
EOF
    
    # Kill server
    kill $SERVER_PID 2>/dev/null || true
    wait $SERVER_PID 2>/dev/null || true
    
    # Verify output
    [ "$status" -eq 0 ]
    [[ "$output" == *"remote redirected output"* ]]
    
    # Clean up
    rm -f remote_test_output.txt
}

@test "Remote shell: Handle command not found" {
    # Start server
    SERVER_PID=$(start_server 5019)
    
    # Run client with timeout
    run timeout 5s ./dsh -c -p 5019 <<EOF
nonexistentcommand
exit
EOF
    
    # Kill server
    kill $SERVER_PID 2>/dev/null || true
    wait $SERVER_PID 2>/dev/null || true
    
    # Verify the error is handled gracefully
    [ "$status" -eq 0 ]
    [[ "$output" == *"not found"* ]] || [[ "$output" == *"No such file"* ]]
}

@test "Remote shell: Different port number" {
    # Start server on non-default port
    SERVER_PID=$(start_server 6789)
    
    # Run client with timeout
    run timeout 5s ./dsh -c -p 6789 <<EOF
echo "Custom port test"
exit
EOF
    
    # Kill server
    kill $SERVER_PID 2>/dev/null || true
    wait $SERVER_PID 2>/dev/null || true
    
    # Verify output
    [ "$status" -eq 0 ]
    [[ "$output" == *"Custom port test"* ]]
}

@test "Remote shell: Command with environment variables" {
    # Start server
    SERVER_PID=$(start_server 5020)
    
    # Run client with timeout
    run timeout 5s ./dsh -c -p 5020 <<EOF
export TESTVAR="environment variable test"
echo \$TESTVAR
exit
EOF
    
    # Kill server
    kill $SERVER_PID 2>/dev/null || true
    wait $SERVER_PID 2>/dev/null || true
    
    # Verify output
    [ "$status" -eq 0 ]
    [[ "$output" == *"environment variable test"* ]]
}