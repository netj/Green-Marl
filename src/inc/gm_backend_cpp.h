#ifndef GM_BACKEND_CPP
#define GM_BACKEND_CPP

#include "gm_backend.h"
#include "gm_misc.h"
#include "gm_code_writer.h"
#include "gm_compile_step.h"
#include "gm_backend_cpp_opt_steps.h"
#include "gm_backend_cpp_gen_steps.h"

#include <list>

//-----------------------------------------------------------------
// interface for graph library Layer
//  ==> will be deprecated
//-----------------------------------------------------------------
class gm_cpp_gen;
class gm_cpplib : public gm_graph_library {
    public:
    gm_cpplib() {main = NULL;}
    gm_cpplib(gm_cpp_gen* gen) {main = gen;}
    void set_main(gm_cpp_gen* gen) {main = gen;}

    virtual const char* get_header_info() {return "gm.h";}
    virtual const char* get_type_string(ast_typedecl* t);
    virtual const char* get_type_string(int prim_type);

    virtual const char* max_node_index(ast_id* graph);
    virtual const char* max_edge_index(ast_id* graph);
    virtual const char* node_index(ast_id* iter);
    virtual const char* edge_index(ast_id* iter);

    virtual bool do_local_optimize();

    virtual void generate_sent_nop(ast_nop* n); 
    virtual void generate_expr_builtin(ast_expr_builtin* e, gm_code_writer& Body);

    virtual bool add_collection_def(ast_id* set);

    virtual void build_up_language_voca(gm_vocabulary& V);

    virtual bool need_up_initializer(ast_foreach* fe);
    virtual bool need_down_initializer(ast_foreach* fe);
    virtual void generate_up_initializer(ast_foreach* fe, gm_code_writer& Body);
    virtual void generate_down_initializer(ast_foreach* fe, gm_code_writer& Body);
    virtual void generate_foreach_header(ast_foreach* fe, gm_code_writer& Body);

    protected:

    private:
        char str_buf[1024*8];
        gm_cpp_gen* main;
};



//-----------------------------------------------------------------
// interface for graph library Layer
//-----------------------------------------------------------------
class gm_cpp_gen : public gm_backend , public gm_code_generator
{
    friend class nop_reduce_scalar;
    public:
        gm_cpp_gen() : fname(NULL), dname(NULL), f_header(NULL), f_body(NULL),_target_omp(false), _pblock(false), gm_code_generator(Body) {
            glib = new gm_cpplib(this);
            init();
        }
        gm_cpp_gen(gm_cpplib* l) : fname(NULL), dname(NULL), f_header(NULL), f_body(NULL), _target_omp(false),_pblock(false), gm_code_generator(Body) {
            assert(l != NULL);
            glib = l;
            glib->set_main(this);
            init();
        }
    protected:
        void init() {
            init_opt_steps();
            init_gen_steps();
            build_up_language_voca();
        }

    public:
        virtual ~gm_cpp_gen() {close_output_files();}
        virtual void setTargetDir(const char* dname);
        virtual void setFileName(const char* fname);

        virtual bool do_local_optimize_lib();
        virtual bool do_local_optimize();
        virtual bool do_generate();
        virtual void do_generate_begin();
        virtual void do_generate_end();

        /*
         */

    protected:
        std::list<gm_compile_step*> opt_steps;
        std::list<gm_compile_step*> gen_steps;

        virtual void build_up_language_voca();
        virtual void init_opt_steps();
        virtual void init_gen_steps();

        virtual void prepare_parallel_for();
        int _ptr, _indent;

    public:
        gm_cpplib* get_lib() {return glib;}

        //std::list<const char*> local_names;

    public:
        virtual void set_target_omp(bool b) {
            _target_omp = b;
        }
        virtual bool is_target_omp() {return _target_omp;}


    protected:
        // data structure for generation 
        char *fname;        // current source file (without extension)
        char *dname;        // output directory
        bool _target_omp;

        gm_code_writer Header;
        gm_code_writer Body;
        FILE *f_header;
        FILE *f_body;
        bool open_output_files();
        void close_output_files();

        gm_cpplib* glib; // graph library

        // some common sentence
        virtual void add_include( const char* str1, gm_code_writer& Out, bool is_clib = true, const char* str2="");
        virtual void add_ifdef_protection( const char* str);


        //------------------------------------------------------------------------------
        // Generate Method from gm_code_generator
        //------------------------------------------------------------------------------
    public:
        virtual void generate_rhs_id(ast_id* i) ; 
        virtual void generate_rhs_field(ast_field* i) ;
        virtual void generate_expr_builtin(ast_expr* e) ; 
        virtual void generate_expr_minmax(ast_expr* e) ;
        virtual void generate_expr_abs(ast_expr* e);
        virtual void generate_expr_inf(ast_expr* e);

