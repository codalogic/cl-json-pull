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

#include "cl-json-pull.h"

#include <cstdio>
#include <cassert>

namespace cljp {    // Codalogic JSON Pull (Parser)

namespace {         // Local implementation details

//----------------------------------------------------------------------------
//                    Local utility functions and classes
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//                             UTF-16 surrogate utilities
//----------------------------------------------------------------------------

bool is_high_surrogate( int code_point )
{
    return code_point >= 0xD800 && code_point <= 0xDBFF;
}

bool is_low_surrogate( int code_point )
{
    return code_point >= 0xDC00 && code_point <= 0xDFFF;
}

int code_point_from_surrogates( int high_surrogate, int low_surrogate )
{
    return ((high_surrogate & 0x3ff) << 10) + (low_surrogate & 0x3ff) + 0x10000;
}

//----------------------------------------------------------------------------
//                             class UTF8Sequence
//----------------------------------------------------------------------------

class UTF8Sequence
{
private:
    char utf8[6];

public:
    UTF8Sequence( int code_point )
    {
        // From rfc3629:
        // Char. number range  |        UTF-8 octet sequence
        //   (hexadecimal)    |              (binary)
        // --------------------+---------------------------------------------
        // 0000 0000-0000 007F | 0xxxxxxx
        // 0000 0080-0000 07FF | 110xxxxx 10xxxxxx
        // 0000 0800-0000 FFFF | 1110xxxx 10xxxxxx 10xxxxxx
        // 0001 0000-0010 FFFF | 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx

        if( code_point <= 0x7f )
            pack_ascii( code_point );
        else if( code_point >= 0x10000 )
            pack( '\xf0', 4, code_point );
        else if( code_point >= 0x00800 )
            pack( '\xe0', 3, code_point );
        else if( code_point >= 0x00080 )
            pack( '\xc0', 2, code_point );
        else
            utf8[0] = '\0';
    };
    operator const char * () const { return utf8; }
    char operator [] ( size_t index ) const { return utf8[index]; }
    char & operator [] ( size_t index ) { return utf8[index]; }
    void copy_to_array( int * p_dest )
    {
        char * p_utf8 = utf8;
        while( (*p_dest++ = (*p_utf8++)&0xff) != '\0' )
        {}
    }
    void copy_to_array( char * p_dest )
    {
        char * p_utf8 = utf8;
        while( (*p_dest++ = *p_utf8++) != '\0' )
        {}
    }

private:
    void pack_ascii( int code_point )
    {
        utf8[0] = code_point;
        utf8[1] = '\0';
    }

    void pack( char marker, size_t length, int code_point )
    {
        utf8[length] = '\0';
        int scaled_code_point = code_point;
        for( int i = length-1; i >= 0; --i )
        {
            utf8[i] = scaled_code_point & 0x3f;
            utf8[i] |= 0x80;
            scaled_code_point >>= 6;
        }
        utf8[0] |= marker;
    }
};

//----------------------------------------------------------------------------
//                   JSON reading functions and classes
//----------------------------------------------------------------------------

inline bool is_separator( int c )
{
    return isspace( c ) || c == ',' || c == ']' || c == '}' ||
            c == Reader::EOM;
}

class HexAccumulator
{
private:
    struct Members {
        int code_point;
        bool is_ok;
        Members() : code_point( 0 ), is_ok( true ) {}
    } m;

public:
    HexAccumulator() {}
    bool accumulate( int c )
    {
        int new_value = 0;
        if( isdigit( c ) )
            new_value = c - '0';
        else if( c >= 'a' && c <= 'f' )
            new_value = c - 'a' + 10;
        else if( c >= 'A' && c <= 'F' )
            new_value = c - 'A' + 10;
        else
            m.is_ok = false;
        m.code_point = m.code_point * 16 + new_value;
        return m.is_ok;
    }

    int code_point() const { return m.code_point; }
    bool is_ok() const { return m.is_ok; }
};

class UnicodeCodePointReader
{
    struct Members {
        ReadUTF8WithUnget & r_input;
        int c;
        Parser::ParserResult result;
        int code_point;

        Members( ReadUTF8WithUnget & r_input_in )
            :
            r_input( r_input_in ),
            c( '\0' ),
            result( Parser::PR_OK ),
            code_point( 0 )
        {}
    } m;

public:
    UnicodeCodePointReader( ReadUTF8WithUnget & r_input_in )
        : m( r_input_in )
    {
        //           %x75 4HEXDIG )  ; uXXXX                U+XXXX
        // We have read "\u" already.  We may have a surrogate
        // -which requires reading 1234\u5678 and converting to UTF-8

        if( read_code_point_code() )
        {
            if( is_low_surrogate( m.code_point ) )
                report_bad_unicode_escape();
            else if( is_high_surrogate( m.code_point ) )
                combine_low_surrogate();
        }
    }

