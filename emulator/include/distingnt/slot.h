/*
MIT License

Copyright (c) 2025 Expert Sleepers Ltd

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef _DISTINGNT_SLOT_H_
#define _DISTINGNT_SLOT_H_

struct _NT_algorithm;
struct _NT_parameterInfo;
typedef union _NT_parameterValue _NT_parameterValue;
typedef unsigned int _NT_guid[4];

class _NT_slot {
public:
	const char *name();
	const _NT_guid *guid();
	
	struct _NT_algorithm *plugin();
	
	unsigned int numParameters();
	struct _NT_parameterInfo *parameterInfo(unsigned int parameterIndex);
	_NT_parameterValue parameterPresetValue(unsigned int parameterIndex);
	_NT_parameterValue parameterValue(unsigned int parameterIndex);
	
	void *refCon;
	
private:
	_NT_slot(const _NT_slot&);
	_NT_slot& operator=(const _NT_slot&);
};

#endif