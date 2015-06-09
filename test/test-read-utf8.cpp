//----------------------------------------------------------------------------
// Copyright (c) 2015, Codalogic Ltd (http://www.codalogic.com)
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

#define MK_STR_WITH_ZEROS( x ) std::string( (x), sizeof(x) - 1 )

TFEATURE( "Test the test ByteTestSequence" )
{
    TTEST( MK_STR_WITH_ZEROS( "" ).size() == 0 );
    TTEST( MK_STR_WITH_ZEROS( "a" ).size() == 1 );
    TTEST( MK_STR_WITH_ZEROS( "\0" ).size() == 1 );
    TTEST( MK_STR_WITH_ZEROS( "a\0" ).size() == 2 );
    TTEST( MK_STR_WITH_ZEROS( "\0a" ).size() == 2 );
    TTEST( MK_STR_WITH_ZEROS( "\0a" )[0] == '\0' );
    TTEST( MK_STR_WITH_ZEROS( "\0a" )[1] == 'a' );
    TTEST( MK_STR_WITH_ZEROS( "\xff""a\0\0" ).size() == 4 );
    TTEST( MK_STR_WITH_ZEROS( "\xff""a\0\0" )[0] == '\xff' );
    TTEST( MK_STR_WITH_ZEROS( "\xff""a\0\0" )[1] == 'a' );
    TTEST( MK_STR_WITH_ZEROS( "\xff""a\0\0" )[2] == '\0' );
    TTEST( MK_STR_WITH_ZEROS( "\xff""a\0\0" )[3] == '\0' );
}

void test_utf_detection(
        const std::string & bytes_in,
        size_t n_to_read_in,
        cljp::ReadUTF8::Modes expected_mode_in )
{
    cljp::ReaderString reader( bytes_in );
    cljp::ReadUTF8 utf8_reader( reader );

    for( size_t i=0; i<n_to_read_in; ++i )
    {
        TCRITICALTEST( utf8_reader.get() != cljp::Reader::EOM );
    }
    TTEST( utf8_reader.mode() == expected_mode_in );
}

void test_utf8_get(
        const std::string & bytes_in,
        size_t n_to_read_in,
        int expected_c_in )
{
    cljp::ReaderString reader( bytes_in );
    cljp::ReadUTF8 utf8_reader( reader );

    int c = cljp::Reader::EOM;
    for( size_t i=0; i<n_to_read_in; ++i )
    {
        c = utf8_reader.get();
        TCRITICALTEST( c != cljp::Reader::EOM );
    }
    TTEST( c == expected_c_in );
}

void test_utf8_get_errored(
        const std::string & bytes_in,
        size_t n_to_read_in )
{
    assert( n_to_read_in >= 1 );

    cljp::ReaderString reader( bytes_in );
    cljp::ReadUTF8 utf8_reader( reader );

    int c = cljp::Reader::EOM;
    for( size_t i=0; i<n_to_read_in-1; ++i )
    {
        c = utf8_reader.get();
        TCRITICALTEST( c != cljp::Reader::EOM );
    }
    c = utf8_reader.get();
    TTEST( c == cljp::Reader::EOM );
}

void test_utf8_get_is_end(
        const std::string & bytes_in,
        size_t n_to_read_in )
{
    test_utf8_get_errored( bytes_in, n_to_read_in );    // Same logic!
}

TFEATURE( "ReadUTF8 - test MK_STR_WITH_ZEROS" )
{
    TCALL( test_utf_detection( MK_STR_WITH_ZEROS( "" ), 0, cljp::ReadUTF8::LEARNING ) );
}

// Supported input combinations are JSON-8OB-16OB-32NB
// OB = Optional BOM, MB = Mandatory BOM and NB = No BOM
//
// Assume first codepoint must be ASCII. In regular expression terms, [\t\r\n {\["tfn0-9]. Non-ASCII implies BOM
//
// Non-BOM patterns
// xx xx -- --  UTF-8
// xx 00 xx --  UTF-16LE
// xx 00 00 xx  UTF-16LE
// xx 00 00 00  UTF-32LE
// 00 xx -- --  UTF-16BE
// 00 00 -- --  UTF-32BE
//
// BOM patterns (from http://unicode.org/faq/utf_bom.html)
// EF BB BF     -> UTF-8
// FE FF        -> UTF-16, big-endian
// FF FE        -> UTF-16, little-endian
// FF FE 00 00  -> UTF-32, little-endian
// 00 00 FE FF  -> UTF-32, big-endian

