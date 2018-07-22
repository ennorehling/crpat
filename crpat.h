#pragma once

struct CR_ParserStruct;
typedef struct CR_ParserStruct *CR_Parser;

#ifdef CR_UNICODE_WCHAR_T
#define CR_UNICODE
#endif

#ifdef CR_UNICODE
#ifdef CR_UNICODE_WCHAR_T
typedef wchar_t CR_Char;
#else
typedef unsigned short CR_Char;
#endif
#else
typedef char CR_Char;
#endif

enum CR_Status {
    CR_STATUS_ERROR = 0,
    CR_STATUS_OK = 1,
    CR_STATUS_SUSPENDED = 2
};

enum CR_Error {
    CR_ERROR_NONE,
    CR_ERROR_NO_MEMORY,
    CR_ERROR_SYNTAX
};

CR_Parser CR_ParserCreate(void);
void CR_ParserFree(CR_Parser parser);
void CR_ParserReset(CR_Parser parser);

typedef void (*CR_ElementHandler) (void *userData, const CR_Char *name,
        const CR_Char **atts);

void CR_SetElementHandler(CR_Parser parser, CR_ElementHandler handler);
enum CR_Status CR_Parse(CR_Parser parser, const char *s, int len, int isFinal);

