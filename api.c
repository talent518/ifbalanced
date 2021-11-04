#include "api.h"

int make_cmd_args(char *line, char *args[], int argc) {
    char *p = line, *pp;
    int argn;

    // skip white space charset
    while(*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') p++;
    if(*p == '\0' || *p == '#') return 0;

    pp = p;
    argn = 0;
    while(*p && argn < argc) {
        if(*p == '"') {
            char c = '"';
            p++;
            while(*p && ((*p != '"' && *p != '\r' && *p != '\n') ||  (*p == '"' && c == '\\'))) c = *p++;
            switch(*p) {
                case '"':
                case '\r':
                case '\n':
                    args[argn++] = pp; 
                    *p++ = '\0';
                    while(*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') p++;
                    pp = p;
                    break;
                default:
                    break;
            }
        } else {
            while(!(*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')) p++;
            args[argn++] = pp;
            *p++ = '\0';
            while(*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') p++;
            pp = p;
        }
    }

    return argn;
}
