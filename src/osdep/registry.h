#pragma once
typedef struct UAEREG {
    HKEY fkey;
    TCHAR* inipath;
} UAEREG;

extern const TCHAR* getregmode();
extern int reginitializeinit(TCHAR** path);
extern void regstatus();

extern int regsetstr(UAEREG*, const TCHAR* name, const TCHAR* str);
extern int regsetint(UAEREG*, const TCHAR* name, int val);
extern int regqueryint(UAEREG*, const TCHAR* name, int* val);
extern int regquerystr(UAEREG*, const TCHAR* name, TCHAR* str, int* size);
extern int regsetlonglong(UAEREG* root, const TCHAR* name, unsigned long long val);
extern int regquerylonglong(UAEREG* root, const TCHAR* name, unsigned long long* val);

extern int regdelete(UAEREG*, const TCHAR* name);
extern void regdeletetree(UAEREG*, const TCHAR* name);

extern int regexists(UAEREG*, const TCHAR* name);
extern int regexiststree(UAEREG*, const TCHAR* name);

extern int regquerydatasize(UAEREG* root, const TCHAR* name, int* size);
extern int regsetdata(UAEREG*, const TCHAR* name, const void* str, int size);
extern int regquerydata(UAEREG* root, const TCHAR* name, void* str, const int* size);

extern int regenumstr(UAEREG*, int idx, TCHAR* name, const int* nsize, TCHAR* str, const int* size);

extern UAEREG* regcreatetree(UAEREG*, const TCHAR* name);
extern void regclosetree(UAEREG* key);
