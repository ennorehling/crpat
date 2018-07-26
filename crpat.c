#include "crpat.h"

#include <stdlib.h>

struct CR_ParserStruct {
    void *m_userData;
    char *m_buffer;
    const char *m_bufferPtr;
    char *m_bufferEnd;
    CR_ElementHandler m_elementHandler;
    enum CR_Error m_errorCode;
    int m_lineNumber;
};

CR_Parser CR_ParserCreate(void)
{
    CR_Parser parser = calloc(1, sizeof(struct CR_ParserStruct));
    return parser;
}

void CR_ParserFree(CR_Parser parser)
{
    CR_ParserReset(parser);
    free(parser);
}

void CR_ParserReset(CR_Parser parser)
{
    parser->m_elementHandler = NULL;
    parser->m_errorCode = CR_ERROR_NONE;
    parser->m_lineNumber = 0;
    parser->m_userData = NULL;
    free(parser->m_buffer);
    parser->m_buffer = NULL;
    parser->m_bufferPtr = NULL;
    parser->m_bufferEnd = NULL;
}

void CR_SetElementHandler(CR_Parser parser, CR_ElementHandler handler)
{
    parser->m_elementHandler = handler;
}

enum CR_Status CR_Parse(CR_Parser parser, const char *s, int len, int isFinal)
{
    parser->m_errorCode = CR_ERROR_NO_MEMORY;
    return CR_STATUS_ERROR;
}

int CR_GetCurrentLineNumber(CR_Parser parser)
{
    return parser->m_lineNumber;
}

int CR_GetErrorCode(CR_Parser parser)
{
    return parser->m_errorCode;
}

const char *CR_ErrorString(enum CR_Error error) 
{
    return "out of memory";
}

void CR_SetUserData(CR_Parser parser, void *userData)
{
    parser->m_userData = userData;
}

void *CR_GetUserData(CR_Parser parser)
{
    return parser->m_userData;
}