    Parser::ParserResult result() const { return m.result; }
    operator Parser::ParserResult() const { return result(); }
    int code_point() const { return m.code_point; }
    UTF8Sequence as_utf8() const { return UTF8Sequence( m.code_point ); }

private:
    int get()
    {
        m.c = m.r_input.get();
        return m.c;
    }

    bool read_code_point_code()
    {
        HexAccumulator accumulator;
        for( size_t i=0; i<4; ++i )
            if( ! accumulator.accumulate( get() ) )
                break;
        if( ! accumulator.is_ok() )
        {
            m.result = Parser::PR_BAD_UNICODE_ESCAPE;
            if( m.c == Reader::EOM )
                m.result = Parser::PR_UNEXPECTED_END_OF_MESSAGE;
            else if( m.c == '"' )
                m.r_input.unget( m.c );     // Push back quote so end-of-string can be found later
            return false;
        }
        m.code_point = accumulator.code_point();
        return true;
    }

    void combine_low_surrogate()
    {
        int high_surrogate = m.code_point;

        if( get() == '\\' && get() == 'u' && read_code_point_code() )
        {
            if( is_low_surrogate( m.code_point ) )
                m.code_point = code_point_from_surrogates( high_surrogate, m.code_point );
            else
                report_bad_unicode_escape();
        }
        else
        {
            report_bad_unicode_escape();
        }
    }

    void report_bad_unicode_escape()
    {
        if( m.result == Parser::PR_OK )     // Don't overwrite an already recorded error
            m.result = Parser::PR_BAD_UNICODE_ESCAPE;
    }
};

class StringReader
{
private:
    struct Members {
        ReadUTF8WithUnget & r_input;
        int c;
        std::string * p_string;
        Parser::ParserResult result;

        Members( ReadUTF8WithUnget & r_input_in, int c_in, std::string * p_string_out )
            : r_input( r_input_in ), c( c_in ), p_string( p_string_out ),
                result( Parser::PR_OK )
        {}
    } m;

public:
    StringReader( ReadUTF8WithUnget & r_input_in, int c_in, std::string * p_string_out )
        : m( r_input_in, c_in, p_string_out )
    {
        // string = quotation-mark *char quotation-mark

        assert( m.c == '"' );

        skip_opening_quotes();

        parse_string();
    }

    Parser::ParserResult result() const { return m.result; }
    operator Parser::ParserResult() const { return result(); }

private:
    void get()
    {
        m.c = m.r_input.get();
    }

    void accept_and_get()
    {
        *m.p_string += m.c;
        m.c = m.r_input.get();
    }

    void skip_opening_quotes()
    {
        get();
    }

    void parse_string()
    {
        // char = unescaped /
        //       escape (
        //           %x22 /          ; "    quotation mark  U+0022
        //           %x5C /          ; \    reverse solidus U+005C
        //           %x2F /          ; /    solidus         U+002F
        //           %x62 /          ; b    backspace       U+0008
        //           %x66 /          ; f    form feed       U+000C
        //           %x6E /          ; n    line feed       U+000A
        //           %x72 /          ; r    carriage return U+000D
        //           %x74 /          ; t    tab             U+0009
        //           %x75 4HEXDIG )  ; uXXXX                U+XXXX
        // escape = %x5C              ; \
        // quotation-mark = %x22      ; "

        while( m.c != '"' && m.c != Reader::EOM )
        {
            if( m.c != '\\' )
                handled_unescaped();
            else
                handled_escaped();
        }

        if( m.c == Reader::EOM )
            m.result = Parser::PR_UNEXPECTED_END_OF_MESSAGE;
    }

    void handled_unescaped()
    {
        // unescaped = %x20-21 / %x23-5B / %x5D-10FFFF
        // %x22 = ", %x5c = \ which are already handled elsewhere

        if( m.c < 0x20 )
            m.result = Parser::PR_BAD_FORMAT_STRING;

        accept_and_get();
    }

    void handled_escaped()
    {
        //           %x22 /          ; "    quotation mark  U+0022
        //           %x5C /          ; \    reverse solidus U+005C
        //           %x2F /          ; /    solidus         U+002F
        //           %x62 /          ; b    backspace       U+0008
        //           %x66 /          ; f    form feed       U+000C
        //           %x6E /          ; n    line feed       U+000A
        //           %x72 /          ; r    carriage return U+000D
        //           %x74 /          ; t    tab             U+0009
        //           %x75 4HEXDIG )  ; uXXXX                U+XXXX

        get();

        if( try_mapping( '"', '"' ) ||
                try_mapping( '\\', '\\' ) ||
                try_mapping( '/', '/' ) ||
                try_mapping( 'b', '\b' ) ||
                try_mapping( 'f', '\f' ) ||
                try_mapping( 'n', '\n' ) ||
                try_mapping( 'r', '\r' ) ||
                try_mapping( 't', '\t' ) ||
                try_mapping_unicode_escape() )
            return; // Success

        else
        {
            record_first_error( Parser::PR_BAD_FORMAT_STRING );

            if( m.c == Reader::EOM )
                m.result = Parser::PR_UNEXPECTED_END_OF_MESSAGE;
        }
    }

