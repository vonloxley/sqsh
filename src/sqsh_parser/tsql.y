%include {
    #include <string.h>
    #include <assert.h>
    #include <stdlib.h>
    
    #include "sqsh_parser.h"
}

%parse_failure {
     //Actually we don't expect everything to parse.
}

%token_type         {char*}
%token_destructor   {free($$);}

prog            ::= statements.
statements      ::= statements statement.
statements      ::= statement.
statement       ::= INSUP TOKEN(B).             {addTableDefNode(B, B);free(B);}
statement       ::= FROM tabledefs.
tabledefs       ::= tabledef.
tabledefs       ::= tabledefs COMMA tabledef.
tabledefs       ::= tabledefs JOIN tabledef.   // Right, "on"-clauses are filtered by the lexer.
tabledef        ::= TOKEN(B).                  {addTableDefNode(B, B); free(B);}
tabledef        ::= TOKEN(B) TOKEN(C).         {addTableDefNode(B, C); free(B); free(C);}
