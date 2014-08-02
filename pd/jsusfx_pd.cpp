#include <fstream>
#include "m_pd.h"
#include "../jsusfx.h"

class JsusFxPd : public JsusFx {
public:
    void displayMsg(char *msg) {
        post(msg);
    }

    void displayError(char *msg) {
        error(msg);
    }
};

typedef struct _jsusfx {
    t_object x_obj;
    t_float x_f;    
    JsusFxPd *fx;
    char scriptpath[1024];
} t_jsusfx;

static t_class *jsusfx_class;

void jsusfx_describe(t_jsusfx *x) {
    post("jsusfx~ script %s : %s", x->scriptpath, x->fx->desc);
    for(int i=0;i<64;i++) {
        if ( x->fx->sliders[i].exists ) {
            Slider *s = &(x->fx->sliders[i]);
            post(" slider%d: %g %g %s", i, s->min, s->max, s->desc);
        } 
    }
}

void *jsusfx_new(t_symbol *notused, long argc, t_atom *argv) {
	if ( argc < 1 || (argv[0]).a_type != A_SYMBOL ) {
		error("jsusfx~: missing script");
		return NULL;
	}
	t_symbol *s = atom_getsymbol(argv);
	t_symbol *dir = canvas_getcurrentdir();
	
	char result[1024], *bufptr;
	int fd = open_via_path(dir->s_name, s->s_name, "", result, &bufptr, 1024, 1);
	if ( fd == 0 ) {
		error("jsusfx~: unable to find script %s", s->s_name);
		return NULL;
	}
	sys_close(fd);
	
    std::ifstream is(result);   
	if ( ! is.is_open() ) {
		error("jsusfx~: error opening file %s", result);
		return NULL;
	}
	
    JsusFxPd *fx = new JsusFxPd();
    if ( fx->compile(is) == false ) {
		delete fx;
		return NULL;
	}    
      
    t_jsusfx *x = (t_jsusfx *)pd_new(jsusfx_class);	
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal);
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal);
    outlet_new(&x->x_obj, gensym("signal"));
    outlet_new(&x->x_obj, gensym("signal"));
    
	strncpy(x->scriptpath, result, 1024);    
	jsusfx_describe(x);
	
    return (x);
}

void jsusfx_free(t_jsusfx *x) {
	delete x->fx;
}

void jsusfx_slider(t_jsusfx *x, t_float id, t_float value) {
	int i = (int) id;
	
	if ( i > 64 || i < 0 ) 
		return;
	
    if ( ! x->fx->sliders[i].exists ) {
        error("jsusfx~: slider number %d is not assigned for this effect", i);
        return;
    }
    x->fx->moveSlider(i, value);
}

t_int *jsusfx_perform(t_int *w) {
    float *ins[2];
	float *outs[2];
	
    t_jsusfx *x = (t_jsusfx *)(w[1]);
    ins[0] = (float *)(w[2]);
    ins[1] = (float *)(w[3]);
    outs[0] = (float *)(w[4]);
    outs[1] = (float *)(w[5]);
    int n = (int)(w[6]);
    
	x->fx->process(ins, outs, n);	
    
    return (w+7);
}

void jsusfx_dsp(t_jsusfx *x, t_signal **sp) {
	x->fx->prepare(sp[0]->s_sr, sp[0]->s_n);
	dsp_add(jsusfx_perform, 6, x, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec, sp[0]->s_n);
}

void jsusfx_tilde_setup(void) {
    jsusfx_class = class_new(gensym("jsusfx~"), (t_newmethod)jsusfx_new, (t_method)jsusfx_free, sizeof(t_jsusfx), 0L, A_GIMME, 0);
    class_addmethod(jsusfx_class, (t_method)jsusfx_dsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(jsusfx_class, (t_method)jsusfx_slider, gensym("slider"), A_FLOAT, A_FLOAT, 0);
    CLASS_MAINSIGNALIN(jsusfx_class, t_jsusfx, x_f);
    
	JsusFx::init();    
}
