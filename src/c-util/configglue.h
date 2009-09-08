#ifndef C_UTIL_CONFIGGLUE_H
#define C_UTIL_CONFIGGLUE_H

/* Common header for parser and lexer */

#include "configfile.h"


typedef struct
{
   ConfigHandle configfile;

   SectionHandle sectionstack [20];
   unsigned int stacktop;

   /* used to construct a multival key */
   struct 
   { 
            char ** keyvals;
            unsigned int count;
            unsigned int maxsize;
   }; 

   int                  parser_error_code;

   /* if error_code != 0 the user needs to free error_string */
   char *               parser_error_string;
   int                  lexer_error_code;
   char *               lexer_error_string;
} ParserParams;

void cfgp_initparams (ParserParams * p, ConfigHandle h);

/* Free private data (but not the ConfigHandle) */
void cfgp_freeparams (ParserParams * p);

int cfgp_parser_error (ParserParams * p, const char* str, 
      unsigned int l1, unsigned int c1, unsigned int l2, unsigned int c2);

int cfgp_lex_error (ParserParams * p, int lineno, int colno, const char * msg); 

/* Return true if parse and lex went ok; false otherwise, and puts
 * error message in buf. Note: ConfigHandle might still contain the partial 
 * parsed tree */
int cfgp_parse_ok (const ParserParams * p, char * buf, int bufsize);

#endif
