#pragma once

struct CR_ParserStruct;
typedef struct CR_ParserStruct *CR_Parser;

CR_Parser CR_ParserCreate();
void CR_PARSERFree(CR_Parser cp);
