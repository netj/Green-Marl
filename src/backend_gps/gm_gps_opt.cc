
#include <stdio.h>
#include "gm_backend_gps.h"
#include "gm_error.h"
#include "gm_code_writer.h"
#include "gm_frontend.h"
#include "gm_transform_helper.h"

void gm_gps_gen::init_opt_steps()
{
    std::list<gm_compile_step*>& L = get_opt_steps();
    L.push_back(GM_COMPILE_STEP_FACTORY(gm_gps_opt_check_synthesizable));      // check if contains DFS, etc
    L.push_back(GM_COMPILE_STEP_FACTORY(gm_gps_opt_analyze_symbol_scope));     // check where symbols are defined
    L.push_back(GM_COMPILE_STEP_FACTORY(gm_gps_opt_check_canonical));          // check if canonical form
    L.push_back(GM_COMPILE_STEP_FACTORY(gm_gps_opt_create_ebb));               // create (Extended) basic block
    L.push_back(GM_COMPILE_STEP_FACTORY(gm_gps_opt_split_comm_ebb));           // split communicating every BB into two
    L.push_back(GM_COMPILE_STEP_FACTORY(gm_gps_opt_analyze_symbol_usage));     // check how symbols are used
    L.push_back(GM_COMPILE_STEP_FACTORY(gm_gps_opt_analyze_symbol_summary));   // make a summary of symbols per BB
    L.push_back(GM_COMPILE_STEP_FACTORY(gm_gps_opt_find_reachable));           // make a list of reachable BB
}

bool gm_gps_gen::do_local_optimize()
{
    //-----------------------------------
    // [TODO]
    // currently, there should be one and only one top-level procedure
    //-----------------------------------
    if (FE.get_num_procs() != 1) {
        gm_backend_error(GM_ERROR_GPS_NUM_PROCS,"");
        return false;
    }
    
    //-----------------------------------
    // prepare backend information struct
    //-----------------------------------
    FE.prepare_proc_iteration();
    ast_procdef* p;
    while ((p = FE.get_next_proc()) != NULL)
    {
        FE.get_proc_info(p)->set_be_info(new gm_gps_beinfo(p));
    }

    //-----------------------------------
    // Now apply all the steps 
    //-----------------------------------
    return gm_apply_compiler_stage(get_opt_steps());
}


//-----------------------------------------------
// for debug
//-----------------------------------------------

extern bool gm_apply_all_proc(gm_compile_step* step);
class gm_print_bb_t : public gm_compile_step 
{
    virtual gm_compile_step* get_instance() {return new gm_print_bb_t();} 
    virtual void process(ast_procdef * p)
    {
        gm_gps_beinfo* info = (gm_gps_beinfo*) FE.get_backend_info(p);
        if (info == NULL) return;
        if (info->get_entry_basic_block() == NULL) return;
        gps_bb_print_all(info->get_entry_basic_block());
    }
};


void gm_gps_gen::print_basicblock()
{
    gm_print_bb_t T;
    gm_apply_all_proc(&T);
}


