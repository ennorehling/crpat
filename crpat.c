#include "crpat.h"

#include <stdlib.h>

struct CR_ParserStruct {
    CR_ElementHandler m_elementHandler;
};

CR_Parser CR_ParserCreate(void)
{
    CR_Parser parser = malloc(sizeof(struct CR_ParserStruct));
    return parser;
}

void CR_ParserFree(CR_Parser parser)
{
    CR_ParserReset(parser);
    free(parser);
}

void CR_ParserReset(CR_Parser parser)
{
}

void CR_SetElementHandler(CR_Parser parser, CR_ElementHandler handler)
{
    parser->m_elementHandler = handler;
}

