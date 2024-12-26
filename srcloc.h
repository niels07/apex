#ifndef SRCLOC_H
#define SRCLOC_H

typedef struct {
    int lineno;
    const char *filename;
} SrcLoc;

#endif