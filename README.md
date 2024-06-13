# dd99 Wayland Library

### * This project is in early development stage.

This project includes a Wayland Protocol Scanner that generates C++ code, and the corresponding Wayland Core Library.

The Wayland Core Library implements marshalling and parsing/dispatching of wayland messages.
The design choices for this project follow the C++ zero-overhead principle (You don't pay for what you don't use) and this library is designed from the ground up with performance and modularity in mind.
As such, this library gives full freedom and responsibility to the user, over almost everything.
The user is specifically expected to take care of the non-trivial matters of object lifetime management, synchronization/multithreading and input/output of marshalled data, including ancillary data required by the protocol.

This freedom also means that this library can readily integrate on any event-loop or design architecture.

See test_sources directory for example code on how to use the library, i/o handling and event-loop integration.

#### A note about multithreading
First some facts:
- Event messages are processed serially, in order, from a buffer of incoming data.
- The next event is not parsed until the event handler for the current event returns.
- To avoid unnecessary data copying, event handlers are called in some cases with read-only references that point directly to the data buffer being processed. This is the case with text strings and arrays.

#### Latest features

Newest feature (v0.1.3): protocol message logging.
All messages sent or received are translated to a human-friendly format and print on the terminal. (Useful for debugging and learning about the protocol).



## Dependencies

### dd99_wayland_scanner
- C++20
- pugixml (library used for xml parsing)

### dd99_wayland (library)
- C++20



## Test sources

### test1.cpp
A simple app demonstrating the creation of an `xdg_toplevel` and a memory-mapped buffer used as a pixel buffer. Protocol messages are logged to the terminal. 
Uses asio standalone library for input/output.
#### dependencies
- asio standalone (library used for I/O)


