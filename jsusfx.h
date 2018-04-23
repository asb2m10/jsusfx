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

#pragma once

#include <string.h>
#include <iostream>
#include <string>
#include <vector>

#include "WDL/eel2/ns-eel.h"
#include "WDL/eel2/ns-eel-int.h"

#ifndef nullptr
	#define nullptr NULL
#endif

class eel_string_context_state;

struct JsusFxGfx;

class WDL_FastString;

class Slider {
public:
    float def, min, max, inc;

    char desc[64];
    EEL_F *owner;
    bool exists;
	
    std::vector<std::string> enumNames;
    bool isEnum;

    Slider() {
        def = min = max = inc = 0;
        exists = false;
        desc[0] = 0;
        isEnum = false;
    }

	static const char *skipWhite(const char *text)
	{
		while ( *text && isspace(*text) )
			text++;
		
		return text;
	}
	
    static const char *nextToken(const char *text)
    {
    	while ( *text && *text != ',' && *text != '=' && *text != '<' && *text != '>' && *text != '{' && *text != '}' )
    		text++;

    	return text;
    }
	
    static void log(const char *text)
    {
    	printf("%s\n", text);
	}

    bool config(char *param) {
        char buffer[1024];
        strncpy(buffer, param, 1024);
                
        def = min = max = inc = 0;
        exists = false;     

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

        *owner = def;
        exists = true;
        return true;
    }

    /**
     * Return true if the value has changed
     */
    bool setValue(float v) {
    	if ( min < max ) {
			if ( v < min ) {
				v = min;
			} else if ( v > max ) {
				v = max;
			}
		} else {
			if ( v < max ) {
				v = max;
			} else if ( v > min ) {
				v = min;
			}
		}
        if ( v == *owner )
            return false;
        *owner = v;
        return true;
    }
	
    float getValue() const {
    	return *owner;
	}
};

struct JsusFx_Sections;

struct JsusFxPathLibrary {
	virtual bool resolveImportPath(const std::string &importPath, const std::string &parentPath, std::string &resolvedPath) {
		return false;
	}
	
	virtual std::istream* open(const std::string &path) {
		return nullptr;
	}
	
	virtual void close(std::istream *&stream) {
	}
};

class JsusFx {
protected:
    NSEEL_CODEHANDLE codeInit, codeSlider, codeBlock, codeSample, codeGfx;

    bool computeSlider;
    void releaseCode();
    bool compileSection(int state, const char *code, int line_offset);
    bool processImport(JsusFxPathLibrary &pathLibrary, const std::string &currentPath, const std::string &importPath, JsusFx_Sections &sections);
    bool readSections(JsusFxPathLibrary &pathLibrary, const std::string &currentPath, std::istream &input, JsusFx_Sections &sections);
    bool compileSections(JsusFx_Sections &sections);

public:
    NSEEL_VMCTX m_vm;
    Slider sliders[64];
    int normalizeSliders;
    char desc[64];
    
    EEL_F *tempo, *play_state, *play_position, *beat_position, *ts_num, *ts_denom;
    EEL_F *ext_noinit, *ext_nodenorm, *pdc_delay, *pdc_bot_cd, *pdc_top_ch;
    EEL_F *srate, *num_ch, *samplesblock;
    EEL_F *spl0, *spl1, *trigger;
	
    JsusFxGfx *gfx;
    int gfx_w;
    int gfx_h;

    JsusFx();
    virtual ~JsusFx();

    bool compile(std::istream &input);
    bool compile(JsusFxPathLibrary &pathLibrary, const std::string &path);
    void prepare(int sampleRate, int blockSize);
    void moveSlider(int idx, float value);
    void process(float **input, float **output, int size);
    void process64(double **input, double **output, int size);
    void draw();
	
    const char * getString(int index, WDL_FastString ** fs);
    
    virtual void displayMsg(const char *fmt, ...) = 0;
    virtual void displayError(const char *fmt, ...) = 0;

    void dumpvars();
    
    static void init();
    
    // ==============================================================
    eel_string_context_state *m_string_context;
};

