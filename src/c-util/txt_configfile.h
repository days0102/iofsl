#ifndef C_UTIL_TXTFILE_CONFIGFILE_H
#define C_UTIL_TXTFILE_CONFIGFILE_H


#include <stdio.h>
#include "configfile.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * ConfigFile implementation that stores data in a text file 
 */


/**
 * returns ConfigHandle, if all is OK *err is set to 0, 
 * otherwise *err is set to a pointer to the error string 
 * (which needs to be freed by the user)
 * NOTE that even if an error occurred, a partial ConfigHandle tree
 * can be returned.
 */
ConfigHandle txtfile_openConfig (const char * filename, char ** err);

/**
 * returns ConfigHandle, if all is OK *err is set to 0, 
 * otherwise *err is set to a pointer to the error string 
 * (which needs to be freed by the user)
 * NOTE that even if an error occurred, a partial ConfigHandle tree
 * can be returned.
 */
ConfigHandle txtfile_openStream (FILE * f, char ** err);


/**
 * Write ConfigHandle to disk (in a format supported by _open).
 * Returns >=0 if all went OK, < 0 otherwise in which 
 * case *err is set to a pointer to an error string, which needs to be
 * freed by the user.
 */
int txtfile_writeConfig (ConfigHandle h, SectionHandle h2, FILE * out, char ** err);

#ifdef __cplusplus
} /* extern "C"  */
#endif

#endif
