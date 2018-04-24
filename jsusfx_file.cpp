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

#define REAPER_GET_INTERFACE(opaque) ((opaque) ? ((JsusFx*)opaque) : nullptr)

static EEL_F NSEEL_CGEN_CALL _file_open(void *opaque, EEL_F *_index)
{
	JsusFxFileAPI *fileAPI = EEL_FILE_GET_INTERFACE(opaque);
	JsusFx *fx = REAPER_GET_INTERFACE(opaque);
	
	const int index = *_index;
	
	WDL_FastString * fs = nullptr;
	const char * filename = fx->getString(index, &fs);
	
	if (filename == nullptr)
		return -1;
	
	return fileAPI->file_open(filename);
}

static EEL_F NSEEL_CGEN_CALL _file_close(void *opaque, EEL_F *_handle)
{
	JsusFxFileAPI *fileAPI = EEL_FILE_GET_INTERFACE(opaque);
	
	const int handle = *_handle;
	
	if (fileAPI->file_close(handle))
		return 0;
	else
		return -1;
}

static EEL_F NSEEL_CGEN_CALL _file_avail(void *opaque, EEL_F *_handle)
{
	JsusFxFileAPI *fileAPI = EEL_FILE_GET_INTERFACE(opaque);
	
  	const int handle = *_handle;
	
  	return fileAPI->file_avail(handle);
}

static EEL_F NSEEL_CGEN_CALL _file_riff(void *opaque, EEL_F *_handle, EEL_F *_numChannels, EEL_F *_sampleRate)
{
	JsusFxFileAPI *fileAPI = EEL_FILE_GET_INTERFACE(opaque);
	
	const int handle = *_handle;
	
	int numChannels;
	int sampleRate;
	
	if (fileAPI->file_riff(handle, numChannels, sampleRate) == false)
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
	
	const int handle = *_handle;
	
	if (fileAPI->file_text(handle) == false)
		return -1;
	
	return 1;
}

static EEL_F NSEEL_CGEN_CALL _file_mem(void *opaque, EEL_F *_handle, EEL_F *_destOffset, EEL_F *_numValues)
{
	JsusFxFileAPI *fileAPI = EEL_FILE_GET_INTERFACE(opaque);
	JsusFx *fx = REAPER_GET_INTERFACE(opaque);
	
	const int handle = *_handle;
	
	const int destOffset = (int)(*_destOffset + 0.001);
	
	EEL_F * dest = NSEEL_VM_getramptr(fx->m_vm, destOffset, nullptr);
	
	if (dest == nullptr)
		return 0;
	
	const int numValues = (int)*_numValues;
	
	return fileAPI->file_mem(handle, dest, numValues);
}

static EEL_F NSEEL_CGEN_CALL _file_var(void *opaque, EEL_F *_handle, EEL_F *dest)
{
	JsusFxFileAPI *fileAPI = EEL_FILE_GET_INTERFACE(opaque);
	
	const int handle = *_handle;
	
	if (fileAPI->file_var(handle, *dest) == false)
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