TFEATURE( "ReadUTF8 - UTF-8 input" )
{
    // xx xx -- --  UTF-8
    TCALL( test_utf_detection( MK_STR_WITH_ZEROS( "ab" ), 1, cljp::ReadUTF8::LEARNING_UTF8_OR_LE ) );
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "ab" ), 1, 'a' ) );
    TCALL( test_utf_detection( MK_STR_WITH_ZEROS( "ab" ), 2, cljp::ReadUTF8::UTF8 ) );
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "ab" ), 2, 'b' ) );

    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "\x7f""b" ), 1, 0x7f ) );
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "\x7f""b" ), 2, 'b' ) );
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "a""\x7f" ), 1, 'a' ) );
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "a""\x7f" ), 2, 0x7f ) );

    TCALL( test_utf8_get_is_end( MK_STR_WITH_ZEROS( "a" ), 2 ) );

    // Multi-byte UTF-8 sequence encountered after detecting UTF-8 mode
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "abc\xE0\xAC\x8Bz" ), 4, 0xE0 ) );
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "abc\xE0\xAC\x8Bz" ), 5, 0xAC ) );
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "abc\xE0\xAC\x8Bz" ), 6, 0x8B ) );
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "abc\xE0\xAC\x8Bz" ), 7, 'z' ) );
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "abc\xE0\xAC\x8Bz\xE0\xAC\x8Bz" ), 8, 0xE0 ) );
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "abc\xE0\xAC\x8Bz\xE0\xAC\x8Bz" ), 9, 0xAC ) );
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "abc\xE0\xAC\x8Bz\xE0\xAC\x8Bz" ), 10, 0x8B ) );
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "abc\xE0\xAC\x8Bz\xE0\xAC\x8Bz" ), 11, 'z' ) );
    TCALL( test_utf8_get_is_end( MK_STR_WITH_ZEROS( "abc\xE0\xAC\x8Bz\xE0\xAC\x8Bz" ), 12 ) );

    // Multi-byte UTF-8 sequence encountered while detecting UTF-8 mode
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "a\xE0\xAC\x8Bz" ), 2, 0xE0 ) );
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "a\xE0\xAC\x8Bz" ), 3, 0xAC ) );
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "a\xE0\xAC\x8Bz" ), 4, 0x8B ) );
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "a\xE0\xAC\x8Bz" ), 5, 'z' ) );
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "a\xE0\xAC\x8Bz\xE0\xAC\x8Bz" ), 6, 0xE0 ) );
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "a\xE0\xAC\x8Bz\xE0\xAC\x8Bz" ), 7, 0xAC ) );
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "a\xE0\xAC\x8Bz\xE0\xAC\x8Bz" ), 8, 0x8B ) );
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "a\xE0\xAC\x8Bz\xE0\xAC\x8Bz" ), 9, 'z' ) );
}

TFEATURE( "ReadUTF8 - UTF-8 input with BOM" )
{
    // EF BB BF     -> UTF-8 BOM
    // Assume most UTF-8 testing is done in the non-BOM case

    TCALL( test_utf_detection( MK_STR_WITH_ZEROS( "\xEF\xBB\xBF""ab" ), 1, cljp::ReadUTF8::UTF8 ) );
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "\xEF\xBB\xBF""ab" ), 1, 'a' ) );
    TCALL( test_utf_detection( MK_STR_WITH_ZEROS( "\xEF\xBB\xBF""ab" ), 2, cljp::ReadUTF8::UTF8 ) );
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "\xEF\xBB\xBF""ab" ), 2, 'b' ) );

    TCALL( test_utf8_get_is_end( MK_STR_WITH_ZEROS( "\xEF\xBB\xBF" ), 1 ) );
    TCALL( test_utf8_get_is_end( MK_STR_WITH_ZEROS( "\xEF\xBB\xBF""a" ), 2 ) );

    // Multi-byte UTF-8 sequence encountered after detecting UTF-8 mode
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "\xEF\xBB\xBF""abc\xE0\xAC\x8Bz" ), 4, 0xE0 ) );
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "\xEF\xBB\xBF""abc\xE0\xAC\x8Bz" ), 5, 0xAC ) );
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "\xEF\xBB\xBF""abc\xE0\xAC\x8Bz" ), 6, 0x8B ) );
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "\xEF\xBB\xBF""abc\xE0\xAC\x8Bz" ), 7, 'z' ) );

    // BOM error cases covered in error section
}

TFEATURE( "ReadUTF8 - UTF-8 input error cases" )
{
    TCALL( test_utf8_get_errored( MK_STR_WITH_ZEROS( "a\0" ), 2 ) );
    TCALL( test_utf8_get_errored( MK_STR_WITH_ZEROS( "ab\0" ), 3 ) );
    TCALL( test_utf8_get_errored( MK_STR_WITH_ZEROS( "abc\x80" ), 4 ) );
    TCALL( test_utf8_get_errored( MK_STR_WITH_ZEROS( "a\x80" ), 2 ) );
    TCALL( test_utf8_get_errored( MK_STR_WITH_ZEROS( "abc\xc1\x80" ), 4 ) );
    TCALL( test_utf8_get_errored( MK_STR_WITH_ZEROS( "a\xc1\x80" ), 2 ) );
    TCALL( test_utf8_get_errored( MK_STR_WITH_ZEROS( "abc\xe0\x90\x80" ), 4 ) );
    TCALL( test_utf8_get_errored( MK_STR_WITH_ZEROS( "a\xe0\x90\x80" ), 2 ) );
    TCALL( test_utf8_get_errored( MK_STR_WITH_ZEROS( "abc\xed\xa0\x80" ), 4 ) );    // High surrogate
    TCALL( test_utf8_get_errored( MK_STR_WITH_ZEROS( "a\xed\xa0\x80" ), 2 ) );      // High surrogate
    TCALL( test_utf8_get_errored( MK_STR_WITH_ZEROS( "abc\xed\xb0\x80" ), 4 ) );    // Low surrogate
    TCALL( test_utf8_get_errored( MK_STR_WITH_ZEROS( "a\xed\xb0\x80" ), 2 ) );      // Low surrogate

    // Bad UTF-8 with BOM sequences
    TCALL( test_utf8_get_errored( MK_STR_WITH_ZEROS( "\xEF""a" ), 1 ) );        // Incomplete BOM
    TCALL( test_utf8_get_errored( MK_STR_WITH_ZEROS( "\xEF\xBB""a" ), 1 ) );

    TCALL( test_utf8_get_errored( MK_STR_WITH_ZEROS( "\xEF\xBB\xBF""\0" ), 1 ) );   // Invalid UTF-8 char following BOM
}

