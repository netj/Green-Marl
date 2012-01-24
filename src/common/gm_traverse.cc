#include "gm_traverse.h"
#include "gm_typecheck.h"
#include "gm_misc.h"

#define POST_APPLY     true
#define PRE_APPLY      false

void ast_procdef::traverse(gm_apply*a, bool is_post, bool is_pre)
{
    bool for_symtab = a->is_for_symtab();
    bool for_id = a->is_for_id();
    bool for_proc = a->is_for_proc();
    
    a->begin_context(this);

    if (is_pre) {
        if (for_symtab)
            apply_symtabs(a, PRE_APPLY);
        if (for_id)
            apply_id(a, PRE_APPLY);
        if (for_proc)
            a->apply(this);
    }


    // traverse body
    ((ast_sent*)get_body())->traverse(a, is_post, is_pre);

    if (is_post) {
        if (for_symtab)
            apply_symtabs(a, POST_APPLY);
        if (for_id)
            apply_id(a, POST_APPLY);
        if (for_proc) {
            if (a->has_separate_post_apply())
                a->apply2(this);
            else
                a->apply(this);
        }
    }

    a->end_context(this);
}

void ast_procdef::apply_id(gm_apply* a, bool is_post)
{
    //--------------------------
    // [todo] fix for name and return type
    //--------------------------
    // [todo] symbol-table for (name && signature, return type)
    // a->apply(get_procname());
    std::list<ast_argdecl*>::iterator it;
    {
        std::list<ast_argdecl*>& args = get_in_args();
        for(it=args.begin();it!=args.end();it++) {
            ast_idlist* idl = (*it)->get_idlist(); 
            idl->apply_id(a, is_post);
        }
    }
    {
        std::list<ast_argdecl*>& args = get_out_args();
        for(it=args.begin();it!=args.end();it++) {
            ast_idlist* idl = (*it)->get_idlist(); 
            idl->apply_id(a, is_post);
        }
    }
}
static void apply_symtab_each(gm_apply* a, gm_symtab* s, int symtab_type, bool is_post)
{
    std::vector<gm_symtab_entry*> v = s->get_entries();
    for(int i=0;i<v.size();i++)
    {
        if (is_post && a->has_separate_post_apply()) {
            a->apply2(v[i], symtab_type);
        }
        else {
            a->apply(v[i], symtab_type);
        }
    }
}

void ast_node::apply_symtabs(gm_apply* a, bool is_post)
{
    assert(has_scope());
    bool post_apply = is_post && a->has_separate_post_apply();
    if (post_apply) {
        a->apply2(get_symtab_var(), (int)GM_SYMTAB_ARG);
    } else {
        a->apply(get_symtab_var(), (int)GM_SYMTAB_ARG);
    }
    apply_symtab_each(a, get_symtab_var(), GM_SYMTAB_ARG, is_post);

    if (post_apply) {
        a->apply2(get_symtab_field(), (int)GM_SYMTAB_FIELD);
    } else {
        a->apply(get_symtab_field(), (int)GM_SYMTAB_FIELD);
    }
    apply_symtab_each(a, get_symtab_field(), GM_SYMTAB_FIELD, is_post);

    if (post_apply) {
        a->apply2(get_symtab_proc(), (int)GM_SYMTAB_PROC);
    } else {
        a->apply(get_symtab_proc(), (int)GM_SYMTAB_PROC);
    }
    apply_symtab_each(a, get_symtab_proc(), GM_SYMTAB_PROC, is_post);
}


//----------------------------------------------------------------------------------
    //    [begin_context] 
    //    apply(sent)
    //    apply(symtab_entry)
    //      ... per sentence-subtype pre: (expr, id, ...)
    //      ...... per sentence-subtype recursive traverse
    //      ... per sentence-subtype post: (expr, id, ...)
    //    apply2(symtab_entry)
    //    apply2(sent)
    //    [end_context]
//----------------------------------------------------------------------------------
void ast_sent::traverse(gm_apply*a, bool is_post, bool is_pre )
{
    if (has_symtab()) a->begin_context(this);
    bool for_symtab = a->is_for_symtab();
    bool for_sent = a->is_for_sent();


    if (is_pre) {
        if (for_sent)
            a->apply(this);
        if (has_symtab() && for_symtab)
            apply_symtabs(a, PRE_APPLY);
    }

    traverse_sent(a,is_post, is_pre);

    if (is_post) {
        if (has_symtab() && for_symtab) {
            apply_symtabs(a, POST_APPLY);
        }
        if (for_sent) {
            if (a->has_separate_post_apply())
                a->apply2(this);
            else
                a->apply(this);
        }
    }


    if (has_symtab()) a->end_context(this);
}


