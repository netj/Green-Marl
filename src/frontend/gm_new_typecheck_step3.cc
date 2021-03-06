
#include "gm_ast.h"
#include "gm_typecheck.h"
#include "gm_error.h"
#include "gm_misc.h"
#include "gm_traverse.h"
#include "gm_frontend.h"
#include "gm_frontend_api.h"
#include "gm_transform_helper.h"
#include "gm_builtin.h"

//----------------------------------------------------------------
// Type-check Step 3: 
//   (1) Resolve type of each expression
//   (2) Check function call arguments
//----------------------------------------------------------------

// resolve type of every sub-expression
class gm_typechecker_stage_3 : public gm_apply
{
public:
    gm_typechecker_stage_3() {
        _is_okay = true;
        set_for_expr(true);
    }

    // post apply
    bool apply(ast_expr* e) 
    {
        bool okay = true;
        switch(e->get_opclass())
        {
            case GMEXPR_ID:
                e->set_type_summary(e->get_id()->getTypeSummary());
                // for comparison 
                { 
                    ast_typedecl* t = e->get_id()->getTypeInfo();
                    if (t->is_node_edge_compatible() ||
                        t->is_collection()) {
                        gm_symtab_entry * g =
                            t->get_target_graph_sym();
                        assert(g!=NULL);
                        e->set_bound_graph(g);
                    }
                }
                break;

            case GMEXPR_FIELD:
                e->set_type_summary(e->get_field()->get_second()->getTargetTypeSummary());
                break;

            case GMEXPR_IVAL:
            case GMEXPR_FVAL:
            case GMEXPR_BVAL:
                // already done
                break;

            case GMEXPR_INF:
                break;  // will be resovled later

            case GMEXPR_LUOP:
            case GMEXPR_UOP: 
                okay = check_uop(e);
                break;
            case GMEXPR_LBIOP:
            case GMEXPR_BIOP:
            case GMEXPR_COMP:
                okay = check_binary(e);
                break;

            case GMEXPR_REDUCE:
                { 
                    ast_expr_reduce*r = (ast_expr_reduce*) e;
                    int b_type = r->get_body()->get_type_summary();
                    if (gm_is_unknown_type(b_type)) {
                      okay = false;
                    }
                    else {
                      r->set_type_summary(b_type); 
                    }
                }
                break;
            case GMEXPR_TER:
                okay = check_ter(e);
                break;

            case GMEXPR_BUILTIN:
                {
                    ast_expr_builtin* b = (ast_expr_builtin*) e;
                    okay = check_builtin(b);
                }
                break;
            case GMEXPR_FOREIGN:
                {
                    e->set_type_summary(GMTYPE_FOREIGN_EXPR);
                    okay = true;
                }
                break;
            default:
                assert(false);
        }

        if (okay) {
            assert(!gm_is_unknown_type(e->get_type_summary()));
        }
        set_okay(okay);
        return okay;
    }

    void set_okay(bool b) {_is_okay = b && _is_okay;}
    bool is_okay() {return _is_okay;}

private:
    bool _is_okay;
    bool check_uop(ast_expr* e);
    bool check_binary(ast_expr* e);
    bool check_ter(ast_expr* e);
    bool check_builtin(ast_expr_builtin* e);
};


extern bool gm_check_biop_rule(int op_type, int l_type, int r_type, int& result_type);

bool gm_typechecker_stage_3::check_ter(ast_expr *e)
{
    int op_type = e -> get_optype();
    int l_type = e->get_left_op()->get_type_summary();
    int r_type = e->get_right_op()->get_type_summary();
    int c_type = e->get_cond_op()->get_type_summary();
    int l = e->get_line(); int c = e->get_col();

    if (gm_is_unknown_type(l_type) || gm_is_unknown_type(r_type) || gm_is_unknown_type(c_type)) {
        return false; // no need to check
    }

    if (!gm_is_boolean_type(c_type)) {
        gm_type_error(GM_ERROR_NEED_BOOLEAN, l, c);
        return false;
    }

    // now check the binary part of the expression
    return check_binary(e);
}

