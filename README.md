# dd99 Wayland Library

### * This project is in early development stages.

This is a client library for the parsing and marshalling of the wayland protocol. Built from the ground up with performance and modularity in mind, this library gives the users full freedom and responsibility over almost everything. The user is specifically expected to handle the non-trivial matters of synchronization and input/output (including ancillary data, required by the protocol).

This freedom also means that this library is easy to integrate on any event-loop or architecture.

Newest feature (v0.1.3): protocol message logging. All messages sent or received are translated to a human-friendly format and print on the terminal. (Useful for debugging and learning about the protocol).

See test_sources for example code on how to use the library.



## Dependencies

### dd99_wayland_scanner
- C++20
- pugixml (library used for xml parsing)

### dd99_wayland (library)
- C++20



## Test sources

### test1.cpp
A simple app demonstrating the creation of an `xdg_toplevel` and a memory-mapped buffer used as a pixel buffer. Protocol messasges are logged to the terminal. 
Uses asio standalone library for input/output.
#### dependencies
- asio standalone (library used for I/O)