TFEATURE( "ReadUTF8 - UTF-16LE input" )
{
    // xx 00        UTF-16LE
    TCALL( test_utf_detection( MK_STR_WITH_ZEROS( "a\0" ), 1, cljp::ReadUTF8::LEARNING_UTF8_OR_LE ) );
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "a\0" ), 1, 'a' ) );
    TCALL( test_utf8_get_is_end( MK_STR_WITH_ZEROS( "a\0" ), 2 ) );

    // xx 00 xx --  UTF-16LE
    TCALL( test_utf_detection( MK_STR_WITH_ZEROS( "a\0\xb\0" ), 1, cljp::ReadUTF8::LEARNING_UTF8_OR_LE ) );
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "a\0\xb\0" ), 1, 'a' ) );
    TCALL( test_utf_detection( MK_STR_WITH_ZEROS( "a\0\xb\0" ), 2, cljp::ReadUTF8::UTF16LE ) );
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "a\0\xb\0" ), 2, 0xb ) );
    TCALL( test_utf8_get_is_end( MK_STR_WITH_ZEROS( "a\0\xb\0" ), 3 ) );

    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "a\0" "\xb\xb" ), 2, 0xE0 ) ); // \xb -> u+0b0b -> UTF-8: E0 AC 8B
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "a\0" "\xb\xb" ), 3, 0xAC ) ); // \xb -> u+0b0b -> UTF-8: E0 AC 8B
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "a\0" "\xb\xb" ), 4, 0x8B ) ); // \xb -> u+0b0b -> UTF-8: E0 AC 8B
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "a\0" "\xb\xb" "z\0" ), 5, 'z' ) );    // \xb -> u+0b0b -> UTF-8: E0 AC 8B
    TCALL( test_utf8_get_is_end( MK_STR_WITH_ZEROS( "a\0" "\xb\xb" "z\0" ), 6 ) );

    // xx 00 00 xx  UTF-16LE
    TCALL( test_utf_detection( MK_STR_WITH_ZEROS( "a\0\0\xb" ), 1, cljp::ReadUTF8::LEARNING_UTF8_OR_LE ) );
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "a\0\0\xb" ), 1, 'a' ) );
    TCALL( test_utf_detection( MK_STR_WITH_ZEROS( "a\0\0\xb" ), 2, cljp::ReadUTF8::UTF16LE ) );
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "a\0\0\xb" ), 2, 0xE0 ) ); // \xb -> u+0b00 -> UTF-8: E0 AC 80
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "a\0\0\xb" ), 3, 0xAC ) ); // \xb -> u+0b00 -> UTF-8: E0 AC 80
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "a\0\0\xb" ), 4, 0x80 ) ); // \xb -> u+0b00 -> UTF-8: E0 AC 80

    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "a\0" "\0\xb" "z\0" "b\0" ), 5, 'z' ) );
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "a\0" "\0\xb" "z\0" "b\0" ), 6, 'b' ) );

    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "a\0" "\0\xb" "z\0" "\0\xb" ), 6, 0xE0 ) );    // \xb -> u+0b00 -> UTF-8: E0 AC 80
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "a\0" "\0\xb" "z\0" "\0\xb" ), 7, 0xAC ) );    // \xb -> u+0b00 -> UTF-8: E0 AC 80
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "a\0" "\0\xb" "z\0" "\0\xb" ), 8, 0x80 ) );    // \xb -> u+0b00 -> UTF-8: E0 AC 80

    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "a\0\0\xb0" ), 2, 0xEB ) );    // \xb0 -> u+b000 -> UTF-8: EB 80 80
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "a\0\0\xb0" ), 3, 0x80 ) );    // \xb0 -> u+b000 -> UTF-8: EB 80 80
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "a\0\0\xb0" ), 4, 0x80 ) );    // \xb0 -> u+b000 -> UTF-8: EB 80 80
}

TFEATURE( "ReadUTF8 - UTF-16LE input with BOM" )
{
    // FF FE        -> UTF-16, little-endian BOM
    // Also:
    // FF FE 00 00  -> UTF-32, little-endian BOM
    TCALL( test_utf_detection( MK_STR_WITH_ZEROS( "\xff\xfe" "a\0\xb\0" ), 1, cljp::ReadUTF8::UTF16LE ) );
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "\xff\xfe" "a\0\xb\0" ), 1, 'a' ) );
    TCALL( test_utf_detection( MK_STR_WITH_ZEROS( "\xff\xfe" "a\0\xb\0" ), 2, cljp::ReadUTF8::UTF16LE ) );
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "\xff\xfe" "a\0\xb\0" ), 2, 0xb ) );
    TCALL( test_utf8_get_is_end( MK_STR_WITH_ZEROS( "\xff\xfe" "a\0\xb\0" ), 3 ) );

    // BOM error cases covered in error section
}

TFEATURE( "ReadUTF8 - UTF-16LE input - verify boundarries in UTF-8 encoding" )
{
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "\x7f\0" ), 1, 0x7f ) );
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "a\0\x7f\0" ), 2, 0x7f ) );
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "a\0\x80\0" ), 2, 0xC2 ) );
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "a\0\x80\0" ), 3, 0x80 ) );
    TCALL( test_utf8_get_is_end( MK_STR_WITH_ZEROS( "a\0\x80\0" ), 4 ) );

    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "a\0\xff\x7" ), 2, 0xDF ) );
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "a\0\xff\x7" ), 3, 0xBF ) );
    TCALL( test_utf8_get_is_end( MK_STR_WITH_ZEROS( "a\0\xff\x7" ), 4 ) );

    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "a\0\x00\x08" ), 2, 0xE0 ) );
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "a\0\x00\x08" ), 3, 0xA0 ) );
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "a\0\x00\x08" ), 4, 0x80 ) );
    TCALL( test_utf8_get_is_end( MK_STR_WITH_ZEROS( "a\0\x00\x08" ), 5 ) );

    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "a\0\xff\xff" ), 2, 0xEF ) );
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "a\0\xff\xff" ), 3, 0xBF ) );
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "a\0\xff\xff" ), 4, 0xBF ) );
    TCALL( test_utf8_get_is_end( MK_STR_WITH_ZEROS( "a\0\xff\xff" ), 5 ) );

    // Boundaries with surrogate values are verified below in surrogate testing
}

