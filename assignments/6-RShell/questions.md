1. How does the remote client determine when a command's output is fully received from the server, and what techniques can be used to handle partial reads or ensure complete message transmission?

```The remote client detects the end of a command's output when it receives the EOF character (0x04) from the server, which marks the completion of the transmission. For partial reads, it continuously loops through recv() calls until it sees this special character, concatenating chunks of data as they arrive. :3```

2. This week's lecture on TCP explains that it is a reliable stream protocol rather than a message-oriented one. Since TCP does not preserve message boundaries, how should a networked shell protocol define and detect the beginning and end of a command sent over a TCP connection? What challenges arise if this is not handled correctly?

```Since TCP is stream-based without message boundaries, our shell needs a protocol-level delimiter like the null byte for client-to-server messages and EOF character for server responses. Without these markers, the receiver would have no way to know when a logical command ends, causing data to be misinterpreted or commands to hang forever waiting for more bytes~```

3. Describe the general differences between stateful and stateless protocols.

```Stateful protocols maintain information about previous interactions between client and server across multiple requests, while stateless protocols treat each request as independent with no memory of past communications. Think of stateful as a conversation where context matters, versus stateless where each message must stand completely on its own.```

4. Our lecture this week stated that UDP is "unreliable". If that is the case, why would we ever use it?

```UDP is perfect when speed matters more than reliability, like in video streaming, online gaming, or DNS lookups. It's way faster than TCP because it doesn't waste time with handshakes, acknowledgments, or retransmissions - it just sends data and moves on. Also it's a way smaller packet size than TCP.```

5. What interface/abstraction is provided by the operating system to enable applications to use network communications?

```The operating system gives us sockets as the main interface for network communication, letting applications create endpoints for sending and receiving data without worrying about the underlying hardware or protocol details.```