        virtual const char* get_type_string(ast_typedecl* t);
        virtual const char* get_type_string(int prim_type); 
        virtual void generate_lhs_id(ast_id* i) ; 
        virtual void generate_lhs_field(ast_field* i) ;
        virtual void generate_sent_nop(ast_nop* n) ;
        virtual void generate_sent_reduce_assign(ast_assign *a) ;
        virtual void generate_sent_defer_assign(ast_assign *a) {assert(false);} // should not be here
        virtual void generate_sent_vardecl(ast_vardecl *a) ;
        virtual void generate_sent_foreach(ast_foreach *a) ;
        virtual void generate_sent_bfs(ast_bfs* b) ;
        virtual void generate_sent_block(ast_sentblock *b);
        virtual void generate_sent_block(ast_sentblock* b, bool need_br);
        virtual void generate_sent_return(ast_return *r);
        virtual void generate_sent_call(ast_call* c);

        virtual void generate_sent_block_enter(ast_sentblock *b);
        virtual void generate_sent_block_exit(ast_sentblock* b);

        virtual void generate_idlist(ast_idlist *i);
        virtual void generate_proc(ast_procdef* proc);
        void generate_proc_decl(ast_procdef* proc, bool is_body_file);


    protected:
        bool is_under_parallel_sentblock() {return _pblock;}
        void set_under_parallel_sentblock(bool b) {_pblock = b;}

        virtual void declare_prop_def(ast_typedecl* t, ast_id* i);

        bool _pblock;

    protected:
        void generate_bfs_def(ast_bfs* bfs);
        void generate_bfs_body_fw(ast_bfs* bfs);
        void generate_bfs_body_bw(ast_bfs* bfs);
        void generate_bfs_navigator(ast_bfs* bfs);


        const char* i_temp;  // temporary variable name
        char temp[2048];
};

extern gm_cpp_gen CPP_BE;

//---------------------------------------------------
// (NOPS) for CPP/CPP_LIB
//---------------------------------------------------
static enum {
    NOP_REDUCE_SCALAR = 1000,
} nop_enum_cpp;


class nop_reduce_scalar: public ast_nop {
public : 
    nop_reduce_scalar() : ast_nop(NOP_REDUCE_SCALAR) {}
    void set_symbols(
        std::list<gm_symtab_entry*>& O,
        std::list<gm_symtab_entry*>& N,
        std::list<int>& R);

    virtual bool do_rw_analysis(); 
    void generate(gm_cpp_gen* gen);

public:
    std::list<gm_symtab_entry*> old_s;
    std::list<gm_symtab_entry*> new_s;
    std::list<int> reduce_op;
};


// LABELS for extra info
//static const char* LABEL_ITER_ALIAS = "LABEL_ITER_ALIAS";
//static const char* LABEL_LIST_OF_SET = "LABEL_LIST_OF_SET";
//static const char* LABEL_NEED_MEM   = "LABEL_NEED_MEM";
static const char* LABEL_PAR_SCOPE  = "LABEL_PAR_SCOPE";

static const char* CPPBE_INFO_HAS_BFS       = "CPPBE_INFO_HAS_BFS";
static const char* CPPBE_INFO_HAS_PROPDECL  = "CPPBE_INFO_HAS_PROPDECL";
static const char* CPPBE_INFO_IS_PROC_ENTRY = "CPPBE_INFO_IS_PROC_ENTRY";
static const char* CPPBE_INFO_BFS_SYMBOLS   = "CPPBE_INFO_BFS_SYMBOLS";
static const char* CPPBE_INFO_BFS_NAME      = "CPPBE_INFO_BFS_NAME";
static const char* CPPBE_INFO_BFS_LIST      = "CPPBE_INFO_BFS_LIST";
static const char* CPPBE_INFO_COLLECTION_LIST      = "CPPBE_INFO_COLLECTION_LIST";
static const char* CPPBE_INFO_COLLECTION_ITERATOR  = "CPPBE_INFO_COLLECTION_ITERATOR";
static const char* CPPBE_INFO_NEIGHBOR_ITERATOR    = "CPPBE_INFO_NEIGHBOR_ITERATOR";
static const char* CPPBE_INFO_USE_REVERSE_EDGE     = "CPPBE_INFO_USE_REVERSE_EDGE";

//----------------------------------------
// For runtime
//----------------------------------------
#define MAX_THREADS     "gm_rt_get_num_threads"         
#define THREAD_ID       "gm_rt_thread_id"         
#define ALLOCATE_BOOL   "gm_rt_allocate_bool"
#define ALLOCATE_LONG   "gm_rt_allocate_long"
#define ALLOCATE_INT    "gm_rt_allocate_int"
#define ALLOCATE_DOUBLE "gm_rt_allocate_double"
#define ALLOCATE_FLOAT  "gm_rt_allocate_float"
#define DEALLOCATE      "gm_rt_deallocate"
#define CLEANUP_PTR     "gm_rt_cleanup"
#define RT_INIT         "gm_rt_initialize"
#define BFS_TEMPLATE    "gm_bfs_template"
#define DFS_TEMPLATE    "gm_dfs_template"
#define DO_BFS_FORWARD  "do_bfs_forward"
#define DO_BFS_REVERSE  "do_bfs_reverse"
#define DO_DFS          "do_dfs"
#define RT_INCLUDE      "gm.h"
#define PREPARE         "prepare"
#define FREEZE          "freeze"
#define MAKE_REVERSE    "make_reverse_edges"

#endif
