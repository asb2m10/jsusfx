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
#include "jsusfx_file.h"
#include "jsusfx_gfx.h"

#include <string.h>
#include <unistd.h>

#include "WDL/ptrlist.h"
#include "WDL/assocarray.h"

#define REAPER_GET_INTERFACE(opaque) ((opaque) ? ((JsusFx*)opaque) : nullptr)

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

// Reaper API

static EEL_F * NSEEL_CGEN_CALL _reaper_spl(void *opaque, EEL_F *n)
{
  JsusFx *ctx = REAPER_GET_INTERFACE(opaque);
  const int index = *n;
  if (index >= 0 && index < ctx->numValidInputChannels)
  	return ctx->spl[index];
  else {
    ctx->dummyValue = 0;
	return &ctx->dummyValue;
  }
}

// todo : remove
static EEL_F NSEEL_CGEN_CALL __stub(void *opaque, INT_PTR np, EEL_F **parms)
{
  return 0.0;
}

//

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

//

bool JsusFx_Slider::config(JsusFx &fx, const int index, const char *param, const int lnumber) {
	char buffer[2048];
	strncpy(buffer, param, 2048);
	
	def = min = max = inc = 0;
	exists = false;
	
	enumNames.clear();
	isEnum = false;
	
	bool hasName = false;

	const char *tmp = strchr(buffer, '>');
	if ( tmp != NULL ) {
		tmp++;
		while (*tmp == ' ')
			tmp++;
		strncpy(desc, tmp, 64);
		tmp = 0;
	} else {
		desc[0] = 0;
	}
	
	tmp = buffer;
	
	if ( isalpha(*tmp) ) {
		// extended syntax of format "slider1:variable_name=5<0,10,1>slider description"
		const char *begin = tmp;
		while ( *tmp && *tmp != '=' )
			tmp++;
		if ( *tmp != '=' ) {
			fx.displayError("Expected '=' at end of slider name %d", lnumber);
			return false;
		}
		const char *end = tmp;
		int len = end - begin;
		if ( len > JsusFx_Slider::kMaxName ) {
			fx.displayError("Slider name too long %d", lnumber);
			return false;
		}
		for ( int i = 0; i < len; ++i )
			name[i] = begin[i];
		name[len] = 0;
		hasName = true;
		tmp++;
	}
	
	if ( !sscanf(tmp, "%f", &def) )
		return false;
	
	tmp = nextToken(tmp);
	
	if ( *tmp != '<' )
	{
		log("slider info is missing");
		return false;
	}
	else
	{
		tmp++;
		
		if ( !sscanf(tmp, "%f", &min) )
		{
			log("failed to read min value");
			return false;
		}
	
		tmp = nextToken(tmp);
		
		if ( *tmp != ',' )
		{
			log("max value is missing");
			return false;
		}
		else
		{
			tmp++;
			
			if ( !sscanf(tmp, "%f", &max) )
			{
				log("failed to read max value");
				return false;
			}
			
			tmp = nextToken(tmp);
			
			if ( *tmp == ',')
			{
				tmp++;
				
				tmp = skipWhite(tmp);
				
				if ( !sscanf(tmp, "%f", &inc) )
				{
					//log("failed to read increment value");
					//return false;
					
					inc = 0;
				}
				
				tmp = nextToken(tmp);
				
				if ( *tmp == '{' )
				{
					isEnum = true;
					
					inc = 1;
					
					tmp++;
					
					while ( true )
					{
						const char *end = nextToken(tmp);
						
						const std::string name(tmp, end);
						
						enumNames.push_back(name);
						
						tmp = end;
						
						if ( *tmp == 0 )
						{
							log("enum value list not properly terminated");
							return false;
						}
						
						if ( *tmp == '}' )
						{
							break;
						}
						
						tmp++;
					}
					
					tmp++;
				}
			}
		}
	}
	
	if (hasName == false) {
		sprintf(name, "slider%d", index);
	}
	
	owner = NSEEL_VM_regvar(fx.m_vm, name);
	
	*owner = def;
	exists = true;
	return true;
}

//

