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
struct JsusFxSerializationData;
struct JsusFxSerializer;

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
    NSEEL_CODEHANDLE codeInit, codeSlider, codeBlock, codeSample, codeGfx, codeSerialize;

    bool computeSlider;
    void releaseCode();
    bool compileSection(int state, const char *code, int line_offset);
    bool processImport(JsusFxPathLibrary &pathLibrary, const std::string &currentPath, const std::string &importPath, JsusFx_Sections &sections);
    bool readHeader(JsusFxPathLibrary &pathLibrary, const std::string &currentPath, std::istream &input);
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
	
	JsusFxSerializer *serializer;

    JsusFx(JsusFxPathLibrary &pathLibrary);
    virtual ~JsusFx();

    bool compile(std::istream &input);
    bool compile(JsusFxPathLibrary &pathLibrary, const std::string &path);
    bool readHeader(JsusFxPathLibrary &pathLibrary, const std::string &path);
    void prepare(int sampleRate, int blockSize);
    void moveSlider(int idx, float value);
    void setMidi(const void * midi, int numBytes);
    bool process(const float **input, float **output, int size, int numInputChannels, int numOutputChannels);
    bool process64(const double **input, double **output, int size, int numInputChannels, int numOutputChannels);
    void draw();
    bool serialize(const bool write);
	
    const char * getString(int index, WDL_FastString ** fs);
	
    bool handleFile(int index, const char *filename);
	
    virtual void displayMsg(const char *fmt, ...) = 0;
    virtual void displayError(const char *fmt, ...) = 0;

    void dumpvars();
    
    static void init();
    
    // ==============================================================
    eel_string_context_state *m_string_context;
};

struct JsusFxSerializationData
{
	// note : Reaper will always save/restore values using single precision floating point, so we don't use EEL_F here,
	// as it may be double or float depending on compile time options
	
	struct Slider
	{
		int index;
		float value;
	};
	
	std::vector<Slider> sliders;
	std::vector<float> vars;
	
	void addSlider(const int index, const float value)
	{
		sliders.resize(sliders.size() + 1);
		
		Slider & slider = sliders.back();
		slider.index = index;
		slider.value = value;
	}
	
	void addVar(const float value)
	{
		vars.push_back(value);
	}
};

struct JsusFxSerializer
{
	virtual int file_avail() const = 0;
	virtual int file_var(EEL_F & value) = 0;
	virtual int file_mem(EEL_F * values, const int numValues) = 0;
};

struct JsusFxSerializer_Basic : JsusFxSerializer
{
	JsusFx * jsusFx;
	JsusFxSerializationData * serializationData;
	bool write;
	
	int varPosition;
	
	JsusFxSerializer_Basic()
		: jsusFx(nullptr)
		, serializationData()
		, write(false)
		, varPosition(0)
	{
	}
	
	void begin(JsusFx & _jsusFx, JsusFxSerializationData & _serializationData, const bool _write)
	{
		jsusFx = &_jsusFx;
		serializationData = &_serializationData;
		write = _write;
		
		varPosition = 0;
		
		if (write)
			saveSliders(*jsusFx, *serializationData);
		else
			restoreSliders(*jsusFx, *serializationData);
	}
	
	static void saveSliders(const JsusFx & jsusFx, JsusFxSerializationData & serializationData)
	{
		for (int i = 0; i < jsusFx.kMaxSliders; ++i)
		{
			if (jsusFx.sliders[i].exists)
			{
				serializationData.addSlider(i, jsusFx.sliders[i].getValue());
			}
		}
	}
	
	static void restoreSliders(JsusFx & jsusFx, const JsusFxSerializationData & serializationData)
	{
		for (int i = 0; i < serializationData.sliders.size(); ++i)
		{
			const JsusFxSerializationData::Slider & slider = serializationData.sliders[i];
			
			if (slider.index >= 0 &&
				slider.index < JsusFx::kMaxSliders &&
				jsusFx.sliders[slider.index].exists)
			{
				jsusFx.sliders[slider.index].setValue(slider.value);
			}
		}
	}
	
	virtual int file_avail() const override
	{
		if (write)
			return -1;
		else
			return varPosition == serializationData->vars.size() ? 0 : 1;
	}
	
	virtual int file_var(EEL_F & value) override
	{
		if (write)
		{
			serializationData->vars.push_back(value);
			
			return 1;
		}
		else
		{
			if (varPosition >= 0 && varPosition < serializationData->vars.size())
			{
				value = serializationData->vars[varPosition];
				varPosition++;
				return 1;
			}
			else
			{
				value = 0.f;
				return 0;
			}
		}
	}
	
	virtual int file_mem(EEL_F * values, const int numValues) override
	{
		if (write)
		{
			for (int i = 0; i < numValues; ++i)
			{
				serializationData->vars.push_back(values[i]);
			}
			return 1;
		}
		else
		{
			if (numValues < 0)
				return 0;
			if (varPosition >= 0 && varPosition + numValues <= serializationData->vars.size())
			{
				for (int i = 0; i < numValues; ++i)
				{
					values[i] = serializationData->vars[varPosition];
					varPosition++;
				}
				return 1;
			}
			else
			{
				for (int i = 0; i < numValues; ++i)
					values[i] = 0.f;
				return 0;
			}
		}
	}
};