    bool try_mapping( int escape_code_in, int mapped_char_in )
    {
        if( m.c == escape_code_in )
        {
            *m.p_string += mapped_char_in;
            get();
            return true;
        }
        return false;
    }

    bool try_mapping_unicode_escape()
    {
        //           %x75 4HEXDIG )  ; uXXXX                U+XXXX
        if( m.c != 'u' )
            return false;

        UnicodeCodePointReader code_point_reader( m.r_input );

        // Allow for a successful operation, without overwriting the error code of a previous unsuccessful operation
        Parser::ParserResult current_result = code_point_reader.result();
        get();

        record_first_error( current_result );

        if( current_result != Parser::PR_OK )
            return false;

        *m.p_string += code_point_reader.as_utf8();
        return true;
    }

    void record_first_error( Parser::ParserResult current_result )
    {
        if( m.result == Parser::PR_OK )
            m.result = current_result;
    }
};

class NumberReader
{
private:
    struct Members {
        ReadUTF8WithUnget & r_input;
        int c;
        Event * p_event;
        Parser::ParserResult result;

        Members( ReadUTF8WithUnget & r_input_in, int c_in, Event * p_event_out )
            : r_input( r_input_in ), c( c_in ), p_event( p_event_out ),
                result( Parser::PR_BAD_FORMAT_NUMBER )
        {}
    } m;

public:
    NumberReader( ReadUTF8WithUnget & r_input_in, int c_in, Event * p_event_out )
        : m( r_input_in, c_in, p_event_out )
    {
        // From RFC4627:
        // number = [ minus ] int [ frac ] [ exp ]

        if( optional_minus() &&
                integer() &&
                optional_frac() &&
                optional_exp() &&
                done() )
        {
            m.p_event->type = Event::T_NUMBER;
            m.result = Parser::PR_OK;
        }

        if( is_separator( m.c ) )
            m.r_input.unget( m.c );
    }
    Parser::ParserResult result() const { return m.result; }
    operator Parser::ParserResult() const { return result(); }

private:
    void accept_and_get()
    {
        m.p_event->value += m.c;
        m.c = m.r_input.get();
    }

    bool optional_minus()
    {
        // minus = %x2D               ; -

        if( m.c == '-' )
            accept_and_get();
        return true;
    }

    bool integer()
    {
        // int = zero / ( digit1-9 *DIGIT )
        // zero = %x30                ; 0
        // digit1-9 = %x31-39         ; 1-9

        if( m.c == '0' )
            accept_and_get();

        else if( m.c >= '1' && m.c <= '9' )
        {
            while( isdigit( m.c ) )
                accept_and_get();
        }

        else
            return false;

        return true;
    }

    bool optional_frac()
    {
        // frac = decimal-point 1*DIGIT
        // decimal-point = %x2E       ; .

        if( m.c == '.' )
        {
            accept_and_get();
            return one_or_more_digits();
        }

        return true;
    }

    bool one_or_more_digits()
    {
        if( ! isdigit( m.c ) )
            return false;
        while( isdigit( m.c ) )
            accept_and_get();
        return true;
    }

    bool optional_exp()
    {
        // exp = e [ minus / plus ] 1*DIGIT
        // e = %x65 / %x45            ; e E
        // minus = %x2D               ; -
        // plus = %x2B                ; +

        if( m.c == 'e' || m.c == 'E' )
        {
            accept_and_get();
            if( optional_sign() && one_or_more_digits() )
                return true;
            return false;
        }
        return true;
    }

    bool optional_sign()
    {
        if( m.c == '-' || m.c == '+' )
            accept_and_get();
        return true;
    }

