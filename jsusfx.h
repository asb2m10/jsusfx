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

class JsusFx;
struct JsusFxFileAPI;
struct JsusFxGfx;
struct JsusFxPathLibrary;

struct JsusFx_FileInfo;
class JsusFx_Slider;
struct JsusFx_Sections;

class WDL_FastString;

//

class JsusFx_Slider {
public:
	static const int kMaxName = 63;
	
    float def, min, max, inc;

	char name[kMaxName + 1];
    char desc[64];
    EEL_F *owner;
    bool exists;
	
    std::vector<std::string> enumNames;
    bool isEnum;

    JsusFx_Slider() {
        def = min = max = inc = 0;
        name[0] = 0;
        desc[0] = 0;
        exists = false;
        owner = nullptr;
        isEnum = false;
    }

    bool config(JsusFx &fx, const int index, const char *param, const int lnumber);
    
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

struct JsusFx_FileInfo {
	std::string filename;
	
	bool isValid() const {
		return !filename.empty();
	}
	
	bool init(const char *_filename) {
		filename = _filename;
		
		return isValid();
	}
};

struct JsusFxPathLibrary {
	virtual ~JsusFxPathLibrary() {
	}
	
	virtual bool resolveImportPath(const std::string &importPath, const std::string &parentPath, std::string &resolvedPath) {
		return false;
	}
	
	virtual bool resolveDataPath(const std::string &importPath, std::string &resolvedPath) {
		return false;
	}
	
	virtual std::istream* open(const std::string &path) {
		return nullptr;
	}
	
	virtual void close(std::istream *&stream) {
	}
};

struct JsusFxPathLibrary_Basic : JsusFxPathLibrary {
	std::string dataRoot;
	
	std::vector<std::string> searchPaths;
	
	JsusFxPathLibrary_Basic(const char * _dataRoot);
	
	void addSearchPath(const std::string & path);
	
	static bool fileExists(const std::string &filename);

	virtual bool resolveImportPath(const std::string &importPath, const std::string &parentPath, std::string &resolvedPath) override;
	virtual bool resolveDataPath(const std::string &importPath, std::string &resolvedPath) override;
	
	virtual std::istream* open(const std::string &path) override;
	virtual void close(std::istream *&stream) override;
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
	static const int kMaxSamples = 64;
	static const int kMaxSliders = 64;
	static const int kMaxFileInfos = 128;
	
    NSEEL_VMCTX m_vm;
    JsusFx_Slider sliders[kMaxSliders];
    int normalizeSliders;
    char desc[64];
    
    EEL_F *tempo, *play_state, *play_position, *beat_position, *ts_num, *ts_denom;
    EEL_F *ext_noinit, *ext_nodenorm, *pdc_delay, *pdc_bot_cd, *pdc_top_ch;
    EEL_F *srate, *num_ch, *samplesblock;
    EEL_F *spl[kMaxSamples], *trigger;
    EEL_F dummyValue;
	
    int numInputs;
    int numOutputs;
    int numValidInputChannels;
	
	JsusFxPathLibrary &pathLibrary;
	
    JsusFxFileAPI *fileAPI;
    JsusFx_FileInfo fileInfos[kMaxFileInfos];

	uint8_t * midi;
    int midiSize;
	
    JsusFxGfx *gfx;
    int gfx_w;
    int gfx_h;

    JsusFx(JsusFxPathLibrary &pathLibrary);
    virtual ~JsusFx();

    bool compile(std::istream &input);
    bool compile(JsusFxPathLibrary &pathLibrary, const std::string &path);
    void prepare(int sampleRate, int blockSize);
    void moveSlider(int idx, float value);
    void setMidi(const void * midi, int numBytes);
    void process(float **input, float **output, int size, int numInputChannels, int numOutputChannels);
    void process64(double **input, double **output, int size, int numInputChannels, int numOutputChannels);
    void draw();
	
    const char * getString(int index, WDL_FastString ** fs);
	
    bool handleFile(int index, const char *filename);
	
    virtual void displayMsg(const char *fmt, ...) = 0;
    virtual void displayError(const char *fmt, ...) = 0;

    void dumpvars();
    
    static void init();
    
    // ==============================================================
    eel_string_context_state *m_string_context;
};

