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

#include "jsusfx.h"
#include "jsusfx_file.h"

#define EEL_FILE_GET_INTERFACE(opaque) ((opaque) ? (((JsusFx*)opaque)->fileAPI) : nullptr)

#define REAPER_GET_INTERFACE(opaque) (*(JsusFx*)opaque)

static EEL_F NSEEL_CGEN_CALL _file_open(void *opaque, EEL_F *_index)
{
	JsusFxFileAPI *fileAPI = EEL_FILE_GET_INTERFACE(opaque);
	JsusFx &jsusFx = REAPER_GET_INTERFACE(opaque);
	
	const int index = *_index;
	
	WDL_FastString * fs = nullptr;
	const char * filename = jsusFx.getString(index, &fs);
	
	if (filename == nullptr)
		return -1;
	
	return fileAPI->file_open(jsusFx, filename);
}

static EEL_F NSEEL_CGEN_CALL _file_close(void *opaque, EEL_F *_handle)
{
	JsusFxFileAPI *fileAPI = EEL_FILE_GET_INTERFACE(opaque);
	JsusFx &jsusFx = REAPER_GET_INTERFACE(opaque);
	
	const int handle = *_handle;
	
	if (fileAPI->file_close(jsusFx, handle))
		return 0;
	else
		return -1;
}

static EEL_F NSEEL_CGEN_CALL _file_avail(void *opaque, EEL_F *_handle)
{
	JsusFxFileAPI *fileAPI = EEL_FILE_GET_INTERFACE(opaque);
	JsusFx &jsusFx = REAPER_GET_INTERFACE(opaque);
	
  	const int handle = *_handle;
	
  	return fileAPI->file_avail(jsusFx, handle);
}

static EEL_F NSEEL_CGEN_CALL _file_riff(void *opaque, EEL_F *_handle, EEL_F *_numChannels, EEL_F *_sampleRate)
{
	JsusFxFileAPI *fileAPI = EEL_FILE_GET_INTERFACE(opaque);
	JsusFx &jsusFx = REAPER_GET_INTERFACE(opaque);
	
	const int handle = *_handle;
	
	int numChannels;
	int sampleRate;
	
	if (fileAPI->file_riff(jsusFx, handle, numChannels, sampleRate) == false)
	{
		*_numChannels = 0;
		*_sampleRate = 0;
		return -1;
	}
	
	*_numChannels = numChannels;
	*_sampleRate = sampleRate;
	
	return 0;
}

static EEL_F NSEEL_CGEN_CALL _file_text(void *opaque, EEL_F *_handle)
{
	JsusFxFileAPI *fileAPI = EEL_FILE_GET_INTERFACE(opaque);
	JsusFx &jsusFx = REAPER_GET_INTERFACE(opaque);
	
	const int handle = *_handle;
	
	if (fileAPI->file_text(jsusFx, handle) == false)
		return -1;
	
	return 1;
}

static EEL_F NSEEL_CGEN_CALL _file_mem(void *opaque, EEL_F *_handle, EEL_F *_destOffset, EEL_F *_numValues)
{
	JsusFxFileAPI *fileAPI = EEL_FILE_GET_INTERFACE(opaque);
	JsusFx &jsusFx = REAPER_GET_INTERFACE(opaque);
	
	const int handle = *_handle;
	
	const int destOffset = (int)(*_destOffset + 0.001);
	
	EEL_F * dest = NSEEL_VM_getramptr(jsusFx.m_vm, destOffset, nullptr);
	
	if (dest == nullptr)
		return 0;
	
	const int numValues = (int)*_numValues;
	
	return fileAPI->file_mem(jsusFx, handle, dest, numValues);
}

static EEL_F NSEEL_CGEN_CALL _file_var(void *opaque, EEL_F *_handle, EEL_F *dest)
{
	JsusFxFileAPI *fileAPI = EEL_FILE_GET_INTERFACE(opaque);
	JsusFx &jsusFx = REAPER_GET_INTERFACE(opaque);
	
	const int handle = *_handle;
	
	if (fileAPI->file_var(jsusFx, handle, *dest) == false)
		return 0;
	else
		return 1;
}

void JsusFxFileAPI::init(NSEEL_VMCTX vm)
{
	NSEEL_addfunc_retval("file_open",1,NSEEL_PProc_THIS,&_file_open);
	NSEEL_addfunc_retval("file_close",1,NSEEL_PProc_THIS,&_file_close);
	NSEEL_addfunc_retval("file_avail",1,NSEEL_PProc_THIS,&_file_avail);
	NSEEL_addfunc_retval("file_riff",3,NSEEL_PProc_THIS,&_file_riff);
	NSEEL_addfunc_retval("file_text",1,NSEEL_PProc_THIS,&_file_text);
	NSEEL_addfunc_retval("file_mem",3,NSEEL_PProc_THIS,&_file_mem);
	NSEEL_addfunc_retval("file_var",2,NSEEL_PProc_THIS,&_file_var);
}

//

#include "riff.h"
#include <assert.h>

#define USE_ISTREAM 1

JsusFx_File::JsusFx_File()
	: stream(nullptr)
	, filename()
	, mode(kMode_None)
	, soundData(nullptr)
	, readPosition(0)
	, vars()
{
}

JsusFx_File::~JsusFx_File()
{
	assert(stream == nullptr);
}

