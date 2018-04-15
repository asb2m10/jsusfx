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

#include "jsusfx.h"

#include <string.h>
#include <unistd.h>

#include "WDL/ptrlist.h"
#include "WDL/assocarray.h"

#define AUTOVAR(name) name = NSEEL_VM_regvar(m_vm, #name); *name = 0
#define AUTOVARV(name,value) name = NSEEL_VM_regvar(m_vm, #name); *name = value

#define EEL_STRING_GET_CONTEXT_POINTER(opaque) (((JsusFx *)opaque)->m_string_context)
#ifdef EEL_STRING_STDOUT_WRITE
  #ifndef EELSCRIPT_NO_STDIO
    #define EEL_STRING_STDOUT_WRITE(x,len) { fwrite(x,len,1,stdout); fflush(stdout); }
  #endif
#endif
#include "WDL/eel2/eel_strings.h"
#include "WDL/eel2/eel_misc.h"
#include "WDL/eel2/eel_fft.h"
#include "WDL/eel2/eel_mdct.h"

#include <fstream> // to check if files exist

struct JsusFx_Section {
	WDL_String code;
	int lineOffset;
};

struct JsusFx_Sections {
	JsusFx_Section init;
	JsusFx_Section slider;
	JsusFx_Section block;
	JsusFx_Section sample;
	JsusFx_Section gfx;
};

JsusFx::JsusFx() {
    m_vm = NSEEL_VM_alloc();
    codeInit = codeSlider = codeBlock = codeSample = codeGfx = NULL;
    NSEEL_VM_SetCustomFuncThis(m_vm,this);

    m_string_context = new eel_string_context_state();
    eel_string_initvm(m_vm);
    computeSlider = false;
    normalizeSliders = 0;
    srate = 0;
	
	gfx = nullptr;
    gfx_w = 0;
    gfx_h = 0;

    AUTOVAR(spl0);
    AUTOVAR(spl1);
    AUTOVAR(srate);
    AUTOVARV(num_ch, 2);
    AUTOVAR(blockPerSample);
    AUTOVAR(samplesblock);
    AUTOVARV(tempo, 120);
    AUTOVARV(play_state, 1);

    char slider_name[16];
    for(int i=0;i<64;i++) {
        sprintf(slider_name, "slider%d", i);
        sliders[i].exists = false;
        sliders[i].owner = NSEEL_VM_regvar(m_vm, slider_name);
    }
}

JsusFx::~JsusFx() {
    releaseCode();
    if (m_vm) 
        NSEEL_VM_free(m_vm);
    delete m_string_context;
}

bool JsusFx::compileSection(int state, const char *code, int line_offset) {
    if ( code[0] == 0 )
        return true;

    char errorMsg[4096];

	//printf("section code:\n");
	//printf("%s", code);

    switch(state) {
    case 0:
        codeInit = NSEEL_code_compile_ex(m_vm, code, line_offset, NSEEL_CODE_COMPILE_FLAG_COMMONFUNCS);
        if ( codeInit == NULL ) {
            snprintf(errorMsg, 4096, "@init line %s", NSEEL_code_getcodeerror(m_vm));
            displayError(errorMsg);
            releaseCode();
            return false;
        }
        break;
    case 1:
        codeSlider = NSEEL_code_compile_ex(m_vm, code, line_offset, NSEEL_CODE_COMPILE_FLAG_COMMONFUNCS);
        if ( codeSlider == NULL ) {
            snprintf(errorMsg, 4096, "@slider line %s", NSEEL_code_getcodeerror(m_vm));
            displayError(errorMsg);
            releaseCode();
            return false;
        }
        break;
    case 2: 
        codeBlock = NSEEL_code_compile_ex(m_vm, code, line_offset, NSEEL_CODE_COMPILE_FLAG_COMMONFUNCS);
        if ( codeBlock == NULL ) {
            snprintf(errorMsg, 4096, "@block line %s", NSEEL_code_getcodeerror(m_vm));
            displayError(errorMsg);
            releaseCode();
            return false;
        }
        break;
    case 3:
        codeSample = NSEEL_code_compile_ex(m_vm, code, line_offset, NSEEL_CODE_COMPILE_FLAG_COMMONFUNCS);
        if ( codeSample == NULL ) {
            snprintf(errorMsg, 4096, "@sample line %s", NSEEL_code_getcodeerror(m_vm));
            displayError(errorMsg);
            releaseCode();
            return false;
        }
        break;
    case 4:
        codeGfx = NSEEL_code_compile_ex(m_vm, code, line_offset, NSEEL_CODE_COMPILE_FLAG_COMMONFUNCS);
        if ( codeGfx == NULL ) {
            snprintf(errorMsg, 4096, "@gfx line %s", NSEEL_code_getcodeerror(m_vm));
            displayError(errorMsg);
            releaseCode();
            return false;
        }
        break;
    default:
        //printf("unknown block");
        break;
    }

    m_string_context->update_named_vars(m_vm);
    return true;
}