JsusFx::JsusFx(JsusFxPathLibrary &_pathLibrary)
	: pathLibrary(_pathLibrary) {
    m_vm = NSEEL_VM_alloc();
    codeInit = codeSlider = codeBlock = codeSample = codeGfx = NULL;
    NSEEL_VM_SetCustomFuncThis(m_vm,this);

    m_string_context = new eel_string_context_state();
    eel_string_initvm(m_vm);
    computeSlider = false;
    normalizeSliders = 0;
    srate = 0;
	
    pathLibrary = _pathLibrary;
	
    fileAPI = nullptr;
	
	gfx = nullptr;
    gfx_w = 0;
    gfx_h = 0;
	
    for (int i = 0; i < kMaxSamples; ++i) {
    	char name[16];
    	sprintf(name, "spl%d", i);
		
    	spl[i] = NSEEL_VM_regvar(m_vm, name);
    	*spl[i] = 0;
	}
	
	numInputs = 0;
	numOutputs = 0;
	
	numValidInputChannels = 0;
	
    AUTOVAR(srate);
    AUTOVARV(num_ch, 2);
    AUTOVAR(samplesblock);
    AUTOVARV(tempo, 120);
    AUTOVARV(play_state, 1);
	
	// Reaper API
	NSEEL_addfunc_varparm("slider_automate",1,NSEEL_PProc_THIS,&__stub);
	NSEEL_addfunc_varparm("sliderchange",1,NSEEL_PProc_THIS,&__stub);
	NSEEL_addfunc_varparm("slider",1,NSEEL_PProc_THIS,&__stub); // todo : support this syntax: slider(index) = x
	NSEEL_addfunc_retptr("spl",1,NSEEL_PProc_THIS,&_reaper_spl);
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
            return false;
        }
        break;
    case 1:
        codeSlider = NSEEL_code_compile_ex(m_vm, code, line_offset, NSEEL_CODE_COMPILE_FLAG_COMMONFUNCS);
        if ( codeSlider == NULL ) {
            snprintf(errorMsg, 4096, "@slider line %s", NSEEL_code_getcodeerror(m_vm));
            displayError(errorMsg);
            return false;
        }
        break;
    case 2: 
        codeBlock = NSEEL_code_compile_ex(m_vm, code, line_offset, NSEEL_CODE_COMPILE_FLAG_COMMONFUNCS);
        if ( codeBlock == NULL ) {
            snprintf(errorMsg, 4096, "@block line %s", NSEEL_code_getcodeerror(m_vm));
            displayError(errorMsg);
            return false;
        }
        break;
    case 3:
        codeSample = NSEEL_code_compile_ex(m_vm, code, line_offset, NSEEL_CODE_COMPILE_FLAG_COMMONFUNCS);
        if ( codeSample == NULL ) {
            snprintf(errorMsg, 4096, "@sample line %s", NSEEL_code_getcodeerror(m_vm));
            displayError(errorMsg);
            return false;
        }
        break;
    case 4:
        // ignore block if there is no gfx implemented
        if ( gfx == NULL )
            return true;

        codeGfx = NSEEL_code_compile_ex(m_vm, code, line_offset, NSEEL_CODE_COMPILE_FLAG_COMMONFUNCS);
        if ( codeGfx == NULL ) {
            snprintf(errorMsg, 4096, "@gfx line %s", NSEEL_code_getcodeerror(m_vm));
            displayError(errorMsg);
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

static char *trim(char *line, bool trimStart, bool trimEnd)
{
	if (trimStart) {
		while (*line && isspace(*line))
			line++;
	}
	
	if (trimEnd) {
		char *last = line;
		while (last[0] && last[1])
			last++;
		for (char *b = last; isspace(*b) && b >= line; b--)
			*b = 0;
	}
	
	return line;
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
			
			b = trim(b, false, true);
			
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
            else if ( ! strnicmp(b, "gfx", 3) ) {
            	if ( sscanf(b+3, "%d %d", &gfx_w, &gfx_h) != 2 ) {
            		gfx_w = 0;
            		gfx_h = 0;
				}
                section = &sections.gfx;
			}
			
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
                if ( target < 0 || target >= kMaxSliders )
                    continue;
				
				JsusFx_Slider &slider = sliders[target];
				
                char *p = line+7;
                while ( *p && *p != ':' )
                	p++;
                if ( *p != ':' )
                	continue;
				p++;
					
                if ( ! slider.config(*this, target, p, lnumber) ) {
                    displayError("Incomplete slider line %d", lnumber);
                    return false;
                }
                trim(slider.desc, false, true);
				
                continue;
            }
            else if ( ! strncmp(line, "desc:", 5) ) {
            	char *src = line+5;
            	src = trim(src, true, true);
                strncpy(desc, src, 64);
                continue;
            }
            else if ( ! strnicmp(line, "filename:", 9) ) {
				
            	// filename:0,filename.wav
				
            	char *src = line+8;
				
            	src = trim(src, true, false);
				
            	if ( *src != ':' )
            		return false;
				src++;
				
				src = trim(src, true, false);
				
				int index;
				if ( sscanf(src, "%d", &index) != 1 )
					return false;
				while ( isdigit(*src) )
					src++;
				
				src = trim(src, true, false);
				
				if ( *src != ',' )
					return false;
				src++;
				
				src = trim(src, true, true);
				
				std::string resolvedPath;
				if ( pathLibrary.resolveImportPath(src, path, resolvedPath) ) {
					if ( gfx != nullptr && ! gfx->handleFile(index, resolvedPath.c_str() ) ) {
						return false;
					}
				}
            }
            else if ( ! strncmp(line, "import ", 7) ) {
				char *src = line+7;
				src = trim(src, true, true);
					
				if (*src) {
					processImport(pathLibrary, path, src, sections);
				}
                continue;
            }
            else if ( ! strncmp(line, "in_pin:", 7) ) {
            	numInputs++;
            }
            else if ( ! strncmp(line, "out_pin:", 8) ) {
            	numOutputs++;
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
	
	if (result == false)
		releaseCode();
	
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
    *samplesblock = blockSize;
    NSEEL_code_execute(codeInit);
}

void JsusFx::moveSlider(int idx, float value) {
    if ( idx < 0 || idx >= kMaxSliders || !sliders[idx].exists )
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

void JsusFx::process(float **input, float **output, int size, int numInputChannels, int numOutputChannels) {
    if ( codeSample == NULL )
        return;

    if ( computeSlider ) {
        NSEEL_code_execute(codeSlider);
        computeSlider = false;      
    }
	
    numValidInputChannels = numInputChannels;
	
    *samplesblock = size;
    NSEEL_code_execute(codeBlock);
    for(int i=0;i<size;i++) {
    	for (int c = 0; c < numInputChannels; ++c)
        	*spl[c] = input[c][i];
        NSEEL_code_execute(codeSample);
    	for (int c = 0; c < numOutputChannels; ++c)
        	output[c][i] = *spl[c];
    }       
}

void JsusFx::process64(double **input, double **output, int size, int numInputChannels, int numOutputChannels) {
    if ( codeSample == NULL )
        return;

    if ( computeSlider ) {
        NSEEL_code_execute(codeSlider);
        computeSlider = false;
    }
	
    numValidInputChannels = numInputChannels;

    *samplesblock = size;
    NSEEL_code_execute(codeBlock);
    for(int i=0;i<size;i++) {
    	for (int c = 0; c < numInputChannels; ++c)
        	*spl[c] = input[c][i];
        NSEEL_code_execute(codeSample);
    	for (int c = 0; c < numOutputChannels; ++c)
        	output[c][i] = *spl[c];
    }
}

void JsusFx::draw() {
    if ( codeGfx == NULL )
        return;

    NSEEL_code_execute(codeGfx);
}

const char * JsusFx::getString(const int index, WDL_FastString ** fs) {
	void * opaque = this;
	return EEL_STRING_GET_FOR_INDEX(index, fs);
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

	numInputs = 0;
	numOutputs = 0;
	
    for(int i=0;i<kMaxSliders;i++)
    	sliders[i].exists = false;
	
	gfx_w = 0;
	gfx_h = 0;
	
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
        if ( target >= 0 && target < JsusFx::kMaxSliders ) {
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

