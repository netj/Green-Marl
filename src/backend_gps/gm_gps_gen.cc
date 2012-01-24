
#include <stdio.h>
#include "gm_backend_gps.h"
#include "gm_error.h"
#include "gm_code_writer.h"
#include "gm_frontend.h"
#include "gm_transform_helper.h"

//--------------------------------------------------------------
// A Back-End for GPS generation
//--------------------------------------------------------------
void gm_gps_gen::setTargetDir(const char* d)
{
    if (dname!= NULL)
        printf("%s = \n", dname);
    assert(d!=NULL);
    if (dname != NULL)
        delete [] dname;
    dname = new char[strlen(d)+1];
    strcpy(dname, d);
}

void gm_gps_gen::setFileName(const char* f)
{
    assert(f!=NULL);
    if (fname != NULL)
        delete [] fname;
    fname = new char[strlen(f)+1];
    strcpy(fname, f);
}

bool gm_gps_gen::open_output_files()
{
    char temp[1024];
    assert(dname!=NULL);
    assert(fname!=NULL);

    sprintf(temp, "%s/%s.java", dname, fname);
    f_body = fopen(temp, "w");
    if (f_body == NULL) {
        gm_backend_error(GM_ERROR_FILEWRITE_ERROR, temp);
        return false;
    }
    Body.set_output_file(f_body);

    get_lib()->set_code_writer(&Body);

    return true;
}
void gm_gps_gen::close_output_files()
{
    if (f_body!=NULL) {Body.flush();fclose(f_body); f_body = NULL;}
}

void gm_gps_gen::init_gen_steps()
{
    std::list<gm_compile_step*>& L = get_gen_steps();
    L.push_back(GM_COMPILE_STEP_FACTORY(gm_gps_gen_class));
}

//----------------------------------------------------
// Main Generator
//----------------------------------------------------
bool gm_gps_gen::do_generate()
{

    if (!open_output_files())
        return false;

    bool b = gm_apply_compiler_stage(get_gen_steps());
    assert(b == true);

    close_output_files();

    return true;
}


void gm_gps_gen::end_class()
{
    Body.pushln("}");
}

void gm_gps_gen::generate_proc(ast_procdef* proc)
{
    write_headers();
    begin_class();
    do_generate_master();


    do_generate_vertex();
    end_class();
}

void gm_gps_gen_class::process(ast_procdef* proc)
{
    GPS_BE.generate_proc(proc);
}
