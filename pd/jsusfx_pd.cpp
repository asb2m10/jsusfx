/*
 * Copyright 2014-2019 Pascal Gauthier
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


#include <stdarg.h>
#include <fstream>

#if defined(_LANGUAGE_C_PLUS_PLUS) || defined(__cplusplus)
extern "C" {
#endif

#ifdef __linux__
    #include "pd/m_pd.h"
    #include "pd/s_stuff.h"
#else 
    #include "m_pd.h"
    #include "s_stuff.h"
#endif

#if defined(_LANGUAGE_C_PLUS_PLUS) || defined(__cplusplus)
}
#endif

#include "WDL/mutex.h"
#include "jsusfx.h"

#define REAPER_GET_INTERFACE(opaque) ((opaque) ? ((JsusFxPd*)opaque) : nullptr)

using namespace std;

// The maximum of signal inlet/outlet; PD seems to have a limitation to 18 inlets ...
const int MAX_SIGNAL_PORT = 8;

class JsusFxPdPath : public JsusFxPathLibrary_Basic {
public:
    JsusFxPdPath(const char * _dataRoot) : JsusFxPathLibrary_Basic(_dataRoot) {
    }
    
    bool resolveImportPath(const string &importPath, const string &parentPath, string &resolvedPath) {
        const size_t pos = parentPath.rfind('/', '\\');
        if ( pos != string::npos )
            resolvedPath = parentPath.substr(0, pos + 1);
        
        if ( fileExists(resolvedPath + importPath) ) {
            resolvedPath = resolvedPath + importPath;
            return true;
        }
        
        string searchDir;
        // check if the import parentPath is a script and remove the extension (if needed)
        // usually the directory should contain the import script
        const size_t dotPos = parentPath.rfind(".");
        if ( dotPos != string::npos && dotPos > pos ) {
            searchDir = parentPath.substr(0, dotPos);
        } else {
            searchDir = dataRoot;
        }
        
        char result[1024];
        char *bufptr;
        int fd = open_via_path(searchDir.c_str(), importPath.c_str(), "", result, &bufptr, 1023, 1);
        if ( fd < 0 || result[0] == 0 ) {
            return false;
        }
        sys_close(fd);
        resolvedPath = result;
        resolvedPath += '/';
        resolvedPath += importPath;
        return true;
    }
    
    bool resolveDataPath(const string &importPath, string &resolvedPath) {
        char result[1024];
        char *bufptr;
        int fd = open_via_path(dataRoot.c_str(), importPath.c_str(), "", result, &bufptr, 1023, 1);
        if ( fd < 0 || result[0] == 0 ) {
            return false;
        }
        sys_close(fd);
        resolvedPath = result;
        resolvedPath += '/';
        resolvedPath += importPath;
        return true;
    }
};

static EEL_F NSEEL_CGEN_CALL midisend(void *opaque, INT_PTR np, EEL_F **parms);

class JsusFxPd : public JsusFx {
public:
    static const int kMidiBufferSize = 4096;
    
    uint8_t midiHead[kMidiBufferSize];
    uint8_t midiPreStream[4];
    int midiPre = 0, midiExpt = 0;
    uint8_t midiOutBuffer[kMidiBufferSize];
    int midiOutSize = 0;
    
    // this is used to indicate if dsp is on (and midi parsing should be done)
    bool dspOn;
    WDL_Mutex dspLock;

    JsusFxPd(JsusFxPathLibrary &pathLibrary) : JsusFx(pathLibrary) {
        midi = &midiHead[0];
        NSEEL_addfunc_varparm("midisend",3,NSEEL_PProc_THIS,&midisend);
    }
    
    ~JsusFxPd() {
    }

    // we pre-stream the byte messages since it might come in between dsp cycles
    void midiin(uint8_t b) {
        // midi is read in the dsp thread. Do accumulate stuff if you don't read it
        if ( ! dspOn )
            return;
        
        // in sysex stream, ignore everything
        if ( midiExpt == -1) {
            if ( b == 0xf0 ) {
                midiExpt = 0;
                return;
            }
        }
        
        // nothing is expected; new midi message
        if ( midiExpt == 0 ) {
            if ((b & 0xf0) == 0xf0) {
                midiExpt = -1;
                return;
            }
            
            midiPre = 1;
            midiPreStream[0] = b;
            
            switch(b & 0xf0) {
                case 0xc0:
                case 0xd0:
                    midiExpt = 2;
                    break;
                default:
                    midiExpt = 3;
            }
            return;
        }
    
        midiPreStream[midiPre++] = b;
        if ( midiPre >= midiExpt) {
            if ( midiSize + midiExpt >= kMidiBufferSize ) {
                post("jsusfx~: midi buffer full");
                dspOn = false;
                return;
            }
        
            for(int i=0;i<midiExpt;i++) {
                midi[midiSize++] = midiPreStream[i];
            }
            midiExpt = 0;
        }
    }
    
    void displayMsg(const char *fmt, ...) {
        char output[4096];
        va_list argptr;
        va_start(argptr, fmt);
        vsnprintf(output, 4095, fmt, argptr);
        va_end(argptr);

        post("%s", output);
    }

    void displayError(const char *fmt, ...) {
        char output[4096];
        va_list argptr;
        va_start(argptr, fmt);
        vsnprintf(output, 4095, fmt, argptr);
        va_end(argptr);

        error("%s", output);
    }

    void flushMidi() {
        midi = &midiHead[0];
        midiSize = 0;
    }
};

static EEL_F NSEEL_CGEN_CALL midisend(void *opaque, INT_PTR np, EEL_F **parms) {
    JsusFxPd *ctx = REAPER_GET_INTERFACE(opaque);
    
    if ( JsusFxPd::kMidiBufferSize <= ctx->midiOutSize + 3 ) {
        return 1;
    }

    ctx->midiOutBuffer[ctx->midiOutSize++] = *parms[1];
    if ( np >= 4 ) {
        ctx->midiOutBuffer[ctx->midiOutSize++] = *parms[2];
        ctx->midiOutBuffer[ctx->midiOutSize++] = *parms[3];
    } else {
        int v = *parms[2];
        ctx->midiOutBuffer[ctx->midiOutSize++] = v % 256;
        ctx->midiOutBuffer[ctx->midiOutSize++] = v / 256;
    }
    
    return 0;
}

typedef struct _jsusfx {
    t_object x_obj;
    t_float x_f;
    JsusFxPd *fx;
    JsusFxPdPath *path;
    char scriptpath[1024];
    t_clock *x_clock;
    bool bypass;
    bool user_bypass;
    int pinIn, pinOut;
    t_int **dspVect;
    t_outlet *midiout;
} t_jsusfx;

static t_class *jsusfx_class;
static t_class *jsfx_class;
static t_class *slider_proxy;

typedef struct _inlet_proxy {
    t_object x_obj;
    t_jsusfx *peer;
    int idx;
} t_inlet_proxy;

string getFileName(const string &s) {
    char sep = '/';
    size_t i = s.rfind(sep, s.length());
    if (i != string::npos) {
        return s.substr(i+1, s.length() - i);
    }
    return s;
}

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
    string filename = string(newFile->s_name);

    if ( newFile != NULL && newFile->s_name[0] != 0) {
        string result;

        // find if the file exists with the .jsfx suffix
        if ( ! x->path->resolveDataPath(string(filename), result) ) {
            // maybe it isn't specified, try with the .jsfx
            filename += ".jsfx";
            
            if ( ! x->path->resolveDataPath(string(filename), result) ) {
                error("jsusfx~: unable to find script %s", newFile->s_name);
                return;
            }
        }
        strncpy(x->scriptpath, filename.c_str(), 1023);
    } else {
        if ( x->scriptpath[0] == 0 )
            return;
    }

    x->fx->dspLock.Enter();
    if ( x->fx->compile(*(x->path), x->scriptpath, 0) ) {
        if ( x->fx->srate != 0 )
            x->fx->prepare(*(x->fx->srate), *(x->fx->samplesblock));
        x->bypass = false;
    } else {
        x->bypass = true;
    }
    x->fx->dspLock.Leave();
}

void jsusfx_slider(t_jsusfx *x, t_float id, t_float value) {
    int i = (int) id;
    if ( i > 64 || i < 0 )
        return;
    if ( ! x->fx->sliders[i].exists ) {
        error("jsusfx~: slider number %d is not assigned for this effect", i);
        return;
    }
    x->fx->moveSlider(i, value, 1);
}

void jsusfx_uslider(t_jsusfx *x, t_float id, t_float value) {
    int i = (int) id;
    if ( i > 64 || i < 0 )
        return;
    if ( ! x->fx->sliders[i].exists ) {
        error("jsusfx~: slider number %d is not assigned for this effect", i);
        return;
    }
    x->fx->moveSlider(i, value, 0);
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
        
        if ( x->pinIn == x->pinOut ) {
            for(int i=0;i<x->pinOut;i++) {
                for(int j=0;j<n;j++) {
                    outs[i][j] = ins[i][j];
                }
            }
        } else {
            for(int i=0;i<x->pinOut;i++) {
                for(int j=0;j<n;j++)
                    outs[i][j] = 0;
            }
        }
        
    } else {
        x->fx->process(ins, outs, n, x->pinIn, x->pinOut);
        x->fx->dspLock.Leave();
    }

    if ( x->fx->midiOutSize != 0 )
        clock_delay(x->x_clock, 0);
    
    x->fx->flushMidi();
    
    return (w+argc);
}

void jsusfx_dsp(t_jsusfx *x, t_signal **sp) {
    x->fx->prepare(sp[0]->s_sr, sp[0]->s_n);
    
    x->dspVect[0] = (t_int *) x;
    int i, j;
    for (i=0; i<x->pinIn; i++) {
        x->dspVect[i+1] = (t_int*)sp[i]->s_vec;
        //post("jsusfx~ dsp-vecin: %x", x->dspVect[i+1]);
    }
    
    for (j=0;j<x->pinOut; j++) {
        x->dspVect[i+j+1] = (t_int*)sp[i+j]->s_vec;
        //post("jsusfx~ dsp-vecout: %x", x->dspVect[i+j+1]);
    }
    x->dspVect[i+j+1] = (t_int *) ((long)sp[0]->s_n);
    x->fx->dspOn = true;
    
    dsp_addv(jsusfx_perform, x->pinIn + x->pinOut + 2, (t_int*)x->dspVect);
}

static void jsusfx_midiout(t_jsusfx *x) {
    if ( x->fx->midiOutSize >= JsusFxPd::kMidiBufferSize ) {
        post("jsusfx~: midiout buffer full");
    } else {
        for(int i=0;i<x->fx->midiOutSize;i++) {
            outlet_float(x->midiout, x->fx->midiOutBuffer[i]);
        }
    }
    x->fx->midiOutSize = 0;
}

void *jsusfx_new(t_symbol *notused, long argc, t_atom *argv) {
    t_jsusfx *x = (t_jsusfx *)pd_new(jsusfx_class);
    x->path = new JsusFxPdPath(canvas_getcurrentdir()->s_name);
    x->bypass = true;
    x->user_bypass = false;
    x->scriptpath[0] = 0;
    x->fx = new JsusFxPd(*(x->path));
    x->x_clock = clock_new(x, (t_method)jsusfx_midiout);

    x->pinIn = 2;
    x->pinOut = 2;

    if ( argc < 1 ) {
        post("jsusfx~: missing script");
        x->scriptpath[0] = 0;
    } else {
        int argPos = 0;

        if ( (argv[0]).a_type == A_SYMBOL ) {
            t_symbol *s = atom_getsymbol(argv);
            jsusfx_compile(x, s);
            if (! x->bypass) { 
                jsusfx_describe(x);
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
    
    x->midiout = outlet_new(&x->x_obj, &s_float);
    return (x);
}

void jsusfx_free(t_jsusfx *x) {
    clock_free(x->x_clock);
    t_freebytes(x->dspVect, sizeof(t_int) * (x->pinIn + x->pinOut + 2));
    delete x->fx;
    delete x->path;
}

void *jsfx_new(t_symbol *objectname, long argc, t_atom *argv) {
    t_symbol *script = NULL;

    if ( argc < 1 ) {
        if ( gensym("jsfx~") != objectname ) {
            script = objectname;
        }
    } else {
        if ( (argv[0]).a_type == A_SYMBOL ) {
            script = atom_getsymbol(argv);
        }
    }

    if ( script == NULL || script->s_name[0] == 0) {
        error("jsfx~: missing script");
        return NULL;
    }
    
    t_jsusfx *x = (t_jsusfx *)pd_new(jsusfx_class);
    x->path = new JsusFxPdPath(canvas_getcurrentdir()->s_name);
    x->bypass = true;
    x->user_bypass = false;
    x->scriptpath[0] = 0;
    x->fx = new JsusFxPd(*(x->path));
    x->x_clock = clock_new(x, (t_method)jsusfx_midiout);
    
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
            t_inlet_proxy *proxy = (t_inlet_proxy *) pd_new(slider_proxy);
            proxy->idx = i;
            proxy->peer = x;
            inlet_new(&x->x_obj, &proxy->x_obj.ob_pd, 0, 0);
        }
    }

    x->midiout = outlet_new(&x->x_obj, &s_float);
    
    return (x);
}

void jsfx_free(t_jsusfx *x) {
    clock_free(x->x_clock);
    delete x->fx;
    delete x->path;
    // delete also the inlet proxy or it is done automatically ?
}

static void slider_float(t_inlet_proxy *proxy, t_float f) {
    proxy->peer->fx->moveSlider(proxy->idx, f, 0);
}

static void jsusfx_midi(t_jsusfx *x, t_float f) {
    x->fx->midiin(f);
}

static void jsusfx_list(t_jsusfx *x, t_symbol *c, int ac, t_atom *av) {
    for(int i=0;i<ac;i++) {
        if ( av[i].a_type == A_FLOAT )
            x->fx->midiin(atom_getfloat(&(av[i])));
    }
}

static int jsusfx_loader_pathwise(t_canvas *unused, const char *objectname, const char *path) {
    char dirbuf[MAXPDSTRING];
    char *ptr;
    int fd;

    if(!path)
        return 0;
    
    fd = sys_trytoopenone(path, objectname, ".jsfx", dirbuf, &ptr, MAXPDSTRING, 1);
    if (fd>0) {
        sys_close(fd);
        class_addcreator((t_newmethod) jsfx_new, gensym(objectname), A_GIMME, 0);
        return 1;          
    }

    return 0;
}

extern "C" {
    void jsusfx_tilde_setup(void) {
        t_symbol *midi = gensym("midi");
        
        jsusfx_class = class_new(gensym("jsusfx~"), (t_newmethod)jsusfx_new, (t_method)jsusfx_free, sizeof(t_jsusfx), 0L, A_GIMME, 0);
        class_addmethod(jsusfx_class, (t_method)jsusfx_dsp, gensym("dsp"), A_CANT, 0);
        class_addmethod(jsusfx_class, (t_method)jsusfx_slider, gensym("slider"), A_FLOAT, A_FLOAT, 0);
        class_addmethod(jsusfx_class, (t_method)jsusfx_uslider, gensym("uslider"), A_FLOAT, A_FLOAT, 0);
        class_addmethod(jsusfx_class, (t_method)jsusfx_compile, gensym("compile"), A_DEFSYMBOL, 0);
        class_addmethod(jsusfx_class, (t_method)jsusfx_describe, gensym("describe"), A_NULL, 0);
        class_addmethod(jsusfx_class, (t_method)jsusfx_dumpvars, gensym("dumpvars"), A_NULL, 0);
        class_addmethod(jsusfx_class, (t_method)jsusfx_bypass, gensym("bypass"), A_FLOAT, 0);
        class_addmethod(jsusfx_class, (t_method)jsusfx_midi, midi, A_FLOAT, 0);
        class_addmethod(jsusfx_class, (t_method)jsusfx_list, midi, A_GIMME, 0);
        class_addlist(jsusfx_class, (t_method)jsusfx_list);
        CLASS_MAINSIGNALIN(jsusfx_class, t_jsusfx, x_f);

        jsfx_class = class_new(gensym("jsfx~"), (t_newmethod)jsfx_new, (t_method)jsfx_free, sizeof(t_jsusfx), 0L, A_GIMME, 0);
        class_addmethod(jsfx_class, (t_method)jsusfx_slider, gensym("slider"), A_FLOAT, A_FLOAT, 0);
        class_addmethod(jsfx_class, (t_method)jsusfx_uslider, gensym("uslider"), A_FLOAT, A_FLOAT, 0);
        class_addmethod(jsfx_class, (t_method)jsusfx_dsp, gensym("dsp"), A_CANT, 0);
        class_addmethod(jsfx_class, (t_method)jsusfx_bypass, gensym("bypass"), A_FLOAT, 0);
        class_addmethod(jsfx_class, (t_method)jsusfx_describe, gensym("describe"), A_NULL, 0);
        class_addmethod(jsfx_class, (t_method)jsusfx_dumpvars, gensym("dumpvars"), A_NULL, 0);
        class_addmethod(jsusfx_class, (t_method)jsusfx_midi, midi, A_FLOAT, 0);
        class_addmethod(jsusfx_class, (t_method)jsusfx_list, midi, A_GIMME, 0);
        class_addlist(jsfx_class, (t_method)jsusfx_list);
        CLASS_MAINSIGNALIN(jsfx_class, t_jsusfx, x_f);

        slider_proxy = class_new(gensym("slider_proxy"), NULL, NULL, sizeof(t_inlet_proxy), CLASS_PD|CLASS_NOINLET, A_NULL);
        class_addfloat(slider_proxy, (t_method)slider_float);
        
        int maj=0,min=0,bug=0;
        sys_getversion(&maj,&min,&bug);
        if((maj==0) && (min>46)) {
            sys_register_loader((loader_t)jsusfx_loader_pathwise);
        } 

        JsusFx::init();
    }

    void jsfx_tilde_setup(void) {
        jsusfx_tilde_setup();
    }
}