TFEATURE( "ReadUTF8 - UTF-16LE input surrogates" )
{
    TDOC( "UTF16LE surrogate pair in detection phase" );
    TCALL( test_utf_detection( MK_STR_WITH_ZEROS( "a\0\x00\xD8\x00\xDCz\0" ), 2, cljp::ReadUTF8::UTF16LE ) );   // \uD800\uDC00 -> U+10000 -> F0 90 80 80
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "a\0\x00\xD8\x00\xDCz\0" ), 2, 0xf0 ) );
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "a\0\x00\xD8\x00\xDCz\0" ), 3, 0x90 ) );
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "a\0\x00\xD8\x00\xDCz\0" ), 4, 0x80 ) );
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "a\0\x00\xD8\x00\xDCz\0" ), 5, 0x80 ) );
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "a\0\x00\xD8\x00\xDCz\0" ), 6, 'z' ) );
    TCALL( test_utf8_get_is_end( MK_STR_WITH_ZEROS( "a\0\x00\xD8\x00\xDCz\0" ), 7 ) );

    TDOC( "UTF16LE surrogate pair in post-detection phase" );
    TCALL( test_utf_detection( MK_STR_WITH_ZEROS( "a\0b\0c\0\x00\xD8\x00\xDCz\0" ), 2, cljp::ReadUTF8::UTF16LE ) ); // \uD800\uDC00 -> U+10000 -> F0 90 80 80
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "a\0b\0c\0\x00\xD8\x00\xDCz\0" ), 4, 0xf0 ) );
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "a\0b\0c\0\x00\xD8\x00\xDCz\0" ), 5, 0x90 ) );
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "a\0b\0c\0\x00\xD8\x00\xDCz\0" ), 6, 0x80 ) );
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "a\0b\0c\0\x00\xD8\x00\xDCz\0" ), 7, 0x80 ) );
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "a\0b\0c\0\x00\xD8\x00\xDCz\0" ), 8, 'z' ) );
    TCALL( test_utf8_get_is_end( MK_STR_WITH_ZEROS( "a\0b\0c\0\x00\xD8\x00\xDCz\0" ), 9 ) );
}

TFEATURE( "ReadUTF8 - UTF-16LE input error cases" )
{
    // TCALL( test_utf8_get_errored( MK_STR_WITH_ZEROS( "a" ), 1 ) );           // OK - it assumes it's UTF-8
    TCALL( test_utf8_get_errored( MK_STR_WITH_ZEROS( "a\0b" ), 2 ) );           // Truncated
    TCALL( test_utf8_get_errored( MK_STR_WITH_ZEROS( "a\0\x00\xD8" ), 2 ) );    // High surrogate without following low surrogate
    TCALL( test_utf8_get_errored( MK_STR_WITH_ZEROS( "a\0\x00\xD8\0" ), 2 ) );  // High surrogate without following low surrogate
    TCALL( test_utf8_get_errored( MK_STR_WITH_ZEROS( "a\0\x00\xD8z\0" ), 2 ) ); // High surrogate without following low surrogate
    TCALL( test_utf8_get_errored( MK_STR_WITH_ZEROS( "a\0\x00\xDC" ), 2 ) );    // Low surrogate without prior high surrogate

    // Bad BOM sequences
    TCALL( test_utf8_get_errored( MK_STR_WITH_ZEROS( "\xff\x0" "a\0\xb\0" ), 1 ) );
}

