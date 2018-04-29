/*
 * Copyright 2014-2015 Pascal Gauthier
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * *distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

#include <fstream>

#include "ext.h"
#include "ext_obex.h"
#include "ext_common.h"
#include "z_dsp.h"
#include "ext_buffer.h"
#include "ext_critical.h"
#include "ext_path.h"
#include "ext_sysfile.h"

#include "../jsusfx.h"

class JsusFxMax : public JsusFx {
public:    
    void displayMsg(const char *fmt, ...) {
        char output[4096];
        va_list argptr;
        va_start(argptr, fmt);
        vsnprintf(output, 4095, fmt, argptr);
        va_end(argptr);
        
        post("%s\n", output);
    }

    void displayError(const char *fmt, ...) {
        char output[4096];
        va_list argptr;
        va_start(argptr, fmt);
        vsnprintf(output, 4095, fmt, argptr);
        va_end(argptr);
        
        error("%s\n", output);
    }
};

typedef struct _jsusfx {
    t_pxobject l_obj;
    bool bypass;
    
    JsusFxMax *fx;
    
    t_critical critical;
    t_object *m_editor;

    void *outlet1;
    
    char scriptname[MAX_PATH_CHARS];
    short path;
} t_jsusfx;

static t_class *jsusfx_class;

void jsusfx_describe(t_jsusfx *x) {
    post("jsusfx~ script %s : %s", x->scriptname, x->fx->desc);
    for(int i=0;i<64;i++) {
        if ( x->fx->sliders[i].exists ) {
            JsusFx_Slider *s = &(x->fx->sliders[i]);
            post(" slider%d: %g %g %s", i, s->min, s->max, s->desc);
            
            t_atom argv[5];
            atom_setlong(argv, i);
            atom_setfloat(argv+1, s->def);
            atom_setfloat(argv+2, s->min);
            atom_setfloat(argv+3, s->max);
            atom_setsym(argv+4, gensym(s->desc));
            
            outlet_anything(x->outlet1, gensym("slider"), 4, argv);
        } 
    }
}

void jsusfx_showvars(t_jsusfx *x) {
    x->fx->dumpvars();
}

void jsusfx_dblclick(t_jsusfx *x) {
    if (!x->m_editor) {
        t_filehandle fh_read;
        if ( path_opensysfile(x->scriptname, x->path, &fh_read, READ_PERM) ) {
            error("jsusfx~: unable to script file");
            return;
        }
        t_handle texthandle;
        texthandle = sysmem_newhandle(0);
        sysfile_readtextfile(fh_read, texthandle, 0, TEXT_NULL_TERMINATE);
        x->m_editor = reinterpret_cast<t_object *>(object_new(CLASS_NOBOX, gensym("jed"), (t_object *)x, 0));
        object_attr_setchar(x->m_editor, gensym("scratch"), 1);
        object_method(x->m_editor, gensym("settext"), *texthandle, gensym("utf-8"));
        object_method(x->m_editor, gensym("filename"), x->scriptname, x->path);
        sysmem_freehandle(texthandle);
        sysfile_close(fh_read);
    } else {
        object_attr_setchar(x->m_editor, gensym("visible"), 1);
    }
}

void jsusfx_compile(t_jsusfx *x, t_symbol *notused, long argc, t_atom *argv) {
    // new file
    if ( argc >= 1 && atom_gettype(argv) == A_SYM ) {
        t_fourcc filetype = 'TEXT', outtype;
        short path;
        char filename[MAX_PATH_CHARS];
        strcpy(filename, atom_getsym(argv)->s_name);
        if (locatefile_extended(filename, &path, &outtype, &filetype, 1)) {
            error("jsusfx~: script %s not found", filename);
            return;
        }
        
        if ( x->m_editor ) {
            object_method(x->m_editor, gensym("w_close"));
            x->m_editor = NULL;
        }
        
        strncpy(x->scriptname, filename, MAX_PATH_CHARS);
        x->path = path;
    }
    
    char fullpath[1024];
    path_toabsolutesystempath(x->path, x->scriptname, fullpath);
    std::ifstream is(fullpath);
    if ( ! is.is_open() ) {
        error("jsusfx~: error opening file %s", fullpath);
        return;
    }
    
    critical_enter(x->critical);
    if ( x->fx->compile(is) == true ) {
        x->fx->prepare(sys_getsr(), sys_getmaxblksize());
    }
    critical_exit(x->critical);
}

long jsusfx_edsave(t_jsusfx *x, char **ht, long size) {
    t_filehandle fh_write;
    if ( path_opensysfile(x->scriptname, x->path, &fh_write, WRITE_PERM) ) {
        error("jsusfx~: unable to save script file");
        return 1;
    }
    
    sysfile_writetextfile(fh_write, ht, TEXT_LB_NATIVE);
    sysfile_close(fh_write);

    jsusfx_compile(x, NULL, 0, NULL);
    return 0;
}

void jsusfx_edclose(t_jsusfx *x, char **ht, long size) {
    x->m_editor = NULL;
}

void *jsusfx_new(t_symbol *notused, long argc, t_atom *argv) {
    if ( argc < 1 || atom_gettype(argv) != A_SYM ) {
        error("jsusfx~: missing script name");
        return NULL;
    }
    
    t_jsusfx *x = reinterpret_cast<t_jsusfx *>(object_alloc(jsusfx_class));
    t_symbol *s = atom_getsym(argv);
    t_fourcc filetype = 'TEXT', outtype;
    short path;
    char filename[MAX_PATH_CHARS];
    
    strcpy(filename, s->s_name);
    
    if (locatefile_extended(filename, &path, &outtype, &filetype, 1)) {
        t_object *mypatcher;
        object_obex_lookup(x, gensym("#P"), &mypatcher);
        t_symbol *checkExists = object_attr_getsym(mypatcher, gensym("filepath"));
        
        if ( checkExists->s_name[0] == 0 ) {
            error("jsusfx~: patch needs to be saved in order to create new jsusfx script file");
            return NULL;
        }
        
        path = path_getdefault();
                
        t_fourcc type = 'TEXT';
        t_filehandle ref;

        if ( path_createsysfile(filename, path, type, &ref) ) {
            error("jsusfx~: unable to create file");
            return NULL;
        }

        char initText[] = "@sample\nspl0=1\nspl1=-1\n";
        t_handle h = sysmem_newhandle(0);
        sysmem_ptrandhand(initText,h,strlen(initText));
        
        if ( sysfile_writetextfile(ref, h, TEXT_LB_NATIVE) ) {
            error("jsusfx~: unable to write file");
            return NULL;
        }
        
        sysfile_close(ref);
        sysmem_freehandle(h);
    }
    
    strcpy(x->scriptname, filename);
    x->path = path;
    
    char fullpath[MAX_PATH_CHARS];
    path_toabsolutesystempath(path, filename, fullpath);
    std::ifstream is(fullpath);
    if ( ! is.is_open() ) {
        error("jsusfx~: error opening file %s", fullpath);
        return NULL;
    }
    
    x->bypass = false;
    dsp_setup((t_pxobject *)x, 2);
    x->outlet1 = outlet_new((t_object *)x, NULL);
    outlet_new((t_object *)x, "signal");
    outlet_new((t_object *)x, "signal");
    
    critical_new(&(x->critical));
    x->m_editor = NULL;
    JsusFxMax *fx = new JsusFxMax();
    fx->compile(is);
    x->fx = fx;

    /*if ( argc >= 2 && atom_gettype(argv+1) == A_LONG ) {
        x->fx->normalizeSliders = atom_getlong(argv+1);
    } else {
        x->fx->normalizeSliders = 1;
    }
    
    post("normalizer sl %x", x->fx->normalizeSliders);*/
    
    return (x);
}

