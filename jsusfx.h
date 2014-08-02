#pragma once

#include <string.h>
#include <sstream>
#include <iostream>
#include <string>
#include "ext.h"
#include "WDL/eel2/ns-eel.h"

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
			strcpy(desc, tmp+1);
			tmp = 0;
		} else {
			desc[0] = 0;
		}
		
		tmp = strtok(buffer, "<,");
		if ( !sscanf(tmp, "%f", &def) ) {
			return false;
		} 
		
		tmp = strtok(NULL, "<,");
		if ( !sscanf(tmp, "%f", &min) ) {
			return false;
		}
		
		tmp = strtok(NULL, "<,");
		if ( !sscanf(tmp, "%f", &max) ) {
			return false;
		}
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

	// DAW FUNCTION
	EEL_F *tempo, *play_state, *play_position, *beat_position, *ts_num, *ts_denom;

	// DUMMY VALUES
	EEL_F *ext_noinit, *ext_nodenorm, *pdc_delay, *pdc_bot_cd, *pdc_top_ch;

	bool computeSlider;
	void releaseCode();
	bool compileSection(int state, const char *code, int line_offset);

public:
	EEL_F *srate, *num_ch, *blockPerSample;
	EEL_F *spl0, *spl1, *trigger;

    Slider sliders[64];
	int normalizeSliders;
    
	eel_string_context_state *m_string_context;
    
	JsusFx();
	~JsusFx();

	bool compile(std::istream &input);
	void prepare(int sampleRate, int blockSize);
	void moveSlider(int idx, float value);
	void process(float **input, float **output, int size);
	void process64(double **input, double **output, int size);
	
	virtual void displayMsg(const char *msg);
	virtual void displayError(const char *msg);

	static void init();

	char desc[64];
};

