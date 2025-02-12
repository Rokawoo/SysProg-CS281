1. In this assignment I suggested you use `fgets()` to get user input in the main while loop. Why is `fgets()` a good choice for this application?

    > **Answer**: fgets() is ideal for shell input because it safely handles keyword line-based input with built-in buffer overflow protection. It reads until newline or EOF, preserving whitespace and special characters that may be part of shell commands. Unlike scanf() or gets(), it won't overrun the buffer since you specify the maximum length.

2. You needed to use `malloc()` to allocte memory for `cmd_buff` in `dsh_cli.c`. Can you explain why you needed to do that, instead of allocating a fixed-size array?

    > **Answer**: The assignment doesn't require malloc() for cmd_buff. Instead, we use a fixed-size array defined by SH_CMD_MAX. This is appropriate since we have a known maximum command length and don't need dynamic allocation.But... dynamic allocation with malloc() provides flexibility for varying command lengths at runtime. A fixed array would waste memory for short commands and limit maximum command length. malloc() lets us adjust buffer size based on actual needs while preventing again~ buffer overflows. Trick question?!

3. In `dshlib.c`, the function `build_cmd_list(`)` must trim leading and trailing spaces from each command before storing it. Why is this necessary? If we didn't trim spaces, what kind of issues might arise when executing commands in our shell?

    > **Answer**: Trimming spaces is needed because shell command parsing needs to differentiate between intentional spaces (like in arguments) and incidental whitespace. Without trimming, commands like "  ls  -l  " could fail to execute or have incorrect arguments passed. Extra spaces could also cause command name lookup failures or argument parsing errors.

4. For this question you need to do some research on STDIN, STDOUT, and STDERR in Linux. We've learned this week that shells are "robust brokers of input and output". Google _"linux shell stdin stdout stderr explained"_ to get started.

- One topic you should have found information on is "redirection". Please provide at least 3 redirection examples that we should implement in our custom shell, and explain what challenges we might have implementing them.

    > **Answer**:  Output redirection (>) for sending command output to files. Input redirection (<) for reading command input from files. Append redirection (>>) for adding output to existing files. Implementation challenges include: Handling file permissions and creation, managing file descriptor duplication, and proper cleanup after redirection.

- You should have also learned about "pipes". Redirection and piping both involve controlling input and output in the shell, but they serve different purposes. Explain the key differences between redirection and piping.

    > **Answer**: Piping (|) connects processes by sending output from one command directly to another command's input. Redirection (>, <) moves data between processes and files.. Pipes operate in memory between processes, while redirection involves filesystem operations

- STDERR is often used for error messages, while STDOUT is for regular output. Why is it important to keep these separate in a shell?

    > **Answer**: Error messages need to remain visible even when output is redirected. Programs need to distinguish between successful output and error conditions. It enables proper error handling and logging. Users can handle normal output and errors differently.

- How should our custom shell handle errors from commands that fail? Consider cases where a command outputs both STDOUT and STDERR. Should we provide a way to merge them, and if so, how?

    > **Answer**:  Display STDERR in red or with a prefix for visibility. Allow redirection of both streams independently. Support merging streams with 2>&1 syntax (merge using the 2>&1 operator  redirects stderr (fd 2) to stdout (fd 1), allowing both streams to be handled together through a single channel). Return appropriate exit codes for failed commands .Maintain error context through pipes.