#include <fstream>
#include <sstream>

#include "ext.h"
#include "ext_obex.h"
#include "ext_common.h"
#include "z_dsp.h"
#include "ext_buffer.h"
#include "ext_critical.h"

#include "../jsusfx.h"

class JsusFxMax : public JsusFx {
    using JsusFx::displayMsg;
    using JsusFx::displayError;
public:
    void displayMsg(char *msg) {
        post(msg);
    }

    void displayError(char *msg) {
        error(msg);
    }
};

typedef struct _jsusfx {
    t_pxobject l_obj;
    JsusFxMax *fx;
    char scriptname[MAX_PATH_CHARS];
    short path;
    t_object *m_editor;
    t_critical critical;
    void *outlet1;
} t_jsusfx;

static t_class *jsusfx_class;

void jsusfx_describe(t_jsusfx *x) {
    post("jsusfx~ script %s : %s", x->scriptname, x->fx->desc);
    for(int i=0;i<64;i++) {
        if ( x->fx->sliders[i].exists ) {
            Slider *s = &(x->fx->sliders[i]);
            post(" slider%d: %g %g %s", i, s->min, s->max, s->desc);
            
            t_atom argv[5];
            atom_setlong(argv, i);
            atom_setfloat(argv+1, s->def);
            atom_setfloat(argv+2, s->min);
            atom_setfloat(argv+3, s->max);
            atom_setsym(argv+4, gensym(s->desc));
            
            outlet_anything(x->outlet1 , gensym("slider"), 4, argv);
        } 
    }
}

void jsusfx_dblclick(t_jsusfx *x) {
    if (!x->m_editor) {
        char fullpath[1024];
        path_toabsolutesystempath(x->path, x->scriptname, fullpath);
        std::ifstream is(fullpath);
        char stupid[65535];
        is.read(stupid, 65535);
        if ( ! is.is_open() ) {
			error("jsusfx~: error opening file %s", fullpath);
		}
		
		x->m_editor = reinterpret_cast<t_object *>(object_new(CLASS_NOBOX, gensym("jed"), (t_object *)x, 0));
        object_method(x->m_editor, gensym("filename"), gensym(x->scriptname), x->path);
        object_method(x->m_editor, gensym("settext"), stupid, gensym("utf-8"));
    } else {
        object_attr_setchar(x->m_editor, gensym("visible"), 1);
    }
}

long jsusfx_edsave(t_jsusfx *x, char **ht, long size) {
    std::stringstream ss(*ht);
	
	critical_enter(x->critical);
	if ( x->fx->compile(ss) == true ) {
		x->fx->prepare(sys_getsr(), sys_getblksize());
	}
	critical_exit(x->critical);    
	
    return 0;
}

void jsusfx_edclose(t_jsusfx *x, char **ht, long size) {
    x->m_editor = NULL;
}

void *jsusfx_new(t_symbol *notused, long argc, t_atom *argv) {
	if ( argc < 1 || atom_gettype(argv) != A_SYM ) {
		error("jsusfx~: missing script");
		return NULL;
	}
	t_symbol *s = atom_getsym(argv);
	
    t_fourcc filetype = 'TEXT', outtype;
    short path;
    char filename[MAX_PATH_CHARS];
    strcpy(filename, s->s_name);
    if (locatefile_extended(filename, &path, &outtype, &filetype, 1)) {
        error("jsusfx~: script %s not found", s->s_name);
        return NULL;
    }
    
    char fullpath[1024];
    path_toabsolutesystempath(path, filename, fullpath);
    std::ifstream is(fullpath);   
	if ( ! is.is_open() ) {
		error("jsusfx~: error opening file %s", fullpath);
		return NULL;
	}

    JsusFxMax *fx = new JsusFxMax();
    if ( fx->compile(is) == false ) {
		delete fx;
		return NULL;
	}    
    
	t_jsusfx *x = reinterpret_cast<t_jsusfx *>(object_alloc(jsusfx_class));
    strcpy(x->scriptname, s->s_name);
    x->path = path;
    dsp_setup((t_pxobject *)x, 2);
    x->outlet1 = outlet_new((t_object *)x, NULL);
	outlet_new((t_object *)x, "signal");
    outlet_new((t_object *)x, "signal");

	critical_new(&(x->critical));
    x->fx = fx;
    
    if ( argc >= 2 && atom_gettype(argv+1) == A_LONG ) {
		x->fx->normalizeSliders = atom_getlong(argv+1);
	}
	return (x);
}

void jsusfx_free(t_jsusfx *x) {
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
    
    post("sending %g to %s", value, x->fx->sliders[id].desc);
    
    x->fx->moveSlider(id, value);
}


void jsusfx_perform64(t_jsusfx *x, t_object *dsp64, double **ins, long numins, double **outs, long numouts, long sampleframes, long flags, void *userparam) {
	if ( critical_tryenter(x->critical) ) {
		// code is changing, do nothing
		return;
	}
	
    double *inv[2];
    inv[0] = ins[1];
    inv[1] = ins[0];
    
    x->fx->process64(inv, outs, sampleframes);
    
    critical_exit(x->critical);
}

void jsusfx_dsp64(t_jsusfx *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags) {
    x->fx->prepare(samplerate, maxvectorsize);
	object_method(dsp64, gensym("dsp_add64"), x, jsusfx_perform64, 0, NULL);
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

void jsusfx_assist(t_jsusfx *x, void *b, long m, long a, char *s) {
	if (m == ASSIST_INLET) {
        switch(a) {
            case 0: sprintf(s,"(signal) Left Input");	break;
            case 1: sprintf(s,"(signal) Right Input");	break;
            case 2: sprintf(s,"command messages");	break;
        }
	} else {
		switch (a) {
            case 0: sprintf(s,"(signal) Left Output");	break;
            case 1: sprintf(s,"(signal) Right Output");	break;
            case 2: sprintf(s,"jsfx content");	break;
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
    
    //class_addmethod(c, (method)jsusfx_dsp, "dsp", A_CANT, 0);
    
	class_dspinit(c);
	class_register(CLASS_BOX, c);
	jsusfx_class = c;
	
	JsusFx::init();

	return 0;
}


/*
t_int *jsusfx_perform(t_int *w) {
    t_jsusfx *x = reinterpret_cast<t_jsusfx *>(w[1]);
    //int sz = *(w[2]);
    float **ins = reinterpret_cast<float **>(w[3]);
    float **outs = reinterpret_cast<float **>(w[5]);
    x->fx.process(ins, outs, 64);
    return w+6;
}


void jsusfx_dsp(t_jsusfx *x, t_signal **sp, short *count) {
    x->fx.prepare(sp[0]->s_sr, sp[0]->s_size);
    	dsp_add(jsusfx_peform, 4, x, sp[0]->s_vec, sp[1]->s_vec, sp[0]->s_n);
}
*/

