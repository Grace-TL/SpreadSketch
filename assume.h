/* See bmpcount/doc/COPYING for copyright/licensing information. */
/* cestan@cs.ucsd.edu 06/19/2003 */

/* This is an "always on" assert used to test assumptions we rely on. */

#ifndef ASSUME_H
#define ASSUME_H

#include <stdlib.h>
#include <stdio.h>

#define assume(cond,errormessage) {if(!(cond)){ \
 fprintf(stderr,errormessage);abort();}}

#define assume1(cond,errormessage,arg1) {if(!(cond)){ \
 fprintf(stderr,errormessage,arg1);abort();}}

#define assume2(cond,errormessage,arg1,arg2) {if(!(cond)){ \
 fprintf(stderr,errormessage,arg1,arg2);abort();}}

#define assume3(cond,errormessage,arg1,arg2,arg3) {if(!(cond)){ \
 fprintf(stderr,errormessage,arg1,arg2,arg3);abort();}}

#define assume4(cond,errormessage,arg1,arg2,arg3,arg4) {if(!(cond)){ \
 fprintf(stderr,errormessage,arg1,arg2,arg3,arg4);abort();}}

#define assumenotnull(p) {if(p==NULL){\
 fprintf(stderr,"Memory allocation error\n");abort();}}

#endif /* ASSUME_H */

