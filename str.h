#ifndef APX_STR_H
#define APX_STR_H

typedef struct ApexString ApexString;
typedef struct ApexTable ApexStringTable;

extern ApexStringTable *apex_string_table_new(void);
extern ApexString *apex_string_new(ApexStringTable *, const char *);
extern void apex_string_table_destroy(ApexStringTable *);
extern char *apex_string_to_cstr(ApexString *);

#endif
