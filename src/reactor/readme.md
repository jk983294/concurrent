the proactor pattern has operations start regardless of whether they can finish immediately or not, has them performed asynchronously,
and then arranges to deliver notification about their completion.


Either approach can be used for event driven programming.
Using the reactor pattern, a program waits for the event of (for example) a socket being readable and then reads from it.
Using the proactor pattern, the program instead waits for the event of a socket read completing.