void ast_sentblock::traverse_sent(gm_apply*a, bool is_post, bool is_pre )
{
    //a->begin_context(this);

    std::list<ast_sent*>& sents = get_sents();
    std::list<ast_sent*>::iterator i;
    for(i=sents.begin();i!=sents.end();i++) 
        (*i)->traverse(a, is_post, is_pre);

    //a->end_context(this);
}

void ast_vardecl::traverse_sent(gm_apply*a, bool is_post, bool is_pre )
{
    bool for_id = a->is_for_id();
    if (for_id) {
        ast_idlist* idl = get_idlist();

        if (is_pre)
            idl->apply_id(a, PRE_APPLY);
        
        if (is_post)
            idl->apply_id(a, POST_APPLY);
    }
}


void ast_foreach::traverse_sent(gm_apply*a, bool is_post, bool is_pre)
{
    //a->begin_context(this);

    bool for_id = a->is_for_id();

    if (is_pre) {
        if (for_id) {
            ast_id* src = get_source();
            ast_id* it = get_iterator();
            a->apply(src);
            a->apply(it);
        }
    }

    // traverse
    ast_sent* ss = get_body();
    ss->traverse(a, is_post, is_pre);

    ast_expr* f = get_filter();
    if (f!=NULL)
        f->traverse(a, is_post, is_pre);


    if (is_post) {
        if (for_id) {
            ast_id* src = get_source();
            ast_id* id = get_iterator();
            if (a->has_separate_post_apply()) {
                a->apply2(src);
                a->apply2(id);
            } else {
                a->apply(src);
                a->apply(id);
            }
        }
    }
}

void ast_bfs::traverse_sent(gm_apply*a, bool is_post, bool is_pre)
{
    bool for_id = a->is_for_id();

    if (is_pre) {
        if (for_id) {
            ast_id* src = get_source();
            ast_id* it = get_iterator();
            ast_id* root = get_root();
            a->apply(src);
            a->apply(it);
            a->apply(root);
        }
    }

    // traverse
    ast_expr *n = get_navigator(); 
    ast_expr *fc = get_f_filter(); 
    ast_expr *bc = get_b_filter(); 
    if (n!= NULL) n->traverse(a, is_post, is_pre);
    if (fc!= NULL) fc->traverse(a, is_post, is_pre);
    if (bc!= NULL) bc->traverse(a, is_post, is_pre);

    ast_sentblock* fb = get_fbody();
    ast_sentblock* bb = get_bbody();
    if (fb != NULL) fb->traverse(a, is_post, is_pre);
    if (bb != NULL) bb->traverse(a, is_post, is_pre);

    if (is_post) {
        if (for_id) {
            ast_id* src = get_source();
            ast_id* id = get_iterator();
            ast_id* root = get_root();
            if (a->has_separate_post_apply()) {
                a->apply2(src);
                a->apply2(id);
                a->apply2(root);
            }
            else {
                a->apply(src);
                a->apply(id);
                a->apply(root);
            }
        }
    }
}


void ast_assign::traverse_sent(gm_apply*a, bool is_post, bool is_pre)
{
    bool for_id = a->is_for_id();

    if (is_pre) {
        if (for_id) {
            if (get_lhs_type() == GMASSIGN_LHS_SCALA) {
                a->apply(get_lhs_scala());
            } else { // LHS_FIELD
                a->apply(get_lhs_field()->get_first());
                a->apply(get_lhs_field()->get_second());
            }
            if (get_bound() != NULL)  // REDUCE or DEFER
                a->apply(get_bound());
        }
    }

    get_rhs()->traverse(a, is_post, is_pre);

    if (is_post) {
        bool b = a->has_separate_post_apply();
        if (for_id) {
            if (get_lhs_type() == GMASSIGN_LHS_SCALA) {
                if (b) a->apply2(get_lhs_scala());
                else a->apply(get_lhs_scala());
            } else { // LHS_FIELD
                if (b) {
                    a->apply2(get_lhs_field()->get_first());
                    a->apply2(get_lhs_field()->get_second());
                } else {
                    a->apply(get_lhs_field()->get_first());
                    a->apply(get_lhs_field()->get_second());
                }
            }
            if (get_bound() != NULL)  // REDUCE or DEFER
            {
                if (b) a->apply2(get_bound());
                else a->apply(get_bound());
            }
        }
    }
}