bool JsusFx::processImport(JsusFxPathLibrary &pathLibrary, const std::string &path, const std::string &importPath, JsusFx_Sections &sections) {
	bool result = true;
	
	//displayMsg("Importing %s", import.c_str());
	
	std::string resolvedPath;
	if ( ! pathLibrary.resolveImportPath(importPath, path, resolvedPath) ) {
		displayError("Failed to resolve import file path %s", importPath.c_str());
		return false;
	}

	std::istream *is = pathLibrary.open(resolvedPath);

	if ( is != nullptr ) {
		result &= readSections(pathLibrary, resolvedPath, *is, sections);
	} else {
		displayError("Failed to open imported file %s", importPath.c_str());
		result &= false;
	}
	
	pathLibrary.close(is);
	
	return result;
}

bool JsusFx::readSections(JsusFxPathLibrary &pathLibrary, const std::string &path, std::istream &input, JsusFx_Sections &sections) {
    WDL_String * code = nullptr;
    char line[4096];
	
	// are we reading the header or sections?
	bool isHeader = true;
	
    for(int lnumber=1; ! input.eof(); lnumber++) {
		input.getline(line, sizeof(line), '\n');
		
        if ( line[0] == '@' ) {
            char *b = line + 1;
            char *w = b;
            while(isspace(*w))
                w++;
            w = 0;
			
			// we've begun reading sections now
			isHeader = false;
			
			JsusFx_Section *section = nullptr;
            if ( ! strnicmp(b, "init", 4) )
                section = &sections.init;
            else if ( ! strnicmp(b, "slider", 6) )
                section = &sections.slider;
            else if ( ! strnicmp(b, "block", 5) )
                section = &sections.block;
            else if ( ! strnicmp(b, "sample", 6) )
                section = &sections.sample;
            else if ( ! strnicmp(b, "gfx", 3) && sscanf(b+3, "%d %d", &gfx_w, &gfx_h) == 2 )
                section = &sections.gfx;
			
            if ( section != nullptr ) {
				code = &section->code;
				section->lineOffset = lnumber;
			} else {
				code = nullptr;
			}
			
            continue;
        }
        
        if ( code != nullptr ) {
            int l = strlen(line);
			
            if ( l > 0 && line[l-1] == '\r' )
                line[l-1] = 0;
            
            if ( line[0] != 0 ) {
                code->Append(line);
            }
            code->Append("\n");
            continue;
        }

        if (isHeader) {
            if ( ! strnicmp(line, "slider", 6) ) {
                int target = 0;
                if ( ! sscanf(line, "slider%d:", &target) )
                    continue;
                if ( target >= 64 ) 
                    continue;
                char *p = line+8;
                if ( *p == ':' ) 
                    p++;

                if ( ! sliders[target].config(p) ) {
                    displayError("Incomplete slider line %d", lnumber);
                    return false;
                }
                continue;
            }
            if ( ! strncmp(line, "desc:", 5) ) {
            	char *src = line+5;
            	while (*src && *src == ' ')
            		src++;
                strncpy(desc, src, 64);
                continue;
            }
            if ( ! strncmp(line, "import ", 5) ) {
				char *src = line+7;
            	while (*src && *src == ' ')
            		src++;
				if (*src) {
					processImport(pathLibrary, path, src, sections);
				}
                continue;
            }
        }
    }
	
	return true;
}

bool JsusFx::compileSections(JsusFx_Sections &sections) {
	bool result = true;
	
	// 0 init
	// 1 slider
	// 2 block
	// 3 sample
	// 4 gfx
	
	if (sections.init.code.GetLength() != 0)
		result &= compileSection(0, sections.init.code.Get(), sections.init.lineOffset);
	if (sections.slider.code.GetLength() != 0)
		result &= compileSection(1, sections.slider.code.Get(), sections.slider.lineOffset);
	if (sections.block.code.GetLength() != 0)
		result &= compileSection(2, sections.block.code.Get(), sections.block.lineOffset);
	if (sections.sample.code.GetLength() != 0)
		result &= compileSection(3, sections.sample.code.Get(), sections.sample.lineOffset);
	if (sections.gfx.code.GetLength() != 0)
		result &= compileSection(4, sections.gfx.code.Get(), sections.gfx.lineOffset);
	
	return result;
}

bool JsusFx::compile(std::istream &input) {
	releaseCode();
	
	JsusFxPathLibrary pathLibrary;
	std::string path;
	
	// read code for the various sections inside the jsusfx script
	
	JsusFx_Sections sections;
	if ( ! readSections(pathLibrary, path, input, sections) )
		return false;
	
	// compile the sections
	
	if ( ! compileSections(sections) ) {
		releaseCode();
		return false;
	}
	
	computeSlider = 1;
	
	return true;
}

