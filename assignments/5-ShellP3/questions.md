1. Your shell forks multiple child processes when executing piped commands. How does your implementation ensure that all child processes complete before the shell continues accepting user input? What would happen if you forgot to call waitpid() on all child processes?

> My shell tracks each child's PID and uses waitpid() to wait for all of them to finish before accepting new commands. If I forgot to call waitpid(), zombie processes would accumulate in the system, wasting resources, and the shell might continue before command output was complete, causing weird output mixing.

2. The dup2() function is used to redirect input and output file descriptors. Explain why it is necessary to close unused pipe ends after calling dup2(). What could go wrong if you leave pipes open?

> Closing unused pipe ends after dup2() is crucial because open write ends prevent readers from getting EOF signals, which can cause programs to hang indefinitely waiting for more input. Plus, leaving pipes open wastes file descriptors, which are limited resources in Unix systems.

3. Your shell recognizes built-in commands (cd, exit, dragon). Unlike external commands, built-in commands do not require execvp(). Why is cd implemented as a built-in rather than an external command? What challenges would arise if cd were implemented as an external process?

> The cd command must be built-in because it needs to change the current working directory of the shell process itself, not a child process. If cd were an external command, it would change its own directory and then terminate, leaving the parent shell's directory unchanged.

4. Currently, your shell supports a fixed number of piped commands (CMD_MAX). How would you modify your implementation to allow an arbitrary number of piped commands while still handling memory allocation efficiently? What trade-offs would you need to consider?

> I'd replace the fixed-size array with a dynamically allocated linked list that grows as needed for each new piped command. This would allow unlimited commands while only using memory actually needed. The trade-off would be slightly more complex memory management and potential issues if the system runs out of memory or pipe file descriptors.