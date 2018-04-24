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

struct JsusFxFileAPI {
	void init(NSEEL_VMCTX vm);
	
	virtual int file_open(const char * filename) { return -1; }
	virtual bool file_close(const int handle) { return false; } 
	virtual int file_avail(const int handle) { return 0; }
	virtual bool file_riff(const int handle, int & numChannels, int & sampleRate) { return false; }
	virtual bool file_text(const int handle) { return false; }
	virtual int file_mem(const int handle, EEL_F * result, const int numValues) { return 0; }
	virtual bool file_var(const int handle, EEL_F & result) { return false; }
};