TFEATURE( "ReadUTF8 - UTF-16BE input" )
{
    // 00 xx -- --  UTF-16BE
    // Also:
    // 00 00 -- --  UTF-32BE
    // 00 00 FE FF  -> UTF-32, big-endian BOM

    // 00 xx -- --  UTF-16BE
    TCALL( test_utf_detection( MK_STR_WITH_ZEROS( "\0z" ), 1, cljp::ReadUTF8::UTF16BE ) );
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "\0z" ), 1, 'z' ) );
    TCALL( test_utf8_get_is_end( MK_STR_WITH_ZEROS( "\0z" ), 2 ) );

    TCALL( test_utf_detection( MK_STR_WITH_ZEROS( "\0z\0\xb" ), 1, cljp::ReadUTF8::UTF16BE ) );
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "\0z\0\xb" ), 1, 'z' ) );
    TCALL( test_utf_detection( MK_STR_WITH_ZEROS( "\0z\0\xb" ), 2, cljp::ReadUTF8::UTF16BE ) );
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "\0z\0\xb" ), 2, 0xb ) );
    TCALL( test_utf8_get_is_end( MK_STR_WITH_ZEROS( "\0z\0\xb" ), 3 ) );

    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "\0z" "\xb\xb" ), 2, 0xE0 ) ); // \xb -> u+0b0b -> UTF-8: E0 AC 8B
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "\0z" "\xb\xb" ), 3, 0xAC ) ); // \xb -> u+0b0b -> UTF-8: E0 AC 8B
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "\0z" "\xb\xb" ), 4, 0x8B ) ); // \xb -> u+0b0b -> UTF-8: E0 AC 8B
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "\0z" "\xb\xb" "\0z" ), 5, 'z' ) );    // \xb -> u+0b0b -> UTF-8: E0 AC 8B
    TCALL( test_utf8_get_is_end( MK_STR_WITH_ZEROS( "\0z" "\xb\xb" "\0z" ), 6 ) );

    TCALL( test_utf_detection( MK_STR_WITH_ZEROS( "\0z\xb\0" ), 1, cljp::ReadUTF8::UTF16BE ) );
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "\0z\xb\0" ), 1, 'z' ) );
    TCALL( test_utf_detection( MK_STR_WITH_ZEROS( "\0z\xb\0" ), 2, cljp::ReadUTF8::UTF16BE ) );
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "\0z\xb\0" ), 2, 0xE0 ) ); // \xb -> u+0b00 -> UTF-8: E0 AC 80
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "\0z\xb\0" ), 3, 0xAC ) ); // \xb -> u+0b00 -> UTF-8: E0 AC 80
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "\0z\xb\0" ), 4, 0x80 ) ); // \xb -> u+0b00 -> UTF-8: E0 AC 80

    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "\0z" "\xb\0" "\0z" "\0y" ), 5, 'z' ) );
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "\0z" "\xb\0" "\0z" "\0y" ), 6, 'y' ) );

    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "\0z" "\xb\0" "\0z" "\xb\0" ), 6, 0xE0 ) );    // \xb -> u+0b00 -> UTF-8: E0 AC 80
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "\0z" "\xb\0" "\0z" "\xb\0" ), 7, 0xAC ) );    // \xb -> u+0b00 -> UTF-8: E0 AC 80
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "\0z" "\xb\0" "\0z" "\xb\0" ), 8, 0x80 ) );    // \xb -> u+0b00 -> UTF-8: E0 AC 80

    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "\0z\xb0\0" ), 2, 0xEB ) );    // \xb0 -> u+b000 -> UTF-8: EB 80 80
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "\0z\xb0\0" ), 3, 0x80 ) );    // \xb0 -> u+b000 -> UTF-8: EB 80 80
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "\0z\xb0\0" ), 4, 0x80 ) );    // \xb0 -> u+b000 -> UTF-8: EB 80 80
}

TFEATURE( "ReadUTF8 - UTF-16BE input with BOM" )
{
    // FE FF        -> UTF-16, big-endian
    TCALL( test_utf_detection( MK_STR_WITH_ZEROS( "\xfe\xff" "\0a\0\xb" ), 1, cljp::ReadUTF8::UTF16BE ) );
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "\xfe\xff" "\0a\0\xb" ), 1, 'a' ) );
    TCALL( test_utf_detection( MK_STR_WITH_ZEROS( "\xfe\xff" "\0a\0\xb" ), 2, cljp::ReadUTF8::UTF16BE ) );
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "\xfe\xff" "\0a\0\xb" ), 2, 0xb ) );
    TCALL( test_utf8_get_is_end( MK_STR_WITH_ZEROS( "\xfe\xff" "\0a\0\xb" ), 3 ) );

    // BOM error cases covered in error section
}

TFEATURE( "ReadUTF8 - UTF-16BE input - verify boundarries in UTF-8 encoding" )
{
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "\0\x7f" ), 1, 0x7f ) );
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "\0z\0\x7f" ), 2, 0x7f ) );
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "\0z\0\x80" ), 2, 0xC2 ) );
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "\0z\0\x80" ), 3, 0x80 ) );
    TCALL( test_utf8_get_is_end( MK_STR_WITH_ZEROS( "\0z\0\x80" ), 4 ) );

    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "\0z\x7\xff" ), 2, 0xDF ) );
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "\0z\x7\xff" ), 3, 0xBF ) );
    TCALL( test_utf8_get_is_end( MK_STR_WITH_ZEROS( "\0z\x7\xff" ), 4 ) );

    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "\0z\x08\x00" ), 2, 0xE0 ) );
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "\0z\x08\x00" ), 3, 0xA0 ) );
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "\0z\x08\x00" ), 4, 0x80 ) );
    TCALL( test_utf8_get_is_end( MK_STR_WITH_ZEROS( "\0z\x08\x00" ), 5 ) );

    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "\0z\xff\xff" ), 2, 0xEF ) );
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "\0z\xff\xff" ), 3, 0xBF ) );
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "\0z\xff\xff" ), 4, 0xBF ) );
    TCALL( test_utf8_get_is_end( MK_STR_WITH_ZEROS( "\0z\xff\xff" ), 5 ) );

    // Boundaries with surrogate values are verified below in surrogate testing
}

TFEATURE( "ReadUTF8 - UTF-16BE input surrogates" )
{
    TDOC( "UTF16BE surrogate pair in detection phase" );
    TCALL( test_utf_detection( MK_STR_WITH_ZEROS( "\0z\xD8\x00\xDC\x00\0z" ), 2, cljp::ReadUTF8::UTF16BE ) );   // \uD800\uDC00 -> U+10000 -> F0 90 80 80
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "\0z\xD8\x00\xDC\x00\0z" ), 2, 0xf0 ) );
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "\0z\xD8\x00\xDC\x00\0z" ), 3, 0x90 ) );
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "\0z\xD8\x00\xDC\x00\0z" ), 4, 0x80 ) );
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "\0z\xD8\x00\xDC\x00\0z" ), 5, 0x80 ) );
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "\0z\xD8\x00\xDC\x00\0z" ), 6, 'z' ) );
    TCALL( test_utf8_get_is_end( MK_STR_WITH_ZEROS( "\0z\xD8\x00\xDC\x00\0z" ), 7 ) );

    TDOC( "UTF16BE surrogate pair in post-detection phase" );
    TCALL( test_utf_detection( MK_STR_WITH_ZEROS( "\0z\0m\0n\xD8\x00\xDC\x00\0z" ), 2, cljp::ReadUTF8::UTF16BE ) ); // \uD800\uDC00 -> U+10000 -> F0 90 80 80
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "\0z\0m\0n\xD8\x00\xDC\x00\0z" ), 4, 0xf0 ) );
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "\0z\0m\0n\xD8\x00\xDC\x00\0z" ), 5, 0x90 ) );
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "\0z\0m\0n\xD8\x00\xDC\x00\0z" ), 6, 0x80 ) );
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "\0z\0m\0n\xD8\x00\xDC\x00\0z" ), 7, 0x80 ) );
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "\0z\0m\0n\xD8\x00\xDC\x00\0z" ), 8, 'z' ) );
    TCALL( test_utf8_get_is_end( MK_STR_WITH_ZEROS( "\0z\0m\0n\xD8\x00\xDC\x00\0z" ), 9 ) );
}

