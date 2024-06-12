!This project is in early development stages.

This is a client library for the parsing and marshalling of the wayland protocol. Built from the ground up with performance and modularity in mind, this library gives the users full freedom and responsibility over almost everything. The user is specifically expected to handle the non-trivial matters of synchronization and input/output (including ancillary data, required by the protocol).

This freedom also means that this library is easy to integrate on any event-loop or architecture.

Newest feature (v0.1.3): wire logging. All messages sent or received are translated to a human-friendly format and print on the terminal. (Useful for debugging and learning about the protocol).

See test_sources for example code on how to use the library.
