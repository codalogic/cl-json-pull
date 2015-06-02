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

#include "cl-json-pull.h"

TFEATURE( "Object member testing" )
{
    TTODO( "Test that 'name' in a 'member' is correct format (inc opening quotes)" );
}

TFEATURE( "UTFConverter" )
{
    TTODO( "class UTFConverter" );
}

TFEATURE( "ReadUTF8" )
{
    TTODO( "class ReadUTF8" );

    TTODO( "ReadUTF8: test reading in UTF8" );
    TTODO( "ReadUTF8: test reading in UTF16LE" );
    TTODO( "ReadUTF8: test reading in UTF16BE" );
    TTODO( "ReadUTF8: test reading in UTF32LE" );
    TTODO( "ReadUTF8: test reading in UTF32BE" );
	// xx xx -- --  UTF-8
	// xx 00 xx --  UTF-16LE
	// xx 00 00 xx  UTF-16LE
	// xx 00 00 00  UTF-32LE
	// 00 xx -- --  UTF-16BE
	// 00 00 -- --  UTF-32BE
	// Assume first codepoint must be ASCII. In regular expression terms, [\t\r\n {\["tfn0-9]. Non-ASCII implies BOM
	//
	// Support JSON-8OB-16OB-32NB
	// OB = Optional BOM, MB = Mandatory BOM and NB = No BOM
}

TFEATURE( "TODOs" )
{
    TTODO( "Reading multiple JSON msgs from single file/stream" );
    // Need to reset JSON state (but not UTF detection) so multiple JSON msgs can be read from single file
}
