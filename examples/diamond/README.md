NOTE: cQASM read/write for the special Diamond instructions that take a number
of literal arguments doesn't work anymore, due to changes in the IR and cQASM
I/O passes. Specifically, the Diamond code uses annotations for the additional
arguments because the old IR didn't support multiple literal arguments, which
the new IR and cQASM I/O passes don't know about.

Ideally, the old IR will have been phased out by the time Diamond development
will continue, in which case all passes should simply be ported to the new IR,
which supports any number and combination of instruction arguments. Failing
that, the new-to-old and old-to-new IR conversion logic should be made aware
of the Diamond annotations and act accordingly as a special case.

In 7ef963b everything should still work, use this as a reference for what it
should do!
