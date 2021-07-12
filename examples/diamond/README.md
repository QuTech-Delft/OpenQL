NOTE: cQASM read via the reader pass for the special Diamond instructions that
take a number of literal arguments doesn't work anymore, due to changes in the
IR and cQASM I/O passes. Specifically, the Diamond code uses annotations for
the additional arguments because the old IR didn't support multiple literal
arguments. The old <-> new IR conversion logic knows about them and handles
this accordingly if the annotations exist, but the cQASM reader pass doesn't
know about them and thus doesn't add the annotations.

To resolve this, the Diamond code generation pass should be upgraded to the new
IR, and the platform configuration should be updated to include instruction
prototype information that includes the special operands. This properly
configures the cQASM reader pass and avoids the new-to-old IR conversion that
would otherwise have broken due to lack of the annotations.

In 7ef963b everything should still work, use this as a reference for what it
should do!
