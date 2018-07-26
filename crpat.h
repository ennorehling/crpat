#pragma once

struct CR_ParserStruct;
typedef struct CR_ParserStruct *CR_Parser;

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

typedef void(*CR_ElementHandler) (void *userData, const char *name,
    const char **atts);
typedef void(*CR_PropertyHandler) (void *userData, const char *name,
    const char *value);
typedef void(*CR_TextHandler) (void *userData, const char *value);

void CR_SetElementHandler(CR_Parser parser, CR_ElementHandler handler);
void CR_SetPropertyHandler(CR_Parser parser, CR_PropertyHandler handler);
void CR_SetTextHandler(CR_Parser parser, CR_TextHandler handler);
enum CR_Status CR_Parse(CR_Parser parser, const char *s, int len, int isFinal);
int CR_GetCurrentLineNumber(CR_Parser parser);
int CR_GetErrorCode(CR_Parser parser);
const char *CR_ErrorString(enum CR_Error error);
void CR_SetUserData(CR_Parser parser, void *userData);
void *CR_GetUserData(CR_Parser parser);
