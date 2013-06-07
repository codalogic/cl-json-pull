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

#include "clunit.h"

#include "cl-json-puller.h"

TFUNCTION( reader_memory )
{
	TDOC( "reader_memory" );
	
	{
	std::string in( "abc" );
	
	cljp::ReaderString reader( in );
	
	TTEST( reader.get() == 'a' );
	TTEST( reader.get() == 'b' );
	reader.unget( 'f' );
	TTEST( reader.get() == 'f' );
	reader.unget( 'g' );
	reader.unget( 'h' );
	TTEST( reader.get() == 'h' );
	TTEST( reader.get() == 'g' );
	TTEST( reader.get() == 'c' );
	TTEST( reader.get() == cljp::Reader::EOM );
	TTEST( reader.get() == cljp::Reader::EOM );
	reader.unget( cljp::Reader::EOM );
	TTEST( reader.get() == cljp::Reader::EOM );
	
	reader.unget( 'g' );
	reader.unget( 'h' );
	TTEST( reader.get() == 'h' );
	TTEST( reader.get() == 'g' );
	TTEST( reader.get() == cljp::Reader::EOM );
	}
	
	{
	std::string in( "" );
	
	cljp::ReaderString reader( in );
	
	TTEST( reader.get() == cljp::Reader::EOM );
	}
	
	TTODO( "Test rewind, including unget then rewind" );
}

TFUNCTION( reader_file )
{
	TDOC( "reader_file" );

	TTODO( "reader_file" );
}
