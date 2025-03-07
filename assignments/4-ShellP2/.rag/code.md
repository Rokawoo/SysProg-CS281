# Code Grading Instructions

- dshlib.c must implement a exec_local_cmd_loop() func that:
  - runs a loop collecting user input
  - parses commands and arguments from each line of input
  - determines if the parsed command is an "external command"
- the parsed commands must:
  - have trailing and leading whitespace removed
  - must preserve whitepsace between double quotes (i.e. perserve echo "hello     world")
  - must treat double-quoted string as a single argument
- the code must implement "cd" as an internal command
  - if cd is provided with arguments, attmpt to change the current process working directory to the provided argument
  - if no argument is provided to cd, it should do nothing
- the code must implement "exit" as an internal command, exiting with a return code of 0
- for all other commands, they should be treated as external and executed with fork/exec
- external commands with fork/exec do not require error code checking for this assignment, accept for extra credit (defined below)
- do not reduce grade for missing error checking, except for the extra credit
- students are not required to implement redirection or pipes in this assignment
- extra credit, these are optional requirements:
  - check for all file-related status code (see errno.h) that you might expect when trying to invoke a binary from $PATH, for example when the file does not exist or when it is not executable
  - print a suitable message for each detected file-related error
  - implement a "rc" builtin command that prints the error code of the last operation that failed (i.e. mimic behavior of "echo $?")

# Possible Grade

- 40 points:  Correct implementation of required functionality
- 5 points:  Code quality (how easy is your solution to follow)
- 10 points:  [EXTRA CREDIT] handle return codes for execvp