TFEATURE( "ReadUTF8 - UTF-16BE input error cases" )
{
    // TCALL( test_utf8_get_errored( MK_STR_WITH_ZEROS( "a" ), 1 ) );           // OK - it assumes it's UTF-8
    TCALL( test_utf8_get_errored( MK_STR_WITH_ZEROS( "\0" ), 1 ) );                 // Truncated
    TCALL( test_utf8_get_errored( MK_STR_WITH_ZEROS( "\0a\0" ), 2 ) );              // Truncated
    TCALL( test_utf8_get_errored( MK_STR_WITH_ZEROS( "\0a\xD8\x00" ), 2 ) );        // High surrogate without following low surrogate
    TCALL( test_utf8_get_errored( MK_STR_WITH_ZEROS( "\0a\xD8\x00\xDC" ), 2 ) );    // High surrogate without following low surrogate
    TCALL( test_utf8_get_errored( MK_STR_WITH_ZEROS( "\0a\xD8\x00\0" ), 2 ) );      // High surrogate without following low surrogate
    TCALL( test_utf8_get_errored( MK_STR_WITH_ZEROS( "\0a\xD8\x00\0z" ), 2 ) );     // High surrogate without following low surrogate
    TCALL( test_utf8_get_errored( MK_STR_WITH_ZEROS( "\0a\xDC\x00" ), 2 ) );        // Low surrogate without prior high surrogate

    // Bad BOM sequences
    TCALL( test_utf8_get_errored( MK_STR_WITH_ZEROS( "\xfe\x0" "a\0\xb\0" ), 1 ) );
}

TFEATURE( "ReadUTF8 - UTF-32LE input" )
{
    // xx 00 00 00  UTF-32LE
    TCALL( test_utf_detection( MK_STR_WITH_ZEROS( "z\0\0\0" ), 1, cljp::ReadUTF8::LEARNING_UTF8_OR_LE ) );
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "z\0\0\0" ), 1, 'z' ) );
    TCALL( test_utf8_get_is_end( MK_STR_WITH_ZEROS( "z\0\0\0" ), 2 ) );

    TCALL( test_utf_detection( MK_STR_WITH_ZEROS( "z\0\0\0a\0\0\0" ), 2, cljp::ReadUTF8::UTF32LE ) );
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "z\0\0\0a\0\0\0" ), 2, 'a' ) );
    TCALL( test_utf8_get_is_end( MK_STR_WITH_ZEROS( "z\0\0\0a\0\0\0" ), 3 ) );
}

TFEATURE( "ReadUTF8 - UTF-32LE input with BOM" )
{
    // FF FE 00 00  -> UTF-32, little-endian BOM
    // Also:
    // FF FE        -> UTF-16, little-endian BOM
    TCALL( test_utf_detection( MK_STR_WITH_ZEROS(   "\xff\xfe\0\0" "a\0\0\0" "\xb\0\0\0" ), 1, cljp::ReadUTF8::UTF32LE ) );
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS(        "\xff\xfe\0\0" "a\0\0\0" "\xb\0\0\0" ), 1, 'a' ) );
    TCALL( test_utf_detection( MK_STR_WITH_ZEROS(   "\xff\xfe\0\0" "a\0\0\0" "\xb\0\0\0" ), 2, cljp::ReadUTF8::UTF32LE ) );
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS(        "\xff\xfe\0\0" "a\0\0\0" "\xb\0\0\0" ), 2, 0xb ) );
    TCALL( test_utf8_get_is_end( MK_STR_WITH_ZEROS( "\xff\xfe\0\0" "a\0\0\0" "\xb\0\0\0" ), 3 ) );

    // BOM error cases covered in error section
}

