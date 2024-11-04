# haitchteep

A very basic HTTP server written in C. Currently uses `poll(2)` but in the future I would hope to be able to write my own event loop implementation using `kqueue`, `epoll` and whatever Windows does (Fast I/O?)

## Other Ideas
* Potentially add simple config file, something like nginx but super barebones
* A simple JS binding, using quickjs?
