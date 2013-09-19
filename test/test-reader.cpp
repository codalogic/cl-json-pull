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

TFEATURE( "class ReaderMemory" )
{
    {
    std::string in( "abc" );

    cljpp::ReaderString reader( in );

    TTEST( reader.get() == 'a' );
    TTEST( reader.get() == 'b' );
    TTEST( reader.get() == 'c' );
    TTEST( reader.get() == cljpp::Reader::EOM );
    TTEST( reader.get() == cljpp::Reader::EOM );
    }

    {
    std::string in( "" );

    cljpp::ReaderString reader( in );

    TTEST( reader.get() == cljpp::Reader::EOM );
    }
}

TFEATURE( "class ReaderFile" )
{
    const char * p_test_file_name = "Reader-test-abc.txt";

    {
    std::ofstream fout( p_test_file_name );
    TCRITICALTEST( fout.is_open() );

    fout << "abc\xf2";
    }

    {
    cljpp::ReaderFile reader( p_test_file_name );

    TCRITICALTEST( reader.is_open() );

    TTEST( reader.get() == 'a' );
    TTEST( reader.get() == 'b' );
    TTEST( reader.get() == 'c' );
    TTEST( reader.get() == 0x00f2 );
    TTEST( reader.get() == cljpp::Reader::EOM );

    reader.rewind();
    TTEST( reader.get() == 'a' );
    }
}

TFEATURE( "class ReadUTF8WithUnget" )
{
    {
    std::string in( "abc" );

    cljpp::ReaderString reader( in );

    cljpp::ReadUTF8WithUnget input( reader );

    TTEST( input.get() == 'a' );
    TTEST( input.get() == 'b' );
    input.unget( 'f' );
    TTEST( input.get() == 'f' );
    input.unget( 'g' );
    input.unget( 'h' );
    TTEST( input.get() == 'h' );
    TTEST( input.get() == 'g' );
    TTEST( input.get() == 'c' );
    TTEST( input.get() == cljpp::Reader::EOM );
    TTEST( input.get() == cljpp::Reader::EOM );
    input.unget( cljpp::Reader::EOM );
    TTEST( input.get() == cljpp::Reader::EOM );

    input.unget( 'g' );
    input.unget( 'h' );
    TTEST( input.get() == 'h' );
    TTEST( input.get() == 'g' );
    TTEST( input.get() == cljpp::Reader::EOM );
    }

    {
    std::string in( "" );

    cljpp::ReaderString reader( in );

    cljpp::ReadUTF8WithUnget input( reader );

    TTEST( input.get() == cljpp::Reader::EOM );
    }

    {
    std::string in( "a  b\n\t c" );

    cljpp::ReaderString reader( in );

    cljpp::ReadUTF8WithUnget input( reader );

    TTEST( input.get_non_ws() == 'a' );
    TTEST( input.get_non_ws() == 'b' );
    TTEST( input.get_non_ws() == 'c' );
    }
}
