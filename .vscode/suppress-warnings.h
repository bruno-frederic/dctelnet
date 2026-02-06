#ifdef __INTELLISENSE__
// Workaround for VBCC compiler:
// The custom syntax for Inline-Assembly functions triggers IntelliSense errors 145 and 147.
// See VBCC Manual §3.5.3 “Inline-Assembly Functions” for details.
// These pragmas disable those errors in IntelliSense under VS Code.

#pragma diag_suppress 145   // function "__foo(...)" may not be initialized (145)
//TODO useless ? #pragma diag_suppress 147   // declaration is incompatible with "__foo(...)"

#endif
