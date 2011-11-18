#ifndef GM_ERROR_H
#define GM_ERROR_H

#include "gm_ast.h"
#include "gm_typecheck.h"

static enum {
 GM_ERROR_UNKNOWN,    
 GM_ERROR_UNDEFINED, 
 GM_ERROR_UNDEFINED_FIELD, 
 GM_ERROR_UNDEFINED_FIELD_GRAPH, 
 //GM_ERROR_MULTIPLE_TARGET, 
 GM_ERROR_PROPERTY_ARGUMENT,
 GM_ERROR_NONGRAPH_TARGET,
 GM_ERROR_DUPLICATE,
 GM_ERROR_NONGRAPH_FIELD,
 GM_ERROR_OPERATOR_MISMATCH,
 GM_ERROR_COMPARE_MISMATCH,
 GM_ERROR_READONLY,
 GM_ERROR_TARGET_MISMATCH,
 GM_ERROR_ASSIGN_TYPE_MISMATCH,
 GM_ERROR_WRONG_PROPERTY,
 //GM_ERROR_INVALID_ITERATOR,
 GM_ERROR_NEED_NODE_ITERATION,
 GM_ERROR_NEED_BFS_ITERATION,
 GM_ERROR_NEED_BOOLEAN,
 GM_ERROR_UNBOUND_REDUCE,
 GM_ERROR_NEED_ITERATOR,
 GM_ERROR_DOUBLE_BOUND_ITOR,
 GM_ERROR_DOUBLE_BOUND_OP,
 //GM_ERROR_WRITE_TO_BOUND_ID,
 //GM_ERROR_WRITE_TO_BOUND_FIELD,
 GM_ERROR_GRAPH_REDUCE,
 GM_ERROR_TYPE_CONVERSION,
 GM_ERROR_TYPE_CONVERSION_BOOL_NUM,

 GM_ERROR_NONSET_TARGET,
 GM_ERROR_NEED_ORDER,

 // Conflict erors
 GM_ERROR_READ_REDUCE_CONFLICT,
 GM_ERROR_WRITE_REDUCE_CONFLICT,
 GM_ERROR_WRITE_WRITE_CONFLICT,
 GM_ERROR_READ_WRITE_CONFLICT,

 GM_ERROR_GROUP_MISMATCH, // error in group assignment
 GM_ERROR_INVALID_BUILTIN, // Invalid builtin
 GM_ERROR_INVALID_BUILTIN_ARG_COUNT, // Invalid builtin
 GM_ERROR_INVALID_BUILTIN_ARG_TYPE,

 GM_ERROR_RETURN_FOR_VOID,
 GM_ERROR_NO_VOID_RETURN,
 GM_ERROR_RETURN_MISMATCH,


 // BACKED ERROS
 GM_ERROR_FILEWRITE_ERROR,


 GM_ERROR_END   // END_MARKER
} GM_ERRORS_AND_WARNINGS;

extern void gm_type_error(int errno, ast_id* id, const char* str1 ="", const char* str2="");
extern void gm_type_error(int errno, ast_id* id, ast_id* id2);
extern void gm_type_error(int errno, int l, int c, const char* str1="", const char* str2="");
extern void gm_type_error(int errno, const char* str);

//extern void gm_conf_error(int errno, gm_symtab_entry* target, ast_id* evidence1);
extern void gm_conf_error(int errno, gm_symtab_entry* target, ast_id* ev1,  ast_id* ev2, bool is_warning);
//extern void gm_conf_warning(int errno, gm_symtab_entry* target, ast_id* evidence1, ast_id* evidence2);

extern void gm_backend_error(int errno, const char* str1, const char* str2="");


extern void gm_set_current_filename(char* fname); 
extern char* gm_get_current_filename();
extern void gm_set_curr_procname(char* pname); 




#endif