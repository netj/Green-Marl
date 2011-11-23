#include "gm_gps_basicblock.h"
#include "gm_frontend.h"
#include "gm_backend_gps.h"
#include <set>


extern void gm_flush_reproduce(); 
extern void gm_newline_reproduce();

void gm_gps_basic_block::print()
{
    printf("[--------------------\n");
    printf("%d (%s):\n", id,
            (type==GM_GPS_BBTYPE_SEQ) ?        "SEQ" :
            (type==GM_GPS_BBTYPE_IF_COND) ?    "IF" :
            (type==GM_GPS_BBTYPE_WHILE_COND) ? "WHILE" :
            (type==GM_GPS_BBTYPE_DOWHILE_COND) ? "DOWHILE" :
            (type==GM_GPS_BBTYPE_BEGIN_VERTEX) ? "PAR" : "???"
            );
    //printf("\tnum_entries:%d\n", entries.size());
    printf("\t[ ");
    for(int i=0;i<entries.size();i++) {
        printf("%d ", entries[i]->get_id());
    }
    printf("]=>...\n");

    printf("\n");
    if(type==GM_GPS_BBTYPE_IF_COND) {
        prepare_iter();
        ast_sent* s = get_next(); // should be only one sentence (if)

        ast_if* i = (ast_if*) s;
        i->get_cond()-> reproduce(1);

        gm_newline_reproduce(); 
        gm_flush_reproduce(); 

    } else {

        prepare_iter();
        ast_sent* s = get_next();
        while (s!=NULL)
        { 
            s->reproduce(1);
            s = get_next();
            gm_newline_reproduce(); 
        }
        gm_flush_reproduce(); 
    }

    printf("\t...=>[ ");
    for(int i=0;i<exits.size();i++) {
        printf("%d ", exits[i]->get_id());
    }
    printf("]\n");

    printf("--------------------]\n");
}

// depth-first recurse
static void bb_apply_recurse(std::set<gm_gps_basic_block*>& set, gm_gps_basic_block* B, gps_apply_bb* apply)
{
    apply->apply(B);
    set.insert(B);
    for(int i=0;i <B->get_num_exits(); i++)
    {
        gm_gps_basic_block* b = B->get_nth_exit(i);

        if (set.find(b) == set.end()) 
        {
            bb_apply_recurse(set, b, apply);
        }
    }
}

// return or of has_changed
void gps_bb_apply_only_once(gm_gps_basic_block* entry, gps_apply_bb* apply)
{
    std::set<gm_gps_basic_block*> set;
    set.clear();
    bb_apply_recurse(set, entry, apply);
}

bool  gps_bb_apply_until_no_change(gm_gps_basic_block* entry, gps_apply_bb* apply)
{
    std::set<gm_gps_basic_block*> set;
    bool b = false;
    do {
        apply->set_changed(false);
        set.clear();
        bb_apply_recurse(set, entry, apply);
        if (apply->has_changed()) b = true;
    } while (apply->has_changed());

    return b; // return true if changed at least once
}


class gps_print_apply : public gps_apply_bb {
    virtual void apply(gm_gps_basic_block* b) {
        b->print();
    }
};

void gps_bb_print_all(gm_gps_basic_block* entry)  // return or of has_changed
{
     gps_print_apply G;
     gps_bb_apply_only_once(entry, &G);
}