void jsusfx_free(t_jsusfx *x) {
    if ( x->m_editor )
        object_method(x->m_editor, gensym("w_close"));
    dsp_free((t_pxobject*)x);
    
    critical_free(x->critical);
    delete x->fx;
}

void jsusfx_slider(t_jsusfx *x, t_int id, double value) {
    if ( id >= 64 || id < 0 ) 
        return;
            
    if ( ! x->fx->sliders[id].exists ) {
        error("jsusfx~: slider number %d is not assigned for this effect", id);
        return;
    }

    x->fx->moveSlider(id, value);
}

void jsusfx_bypass(t_jsusfx *x, t_int id) {
    x->bypass = (bool) id;
}

void jsusfx_perform64(t_jsusfx *x, t_object *dsp64, double **ins, long numins, double **outs, long numouts, long sampleframes, long flags, void *userparam) {
    const double *inv[2];
    inv[0] = ins[1];
    inv[1] = ins[0];
    
    if ( x->bypass )
        goto bypass;
    
    if ( critical_tryenter(x->critical) )
        goto bypass;

    x->fx->process64(inv, outs, sampleframes, 2, 2);
    
    critical_exit(x->critical);
    return;
    
bypass:
    for(int i=0;i<sampleframes;i++) {
        outs[0][i] = inv[0][i];
        outs[1][i] = inv[1][i];
    }
    
}

void jsusfx_dsp64(t_jsusfx *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags) {
    x->fx->prepare(samplerate, maxvectorsize);
    object_method(dsp64, gensym("dsp_add64"), x, jsusfx_perform64, 0, NULL);
}

void jsusfx_assist(t_jsusfx *x, void *b, long m, long a, char *s) {
    if (m == ASSIST_INLET) {
        switch(a) {
            case 0: sprintf(s,"(signal) Left Input");   break;
            case 1: sprintf(s,"(signal) Right Input");  break;
        }
    } else {
        switch (a) {
            case 0: sprintf(s,"(signal) Left Output");  break;
            case 1: sprintf(s,"(signal) Right Output"); break;
            case 2: sprintf(s,"jsfx info"); break;
        }
    }
}

int C74_EXPORT main(void) {
    t_class *c = class_new("jsusfx~", (method)jsusfx_new, (method)jsusfx_free, sizeof(t_jsusfx), 0L, A_GIMME, 0);
    
    class_addmethod(c, (method)jsusfx_dsp64, "dsp64", A_CANT, 0);
    class_addmethod(c, (method)jsusfx_slider, "slider", A_LONG, A_FLOAT, 0);
    class_addmethod(c, (method)jsusfx_assist, "assist", A_CANT, 0);
    class_addmethod(c, (method)jsusfx_compile, "compile", A_GIMME, 0);
    class_addmethod(c, (method)jsusfx_dblclick, "dblclick", A_CANT, 0);
    class_addmethod(c, (method)jsusfx_edsave, "edsave", A_CANT, 0);
    class_addmethod(c, (method)jsusfx_edclose, "edclose", A_CANT, 0);
    class_addmethod(c, (method)jsusfx_bypass, "bypass", A_LONG, 0);
    class_addmethod(c, (method)jsusfx_describe, "describe", 0);
    class_addmethod(c, (method)jsusfx_showvars, "showvars", 0);
    
    class_dspinit(c);
    class_register(CLASS_BOX, c);
    jsusfx_class = c;
    
    JsusFx::init();

    return 0;
}
