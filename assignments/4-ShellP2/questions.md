1. Can you think of why we use `fork/execvp` instead of just calling `execvp` directly? What value do you think the `fork` provides?

    > **Answer**: Fork lets us create a child process that runs the command while our shell continues to exist. Without fork, execvp would replace our shell with the command, so our shell would terminate after running just one command. >.>

2. What happens if the fork() system call fails? How does your implementation handle this scenario?

    > **Answer**: If fork() fails, it returns a negative value. My implementation handles this by printing an error message with perror("fork"), updating the return code, freeing allocated memory, and continuing the shell loop rather than crashing.

3. How does execvp() find the command to execute? What system environment variable plays a role in this process?

    > **Answer**: execvp() searches for the command in directories listed in the PATH environment variable. The 'p' in execvp means it uses PATH to locate executables, so users don't need to type absolute paths for common commands.

4. What is the purpose of calling wait() in the parent process after forking? What would happen if we didn’t call it?

    > **Answer**: wait() makes the parent process wait for the child to finish, preventing zombie processes bleeeegh. Without it, child processes would become zombies, consuming system resources, and the shell wouldn't know when commands finished or their exit status.

5. In the referenced demo code we used WEXITSTATUS(). What information does this provide, and why is it important?

    > **Answer**: WEXITSTATUS() extracts the actual exit code from the status returned by wait(). It's important because it lets the shell know if commands succeeded (exit code 0) or failed (non-zero), allowing for error tracking and conditional execution. :3

6. Describe how your implementation of build_cmd_buff() handles quoted arguments. Why is this necessary?

    > **Answer**: My implementation tracks quote state with a boolean flag and preserves spaces inside quotes. This is necessary because users need to pass multi-word arguments as a single unit (like filenames with spaces) to commands.

7. What changes did you make to your parsing logic compared to the previous assignment? Were there any unexpected challenges in refactoring your old code?

    > **Answer**: I switched from splitting by pipes to focusing on a single command, and changed from storing exe/args separately to using an argv array. The trickiest part was adapting the quote handling to work with the new structure while maintaining the whitespace preservation logic. It was a lot... :P
8. For this quesiton, you need to do some research on Linux signals. You can use [this google search](https://www.google.com/search?q=Linux+signals+overview+site%3Aman7.org+OR+site%3Alinux.die.net+OR+site%3Atldp.org&oq=Linux+signals+overview+site%3Aman7.org+OR+site%3Alinux.die.net+OR+site%3Atldp.org&gs_lcrp=EgZjaHJvbWUyBggAEEUYOdIBBzc2MGowajeoAgCwAgA&sourceid=chrome&ie=UTF-8) to get started.

- What is the purpose of signals in a Linux system, and how do they differ from other forms of interprocess communication (IPC)?

    > **Answer**: Signals are lightweight messages sent to processes for event handling/control. Unlike other IPC methods (pipes, shared memory), signals are asynchronous, don't carry much data, and are primarily used for process control rather than data transfer.

- Find and describe three commonly used signals (e.g., SIGKILL, SIGTERM, SIGINT). What are their typical use cases?

    > **Answer**: SIGINT (Ctrl+C) interrupts programs for graceful termination; SIGTERM asks processes to clean up and exit normally (used by 'kill'); SIGKILL forces immediate termination without cleanup when a process is unresponsive (can't be caught/ignored). We talked about these in class today!!!

- What happens when a process receives SIGSTOP? Can it be caught or ignored like SIGINT? Why or why not?

    > **Answer**: SIGSTOP suspends a process's execution until it receives SIGCONT. Unlike SIGINT, it cannot be caught or ignored because it's designed as a mandatory control mechanism that ensures processes can always be suspended by the OS or users regardless of the process's wishes.