bool JsusFx_File::open(JsusFx & jsusFx, const char * _filename)
{
	// reset
	
	filename.clear();
	
	// check if file exists
	
	stream = jsusFx.pathLibrary.open(_filename);
	
	if (stream == nullptr)
	{
		jsusFx.displayError("failed to open file: %s", _filename);
		return false;
	}
	else
	{
		filename = _filename;
		return true;
	}
}

void JsusFx_File::close(JsusFx & jsusFx)
{
	vars.clear();
	
	delete soundData;
	soundData = nullptr;
	
	jsusFx.pathLibrary.close(stream);
}

bool JsusFx_File::riff(int & numChannels, int & sampleRate)
{
	assert(mode == kMode_None);
	
	numChannels = 0;
	sampleRate = 0;
	
	// reset read state
	
	mode = kMode_None;
	readPosition = 0;
	
	// load RIFF file
	
	bool success = true;

#if USE_ISTREAM
	success &= stream != nullptr;
	
	int size = 0;;
	
	if (success)
	{
		try
		{
			stream->seekg(0, std::ios_base::end);
			success &= stream->fail() == false;
			
			size = stream->tellg();
			success &= size != -1;
			
			stream->seekg(0, std::ios_base::beg);
			success &= stream->fail() == false;
		}
		catch (std::exception & e)
		{
			success &= false;
		}
	}
	
	success &= size > 0;
	
	uint8_t * bytes = nullptr;
	
	if (success)
	{
		bytes = new uint8_t[size];
	
		try
		{
			stream->read((char*)bytes, size);
			
			success &= stream->fail() == false;
		}
		catch (std::exception & e)
		{
			success &= false;
		}
	}
#else
 	FILE * file = fopen(filename.c_str(), "rb");
	
	success &= file != nullptr;
	
	int size = 0;
	
	if (success)
	{
		success &= fseek(file, 0, SEEK_END) == 0;
		size = ftell(file);
		success &= fseek(file, 0, SEEK_SET) == 0;
	}
	
	success &= size > 0;

	uint8_t * bytes = nullptr;
	
	if (success)
	{
		bytes = new uint8_t[size];
	
		success &= fread(bytes, size, 1, file) == 1;
	}
	
	if (file != nullptr)
	{
		fclose(file);
		file = nullptr;
	}
#endif
	
	if (success)
	{
		soundData = loadRIFF(bytes, size);
	}
	
	if (bytes != nullptr)
	{
		delete [] bytes;
		bytes = nullptr;
	}
	
	if (soundData == nullptr || (soundData->channelSize != 2 && soundData->channelSize != 4))
	{
		if (soundData != nullptr)
		{
			delete soundData;
			soundData = nullptr;
		}
		
		return false;
	}
	else
	{
		mode = kMode_Sound;
		numChannels = soundData->channelCount;
		sampleRate = soundData->sampleRate;
		return true;
	}
}

bool JsusFx_File::text()
{
	assert(mode == kMode_None);
	
	// reset read state
	
	mode = kMode_None;
	readPosition = 0;
	
	// load text file
	
	try
	{
		if (stream == nullptr)
		{
			return false;
		}
		
		while (!stream->eof())
		{
			char line[2048];
			
			stream->getline(line, sizeof(line), '\n');
			
			// a poor way of skipping comments. assume / is the start of // and strip anything that come after it
			char * pos = strchr(line, '/');
			if (pos != nullptr)
				*pos = 0;
			
			const char * p = line;
			
			for (;;)
			{
				// skip trailing white space
				
				while (*p && isspace(*p))
					p++;
				
				// reached end of the line ?
				
				if (*p == 0)
					break;
				
				// parse the value
				
				double var;
				
				if (sscanf(p, "%lf", &var) == 1)
					vars.push_back(var);
				
				// skip the value
				
				while (*p && !isspace(*p))
					p++;
			}
		}
		
		mode = kMode_Text;
		
		return true;
	}
	catch (std::exception & e)
	{
		return false;
	}
}

int JsusFx_File::avail() const
{
	if (mode == kMode_None)
		return 0;
	else if (mode == kMode_Text)
		return readPosition == vars.size() ? 0 : 1;
	else if (mode == kMode_Sound)
		return soundData->sampleCount * soundData->channelCount - readPosition;
	else
		return 0;
}

bool JsusFx_File::mem(const int numValues, EEL_F * dest)
{
	if (mode == kMode_None)
		return false;
	else if (mode == kMode_Text)
		return false;
	else if (mode == kMode_Sound)
	{
		if (numValues > avail())
			return false;
		for (int i = 0; i < numValues; ++i)
		{
			const int channelIndex = readPosition / soundData->sampleCount;
			const int sampleIndex = readPosition % soundData->sampleCount;
			
			if (soundData->channelSize == 2)
			{
				const short * values = (short*)soundData->sampleData;
				
				dest[i] = values[sampleIndex * soundData->channelCount + channelIndex] / float(1 << 15);
			}
			else if (soundData->channelSize == 4)
			{
				const float * values = (float*)soundData->sampleData;
				
				dest[i] = values[sampleIndex * soundData->channelCount + channelIndex];
			}
			else
			{
				assert(false);
			}
			
			readPosition++;
		}
		
		return true;
	}
	else
		return false;
}

bool JsusFx_File::var(EEL_F & value)
{
	if (mode == kMode_None)
		return false;
	else if (mode == kMode_Text)
	{
		if (avail() < 1)
			return false;
		value = vars[readPosition];
		readPosition++;
		return true;
	}
	else if (mode == kMode_Sound)
		return false;
	else
		return false;
}
