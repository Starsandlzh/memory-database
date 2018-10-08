

#ifndef PARSER_H
#define PARSER_H

#include<vector>
#include<string>

class Parser
{
    public:

        Parser(char sep) : sep_(sep) { }

        int parse_line(char* line, const char** result);
    private:
        char sep_;
};

#endif // PARSER_H