TFEATURE( "ReadUTF8 - UTF-32LE input - verify boundarries in UTF-8 encoding" )
{
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "\x7f\0\0\0" ), 1, 0x7f ) );
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "a\0\0\0" "\x7f\0\0\0" ), 2, 0x7f ) );
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "a\0\0\0" "\x80\0\0\0" ), 2, 0xC2 ) );
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "a\0\0\0" "\x80\0\0\0" ), 3, 0x80 ) );
    TCALL( test_utf8_get_is_end( MK_STR_WITH_ZEROS( "a\0\0\0" "\x80\0\0\0" ), 4 ) );

    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "a\0\0\0" "\xff\x7\0\0" ), 2, 0xDF ) );
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "a\0\0\0" "\xff\x7\0\0" ), 3, 0xBF ) );
    TCALL( test_utf8_get_is_end( MK_STR_WITH_ZEROS( "a\0\0\0" "\xff\x7\0\0" ), 4 ) );

    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "a\0\0\0" "\x00\x08\0\0" ), 2, 0xE0 ) );
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "a\0\0\0" "\x00\x08\0\0" ), 3, 0xA0 ) );
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "a\0\0\0" "\x00\x08\0\0" ), 4, 0x80 ) );
    TCALL( test_utf8_get_is_end( MK_STR_WITH_ZEROS( "a\0\0\0" "\x00\x08\0\0" ), 5 ) );

    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "a\0\0\0" "\xff\xff\0\0" ), 2, 0xEF ) );
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "a\0\0\0" "\xff\xff\0\0" ), 3, 0xBF ) );
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "a\0\0\0" "\xff\xff\0\0" ), 4, 0xBF ) );
    TCALL( test_utf8_get_is_end( MK_STR_WITH_ZEROS( "a\0\0\0" "\xff\xff\0\0" ), 5 ) );

    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "a\0\0\0" "\x00\x00\x01\0" ), 2, 0xF0 ) );
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "a\0\0\0" "\x00\x00\x01\0" ), 3, 0x90 ) );
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "a\0\0\0" "\x00\x00\x01\0" ), 4, 0x80 ) );
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "a\0\0\0" "\x00\x00\x01\0" ), 5, 0x80 ) );
    TCALL( test_utf8_get_is_end( MK_STR_WITH_ZEROS( "a\0\0\0" "\x00\x00\x01\0" ), 6 ) );

    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "a\0\0\0" "\xff\xff\x10\0" ), 2, 0xF4 ) );
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "a\0\0\0" "\xff\xff\x10\0" ), 3, 0x8F ) );
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "a\0\0\0" "\xff\xff\x10\0" ), 4, 0xBF ) );
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "a\0\0\0" "\xff\xff\x10\0" ), 5, 0xBF ) );
    TCALL( test_utf8_get_is_end( MK_STR_WITH_ZEROS( "a\0\0\0" "\xff\xff\x10\0" ), 6 ) );
}

TFEATURE( "ReadUTF8 - UTF-32LE input error cases" )
{
    // Surrogates not permitted
    TCALL( test_utf8_get_errored( MK_STR_WITH_ZEROS( "a\0\0\0" "\0\xd8\0\0" ), 2 ) );
    TCALL( test_utf8_get_errored( MK_STR_WITH_ZEROS( "a\0\0\0" "\0\xdc\0\0" ), 2 ) );

    // Bad BOM sequences
    TCALL( test_utf8_get_errored( MK_STR_WITH_ZEROS( "\xfe\x0\0\0" "a\0\0\0" "\xb\0\0\0" ), 1 ) );
}

TFEATURE( "ReadUTF8 - UTF-32BE input" )
{
    // 00 00 -- --  UTF-32BE
    // Also:
    // 00 xx -- --  UTF-16BE
    // 00 00 FE FF  -> UTF-32, big-endian BOM

    TCALL( test_utf_detection( MK_STR_WITH_ZEROS( "\0\0\0z" ), 1, cljp::ReadUTF8::UTF32BE ) );
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "\0\0\0z" ), 1, 'z' ) );
    TCALL( test_utf8_get_is_end( MK_STR_WITH_ZEROS( "\0\0\0z" ), 2 ) );

    TCALL( test_utf_detection( MK_STR_WITH_ZEROS( "\0\0\0z\0\0\0a" ), 2, cljp::ReadUTF8::UTF32BE ) );
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "\0\0\0z\0\0\0a" ), 2, 'a' ) );
    TCALL( test_utf8_get_is_end( MK_STR_WITH_ZEROS( "\0\0\0z\0\0\0a" ), 3 ) );
}

TFEATURE( "ReadUTF8 - UTF-32BE input with BOM" )
{
    // 00 00 FE FF  -> UTF-32, big-endian BOM
    // Also:
    // 00 00 -- --  UTF-32BE
    // 00 xx -- --  UTF-16BE

    TCALL( test_utf_detection( MK_STR_WITH_ZEROS(   "\0\0\xfe\xff" "\0\0\0a" "\0\0\0\xb" ), 1, cljp::ReadUTF8::UTF32BE ) );
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS(        "\0\0\xfe\xff" "\0\0\0a" "\0\0\0\xb" ), 1, 'a' ) );
    TCALL( test_utf_detection( MK_STR_WITH_ZEROS(   "\0\0\xfe\xff" "\0\0\0a" "\0\0\0\xb" ), 2, cljp::ReadUTF8::UTF32BE ) );
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS(        "\0\0\xfe\xff" "\0\0\0a" "\0\0\0\xb" ), 2, 0xb ) );
    TCALL( test_utf8_get_is_end( MK_STR_WITH_ZEROS( "\0\0\xfe\xff" "\0\0\0a" "\0\0\0\xb" ), 3 ) );

    // BOM error cases covered in error section
}

