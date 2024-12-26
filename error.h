#ifndef ERROR_H
#define ERROR_H

extern void apexErr_print(const char *fmt, ...);
extern void apexErr_fatal(const char *fmt, ...);
extern void apexErr_syntax(int lineno, const char *fmt, ...);
extern void apexErr_runtime(int lineno, const char *fmt, ...);

#endif /* ERROR_H */