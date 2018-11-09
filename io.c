
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "io.h"
#include "error.h"
#include "malloc.h"

#define BUF_SIZE 256

struct ApexStream {
    FILE *f;
    char buf[BUF_SIZE];
    size_t size;
    size_t i;
    int eof;
};

static void io_error(const char *fmt, const char *arg) {
    char msg[256];

    sprintf(msg, fmt, arg);
    if (errno) {
        apex_error_throw(APEX_ERROR_CODE_IO, "%s: %s", msg, strerror(errno));
    } else {
        apex_error_throw(APEX_ERROR_CODE_IO, msg);
    }
}

static void read_chunk(ApexStream *stream) {
    stream->size = fread(stream->buf, 1, BUF_SIZE, stream->f);
    stream->i = 0;
    if (stream->size == BUF_SIZE) {
        return;
    }
    if (ferror(stream->f)) {
        io_error("read error", NULL);
    }
    if (feof(stream->f)) {
        stream->eof = 1;
    }
}

ApexStream *apex_stream_open(const char *path) {
    ApexStream *stream = APEX_NEW(ApexStream); 

    stream->f = fopen(path, "r");
    if (!stream->f) {
        io_error("could not open file '%s'", path);
        return NULL;
    }
    stream->eof = 0;
    read_chunk(stream);
    return stream;
}

void apex_stream_close(ApexStream *stream) {
    fclose(stream->f);
    apex_free(stream);
}

char apex_stream_getch(ApexStream *stream) {
    if (stream->i == stream->size) {
        if (!stream->eof) {
            read_chunk(stream);
        } else {
            return '\0';
        }
    }
    return stream->buf[stream->i++];
}
