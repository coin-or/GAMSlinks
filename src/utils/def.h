// Copyright (C) GAMS Development and others 2022
// All Rights Reserved.
// This code is published under the Eclipse Public License.
//
// Author: Stefan Vigerske

#ifndef DEF_H
#define DEF_H

#define CHECK( x ) \
   do \
   { \
      RETURN _retcode = (x); \
      if( _retcode != RETURN_OK ) \
      { \
         fprintf(stderr, __FILE__ ":%d Error %d from call ", __LINE__, _retcode); \
         fputs(#x "\n", stderr); \
         return _retcode; \
      } \
   } while( 0 )

typedef enum {
    RETURN_OK = 0,
    RETURN_ERROR = 1
} RETURN;

struct gmoRec;
struct gevRec;

#endif
