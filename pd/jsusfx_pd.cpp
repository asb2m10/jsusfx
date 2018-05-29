/*
 * Copyright 2014-2018 Pascal Gauthier
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
#include "m_pd.h"
#include "WDL/mutex.h"
#include "jsusfx.h"
#include <stdarg.h>

// The maximum of signal inlet/outlet
const int MAX_SIGNAL_PORT = 16;

class JsusFxPdPath : public JsusFxPathLibrary_Basic {
public:
    JsusFxPdPath(const char * _dataRoot) : JsusFxPathLibrary_Basic(_dataRoot) {
    }
    
    bool resolveImportPath(const std::string &importPath, const std::string &parentPath, std::string &resolvedPath) {
        char result[1024];
        char *bufptr;
        int fd = open_via_path(parentPath.c_str(), importPath.c_str(), "", result, &bufptr, 1023, 1);
        if ( fd < 0 || result[0] == 0 ) {
            return false;
        }
        sys_close(fd);        
        resolvedPath = result;        
        return true;
    }
    
    bool resolveDataPath(const std::string &importPath, std::string &resolvedPath) {
        char result[1024];
        char *bufptr;
        int fd = open_via_path(dataRoot.c_str(), importPath.c_str(), "", result, &bufptr, 1023, 1);
        if ( fd < 0 || result[0] == 0 ) {
            return false;
        }
        sys_close(fd);        
        resolvedPath = result;
        return true;
    }
};

class JsusFxPd : public JsusFx {
public:
    JsusFxPd(JsusFxPathLibrary &pathLibrary) : JsusFx(pathLibrary) {   
    }
    
    void displayMsg(const char *fmt, ...) {
        char output[4096];
        va_list argptr;
        va_start(argptr, fmt);
        vsnprintf(output, 4095, fmt, argptr);
        va_end(argptr);

        post(output);
    }

    void displayError(const char *fmt, ...) {
        char output[4096];
        va_list argptr;
        va_start(argptr, fmt);
        vsnprintf(output, 4095, fmt, argptr);
        va_end(argptr);

        error("%s", output);
    }

    WDL_Mutex dspLock;
};

typedef struct _jsusfx {
    t_object x_obj;
    t_float x_f;
    JsusFxPd *fx;
    JsusFxPdPath *path;
    char scriptpath[2048];
    bool bypass;
    bool user_bypass;
    int pinIn, pinOut;
    t_int **dspVect;
} t_jsusfx;

static t_class *jsusfx_class;
static t_class *jxrt_class;
static t_class *inlet_proxy;

typedef struct _inlet_proxy {
    t_object x_obj;
    t_jsusfx *peer;
    int idx;
} t_inlet_proxy;

void jsusfx_describe(t_jsusfx *x) {
    post("jsusfx~ script %s : %s", x->scriptpath, x->fx->desc);
    for(int i=0;i<64;i++) {
        if ( x->fx->sliders[i].exists ) {
            JsusFx_Slider *s = &(x->fx->sliders[i]);
            if ( s->inc == 0 )
                post(" slider%d: %g %g %s [%g]", i, s->min, s->max, s->desc, *(s->owner));
            else
                post(" slider%d: %g %g (%g) %s [%g]", i, s->min, s->max, s->inc, s->desc, *(s->owner));
        }
    }
}

void jsusfx_dumpvars(t_jsusfx *x) {
    post("jsusfx~ vars for: %s =========", x->fx->desc);
    if ( x->fx != NULL )
        x->fx->dumpvars();
}

void jsusfx_compile(t_jsusfx *x, t_symbol *newFile) {
    x->bypass = true;
    std::ifstream *is;

    if ( newFile != NULL && newFile->s_name[0] != 0) {
        std::string result;

        if ( ! x->path->resolveDataPath(std::string(newFile->s_name), result) ) {
            error("jsusfx~: unable to find script %s", newFile->s_name);
            return;
        }

        result += '/';
        result += newFile->s_name;

        is = new std::ifstream(result);
        if ( ! is->is_open() ) {
            error("jsusfx~: error opening file %s", result.c_str());
            delete is;
            return;
        }
        strncpy(x->scriptpath, result.c_str(), 1024);
    } else {
        if ( x->scriptpath[0] == 0 )
            return;
        is = new std::ifstream(x->scriptpath);
        if ( ! is->is_open() ) {
            error("jsusfx~: error opening file %s", x->scriptpath);
            delete is;
            return;
        }
    }

    x->fx->dspLock.Enter();
    if ( x->fx->compile(*is) ) {
        if ( x->fx->srate != 0 )
            x->fx->prepare(*(x->fx->srate), *(x->fx->samplesblock));
        x->bypass = false;
    } else {
        x->bypass = true;
    }
    x->fx->dspLock.Leave();

    delete is;

    if ( ! x->bypass )
        jsusfx_describe(x);
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

void jsusfx_bypass(t_jsusfx *x, t_float id) {
    x->user_bypass = id != 0;
}

t_int *jsusfx_perform(t_int *w) {
    const float *ins[MAX_SIGNAL_PORT];
    float *outs[MAX_SIGNAL_PORT];
    int argc = 2;
    
    t_jsusfx *x = (t_jsusfx *)(w[1]);
    for(int i=0;i<x->pinIn;i++)
        ins[i] = (float *)(w[argc++]);
    for(int i=0;i<x->pinOut;i++)
        outs[i] = (float *)(w[argc++]);
    int n = (int)(w[argc++]);
    
    if ( (x->bypass || x->user_bypass) || x->fx->dspLock.TryEnter() ) {
        //x->fx->displayMsg("system is bypassed");
        for(int i=0;i<x->pinOut;i++) {
            for(int j=0;j<n;j++)
                outs[i][j] = 0;
        }
    } else {
        x->fx->process(ins, outs, n, x->pinIn, x->pinOut);
        x->fx->dspLock.Leave();
    }

    return (w+argc);
}

void jsusfx_dsp(t_jsusfx *x, t_signal **sp) {
    x->fx->prepare(sp[0]->s_sr, sp[0]->s_n);
    
    x->dspVect[0] = (t_int *) x;
    int i, j;
    for (i=0; i<x->pinIn; i++) {
        x->dspVect[i+1] = (t_int*)sp[i]->s_vec;
        post("jsusfx~ dsp-vecin: %x", x->dspVect[i+1]);
    }
    
    for (j=0;j<x->pinOut; j++) {
        x->dspVect[i+j+1] = (t_int*)sp[i+j]->s_vec;
        post("jsusfx~ dsp-vecout: %x", x->dspVect[i+j+1]);
    }
    x->dspVect[i+j+1] = (t_int *) ((long)sp[0]->s_n);

    dsp_addv(jsusfx_perform, x->pinIn + x->pinOut + 2, (t_int*)x->dspVect);
}

void *jsusfx_new(t_symbol *notused, long argc, t_atom *argv) {
    t_jsusfx *x = (t_jsusfx *)pd_new(jsusfx_class);
    x->path = new JsusFxPdPath(canvas_getcurrentdir()->s_name);
    x->bypass = true;
    x->user_bypass = false;
    x->scriptpath[0] = 0;
    x->fx = new JsusFxPd(*(x->path));
    x->fx->normalizeSliders = 1;

    x->pinIn = 2;
    x->pinOut = 2;

    if ( argc < 1 ) {
        post("jsusfx~: missing script");
    } else {
        int argPos = 0;

        if ( (argv[0]).a_type == A_SYMBOL ) {
            t_symbol *s = atom_getsymbol(argv);
            jsusfx_compile(x, s);
            if (! x->bypass) { 
                // the first compile will permantly set the number of pins for this instance. (unless it is
                // specified in the pd object arguments)
                x->pinIn = x->fx->numInputs;
                x->pinOut = x->fx->numOutputs;
            }
            argPos++;
        }
        
        if ( argc > argPos ) {
            if ( (argv[argPos]).a_type == A_FLOAT )
                x->pinIn = atom_getfloat(&argv[argPos]);
        }
        argPos++;

        if ( argc > argPos ) {
            if ( (argv[argPos]).a_type == A_FLOAT )
                x->pinOut = atom_getfloat(&argv[argPos]);
        }
    }
    
    if ( x->pinIn > MAX_SIGNAL_PORT )
        x->pinIn = MAX_SIGNAL_PORT;
    // we cannot set an object without signal inlet since it is created by default
    // There is probably a better to do this.
    if ( x->pinIn < 1 )
        x->pinIn = 1;
    if ( x->pinOut > MAX_SIGNAL_PORT )
        x->pinOut = MAX_SIGNAL_PORT;
    if ( x->pinOut < 0 ) 
        x->pinOut = 0;

    for(int i=1;i<x->pinIn;i++)
        signalinlet_new(&x->x_obj, 0);

    for(int i=0;i<x->pinOut;i++)
        outlet_new(&x->x_obj, gensym("signal"));

    x->dspVect = (t_int **)t_getbytes(sizeof(t_int *) * (x->pinIn + x->pinOut + 2));
    return (x);
}

void jsusfx_free(t_jsusfx *x) {
    t_freebytes(x->dspVect, sizeof(t_int) * (x->pinIn + x->pinOut + 2));
    delete x->fx;
    delete x->path;
}

void *jxrt_new(t_symbol *script) {
    if ( script == NULL || script->s_name[0] == 0) {
        error("jsusfx~: missing script");
        return NULL;
    }
    
    t_jsusfx *x = (t_jsusfx *)pd_new(jsusfx_class);
    x->path = new JsusFxPdPath(canvas_getcurrentdir()->s_name);
    x->bypass = true;
    x->user_bypass = false;
    x->scriptpath[0] = 0;
    x->fx = new JsusFxPd(*(x->path));
    x->fx->normalizeSliders = 1;
    
    x->pinIn = 2;
    x->pinOut = 2;
    
    jsusfx_compile(x, script);
    if (x->bypass == true) {
        //something went wrong with the compilation. bailout
        delete x->fx;
        delete x->path;
        return NULL;
    }
    
    if ( x->pinIn > MAX_SIGNAL_PORT )
        x->pinIn = MAX_SIGNAL_PORT;
    if ( x->pinIn < 1 )
        x->pinIn = 1;
    if ( x->pinOut > MAX_SIGNAL_PORT )
        x->pinOut = MAX_SIGNAL_PORT;
    if ( x->pinOut < 0 )
        x->pinOut = 0;
    
    for(int i=1;i<x->pinIn;i++)
        signalinlet_new(&x->x_obj, 0);
    
    for(int i=0;i<x->pinOut;i++)
        outlet_new(&x->x_obj, gensym("signal"));
    
    x->dspVect = (t_int **)t_getbytes(sizeof(t_int *) * (x->pinIn + x->pinOut + 2));
    
    for(int i=1;i<64;i++) {
        if ( x->fx->sliders[i].exists ) {
            t_inlet_proxy *proxy = (t_inlet_proxy *) pd_new(inlet_proxy);
            proxy->idx = i;
            proxy->peer = x;
            inlet_new(&x->x_obj, &proxy->x_obj.ob_pd, 0, 0);
        } else {
            break;
        }
    }

    return (x);
}

void jxrt_free(t_jsusfx *x) {
    delete x->fx;
    delete x->path;
    // delete also the inlet proxy or it is done automatically ?
}

static void inlet_float(t_inlet_proxy *proxy, t_float f) {
    proxy->peer->fx->moveSlider(proxy->idx, f);
}

extern "C" {
    void jsusfx_tilde_setup(void) {
        jsusfx_class = class_new(gensym("jsusfx~"), (t_newmethod)jsusfx_new, (t_method)jsusfx_free, sizeof(t_jsusfx), 0L, A_GIMME, 0);
        class_addmethod(jsusfx_class, (t_method)jsusfx_dsp, gensym("dsp"), A_CANT, 0);
        class_addmethod(jsusfx_class, (t_method)jsusfx_slider, gensym("slider"), A_FLOAT, A_FLOAT, 0);
        class_addmethod(jsusfx_class, (t_method)jsusfx_compile, gensym("compile"), A_DEFSYMBOL, 0);
        class_addmethod(jsusfx_class, (t_method)jsusfx_describe, gensym("describe"), A_NULL, 0);
        class_addmethod(jsusfx_class, (t_method)jsusfx_dumpvars, gensym("dumpvars"), A_NULL, 0);
        class_addmethod(jsusfx_class, (t_method)jsusfx_bypass, gensym("bypass"), A_FLOAT, 0);
        CLASS_MAINSIGNALIN(jsusfx_class, t_jsusfx, x_f);

        jxrt_class = class_new(gensym("jxrt~"), (t_newmethod)jxrt_new, (t_method)jxrt_free, sizeof(t_jsusfx), 0L, A_SYMBOL, 0);
        class_addmethod(jxrt_class, (t_method)jsusfx_dsp, gensym("dsp"), A_CANT, 0);
        class_addmethod(jxrt_class, (t_method)jsusfx_bypass, gensym("bypass"), A_FLOAT, 0);
        class_addmethod(jxrt_class, (t_method)jsusfx_describe, gensym("describe"), A_NULL, 0);
        class_addmethod(jxrt_class, (t_method)jsusfx_dumpvars, gensym("dumpvars"), A_NULL, 0);
        CLASS_MAINSIGNALIN(jxrt_class, t_jsusfx, x_f);

        inlet_proxy = class_new(gensym("jxrt_inlet_proxy"), NULL,NULL, sizeof(t_inlet_proxy), CLASS_PD|CLASS_NOINLET, A_NULL);
        class_addfloat(inlet_proxy, (t_method)inlet_float);

        JsusFx::init();
    }
}
