//----------------------------------------------------------------------------
// Copyright (c) 2013, Codalogic Ltd (http://www.codalogic.com)
// All rights reserved.
//
// The license for this file is based on the BSD-3-Clause license
// (http://www.opensource.org/licenses/BSD-3-Clause).
//
// Redistribution and use in source and binary forms, with or without 
// modification, are permitted provided that the following conditions 
// are met:
//
// - Redistributions of source code must retain the above copyright notice, 
//   this list of conditions and the following disclaimer.
// - Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
// - Neither the name Codalogic Ltd nor the names of its contributors may be
//   used to endorse or promote products derived from this software without 
//   specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
// POSSIBILITY OF SUCH DAMAGE.
//----------------------------------------------------------------------------

#include "cl-json-puller.h"

#include <cstdio>

namespace cljp {	// Codalogic JSON Puller

//----------------------------------------------------------------------------
//                             class Reader
//----------------------------------------------------------------------------

const int Reader::EOM = -1;

//----------------------------------------------------------------------------
//                             class ReaderMemory
//----------------------------------------------------------------------------

ReaderMemory::ReaderMemory( const char * p_start_in, const char * p_end_in )
	: m( p_start_in, p_end_in )
{
}

int ReaderMemory::do_get()
{
	if( m.p_now < m.p_end )
		return static_cast< unsigned char >( *m.p_now++ );
	return EOM;
}

void ReaderMemory::do_rewind()
{
	m.p_now = m.p_start;
}

//----------------------------------------------------------------------------
//                             class ReaderFile
//----------------------------------------------------------------------------

ReaderFile::ReaderFile( const char * p_file_name_in )
	: m( fopen( p_file_name_in, "r" ) )
{
}

ReaderFile::ReaderFile( FILE * h_fin_in )
	: m( h_fin_in )
{
}

ReaderFile::~ReaderFile()
{
	if( m.is_close_on_destruct_required )
		fclose( m.h_fin );
}

int ReaderFile::do_get()
{
	if( ! is_open() )
		return EOM;
	int c = fgetc( m.h_fin );
	if( c == EOF )
		return EOM;
	return c;
}

void ReaderFile::do_rewind()
{
	if( is_open() )
		fseek( m.h_fin, 0, SEEK_SET );
}

void ReaderFile::close_on_destruct( bool is_close_on_destruct_required )
{
	m.is_close_on_destruct_required = is_close_on_destruct_required;
}

//----------------------------------------------------------------------------
//                             class UTFConverter
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//                               class ReadUTF8
//----------------------------------------------------------------------------

int ReadUTF8::get()
{
	return m.r_reader.get();
}

//----------------------------------------------------------------------------
//                           class ReadUTF8WithUnget
//----------------------------------------------------------------------------

int ReadUTF8WithUnget::get()
{
	if( ! m.unget_buffer.empty() )
	{
		int result = m.unget_buffer.back();
		m.unget_buffer.pop_back();
		return result;
	}
	
	return m.read_utf8.get();
}

void ReadUTF8WithUnget::unget( int c )
{
	m.unget_buffer.push_back( c );
}

//----------------------------------------------------------------------------
//                               class Parser
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//                               class SmartParser
//----------------------------------------------------------------------------

}	// End of namespace cljp
