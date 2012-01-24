#ifndef GM_GPS_BASICBLOCK_H
#define GM_GPS_BASICBLOCK_H

#include "gm_ast.h"
#include "gm_traverse.h"
#include "gps_syminfo.h"
#include <list>

enum {
    GM_GPS_BBTYPE_SEQ,
    GM_GPS_BBTYPE_IF_COND,
    GM_GPS_BBTYPE_WHILE_COND,
    GM_GPS_BBTYPE_BEGIN_VERTEX,
};

class gm_gps_basic_block {
    public:
    gm_gps_basic_block(int _id, int _type=GM_GPS_BBTYPE_SEQ): id(_id), type(_type), after_vertex(false),
    _has_sender(false) {}
    virtual ~gm_gps_basic_block() {
        std::map<gm_symtab_entry*, gps_syminfo*>::iterator I;
        for(I=symbols.begin(); I!=symbols.end();I++)
        {
            gps_syminfo* s = I->second;
            delete s;
        }
    }

    void prepare_iter() { I = sents.begin();}
    ast_sent* get_next() { if (I!=sents.end()) {ast_sent* s = *I; I++;return s;} else return NULL; }
    std::list<ast_sent*>& get_sents() {return sents;}
    void add_sent(ast_sent* s) {sents.push_back(s);}
    int get_num_sents() {return sents.size();}
    ast_sent* get_1st_sent() {return sents.front();}

    int get_id() {return id;}
    int get_type() {return type;}
    bool is_after_vertex() {return after_vertex;}
    void set_type(int t) {type = t;}
    void set_id(int i) {id = i;}
    void set_after_vertex(bool b) {after_vertex =  b;}

    int get_num_exits() {return exits.size();}
    gm_gps_basic_block* get_nth_exit(int n) {return exits[n];}

    //-------------------------------
    // if: then[0], else[1]
    // while: body[0], exit[1]
    //-------------------------------
    void add_exit(gm_gps_basic_block* b, bool add_reverse=true) {
        exits.push_back(b);
        if (add_reverse)
            b->add_entry(this); // add reverse link
    }
    void remove_all_exits()   {exits.clear(); }
    void remove_all_entries() {entries.clear(); }

    int get_num_entries() {return entries.size();}
    void add_entry(gm_gps_basic_block* b) {
        entries.push_back(b);
    }
    void update_entry_from(gm_gps_basic_block* old, gm_gps_basic_block* to)
    {
        for(int i =0;i<entries.size();i++)
        {
            if (entries[i] == old) {
                entries[i] = to;
                return;
            }
        }
        assert(false);
    }

    gm_gps_basic_block* get_nth_entry(int n) {return entries[n];}

    // for debug
    void print();
    void reproduce_sents();

    bool is_vertex() {return (get_type() == GM_GPS_BBTYPE_BEGIN_VERTEX);}
    bool has_sender() {return _has_sender;}
    void set_has_sender(bool b) {_has_sender = b;}

    // multiple inner loops?
    void         add_receiver_loop  (ast_foreach* fe)              {receivers.push_back(fe);}
    std::list<ast_foreach*>&  get_receiver_loops()                 {return receivers;}
    bool  has_receiver_loops()     {return (receivers.size() >0);}
    void  clear_receiver_loops()   {receivers.clear();}

private:
    std::list<ast_sent*>::iterator I;
    std::list<ast_sent*> sents;

    std::list<ast_foreach*> receivers;

    std::vector<gm_gps_basic_block*> exits;
    std::vector<gm_gps_basic_block*> entries;
    int id;
    int type;  // GM_GPS_BBTYPE_...
    bool after_vertex;
    bool _has_sender;

    // map of used symbols inside this BB
    std::map<gm_symtab_entry*, gps_syminfo*> symbols;

public:
    gps_syminfo* find_symbol_info(gm_symtab_entry *sym) {
        if (symbols.find(sym) == symbols.end())
            return NULL;
        else return symbols.find(sym)->second;
    }
    void add_symbol_info(gm_symtab_entry *sym,gps_syminfo* info)
    {
        symbols[sym] = info;
    }

    std::map<gm_symtab_entry*, gps_syminfo*>& get_symbols() {return symbols;}

};

class gps_apply_bb {
public:
    virtual void apply(gm_gps_basic_block* b)=0;
    virtual bool has_changed() {return changed;}
    virtual void set_changed(bool b) {changed = b;} 
protected:
    bool changed;
};

class gps_apply_bb_ast : public gm_apply, public gps_apply_bb 
{
public:
    gps_apply_bb_ast() : _under_receiver(false) {}

    // defined in gm_gps_misc.cc
    virtual void apply(gm_gps_basic_block* b);
    gm_gps_basic_block *get_curr_BB() {return _curr;}
    void set_is_post(bool b) {_is_post = b;}
    void set_is_pre(bool b) {_is_pre = b;}
    bool is_post() {return _is_post;}
    bool is_pre() {return _is_pre;}


    // set by traverse engine
protected:
    gm_gps_basic_block *_curr;
    bool _is_post;
    bool _is_pre;

    bool is_under_receiver_traverse() {return _under_receiver;}
    void  set_under_receiver_traverse(bool b) {_under_receiver = b;}

    bool _under_receiver;
};

bool gps_bb_apply_until_no_change(gm_gps_basic_block* entry, gps_apply_bb* apply);
void gps_bb_apply_only_once(gm_gps_basic_block* entry, gps_apply_bb* apply); 

void gps_bb_print_all(gm_gps_basic_block* entry); 
void gps_bb_traverse_ast(gm_gps_basic_block* entry, 
                         gps_apply_bb_ast* apply, bool is_post, bool is_pre);


#endif