    bool done()
    {
        return is_separator( m.c );
    }
};

}   // End of anonymous namespace

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
    if( m.h_fin && m.is_close_on_destruct_required )
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
//                               class ReadUTF8
//----------------------------------------------------------------------------

int ReadUTF8::get()
{
    // Supported input combinations are JSON-8OB-16OB-32NB
    // OB = Optional BOM, MB = Mandatory BOM and NB = No BOM
    //
    // Assume first code_point must be ASCII. In regular expression terms, [\t\r\n {\["tfn0-9]. Non-ASCII implies BOM
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

    if( m.p_utf8_buffer )
    {
        if( *m.p_utf8_buffer != '\0' )
            return *m.p_utf8_buffer++;
        m.p_utf8_buffer = 0;
    }

    switch( m.mode )
    {
    case LEARNING:
        return state_learning();

    case LEARNING_UTF8_OR_LE:
        return state_learning_utf8_or_le();

    case UTF8:
        {
        int c = m.r_reader.get();
        if( c > 0 && c <= 0x7f )
            return c;
        return state_utf8_reading_non_ascii( c );
        }

    case UTF16LE:
        return state_utf16le();

    case UTF16BE:
        return state_utf16be();

    case UTF32LE:
        return state_utf32le();

    case UTF32BE:
        return state_utf32be();

    case ERRORED:
        return Reader::EOM;
    }

    assert( 0 );    // Shouldn't get here
    return in_error();
}

ReadUTF8::CharPair ReadUTF8::get_pair()
{
    CharPair pair;

    pair.c1 = m.r_reader.get();

    if( pair.c1 == Reader::EOM )
    {
        pair.c2 = Reader::EOM;
        return pair;
    }

    pair.c2 = m.r_reader.get();

    if( pair.c2 == Reader::EOM )
    {
        pair.c1 = Reader::EOM;
        return pair;
    }

    return pair;
}

ReadUTF8::CharQuad ReadUTF8::get_quad()
{
    CharQuad quad;

    CharPair pair1 = get_pair();

    if( pair1.is_eom() )
    {
        quad.c1 = quad.c2 = quad.c3 = quad.c4 = Reader::EOM;
        return quad;
    }

    CharPair pair2 = get_pair();

    if( pair2.is_eom() )
    {
        quad.c1 = quad.c2 = quad.c3 = quad.c4 = Reader::EOM;
        return quad;
    }

    quad.c1 = pair1.c1; quad.c2 = pair1.c2;
    quad.c3 = pair2.c1; quad.c4 = pair2.c2;

    return quad;
}

int ReadUTF8::state_learning()
{
    int c = m.r_reader.get();

    if( c == Reader::EOM )
        return in_error();

    if( c == 0 )
        return state_learning_utf16be_or_utf32be();

    else if( c <= 0x7f )
    {
        // xx xx -- --  UTF-8
        // xx 00 xx --  UTF-16LE
        // xx 00 00 xx  UTF-16LE
        // xx 00 00 00  UTF-32LE
        //  ^ here
        m.mode = LEARNING_UTF8_OR_LE;
        return c;
    }

    else if( c == 0xef )
        return state_expecting_utf8_with_bom();

    else if( c == 0xfe )
        return state_expecting_utf16be_with_bom();

    else if( c == 0xff )
        return state_expecting_utf16le_or_utf32le_with_bom();

    // First char must be ASCII, so any other non-ASCII character is an error
    return in_error();
}

int ReadUTF8::state_learning_utf8_or_le()
{
    int c = m.r_reader.get();

    if( c == Reader::EOM )
        return in_error();

    if( c > 0 )
    {
        // xx xx -- --  UTF-8
        //     ^ here
        m.mode = UTF8;
        if( c <= 0x7f )
            return c;
        return state_utf8_reading_non_ascii( c );
    }

    else if( c == 0 )
    {
        // xx 00 xx --  UTF-16LE
        // xx 00 00 xx  UTF-16LE
        // xx 00 00 00  UTF-32LE
        //     ^ here
        CharPair pair = get_pair();

        if( pair.is_eom() )
            return in_error();

        if( pair.c1 > 0 || pair.c2 > 0 )
        {
            // xx 00 xx --  UTF-16LE
            // xx 00 00 xx  UTF-16LE
            //           ^ here
            m.mode = UTF16LE;
            return construct_utf8_from_utf16le( pair );
        }

        // c1 == 0 && c2 == 0
        // xx 00 00 00  UTF-32LE
        //           ^ here
        m.mode = UTF32LE;
        return state_utf32le();
    }

    assert( 0 );    // Shouldn't get here
    return in_error();
}

int ReadUTF8::state_expecting_utf8_with_bom()
{
    if( m.r_reader.get() != 0xBB || m.r_reader.get() != 0xBF )  // Next two bytes must be 0xBB 0xBF
        return in_error();

    m.mode = UTF8;

    int c = m.r_reader.get();

    if( c == Reader::EOM )
        return in_error();

    if( c > 0 && c <= 0x7f )
        return c;
    return state_utf8_reading_non_ascii( c );
}

int ReadUTF8::state_expecting_utf16le_or_utf32le_with_bom()
{
    // FF FE        -> UTF-16, little-endian
    // FF FE 00 00  -> UTF-32, little-endian

    if( m.r_reader.get() != 0xFE )
        return in_error();

    // FF FE 00 00  -> UTF-32, little-endian
    //     ^ here

    CharPair pair = get_pair();

    if( pair.is_eom() )
        return in_error();

    if( pair.c1 == 0 && pair.c2 == 0 )
    {
        m.mode = UTF32LE;
        return state_utf32le();
    }

    m.mode = UTF16LE;
    return construct_utf8_from_utf16le( pair );
}

int ReadUTF8::state_learning_utf16be_or_utf32be()
{
    // 00 xx -- --  UTF-16BE
    // 00 00 -- --  UTF-32BE
    // 00 00 FE FF  -> UTF-32, big-endian BOM
    //  ^ here

    int c = m.r_reader.get();

    if( c == Reader::EOM )
        return in_error();

    if( c > 0 )
    {
        m.mode = UTF16BE;
        return c;
    }

    return state_learning_utf32be_possibly_with_bom();
}

int ReadUTF8::state_expecting_utf16be_with_bom()
{
    // FE FF        -> UTF-16, big-endian

    int c = m.r_reader.get();

    if( c != 0xff )
        return in_error();

    m.mode = UTF16BE;

    return state_utf16be();
}

int ReadUTF8::state_learning_utf32be_possibly_with_bom()
{
    // 00 00 -- --  UTF-32BE
    // 00 00 FE FF  -> UTF-32, big-endian BOM
    //     ^ here

    m.mode = UTF32BE;

    CharPair pair = get_pair();

    if( pair.is_eom() )
        return in_error();

    if( pair.c1 == 0xfe )   // BOM marker
    {
        if( pair.c2 != 0xff )
            return in_error();

        return state_utf32be();
    }

    int code_point = pair.to_big_endian_code_point();
    if( is_high_surrogate( code_point ) || is_low_surrogate( code_point ) )
        return in_error();

    return construct_utf8( code_point );
}

int ReadUTF8::state_utf8_reading_non_ascii( int c )
{
    if( c == Reader::EOM )
        return in_error();

    m.utf8_buffer[0] = c;

    size_t n_to_read;

    // From rfc3629:
    // Char. number range  |        UTF-8 octet sequence
    //   (hexadecimal)    |              (binary)
    // --------------------+---------------------------------------------
    // 0000 0000-0000 007F | 0xxxxxxx
    // 0000 0080-0000 07FF | 110xxxxx 10xxxxxx
    // 0000 0800-0000 FFFF | 1110xxxx 10xxxxxx 10xxxxxx
    // 0001 0000-0010 FFFF | 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx

    if( c >= 0xf0 )
        n_to_read = 4 - 1;  // Already read one byte
    else if( c >= 0xe0 )
        n_to_read = 3 - 1;
    else if( c >= 0xc0 )
        n_to_read = 2 - 1;
    else
        return in_error();

    for( size_t i=0; i<n_to_read; ++i )
    {
        c = m.r_reader.get();
        if( c < 0x80 || c > 0xbf )
            return in_error();
        m.utf8_buffer[i+1] = c;
    }
    m.utf8_buffer[n_to_read+1] = '\0';

    int c1 = m.utf8_buffer[0];
    int c2 = m.utf8_buffer[1];

    if( c1 == 0xef && c2 == 0xbe && ( m.utf8_buffer[2] == 0xbe || m.utf8_buffer[2] == 0xbf ) ) // U+FFFE & U+FFFF are illegal
        return in_error();

    // From RFC 3629 - Make sure sequence is valid (e.g. doesn't encode a surrogate etc.)
    // UTF8-octets = *( UTF8-char )
    // UTF8-char   = UTF8-1 / UTF8-2 / UTF8-3 / UTF8-4
    // UTF8-1      = %x00-7F - (Handled outside this function)
    // UTF8-2      = %xC2-DF UTF8-tail
    // UTF8-3      = %xE0 %xA0-BF UTF8-tail / %xE1-EC 2( UTF8-tail ) /
    //               %xED %x80-9F UTF8-tail / %xEE-EF 2( UTF8-tail )
    // UTF8-4      = %xF0 %x90-BF 2( UTF8-tail ) / %xF1-F3 3( UTF8-tail ) /
    //               %xF4 %x80-8F 2( UTF8-tail )
    // UTF8-tail   = %x80-BF
    if( (c1 >= 0xc2 && c1 <= 0xdf) ||
            (c1 == 0xe0 && (c2 >= 0xa0 && c2 <= 0xbf)) ||
            (c1 >= 0xe1 && c1 <= 0xec) ||
            (c1 == 0xed && (c2 >= 0x80 && c2 <= 0x9f)) ||
            (c1 >= 0xee && c1 <= 0xef) ||
            (c1 == 0xf0 && (c2 >= 0x90 && c2 <= 0xbf)) ||
            (c1 >= 0xf1 && c1 <= 0xf3) ||
            (c1 == 0xf4 && (c2 >= 0x80 && c2 <= 0x8f)) )
    {
        // All OK
        m.p_utf8_buffer = &(m.utf8_buffer[1]);
        return m.utf8_buffer[0];
    }

    return in_error();
}

int ReadUTF8::state_utf16le()
{
    CharPair pair = get_pair();

    if( pair.is_eom() )
        return in_error();

    return construct_utf8_from_utf16le( pair );
}

int ReadUTF8::construct_utf8_from_utf16le( CharPair pair )
{
    int code_point = pair.to_little_endian_code_point();

    if( is_low_surrogate( code_point ) )
        return in_error();

    if( is_high_surrogate( code_point ) )
    {
        CharPair expected_low_surrogate_pair = get_pair();
        if( expected_low_surrogate_pair.is_eom() )
            return in_error();
        int expected_low_surrogate_code_point = expected_low_surrogate_pair.to_little_endian_code_point();
        if( ! is_low_surrogate( expected_low_surrogate_code_point ) )
            return in_error();
        code_point = code_point_from_surrogates( code_point, expected_low_surrogate_code_point );
    }
    return construct_utf8( code_point );
}

int ReadUTF8::state_utf16be()
{
    CharPair pair = get_pair();

    if( pair.is_eom() )
        return in_error();

    return construct_utf8_from_utf16be( pair );
}

int ReadUTF8::construct_utf8_from_utf16be( CharPair pair )
{
    int code_point = pair.to_big_endian_code_point();

    if( is_low_surrogate( code_point ) )
        return in_error();

    if( is_high_surrogate( code_point ) )
    {
        CharPair expected_low_surrogate_pair = get_pair();
        if( expected_low_surrogate_pair.is_eom() )
            return in_error();
        int expected_low_surrogate_code_point = expected_low_surrogate_pair.to_big_endian_code_point();
        if( ! is_low_surrogate( expected_low_surrogate_code_point ) )
            return in_error();
        code_point = code_point_from_surrogates( code_point, expected_low_surrogate_code_point );
    }
    return construct_utf8( code_point );
}

int ReadUTF8::state_utf32le()
{
    CharQuad quad = get_quad();

    if( quad.is_eom() )
        return in_error();

    int code_point = quad.to_little_endian_code_point();
    if( is_high_surrogate( code_point ) || is_low_surrogate( code_point ) )
        return in_error();

    return construct_utf8( code_point );
}

int ReadUTF8::state_utf32be()
{
    CharQuad quad = get_quad();

    if( quad.is_eom() )
        return in_error();

    int code_point = quad.to_big_endian_code_point();
    if( is_high_surrogate( code_point ) || is_low_surrogate( code_point ) )
        return in_error();

    return construct_utf8( code_point );
}

int ReadUTF8::construct_utf8( int code_point )
{
    UTF8Sequence( code_point ).copy_to_array( m.utf8_buffer );
    m.p_utf8_buffer = &(m.utf8_buffer[1]);
    return m.utf8_buffer[0];
}

void ReadUTF8::rewind()
{
    m.r_reader.rewind();
    m.mode = LEARNING;
    m.p_utf8_buffer = 0;
}

//----------------------------------------------------------------------------
//                           class ReadUTF8WithUnget
//----------------------------------------------------------------------------

int ReadUTF8WithUnget::get()
{
    if( ! m.unget_buffer.empty() )
    {
        int result = m.unget_buffer.top();
        m.unget_buffer.pop();
        return result;
    }

    return m.read_utf8.get();
}


int ReadUTF8WithUnget::get_non_ws()
{
    int c = get();
    while( isspace( c ) )
        c = get();
    return c;
}

void ReadUTF8WithUnget::unget( int c )
{
    m.unget_buffer.push( c );
}

void ReadUTF8WithUnget::rewind()
{
    return m.read_utf8.rewind();
}

//----------------------------------------------------------------------------
//                             class Event
//----------------------------------------------------------------------------

bool Event::is_int() const
{
    // Assumes that during parsing format has been validated as a number
    return is_number() && value.find_first_of( ".eE" ) == std::string::npos;
}

bool Event::to_bool() const
{
    switch( type )
    {
    case T_BOOLEAN:
        return value != "false";
    case T_STRING:
        return ! value.empty();
    case T_NUMBER:
        return to_float() != 0.0;
    case T_NULL:
    case T_OBJECT_START:
    case T_OBJECT_END:
    case T_ARRAY_START:
    case T_ARRAY_END:
    case T_UNKNOWN:
    default:    // In case we extend teh types later
        return false;
    }
}

double Event::to_float() const
{
    switch( type )
    {
    case T_NUMBER:
        return atof( value.c_str() );
    default:
        return to_bool() ? 1.0 : 0.0;
    }
}

int Event::to_int() const
{
    return static_cast<int>( to_float() );
}

long Event::to_long() const
{
    return static_cast<long>( to_float() );
}

//----------------------------------------------------------------------------
//                               class Parser
//----------------------------------------------------------------------------

Parser::ParserResult Parser::get( Event * p_event_out )
{
    if( m.last_result != PR_OK )
        return PR_UNABLE_TO_CONTINUE_DUE_TO_ERRORS;

    if( context() == C_DONE )
        return PR_END_OF_MESSAGE;   // "End of message" is not treated as an error.

    m.p_event_out =  p_event_out;
    m.p_event_out->clear();

    get_non_ws();

    if( m.c == Reader::EOM )
    {
        if( context() == C_OUTER )
            return (m.last_result = PR_END_OF_MESSAGE);
        return report_error( PR_UNEXPECTED_END_OF_MESSAGE );
    }

    switch( context() )
    {
    case C_OUTER:
        return get_outer();

    case C_START_OBJECT:
        return get_start_object();

    case C_IN_OBJECT:
        return get_in_object();

    case C_START_ARRAY:
        return get_start_array();

    case C_IN_ARRAY:
        return get_in_array();

    case C_DONE:
        assert( 0 );    // Handled above
    }

    assert( 0 );    // Shouldn't get here
    return report_error( PR_UNDOCUMENTED_FAIL );
}

void Parser::new_message()
{
    m.new_message();
}

Parser::ParserResult Parser::get_outer()
{
    // JSON-text = value

    m.context_stack.top() = Parser::C_DONE;

    Parser::ParserResult result = get_value();

    conditional_context_update_for_nesting_increase();

    if( result != PR_OK )
        return report_error( result );

    return PR_OK;
}

Parser::ParserResult Parser::get_start_object()
{
    if( m.c == '}' )
    {
        m.p_event_out->type = Event::T_OBJECT_END;
        return context_update_for_object();
    }

    else if( is_unexpected_array_close() )
        return unexpected_array_close_error();

    return get_for_object();
}

Parser::ParserResult Parser::get_in_object()
{
    if( m.c == '}' )
    {
        m.p_event_out->type = Event::T_OBJECT_END;
        return context_update_for_object();
    }

    else if( is_unexpected_array_close() )
        return unexpected_array_close_error();

    if( m.c != ',' )
        return report_error( PR_EXPECTED_COMMA_OR_END_OF_OBJECT );

    get_non_ws();

    if( is_unexpected_close() )
        return unexpected_close_error();

    return get_for_object();
}

Parser::ParserResult Parser::get_for_object()
{
    ParserResult get_member_result = get_member();

    ParserResult context_update_result = context_update_for_object();

    if( get_member_result != PR_OK )
        return get_member_result;

    return context_update_result;
}

Parser::ParserResult Parser::get_start_array()
{
    if( m.c == ']' )
    {
        m.p_event_out->type = Event::T_ARRAY_END;
        return context_update_for_array();
    }

    else if( is_unexpected_object_close() )
        return unexpected_object_close_error();

    return get_for_array();
}

Parser::ParserResult Parser::get_in_array()
{
    if( m.c == ']' )
    {
        m.p_event_out->type = Event::T_ARRAY_END;
        return context_update_for_array();
    }

    else if( is_unexpected_object_close() )
        return unexpected_object_close_error();

    if( m.c != ',' )
        return report_error( PR_EXPECTED_COMMA_OR_END_OF_ARRAY );

    get_non_ws();

    if( is_unexpected_close() )
        return unexpected_close_error();

    return get_for_array();
}

Parser::ParserResult Parser::get_for_array()
{
    ParserResult get_value_result = get_value();

    ParserResult context_update_result = context_update_for_array();

    if( get_value_result != PR_OK )
        return get_value_result;

    return context_update_result;
}

Parser::ParserResult Parser::get_member()
{
    // member = string name-separator value

    ParserResult result = get_name();

    if( result == PR_OK )
        result = skip_name_separator();

    if( result == PR_OK )
        result = get_value();

    return result;
}

Parser::ParserResult Parser::get_name()
{
    if( m.c != '"' )
        return report_error( PR_EXPECTED_MEMBER_NAME );

    Parser::ParserResult result = StringReader( m.input, m.c, &m.p_event_out->name );

    if( result != PR_OK )
        return report_error( result );

    return PR_OK;
}

Parser::ParserResult Parser::skip_name_separator()
{
    get_non_ws();

    if( m.c != ':' )
        return report_error( PR_EXPECTED_COLON_NAME_SEPARATOR );

    get_non_ws();

    if( m.c == Reader::EOM )
        return report_error( PR_UNEXPECTED_END_OF_MESSAGE );

    return PR_OK;
}

Parser::ParserResult Parser::get_value()
{
    // value = false / null / true / object (start) / array (start) / number / string

    if( m.c == '"' )
        return get_string();

    else if( m.c == 'f' )
        return get_false();

    else if( m.c == 't' )
        return get_true();

    else if( m.c == 'n' )
        return get_null();

    else if( is_number_start_char() )
        return get_number();

    else if( m.c == '{' )
    {
        m.p_event_out->type = Event::T_OBJECT_START;
        return PR_OK;
    }

    else if( m.c == '[' )
    {
        m.p_event_out->type = Event::T_ARRAY_START;
        return PR_OK;
    }

    else
    {
        // An unexpected character has been received - sort out a suitable error code

        if( is_invalid_json_number_start_char() )
        {
            m.p_event_out->type = Event::T_NUMBER;
            read_to_non_quoted_value_end();
            return report_error( PR_BAD_FORMAT_NUMBER );
        }

        else if( is_unexpected_close() )
            return unexpected_close_error();

        read_to_non_quoted_value_end();

        return report_error( PR_UNRECOGNISED_VALUE_FORMAT );
    }
}

Parser::ParserResult Parser::get_false()
{
    return get_constant_string( "false", Event::T_BOOLEAN, PR_BAD_FORMAT_FALSE );
}

Parser::ParserResult Parser::get_true()
{
    return get_constant_string( "true", Event::T_BOOLEAN, PR_BAD_FORMAT_TRUE );
}

Parser::ParserResult Parser::get_null()
{
    return get_constant_string( "null", Event::T_NULL, PR_BAD_FORMAT_NULL );
}

Parser::ParserResult Parser::get_constant_string(
                                        const char * const p_chars_start,
                                        Event::Type on_success_type,
                                        ParserResult on_error_code )
{
    read_to_non_quoted_value_end();

    if( m.p_event_out->value != p_chars_start )
        return report_error( on_error_code );

    m.p_event_out->type = on_success_type;
    return PR_OK;
}

bool Parser::is_number_start_char()
{
    // number = [ minus ] int [ frac ] [ exp ]
    // int = zero / ( digit1-9 *DIGIT )

    return m.c == '-' || (m.c >= '0' && m.c <= '9');
}

bool Parser::is_invalid_json_number_start_char()
{
    // JSON numbers can't start with + and .

    return m.c == '+' || m.c == '.';
}

Parser::ParserResult Parser::get_number()
{
    Parser::ParserResult result = NumberReader( m.input, m.c, m.p_event_out );

    if( result != PR_OK )
        return report_error( result );

    return PR_OK;
}

Parser::ParserResult Parser::get_string()
{
    m.p_event_out->type = Event::T_STRING;

    Parser::ParserResult result = StringReader( m.input, m.c, &m.p_event_out->value );

    if( result != PR_OK )
        return report_error( result );

    return PR_OK;
}

void Parser::read_to_non_quoted_value_end()
{
    m.p_event_out->value += m.c;
    while( get() )
    {
        if( is_separator() )
            break;
        m.p_event_out->value += m.c;
    }
    unget();
}

bool Parser::is_separator()
{
    return cljp::is_separator( m.c );
}

bool Parser::is_unexpected_object_close()
{
    return m.c == '}';
}

Parser::ParserResult Parser::unexpected_object_close_error()
{
    return report_error( PR_UNEXPECTED_OBJECT_CLOSE );
}

bool Parser::is_unexpected_array_close()
{
    return m.c == ']';
}

Parser::ParserResult Parser::unexpected_array_close_error()
{
    return report_error( PR_UNEXPECTED_ARRAY_CLOSE );
}

bool Parser::is_unexpected_close()
{
    return m.c == '}' || m.c == ']';
}

Parser::ParserResult Parser::unexpected_close_error()
{
    if( m.c == '}' )
        return report_error( PR_UNEXPECTED_OBJECT_CLOSE );
    else if( m.c == ']' )
        return report_error( PR_UNEXPECTED_ARRAY_CLOSE );

    assert( 0 );    // Shouldn't get here
    return PR_OK;
}

Parser::ParserResult Parser::context_update_for_object()
{
    if( m.p_event_out->type == Event::T_OBJECT_END )
        m.context_stack.pop();
    else if( m.p_event_out->type == Event::T_ARRAY_END )
        return report_error( PR_UNEXPECTED_ARRAY_CLOSE );
    else
        m.context_stack.top() = C_IN_OBJECT;

    conditional_context_update_for_nesting_increase();

    return PR_OK;
}

Parser::ParserResult Parser::context_update_for_array()
{
    if( m.p_event_out->type == Event::T_ARRAY_END )
        m.context_stack.pop();
    else if( m.p_event_out->type == Event::T_OBJECT_END )
        return report_error( PR_UNEXPECTED_OBJECT_CLOSE );
    else
        m.context_stack.top() = C_IN_ARRAY;

    conditional_context_update_for_nesting_increase();

    return PR_OK;
}

void Parser::conditional_context_update_for_nesting_increase()
{
    if( m.p_event_out->type == Event::T_ARRAY_START )
        m.context_stack.push( C_START_ARRAY );
    else if( m.p_event_out->type == Event::T_OBJECT_START )
        m.context_stack.push( C_START_OBJECT );
}

Parser::ParserResult Parser::report_error( ParserResult error )
{
    m.last_result = error;

    #if CLJP_THROW_ERRORS == 1
        throw( ParserException( error ) );
    #endif;
    return error;
}

//----------------------------------------------------------------------------
//                               class SmartParser
//----------------------------------------------------------------------------

}   // End of namespace cljp