// comparison (eq, neq and less)
bool gm_typechecker_stage_3::check_binary(ast_expr* e)
{
    int op_type = e->get_optype();
    int l_type = e->get_left_op()->get_type_summary();
    int r_type = e->get_right_op()->get_type_summary();
    int l = e->get_line(); int c = e->get_col();

    // result is always BOOL
    if (gm_is_boolean_op(op_type) || gm_is_eq_or_less_op(op_type))
        e->set_type_summary(GMTYPE_BOOL); 

    if (gm_is_unknown_type(l_type) || gm_is_unknown_type(r_type)) {
        return false; // no need to check any further
    }
    // node/edge
    if (gm_has_target_graph_type(l_type))
    {
        gm_symtab_entry* l_sym = 
            e->get_left_op()->get_bound_graph();
        gm_symtab_entry* r_sym = 
            e->get_right_op()->get_bound_graph();

        assert(l_sym != NULL);
        assert(r_sym != NULL);

        if (l_sym != r_sym) {
            gm_type_error( GM_ERROR_TARGET_MISMATCH, l, c);
            return false;
        }

        e->set_bound_graph(l_sym);
    }

    int result_type; 
    int l_new;
    int r_new;
    bool w1_warn;
    bool w2_warn;

    bool okay = 
        gm_is_compatible_type(op_type, l_type, r_type,
                            result_type, l_new, r_new,
                            w1_warn,w2_warn);

    if (!okay) {
        gm_type_error(GM_ERROR_OPERATOR_MISMATCH2,
            l, c,
            gm_get_op_string(op_type),
            gm_get_type_string(l_type), 
            gm_get_type_string(r_type));

        return false;
    }

    e->set_type_summary(result_type);

    // [XXX]
    if (w1_warn) {
        printf("warning: type convresion %s->%s\n", gm_get_type_string(l_type), 
                                                    gm_get_type_string(l_new) );
    }
    if (w2_warn) {
        printf("warning: type convresion %s->%s\n", gm_get_type_string(r_type), 
                                                    gm_get_type_string(r_new) );
    }


    return true;
}

bool gm_typechecker_stage_3::check_builtin(ast_expr_builtin* b)
{
    bool okay = true;
    //-------------------------------------------------
    // check arguments
    //-------------------------------------------------
    std::list<ast_expr*>& args = b->get_args();
    std::list<ast_expr*>::iterator I;
    gm_builtin_def*  def = b-> get_builtin_def(); 

    int j = 0;
    for(I=args.begin(); I!= args.end(); I++, j++) {
        ast_expr* e = *I;
        int curr_type = e->get_type_summary();
        int def_type = def->get_arg_type(j);
        if (gm_is_unknown_type(curr_type)) {okay = false; continue;}
        
        // [xxx] to be improved
        //if (!gm_is_compatible_type_for_assign(def_type, curr_type)) {
        int coerced_type;
        bool warning;

        bool okay = gm_is_compatible_type_for_assign(
                    def_type, curr_type,coerced_type, warning);

        if (!okay) {
            char temp[20]; sprintf(temp, "%d", j+1);
            gm_type_error(GM_ERROR_INVALID_BUILTIN_ARG_TYPE,
                    b->get_line(), b->get_col(),
                    b->get_callname(), temp);
            okay = false;
        }
    }

    int fun_ret_type = def->get_result_type_summary();
    b->set_type_summary(fun_ret_type);

    if (gm_has_target_graph_type(fun_ret_type))
    {
        b->set_bound_graph(b->get_driver()->getTypeInfo()->get_target_graph_sym());
       //assert(false); // to be done
    }
    //assert(!gm_has_target_graph_type(fun_ret_type));
    return okay;

}

// type resolve for u-op
bool gm_typechecker_stage_3::check_uop(ast_expr* e)
{
    int op_type = e->get_optype();
    int exp_type = e->get_left_op()->get_type_summary();
    int l = e->get_line(); int c = e->get_col();
    if (gm_is_unknown_type(exp_type)) {
        return false; // no need to check
    }
    if (op_type == GMOP_TYPEC) 
    {
        // should be alredy dest_type;
        int dest_type = e->get_type_summary();
        if (!gm_is_prim_type(dest_type)) { // destination type
            gm_type_error(GM_ERROR_TYPE_CONVERSION, l,c);
            return false;
        }

        if (!gm_is_prim_type(exp_type)) { // source type
            gm_type_error(GM_ERROR_TYPE_CONVERSION, l,c);
            return false;
        } 

        //
        bool possible = 
            (gm_is_numeric_type(dest_type) && gm_is_numeric_type(exp_type)) ||
            (gm_is_boolean_type(dest_type) && gm_is_boolean_type(exp_type)); 
                      
        if (!possible) {
            gm_type_error(GM_ERROR_TYPE_CONVERSION_BOOL_NUM, l,c,"");
            return false;
        }

        return true;
    }
    else if (gm_is_boolean_op(op_type)) { // not
        if (!gm_is_boolean_type(exp_type)) {
            gm_type_error(GM_ERROR_OPERATOR_MISMATCH, 
                l, c, gm_get_op_string(op_type), 
                gm_get_type_string(exp_type));
            return false;
        }

        e->set_type_summary(exp_type);
        return true;
    }
    else if (gm_is_numeric_op(op_type))  { // neg or abs
        if (!gm_is_numeric_type(exp_type)){
            gm_type_error(GM_ERROR_OPERATOR_MISMATCH, 
                l, c, gm_get_op_string(op_type), 
                gm_get_type_string(exp_type));
            return false;
        }

        e->set_type_summary(exp_type);
        return true;
    }

    assert(false);

    return false;
};

void gm_fe_typecheck_step3::process(ast_procdef* p)
{
    gm_typechecker_stage_3 T;
    p->traverse_post(&T);  // post-apply
    set_okay(T.is_okay());
}
