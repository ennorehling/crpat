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
CR_Parser CR_ParserCreate();
void CR_ParserFree(CR_Parser cp);
void CR_ParserReset(CR_Parser cp);

