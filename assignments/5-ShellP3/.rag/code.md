# Code Grading Instructions

Use the requirements and possible grade below. If the requirements are not meant, deduct points from the possible grade.

# REQUIREMENTS

- distinquish internal vs. external commands
- internal commands are implemented in C code
- external commands must be run with fork/exec
- MUST IMPLEMENT INTERNAL COMMANDS:
  - implement "cd" internal command
    - if cd is provided with arguments, attmpt to change the current process working directory to the provided argument
    - if no argument is provided to cd, it should do nothing
  - the code must implement "exit" as an internal command, exiting with a return code of 0
- MUST IMPLEMENT EXTERNAL COMMANDS:
  - all commands that are not internal are considered external
  - if more than one command is provided separated by the pipe char |, those commands must be executed with fork/exec AND be connected with pipes

# OPTIONAL EXTRA CREDIT REQUIREMENTS

- extra credit 1 - implement < and > for redirection bewteen commands
- extra credit 2 - implement >> append redirection

# Possible Grade

- 50 points: Correct implementation of required functionality
- 5 points:  Code quality (how easy is your solution to follow)
- 10 points: Quality and breadth of BATS unit tests
- 10 points: [EXTRA CREDIT 1] handle < and > redirection
- 5 points: [EXTRA CREDIT 2] handle >> append redirection