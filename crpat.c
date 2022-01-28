#if !defined(_CRT_SECURE_NO_WARNINGS) && defined(_MSC_VER)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "crpat.h"

#include <assert.h>
#include <errno.h>
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
    CR_NumberHandler m_numberHandler;
    CR_LocationHandler m_locationHandler;
    CR_TextHandler m_textHandler;
    enum CR_Error m_errorCode;
    int m_lineNumber;
    enum CR_Status m_status;
};

enum CR_Status CR_StopParser(CR_Parser parser)
{
    if (parser->m_status == CR_STATUS_OK) {
        parser->m_status = CR_STATUS_SUSPENDED;
        return CR_STATUS_OK;
    }
    else if (parser->m_status == CR_STATUS_SUSPENDED) {
        parser->m_errorCode = CR_ERROR_SUSPENDED;
    }
    else {
        parser->m_errorCode = CR_ERROR_FINISHED;
    }
    return CR_STATUS_ERROR;
}

CR_Parser CR_ParserCreate(void)
{
    CR_Parser parser = calloc(1, sizeof(struct CR_ParserStruct));
    // CR_ParserReset(parser);
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
    parser->m_status = CR_STATUS_OK;
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

void CR_SetNumberHandler(CR_Parser parser, CR_NumberHandler handler)
{
    parser->m_numberHandler = handler;
}

void CR_SetLocationHandler(CR_Parser parser, CR_LocationHandler handler)
{
    parser->m_locationHandler = handler;
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

static enum CR_Error buffer_append(CR_Parser parser, const char *s, size_t len)
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
        if (!buffer) {
            return CR_ERROR_NO_MEMORY;
        }
        memcpy(buffer, parser->m_bufferPtr, total - len);
        memcpy(buffer + total - len, s, len);
        free(parser->m_buffer);
        parser->m_buffer = buffer;
        parser->m_bufferPtr = parser->m_buffer;
        parser->m_bufferEnd = parser->m_buffer + total;
    }
    return CR_ERROR_NONE;
}

static enum CR_Error handle_line(CR_Parser parser, char * s, size_t len) {
    enum CR_Error err = CR_ERROR_NONE;
    char ch = s[0];
    if (ch == '\"') {
        if (len <= 1) {
            return CR_ERROR_SYNTAX;
        }
        /* either text or string property */
        if (s[len - 1] == '\"') {
            /* it is text */
            if (parser->m_textHandler) {
                char *p = s + 1;
                s[len - 1] = 0;
                while (p) {
                    p = strchr(p, '\\');
                    if (p) {
                        memmove(p, p + 1, len - (p - s) - 1);
                        p = p + 1;
                    }
                }
                err = parser->m_textHandler(parser->m_userData, s + 1);
            }
        }
        else {
            /* must be a string property */
            char *q = strrchr(s, '\"');
            if (!q || q[1] != ';') {
                return CR_ERROR_SYNTAX;
            }
            else if (parser->m_propertyHandler) {
                *q = '\0';
                err = parser->m_propertyHandler(parser->m_userData, q + 2, s + 1);
            }
        }
    }
    else if (ch >= 'A' && ch <= 'Z') {
        /* new element */
        if (parser->m_elementHandler) {
            unsigned int i = 0;
            const char *name = s;
            int keys[CR_MAXATTR];
            while (s && *s && i < CR_MAXATTR) {
                char * p = memchr(s, ' ', len);
                if (p) {
                    len -= 1 + (p - s);
                    *p++ = '\0';
                }
                if ((s = p) != NULL) {
                    keys[i++] = atoi(s);
                }
            }
            err = parser->m_elementHandler(parser->m_userData, name, i, keys);
        }
    }
    else if (ch == '-' || (ch >= '0' && ch <= '9')) {
        long num;
        char *src, *name = memchr(s, ';', len);
        if (!name) {
            return CR_ERROR_SYNTAX;
        }
        else {
            *name++ = '\0';
            num = strtol(s, &src, 10);

            /* integer property */
            if (*src == '\0') {
                if (parser->m_numberHandler) {
                    err = parser->m_numberHandler(parser->m_userData, name, num);
                    goto line_done;
                }
            }
            else if (*src == ' ') {
                if (parser->m_locationHandler) {
                    err = parser->m_locationHandler(parser->m_userData, name, s);
                    goto line_done;
                }
            }
            if (parser->m_propertyHandler) {
                err = parser->m_propertyHandler(parser->m_userData, name, s);
            }
        }
    }
    else {
        return CR_ERROR_SYNTAX;
    }
line_done:
    if (parser->m_errorCode != CR_ERROR_NONE) {
        return parser->m_errorCode;
    }
    return err;
}

static enum CR_Status parse_buffer(CR_Parser parser, int isFinal)
{
    while (parser->m_bufferEnd > parser->m_bufferPtr) {
        enum CR_Error code;
        size_t len = parser->m_bufferEnd - parser->m_bufferPtr;
        char * s = parser->m_bufferPtr;
        char * eol = memchr(s, '\n', len);
        if (eol) {
            while (eol > parser->m_bufferPtr) {
                if (*(eol-1) != '\r') break;
                --eol;
            }
            len = eol - parser->m_bufferPtr;
            *eol = '\0';

            parser->m_bufferPtr += len + 1;
            while (parser->m_bufferPtr < parser->m_bufferEnd) {
                char ch = parser->m_bufferPtr[0];
                if (ch!='\r' && ch!='\n') {
                    break;
                }
                ++parser->m_bufferPtr;
            }
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
        if (parser->m_status != CR_STATUS_OK) {
            return parser->m_status;
        }
    }
    return parser->m_status;
}

enum CR_Status CR_Parse(CR_Parser parser, const char *s, size_t len, int isFinal)
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
    case CR_ERROR_GRAMMAR:
    case CR_ERROR_SYNTAX:
        return CR_T("syntax error");
    case CR_ERROR_SUSPENDED:
        return CR_T("already suspended");
    case CR_ERROR_FINISHED:
        return CR_T("already finished");
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

int CR_ReadFile(CR_Parser parser, const char *filename)
{
    int err = 0;
    unsigned char buf[2048];
    int done = 0;
    size_t len;
    const char *line = (const char *)buf;
    FILE *F = fopen(filename, "rt");

    if (F == NULL) {
        return EINVAL;
    }

    len = fread(buf, 1, sizeof(buf), F);
    if (len >= 3 && buf[0] == 0xef) {
        /* skip BOM */
        len -= 3;
        line += 3;
    }

    while (!done) {
        if (ferror(F)) {
            fprintf(stderr,
                "read error at line %d of %s: %s\n",
                CR_GetCurrentLineNumber(parser),
                filename, strerror(errno));
            err = errno;
            break;
        }
        done = feof(F);
        if (CR_Parse(parser, line, len, done) == CR_STATUS_ERROR) {
            fprintf(stderr,
                "parse error at line %d of %s: %s\n",
                CR_GetCurrentLineNumber(parser),
                filename, CR_ErrorString(CR_GetErrorCode(parser)));
            err = -1;
            break;
        }
        len = fread(buf, 1, sizeof(buf), F);
        line = (const char *)buf;
    }
    fclose(F);
    return err;
}
