#include "crpat.h"

#include <assert.h>
#include <string.h>
#include <stdlib.h>

#define CR_MAXATTR 4

#ifdef CR_GETTEXT
#include <gettext.h>
#define CR_T(x) gettext(x)
#else
#define CR_T(x) (x)
#endif

struct CR_ParserStruct {
    void *m_userData;
    char *m_buffer;
    char *m_bufferPtr;
    const char *m_bufferEnd;
    CR_ElementHandler m_elementHandler;
    CR_PropertyHandler m_propertyHandler;
    CR_TextHandler m_textHandler;
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
    parser->m_propertyHandler = NULL;
    parser->m_textHandler = NULL;
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

void CR_SetPropertyHandler(CR_Parser parser, CR_PropertyHandler handler)
{
    parser->m_propertyHandler = handler;
}

void CR_SetTextHandler(CR_Parser parser, CR_TextHandler handler)
{
    parser->m_textHandler = handler;
}

static void buffer_free(CR_Parser parser)
{
    free(parser->m_buffer);
    parser->m_bufferEnd = parser->m_bufferPtr = parser->m_buffer = NULL;
}

static enum CR_Error buffer_append(CR_Parser parser, const char *s, int len)
{
    if (parser->m_buffer == NULL) {
        parser->m_buffer = malloc(len);
        if (!parser->m_buffer) {
            return CR_ERROR_NO_MEMORY;
        }
        memcpy(parser->m_buffer, s, len);
        parser->m_bufferPtr = parser->m_buffer;
        parser->m_bufferEnd = parser->m_buffer + len;
    }
    else {
        size_t total = len;
        char * buffer;
        total += (parser->m_bufferEnd - parser->m_bufferPtr);
        buffer = malloc(total);
        memcpy(buffer, parser->m_bufferPtr, total - len);
        memcpy(buffer + total - len, s, len);
        free(parser->m_buffer);
        parser->m_buffer = buffer;
        if (!parser->m_buffer) {
            return CR_ERROR_NO_MEMORY;
        }
        parser->m_bufferPtr = parser->m_buffer;
        parser->m_bufferEnd = parser->m_buffer + total;
    }
    return CR_ERROR_NONE;
}

static enum CR_Error handle_line(CR_Parser parser, char * s, size_t len) {
    char ch = s[0];
    if (ch == '\"') {
        if (len <= 1) {
            return CR_ERROR_SYNTAX;
        }
        /* either text or string property */
        if (s[len - 1] == '\"') {
            /* it is text */
            if (parser->m_textHandler) {
                s[len - 1] = 0;
                parser->m_textHandler(parser->m_userData, s + 1);
            }
        }
        else {
            /* must be a string property */
            char *q = strrchr(s, '\"');
            if (!q || q[1] != ';') {
                return CR_ERROR_SYNTAX;
            }
            if (parser->m_propertyHandler) {
                *q = '\0';
                parser->m_propertyHandler(parser->m_userData, q + 2, s + 1);
            }
        }
    }
    else if (ch >= 'A' && ch <= 'Z') {
        /* new element */
        if (parser->m_elementHandler) {
            int i = 0;
            const char *name = s;
            const char *atts[CR_MAXATTR];
            while (s && *s && i < CR_MAXATTR) {
                char * p = memchr(s, ' ', len);
                if (p) {
                    len -= 1 + (p - s);
                    *p++ = '\0';
                }
                atts[i++] = s = p;
            }
            parser->m_elementHandler(parser->m_userData, name, atts);
        }
    }
    else if (ch == '-' || (ch >= '0' && ch <= '9')) {
        /* integer property */
        if (parser->m_propertyHandler) {
            char * name = memchr(s, ';', len);
            if (name) {
                *name++ = '\0';
            }
            else {
                return CR_ERROR_SYNTAX;
            }
            parser->m_propertyHandler(parser->m_userData, name, s);
        }
    }
    else {
        return CR_ERROR_SYNTAX;
    }
    return CR_ERROR_NONE;
}

static enum CR_Status parse_buffer(CR_Parser parser, int isFinal)
{
    while (parser->m_bufferEnd > parser->m_bufferPtr) {
        enum CR_Error code;
        size_t len = parser->m_bufferEnd - parser->m_bufferPtr;
        char * s = parser->m_bufferPtr;
        char * eol = memchr(s, '\n', len);
        if (eol) {
            len = eol - parser->m_bufferPtr;
            *eol = '\0';
            parser->m_bufferPtr += len + 1;
        }
        else if (isFinal) {
            /* parse until EOF */
            parser->m_bufferPtr += len;
            assert(parser->m_bufferPtr == parser->m_bufferEnd);
        }
        else {
            /* we are not at EOF yet, wait for more data */
            break;
        }
        ++parser->m_lineNumber;
        code = handle_line(parser, s, len);
        if (code != CR_ERROR_NONE) {
            parser->m_errorCode = code;
            return CR_STATUS_ERROR;
        }
    }
    return CR_STATUS_OK;
}

enum CR_Status CR_Parse(CR_Parser parser, const char *s, int len, int isFinal)
{
    enum CR_Error code;
    if (parser->m_bufferPtr >= parser->m_bufferEnd) {
        buffer_free(parser);
    }

    code = buffer_append(parser, s, len);
    if (code != CR_ERROR_NONE) {
        parser->m_errorCode = code;
        return CR_STATUS_ERROR;
    }

    return parse_buffer(parser, isFinal);
}

int CR_GetCurrentLineNumber(CR_Parser parser)
{
    return parser->m_lineNumber;
}

int CR_GetErrorCode(CR_Parser parser)
{
    return parser->m_errorCode;
}

const char *CR_ErrorString(enum CR_Error code) 
{
    switch (code) {
    case CR_ERROR_NONE:
        return NULL;
    case CR_ERROR_NO_MEMORY:
        return CR_T("out of memory");
    case CR_ERROR_SYNTAX:
        return CR_T("syntax error");
    }
    return NULL;
}

void CR_SetUserData(CR_Parser parser, void *userData)
{
    parser->m_userData = userData;
}

void *CR_GetUserData(CR_Parser parser)
{
    return parser->m_userData;
}
