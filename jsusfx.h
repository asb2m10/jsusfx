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

#include "WDL/eel2/ns-eel.h"
#include "WDL/eel2/ns-eel-int.h"

class eel_string_context_state;

class Slider {
public:
	float def, min, max, inc;

	char desc[64];
	EEL_F *owner;
	bool exists;

	Slider() {
		def = min = max = inc = 0;
		exists = false;
		desc[0] = 0;
	}

	bool config(char *param) {
		char buffer[1024];
		strncpy(buffer, param, 1024);
				
		def = min = max = inc = 0;
		exists = false;		

		char *tmp = strchr(buffer, '>');
		if ( tmp != NULL ) {
			strncpy(desc, tmp+1, 64);
			tmp = 0;

			int l = strlen(desc);
			desc[l-1] = 0;
		} else {
			desc[0] = 0;
		}
		
		tmp = strtok(buffer, "<,");
		if ( !sscanf(tmp, "%f", &def) )
			return false;
		
		tmp = strtok(NULL, "<,");
		if ( !sscanf(tmp, "%f", &min) )
			return false;
		
		tmp = strtok(NULL, "<,");
		if ( !sscanf(tmp, "%f", &max) )
			return false;

		tmp = strtok(NULL, "<,");
		if ( tmp != NULL )
			sscanf(tmp, "%f", &inc);

		*owner = def;
		exists = true;
		return true;
	}

	/**
	 * Return true if the value has changed
	 */
	bool setValue(float v) {
		if ( v < min ) {
			v = min;
		} else if ( v > max ) {
			v = max;
		}
		if ( v == *owner )
			return false;
		*owner = v;
		return true;
	}
};

class JsusFx {
protected:
    NSEEL_CODEHANDLE codeInit, codeSlider, codeBlock, codeSample;
    NSEEL_VMCTX m_vm;

	bool computeSlider;
	void releaseCode();
	bool compileSection(int state, const char *code, int line_offset);

public:
    Slider sliders[64];
	int normalizeSliders;
	char desc[64];
    
	EEL_F *tempo, *play_state, *play_position, *beat_position, *ts_num, *ts_denom;
	EEL_F *ext_noinit, *ext_nodenorm, *pdc_delay, *pdc_bot_cd, *pdc_top_ch;
	EEL_F *srate, *num_ch, *blockPerSample;
	EEL_F *spl0, *spl1, *trigger;

	JsusFx();
	virtual ~JsusFx();

	bool compile(std::istream &input);
	void prepare(int sampleRate, int blockSize);
	void moveSlider(int idx, float value);
	void process(float **input, float **output, int size);
	void process64(double **input, double **output, int size);
	
	virtual void displayMsg(const char *fmt, ...) = 0;
	virtual void displayError(const char *fmt, ...) = 0;

    void dumpvars();
    
	static void init();
    
    // ==============================================================
	eel_string_context_state *m_string_context;
};

