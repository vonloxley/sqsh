%{
#include "tsql.h"

#define KW(w) {return w;}

%}

%option noyywrap
%option warn
%option 8bit
%s INFROM
%s INDML


alpha [A-Za-zäöüß_]
anum [A-Za-z0-9_äöüß_\.\-]
anumsp [A-Za-z0-9_äöüß_ \.\-]
ws [ \t\r\n]

%%

<INITIAL>from/{ws}+                            {BEGIN INFROM; KW(FROM);}
<INITIAL>(?i:insert|update|delete)/{ws}+       {BEGIN INDML; KW(INSUP);}
<INITIAL>(?i:insert{ws}+into)/{ws}+            {BEGIN INDML; KW(INSUP);}
<INITIAL,INFROM>(?i:join)/{ws}+                {BEGIN INFROM; KW(JOIN);}
<INFROM>(?i:left|right|outer)/{ws}+            {BEGIN INITIAL;}
<INFROM>(?i:where|on)/{ws}+                    {BEGIN INITIAL;}
<INFROM>{alpha}{anum}*                         {KW(TOKEN);}
<INFROM>\"{alpha}{anumsp}*\"                   {KW(TOKEN);}
<INFROM>\[{alpha}{anumsp}*\]                   {KW(TOKEN);}
<INDML>{alpha}{anum}*                          {BEGIN INITIAL; KW(TOKEN);}
<INDML>\"{alpha}{anumsp}*\"                    {BEGIN INITIAL; KW(TOKEN);}
<INDML>\[{alpha}{anumsp}*\]                    {BEGIN INITIAL; KW(TOKEN);}
<INFROM>,                                      {KW(COMMA);}
<INFROM>"("                                    {BEGIN INITIAL;}
.                                              ;
[\n]                                           ;


%%

int testlex(char *buffer)
{       
        yy_scan_string(buffer);
        int rl;

	while ((rl=yylex())!=0){
            printf("Token: %i yytext: %s\n", rl, yytext);
        }
        yylex_destroy();
	return 0;
}

extern void *ParseAlloc(void *(*mallocProc)(size_t));
extern void ParseFree(void *p, void (*freeProc)(void*));
extern void Parse(void *yyp, int yymajor, void* yyminor);
void parseSql(char* sql) {
    int lr;
    void* pParser = ParseAlloc(malloc);
    
    yy_scan_string(sql);

    while ((lr=yylex())!=0){
        Parse(pParser, lr, strdup(yytext));
    }
    Parse(pParser, 0, NULL);
    ParseFree(pParser, free);
    yylex_destroy();
}
