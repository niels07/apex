#ifndef APX_IO_H
#define APX_IO_H

typedef struct ApexStream ApexStream;

extern ApexStream *apex_stream_open(const char *);
extern void apex_stream_close(ApexStream *);
extern char apex_stream_getch(ApexStream *);

#endif
