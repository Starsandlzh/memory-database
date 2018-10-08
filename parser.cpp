

#include "parser.h"

using namespace std;

int Parser::parse_line(char *line, const char **result)
{
    char* s = line; ///< pointer to the start of token
    char* p = line; ///< pointer to the end of token
    int ret = 0;

    while(*s)
    {
        while(*p != sep_ && *p)
        {
            p++;
        }
        if(s != p)   ///< eat null fields
            result[ret++] = s;
        if(*p)       ///< stopped because of sep
        {
            (*p) = 0;
            s = ++p;
        }
        else
        {
            break;
        }
    }
    return ret;
}