TFEATURE( "ReadUTF8 - UTF-32LE input - verify boundarries in UTF-8 encoding" )
{
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "\0\0\0\x7f" ), 1, 0x7f ) );
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "\0\0\0a" "\0\0\0\x7f" ), 2, 0x7f ) );
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "\0\0\0a" "\0\0\0\x80" ), 2, 0xC2 ) );
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "\0\0\0a" "\0\0\0\x80" ), 3, 0x80 ) );
    TCALL( test_utf8_get_is_end( MK_STR_WITH_ZEROS( "\0\0\0a" "\0\0\0\x80" ), 4 ) );

    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "\0\0\0a" "\0\0\x7\xff" ), 2, 0xDF ) );
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "\0\0\0a" "\0\0\x7\xff" ), 3, 0xBF ) );
    TCALL( test_utf8_get_is_end( MK_STR_WITH_ZEROS( "\0\0\0a" "\0\0\x7\xff" ), 4 ) );

    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "\0\0\0a" "\x00\0\x08\0" ), 2, 0xE0 ) );
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "\0\0\0a" "\x00\0\x08\0" ), 3, 0xA0 ) );
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "\0\0\0a" "\x00\0\x08\0" ), 4, 0x80 ) );
    TCALL( test_utf8_get_is_end( MK_STR_WITH_ZEROS( "\0\0\0a" "\x00\0\x08\0" ), 5 ) );

    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "\0\0\0a" "\0\0\xff\xff" ), 2, 0xEF ) );
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "\0\0\0a" "\0\0\xff\xff" ), 3, 0xBF ) );
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "\0\0\0a" "\0\0\xff\xff" ), 4, 0xBF ) );
    TCALL( test_utf8_get_is_end( MK_STR_WITH_ZEROS( "\0\0\0a" "\0\0\xff\xff" ), 5 ) );

    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "\0\0\0a" "\x00\x01\x00\0" ), 2, 0xF0 ) );
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "\0\0\0a" "\x00\x01\x00\0" ), 3, 0x90 ) );
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "\0\0\0a" "\x00\x01\x00\0" ), 4, 0x80 ) );
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "\0\0\0a" "\x00\x01\x00\0" ), 5, 0x80 ) );
    TCALL( test_utf8_get_is_end( MK_STR_WITH_ZEROS( "\0\0\0a" "\x00\x01\x00\0" ), 6 ) );

    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "\0\0\0a" "\0\x10\xff\xff" ), 2, 0xF4 ) );
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "\0\0\0a" "\0\x10\xff\xff" ), 3, 0x8F ) );
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "\0\0\0a" "\0\x10\xff\xff" ), 4, 0xBF ) );
    TCALL( test_utf8_get( MK_STR_WITH_ZEROS( "\0\0\0a" "\0\x10\xff\xff" ), 5, 0xBF ) );
    TCALL( test_utf8_get_is_end( MK_STR_WITH_ZEROS( "\0\0\0a" "\0\x10\xff\xff" ), 6 ) );
}

TFEATURE( "ReadUTF8 - UTF-32BE input error cases" )
{
    // Surrogates not permitted
    TCALL( test_utf8_get_errored( MK_STR_WITH_ZEROS( "\0\0\0a" "\0\0\xd8\0" ), 2 ) );
    TCALL( test_utf8_get_errored( MK_STR_WITH_ZEROS( "\0\0\0a" "\0\0\xdc\0" ), 2 ) );

    // Bad BOM sequences
    TCALL( test_utf8_get_errored( MK_STR_WITH_ZEROS( "\0\0\xfe\0" "\0\0\0a" "\0\0\0\xb" ), 1 ) );
}

void test_utf8_check_second_code_point(
        cljp::ReadUTF8 & r_utf8_reader, // Must contain atleast 2 code points
        cljp::ReadUTF8::Modes expected_mode_in,
        int expected_c_in )
{
    int c = cljp::Reader::EOM;
    for( size_t i=0; i<2; ++i )
    {
        c = r_utf8_reader.get();
        TCRITICALTEST( c != cljp::Reader::EOM );
    }
    TTEST( r_utf8_reader.mode() == expected_mode_in );
    TTEST( c == expected_c_in );
}

void test_utf8_rewind(
        const std::string & bytes_in,   // Must contain at least 2 code points
        cljp::ReadUTF8::Modes expected_mode_in,
        int expected_c_in )
{
    cljp::ReaderString reader( bytes_in );
    cljp::ReadUTF8 utf8_reader( reader );

    test_utf8_check_second_code_point( utf8_reader, expected_mode_in, expected_c_in );

    utf8_reader.rewind();

    test_utf8_check_second_code_point( utf8_reader, expected_mode_in, expected_c_in );
}

TFEATURE( "ReadUTF8 - Rewind" )
{
    TCALL( test_utf8_rewind( MK_STR_WITH_ZEROS( "ab" ), cljp::ReadUTF8::UTF8, 'b' ) );
    TCALL( test_utf8_rewind( MK_STR_WITH_ZEROS( "\xEF\xBB\xBF""ab" ), cljp::ReadUTF8::UTF8, 'b' ) );
    TCALL( test_utf8_rewind( MK_STR_WITH_ZEROS( "a\0b\0" ), cljp::ReadUTF8::UTF16LE, 'b' ) );
    TCALL( test_utf8_rewind( MK_STR_WITH_ZEROS( "\xff\xfe" "a\0b\0" ), cljp::ReadUTF8::UTF16LE, 'b' ) );
    TCALL( test_utf8_rewind( MK_STR_WITH_ZEROS( "\0z\0b" ), cljp::ReadUTF8::UTF16BE, 'b' ) );
    TCALL( test_utf8_rewind( MK_STR_WITH_ZEROS( "\xfe\xff" "\0a\0b" ), cljp::ReadUTF8::UTF16BE, 'b' ) );
    TCALL( test_utf8_rewind( MK_STR_WITH_ZEROS( "z\0\0\0b\0\0\0" ), cljp::ReadUTF8::UTF32LE, 'b' ) );
    TCALL( test_utf8_rewind( MK_STR_WITH_ZEROS( "\xff\xfe\0\0" "a\0\0\0" "b\0\0\0" ), cljp::ReadUTF8::UTF32LE, 'b' ) );
    TCALL( test_utf8_rewind( MK_STR_WITH_ZEROS( "\0\0\0z\0\0\0b" ), cljp::ReadUTF8::UTF32BE, 'b' ) );
    TCALL( test_utf8_rewind( MK_STR_WITH_ZEROS( "\0\0\xfe\xff" "\0\0\0a" "\0\0\0b" ), cljp::ReadUTF8::UTF32BE, 'b' ) );
}
