#ifndef __INTEGERS_H
#define __INTEGERS_H

#define MAX(A,B) (((A)>(B))?(A):(B))
#define MIN(A,B) (((A)<(B))?(A):(B))
#define RESTRICT_RANGE( a, score, b ) (MIN( MAX( (a), (score)), (b) ))

typedef unsigned long long uqword;
typedef unsigned long udword;
typedef unsigned short uword;
typedef unsigned char ubyte;
typedef signed long long sqword;
typedef signed long sdword;
typedef signed short sword;
typedef signed char sbyte;


#endif