void ast_return::traverse_sent(gm_apply*a, bool is_post, bool is_pre)
{
    if (get_expr()!=NULL)
        get_expr()->traverse(a, is_post, is_pre);
}

void ast_if::traverse_sent(gm_apply*a, bool is_post, bool is_pre)
{
    // traverse only
   get_cond()-> traverse( a, is_post, is_pre);
   get_then()-> traverse( a, is_post, is_pre);
    if (get_else() != NULL) {
        get_else()->traverse(a, is_post, is_pre);
    }
}

void ast_while::traverse_sent(gm_apply*a, bool is_post, bool is_pre)
{
    // traverse only
   get_cond()-> traverse( a, is_post, is_pre);
   get_body()-> traverse( a, is_post, is_pre);
}

void ast_call::traverse_sent(gm_apply*a, bool is_post, bool is_pre)
{
    assert(is_builtin_call());
    b_in->traverse(a, is_post, is_pre);
}



void ast_idlist::apply_id(gm_apply*a, bool is_post_apply)
{
    for(int i=0;i<get_length();i++) {
        ast_id* id = get_item(i);
        if (is_post_apply && a->has_separate_post_apply())
            a->apply2(id);
        else
            a->apply(id);
    }
}

void ast_foreign::traverse_sent(gm_apply* a, bool is_post, bool is_pre)
{
    bool for_id = a->is_for_id();
    bool for_expr = a->is_for_expr();
    bool b = a->has_separate_post_apply();
    if (is_pre) {
        if (for_id) {
            std::list<ast_node*>::iterator I;
            for(I=modified.begin(); I!= modified.end(); I++) {
                if ((*I)->get_nodetype() == AST_ID) {
                    ast_id* id = (ast_id*) (*I);
                    a->apply(id);
                }
                else if ((*I)->get_nodetype() == AST_FIELD) {
                    ast_id* id1 = ((ast_field*) (*I))->get_first();
                    ast_id* id2 = ((ast_field*) (*I))->get_second();
                    a->apply(id1);
                    a->apply(id2);
                }
            }
        }
        if (for_expr) a->apply(expr);
    }

    if (for_expr || for_id) 
        expr->traverse(a, is_post, is_pre);

    if (is_post) {
        if (for_id) {
            std::list<ast_node*>::iterator I;
            for(I=modified.begin(); I!= modified.end(); I++) {
                if ((*I)->get_nodetype() == AST_ID) {
                    ast_id* id = (ast_id*) (*I);
                    if (b) a->apply2(id);
                    else   a->apply(id);
                }
                else if ((*I)->get_nodetype() == AST_FIELD) {
                    ast_id* id1 = ((ast_field*) (*I))->get_first();
                    ast_id* id2 = ((ast_field*) (*I))->get_second();
                    if (b) {  a->apply2(id1);a->apply2(id2);} 
                    else   {  a->apply(id1); a->apply(id2);}
                }
            }
        }
        if (for_expr) {
            if (b) a->apply2(expr);
            else   a->apply(expr);
        }
    }

}

void ast_expr_reduce::traverse(gm_apply*a, bool is_post, bool is_pre)
{
    a->begin_context(this);

    bool for_id = a->is_for_id();
    bool for_expr = a->is_for_expr();
    bool for_symtab = a->is_for_symtab();


    if (is_pre) {
        if (for_symtab) {
            apply_symtabs(a, PRE_APPLY);
        }
        if (for_id) {
            ast_id* src = get_source();
            ast_id* it = get_iterator();
            a->apply(src);
            a->apply(it);
        }
        if (for_expr)
            a->apply(this);
    }

    if (get_filter() != NULL) get_filter()->traverse(a, is_post, is_pre);

    // fixme: get_body might be null; in the middle for syntax-sugar2 transform (Sum=>Foreach)
    if (get_body()!=NULL) get_body()->traverse(a, is_post, is_pre);
    
    if (is_post) {
        bool b = a->has_separate_post_apply();
        if (for_symtab) {
            apply_symtabs(a, POST_APPLY);
        }
        if (for_id) {
            ast_id* src = get_source();
            ast_id* it = get_iterator();
            if (b) {
                a->apply2(src);
                a->apply2(it);
            } else  {
                a->apply(src);
                a->apply(it);
            }
        }
        if (for_expr) {
            if (b) a->apply2(this);
            else a->apply(this);
        }    
    }

    a->end_context(this);
}

