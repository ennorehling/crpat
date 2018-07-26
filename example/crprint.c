#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <crpat.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

static void handle_element(void *udata, const char *name, const char **attr) {
    int i;
    FILE * F = (FILE *)udata;

    fprintf(F, "%s ", name);
    for (i = 0; attr[i]; ++i) {
        fprintf(F, "%s ", attr[i]);
    }
    fputc('\n', F);
}

int main(int argc, char **argv) {
    CR_Parser cp;
    const char *filename = "<stdin>";
    FILE * in = stdin;
    FILE * out = stdout;
    char buf[2048];
    int done = 0, err = 0;

    if (argc > 1) {
        filename = argv[1];
        in = fopen(filename, "rt+");
        if (!in) {
            fprintf(stderr,
                "could not open %s: %s\n",
                filename, strerror(errno));
            return errno;
        }
        fseek(in, 3, SEEK_CUR); /* hack: skip the BOM */
    }

    cp = CR_ParserCreate();
    CR_SetElementHandler(cp, handle_element);
    CR_SetUserData(cp, (void *)out);

    while (!done) {
        size_t len = (int)fread(buf, 1, sizeof(buf), in);
        if (ferror(in)) {
            fprintf(stderr, 
                "read error at line %d of %s: %s\n",
                CR_GetCurrentLineNumber(cp),
                filename, strerror(errno));
            err = errno;
            break;
        }
        done = feof(in);
        if (CR_Parse(cp, buf, len, done) == CR_STATUS_ERROR) {
            fprintf(stderr,
                "parse error at line %d of %s: %s\n",
                CR_GetCurrentLineNumber(cp),
                filename, CR_ErrorString(CR_GetErrorCode(cp)));
            err = -1;
            break;
        }
    }
    CR_ParserFree(cp);
    if (in != stdin) {
        fclose(in);
    }
    return err;
}