/*
 * Copyright 2018 Pascal Gauthier, Marcel Smit
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

#include "WDL/eel2/ns-eel.h"
#include <iostream>

class JsusFx;

struct JsusFxFileAPI {
	void init(NSEEL_VMCTX vm);
	
	virtual int file_open(JsusFx & jsusFx, const char * filename) { return -1; }
	virtual bool file_close(JsusFx & jsusFx, const int handle) { return false; }
	virtual int file_avail(JsusFx & jsusFx, const int handle) { return 0; }
	virtual bool file_riff(JsusFx & jsusFx, const int handle, int & numChannels, int & sampleRate) { return false; }
	virtual bool file_text(JsusFx & jsusFx, const int handle) { return false; }
	virtual int file_mem(JsusFx & jsusFx, const int handle, EEL_F * result, const int numValues) { return 0; }
	virtual bool file_var(JsusFx & jsusFx, const int handle, EEL_F & result) { return false; }
};

//

struct RIFFSoundData;

struct JsusFx_File
{
	enum Mode
	{
		kMode_None,
		kMode_Text,
		kMode_Sound
	};
	
	std::istream * stream;
	
	Mode mode;
	
	RIFFSoundData * soundData;
	
	int readPosition;
	
	std::vector<EEL_F> vars;
	
	JsusFx_File();
	~JsusFx_File();
	
	bool open(JsusFx & jsusFx, const char * _filename);
	void close(JsusFx & jsusFx);
	
	bool riff(int & numChannels, int & sampleRate);
	bool text();
	
	int avail() const;
	
	bool mem(const int numValues, EEL_F * dest);
	bool var(EEL_F & value);
};