void ast_expr_builtin::traverse(gm_apply* a, bool is_post, bool is_pre)
{
    bool for_sent = a->is_for_sent();
    bool for_id = a->is_for_id();
    bool for_expr = a->is_for_expr();

    if (is_pre) {
        if (for_id && (driver != NULL))
            a->apply(driver);
        if (for_expr)
            a->apply(this);
    }

    std::list<ast_expr*>::iterator I;
    for(I = args.begin(); I!= args.end(); I++) {
        ast_expr* e = *I;
        e->traverse(a, is_post, is_pre);
    }

    if (is_post) {
        bool b = a->has_separate_post_apply();
        if (for_id && (driver != NULL))
        {
            if (b) a->apply2(driver);
            else a->apply(driver);
        }
        if (for_expr) {
            if (b) a->apply2(this);
            else a->apply(this);
        }
    }
}

void ast_expr_foreign::apply_id(gm_apply* a, bool apply2)
{
    std::list<ast_node*>::iterator I;
    for(I=parsed_gm.begin(); I!=parsed_gm.end(); I++) {
        ast_node* n = *I;
        if (n==NULL) continue;
        if (n->get_nodetype() == AST_ID) {
            ast_id* id = (ast_id*) n;
            if (apply2) a->apply2(id);
            else a->apply(id);
        }
        else if (n->get_nodetype() == AST_FIELD) {
            ast_field* f = (ast_field*) n;
            if (apply2) {a->apply2(f->get_first()); a->apply2(f->get_second());}
            else        {a->apply(f->get_first());  a->apply(f->get_second());}
        }
    }
}

void ast_expr_foreign::traverse(gm_apply*a, bool is_post, bool is_pre)
{
    bool for_id = a->is_for_id();
    bool for_expr = a->is_for_expr();
    if (is_pre) {
        if (for_id) {
            apply_id(a, false);
        } 
    }

    if (for_expr)
        a->apply(this);

    if (is_post) {
        if (for_id) {
            apply_id(a, a->has_separate_post_apply());
        } 
    }



}

void ast_expr::traverse(gm_apply*a, bool is_post, bool is_pre)
{
    bool for_sent = a->is_for_sent();
    bool for_id = a->is_for_id();
    bool for_expr = a->is_for_expr();
    bool for_symtab = a->is_for_symtab();

    if (!(for_id || for_expr || for_symtab)) return; // no more sentence behind this

    if (for_expr && is_pre)
        a->apply(this);

    bool b = a->has_separate_post_apply();
    switch(get_opclass())
    {
        case GMEXPR_ID:
            if (for_id) {
                if (is_pre) a->apply(get_id());
                if (is_post) {
                    if (b) a->apply2(get_id());
                    else a->apply(get_id());
                }
            }
            break;
        case GMEXPR_FIELD:
            if (for_id) {
                if (is_pre) {
                    a->apply(get_field()->get_first());
                    a->apply(get_field()->get_second());
                } 
                if (is_post) {
                    if (b) {
                        a->apply2(get_field()->get_first());
                        a->apply2(get_field()->get_second());
                    } else {
                        a->apply(get_field()->get_first());
                        a->apply(get_field()->get_second());
                    }
                }
            }
            break;
        case GMEXPR_UOP:
        case GMEXPR_LUOP:
            get_left_op()->traverse(a, is_post, is_pre);
            break;
        case GMEXPR_BIOP:
        case GMEXPR_LBIOP:
        case GMEXPR_COMP:
            get_left_op()->traverse(a, is_post, is_pre);
            get_right_op()->traverse(a, is_post, is_pre);
            break;
        case GMEXPR_TER:
            get_cond_op()->traverse(a, is_post, is_pre);
            get_left_op()->traverse(a, is_post, is_pre);
            get_right_op()->traverse(a, is_post, is_pre);
            break;

        case GMEXPR_IVAL:
        case GMEXPR_FVAL:
        case GMEXPR_BVAL:
        case GMEXPR_INF:
            break;

        case GMEXPR_BUILTIN:
        case GMEXPR_FOREIGN:
        case GMEXPR_REDUCE:
            assert(false); // should not be in here
            break;

            
        default:
            assert(false);
            break;
    }

    if (for_expr && is_post)
    {
        bool b = a->has_separate_post_apply();
        if (b) a->apply2(this);
        else a->apply(this);
    }

    return;
}


// traverse all the sentences, upward.
bool gm_traverse_up_sent(ast_node* n, gm_apply *a)
{
    if (n==NULL) return true;

    else if (n->is_sentence()) {
        bool b = a->apply((ast_sent*) n);
        if (!b) return false;
    }

    return gm_traverse_up_sent(n->get_parent(), a);
}


