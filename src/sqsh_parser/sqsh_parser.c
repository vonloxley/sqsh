/* 
 * File:   main.c
 * Author: K.-M. Hansche
 *
 * Created on 19. Juli 2013, 07:41
 */

#include <stdio.h>
#include <stdlib.h>
#include <search.h>
#include <string.h>
#include <ctype.h>

#include "sqsh_parser.h"

void* def_root = NULL;
aliascallback callback = NULL;
long callbacklparam = 0;

void* xmalloc(size_t size) {
    void* m = malloc(size);
    if (m == NULL) {
        exit(0);
    }
    memset(m, 0, size);
    return m;
}

void free_node(void *nodep) {
    t_tdef* datap;
    datap = (t_tdef*) nodep;

    free(datap->alias);
    free(datap->table);
    free(datap);

}

int def_compare(const void *pa, const void *pb) {
    return strcmp(((t_tdef*) pa)->alias, ((t_tdef*) pb)->alias);
}

void* addTableDefNode(char* table, char* alias) {
    if (table == NULL) return NULL;
    if (alias == NULL || strcmp(alias, "") == 0) alias = table;

    t_tdef* tdef = (t_tdef*) xmalloc(sizeof (t_tdef));
    tdef->table = (char(*)) xmalloc((strlen(table) + 1) * sizeof (char));
    tdef->alias = (char(*)) xmalloc((strlen(alias) + 1) * sizeof (char));

    strcpy(tdef->table, table);
    strcpy(tdef->alias, alias);

    void* p = tsearch(tdef, &def_root, def_compare);
    if (tdef != *(t_tdef **) p) {
        free_node(tdef);
    }

    return p;
}

void callback_walker(const void *nodep, const VISIT which, const int depth) {
    t_tdef* datap;

    datap = *(t_tdef**) nodep;

    switch (which) {
        case preorder:
        case endorder:
            break;
        case postorder:
        case leaf:
            callback(datap->table, datap->alias, callbacklparam);            
            break;
    }
}

void print_walker(const void *nodep, const VISIT which, const int depth) {
    t_tdef* datap;

    datap = *(t_tdef**) nodep;

    switch (which) {
        case preorder:
            break;
        case postorder:
            printf("Table: %s\t alias: %s\t depth: %i\n", datap->table, datap->alias, depth);
            break;
        case endorder:
            break;
        case leaf:
            printf("Table: %s\t alias: %s\t depth: %i\tLeaf\n", datap->table, datap->alias, depth);
            break;
    }
}

void delTableDefs(void) {
#if !defined(_GNU_SOURCE)
    t_tdef* datap;

    while (def_root != NULL) {
        datap = *(t_tdef**)def_root;
        tdelete((void *)datap, &def_root, def_compare);
        free_node(datap);
    }
#else
    tdestroy(def_root, free_node);
    def_root = NULL;
#endif
}

char* getTableForAlias(char* alias) {
    t_tdef tdef;
    t_tdef** res;

    if (alias == NULL) return NULL;

    tdef.alias = alias;
    res = tfind(&tdef, &def_root, def_compare);

    return res != NULL ? (*res)->table : NULL;
}

void getTablesAndAliases(aliascallback cb, long lparam){
    callback=cb;
    callbacklparam=lparam;
    twalk(def_root, callback_walker);
}

