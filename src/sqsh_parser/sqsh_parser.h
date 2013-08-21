/* 
 * File:   sqsh_parser.h
 * Author: K.-M. Hansche
 *
 * Created on 22. Juli 2013, 17:37
 */

#ifndef SQSH_PARSER_H
#define	SQSH_PARSER_H

#ifdef	__cplusplus
extern "C" {
#endif

    typedef void (* aliascallback)(char* table, char* alias, long lparam);
    
    /**
     * Extract table/alias combos from an arbitrary sql-statement.
     * Please call delTabledefs after you are done.
     * 
     * @param sql A complete or incomplete sql-statement.
     */
    extern void parseSql(char* sql);

    /**
     * 
     * @param alias The alias (or tablename, for simplicity) you want to get the 
     * tablename for.
     * @return The tablename or NULL. The string will be freed on delTableDefs.
     */
    extern char* getTableForAlias(char* alias);

    /**
     * Free all resources, including strings from getTableForAlias.
     */
    extern void delTableDefs(void);

    /**
     * Insert a tabledefinition into the global tree.
     * The strings are duplicated internally and freed on delTabledefs.
     * 
     * @param table A tablename
     * @param alias The used alias for that name. May be NULL.
     * @return 
     */
    extern void* addTableDefNode(char* table, char* alias);

    /**
     * Iterate over all data. Calls cb for every table/alias-pair.
     * @param cb A callback defined as typedef void (* aliascallback)(char* table, char* alias, long lparam);
     * @param lparam The user-defined lparam for cb;
     */
    extern void getTablesAndAliases(aliascallback cb, long lparam);
    
    typedef struct {
        char* table;
        char* alias;
    } t_tdef;

#ifdef	__cplusplus
}
#endif

#endif	/* SQSH_PARSER_H */

