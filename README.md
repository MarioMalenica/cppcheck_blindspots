**A collection of C errors and pitfalls not detected by cppcheck**

This is a list of common errors and pitfalls in embedded systems that are not detected by cppcheck and gcc.
They were found during code reviews and debugging sessions.

Most of them are related to the way how C standard libraries are treating NULL pointers and out-of-range array sizes.

But still, cppcheck is a good static analysis tool, especially for one-man-band type of projects.