bool JsusFx::compile(JsusFxPathLibrary &pathLibrary, const std::string &path) {
	releaseCode();
	
	std::istream *input = pathLibrary.open(path);
	if ( input == nullptr ) {
		displayError("Failed to open %s", path.c_str());
		return false;
	}
	
	// read code for the various sections inside the jsusfx script
	
	JsusFx_Sections sections;
	if ( ! readSections(pathLibrary, path, *input, sections) )
		return false;
	
	pathLibrary.close(input);
	
	// compile the sections
	
	if ( ! compileSections(sections) ) {
		releaseCode();
		return false;
	}
	
	computeSlider = 1;
	
	return true;
}

void JsusFx::prepare(int sampleRate, int blockSize) {    
    *srate = (double) sampleRate;
    *blockPerSample = blockSize;
    *samplesblock = blockSize;
    NSEEL_code_execute(codeInit);
}

void JsusFx::moveSlider(int idx, float value) {
    if ( idx >= 64 || !sliders[idx].exists )
        return;

    if ( normalizeSliders != 0 ) {
        float steps = sliders[idx].max - sliders[idx].min;
        value  = (value * steps) / normalizeSliders;
        value += sliders[idx].min;
    }

    if ( sliders[idx].inc != 0 ) {
        int tmp = value / sliders[idx].inc + .5f;
        value = sliders[idx].inc * tmp;
    }

    computeSlider |= sliders[idx].setValue(value);
}

void JsusFx::process(float **input, float **output, int size) {
    if ( codeSample == NULL )
        return;

    if ( computeSlider ) {
        NSEEL_code_execute(codeSlider);
        computeSlider = false;      
    }

    *blockPerSample = size;
    *samplesblock = size;
    NSEEL_code_execute(codeBlock);
    for(int i=0;i<size;i++) {
        *spl0 = input[0][i];
        *spl1 = input[1][i];
        NSEEL_code_execute(codeSample);
        output[0][i] = *spl0;
        output[1][i] = *spl1;
    }       
}

void JsusFx::process64(double **input, double **output, int size) {
    if ( codeSample == NULL )
        return;

    if ( computeSlider ) {
        NSEEL_code_execute(codeSlider);
        computeSlider = false;
    }

    *blockPerSample = size;
    *samplesblock = size;
    NSEEL_code_execute(codeBlock);
    for(int i=0;i<size;i++) {
        *spl0 = input[0][i];
        *spl1 = input[1][i];
        NSEEL_code_execute(codeSample);
        output[0][i] = *spl0;
        output[1][i] = *spl1;
    }
}

void JsusFx::draw() {
    if ( codeGfx == NULL )
        return;

    NSEEL_code_execute(codeGfx);
}

void JsusFx::releaseCode() {
    desc[0] = 0;
    
    if ( codeInit )
        NSEEL_code_free(codeInit);
    if ( codeSlider ) 
        NSEEL_code_free(codeSlider);
    if ( codeBlock  ) 
        NSEEL_code_free(codeBlock);
    if ( codeSample ) 
        NSEEL_code_free(codeSample);
	if ( codeGfx )
		NSEEL_code_free(codeGfx);
        
    codeInit = codeSlider = codeBlock = codeSample = codeGfx = NULL;

    for(int i=0;i<64;i++)
        sliders[i].exists = false;

    NSEEL_VM_remove_unused_vars(m_vm);
    NSEEL_VM_remove_all_nonreg_vars(m_vm);
}

void JsusFx::init() {
    EEL_string_register();
    EEL_fft_register();
    EEL_mdct_register();
    EEL_string_register();
    EEL_misc_register();
}

static int dumpvarsCallback(const char *name, EEL_F *val, void *ctx) {
    JsusFx *fx = (JsusFx *) ctx;
    int target;
        
    if ( sscanf(name, "slider%d", &target) ) {
        if ( target >= 0 && target < 64 ) {
            if ( ! fx->sliders[target].exists ) {
                return 1;
            } else {
                fx->displayMsg("%s --> %f (%s)", name, *val, fx->sliders[target].desc);
                return 1;
            }
        }
    }
    
    fx->displayMsg("%s --> %f", name, *val);
    return 1;
}

void JsusFx::dumpvars() {
    NSEEL_VM_enumallvars(m_vm, dumpvarsCallback, this);
}

#ifndef JSUSFX_OWNSCRIPTMUTEXT
void NSEEL_HOSTSTUB_EnterMutex() { }
void NSEEL_HOSTSTUB_LeaveMutex() { }
#endif

