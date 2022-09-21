/** @file misbehave.c
 *
 * @brief Test program for 15-412 project 3 Spring 2003
 * @author Updated 4/2/04 by Mark T. Tomczak (mtomczak)
 *
 * Switches kernel misbehave mode.
 */

/* Includes */
#include <syscall.h>
#include <syscall_int.h>   /* for misbehave */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>


char *itoa( int n, char *s, int w );

/* Main */
int main( int argc, char *argv[] )
{
  int result;
  int misbehave_val;
        if( argc == 1 ) {
                printf("usage: misbehave <mode>");
                exit( -1 );
        }
  misbehave_val = atoi(argv[1]);

  /* use assembly do directly thunk misbehave */
  asm("int %2":"=a"(result):"S"(misbehave_val),"i"(MISBEHAVE_INT));


	return 0;
}
