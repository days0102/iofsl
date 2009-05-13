#ifndef IOFWDUTIL_SIGNALS_HH
#define IOFWDUTIL_SIGNALS_HH

#include <signal.h>

namespace iofwdutil
{
//===========================================================================

/// Mask all signales for the calling thread
void disableAllSignals (); 

/// Wait for specified signal
/// Returns signal number; throws on error
int waitSignal (const sigset_t * set); 

//===========================================================================
}


#endif
