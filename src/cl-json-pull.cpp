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

#define CLJP_PARSER_PRIVATE \
    int get() { m.c = m.input.get(); return m.c; } \
    int get_non_ws() { m.c = m.input.get_non_ws(); return m.c; } \
    int c() { return m.c; } \
    void unget( int c ) { m.input.unget( c ); } \
    void unget() { m.input.unget( m.c ); } \
    Context context() const { return m.context_stack.top(); } \
    ParserResult get_outer(); \
    ParserResult get_start_object(); \
    ParserResult get_in_object(); \
    ParserResult get_for_object(); \
    ParserResult get_start_array(); \
    ParserResult get_in_array(); \
    ParserResult get_for_array(); \
    ParserResult get_member(); \
    ParserResult get_value(); \
    ParserResult get_false(); \
    ParserResult get_true(); \
    ParserResult get_null(); \
    ParserResult get_constant_string( \
                            const char * const p_chars_start, \
                            Event::Type on_success_type, \
                            ParserResult on_error_code ); \
    bool is_number_start_char(); \
    ParserResult get_number(); \
    ParserResult get_string(); \
    void read_to_non_quoted_value_end(); \
    bool is_separator(); \
    ParserResult context_update_for_object(); \
    ParserResult context_update_for_array(); \
    void context_update_if_nesting(); \


#include "cl-json-pull.h"

#include <cstdio>
#include <cassert>

namespace cljp {    // Codalogic JSON Pull Parser

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
//                             class UTFConverter
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//                               class ReadUTF8
//----------------------------------------------------------------------------

int ReadUTF8::get()
{
    return m.r_reader.get();
}

void ReadUTF8::rewind()
{
    return m.r_reader.rewind();
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


int ReadUTF8WithUnget::get_non_ws()
{
    int c = get();
    while( isspace( c ) )
        c = get();
    return c;
}

void ReadUTF8WithUnget::unget( int c )
{
    m.unget_buffer.push_back( c );
}

void ReadUTF8WithUnget::rewind()
{
    return m.read_utf8.rewind();
}

//----------------------------------------------------------------------------
//               String and Unicode reading functions and classes
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

class UTF8Sequence
{
private:
    char utf8[6];

public:
    UTF8Sequence( int codepoint )
    {
        // From rfc3629:
        // Char. number range  |        UTF-8 octet sequence
        //   (hexadecimal)    |              (binary)
        // --------------------+---------------------------------------------
        // 0000 0000-0000 007F | 0xxxxxxx
        // 0000 0080-0000 07FF | 110xxxxx 10xxxxxx
        // 0000 0800-0000 FFFF | 1110xxxx 10xxxxxx 10xxxxxx
        // 0001 0000-0010 FFFF | 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx

        if( codepoint < 0x7f )
            pack_ascii( codepoint );
        else if( codepoint > 0x10000 )
            pack( '\xf0', 4, codepoint );
        else if( codepoint > 0x080 )
            pack( '\xe0', 3, codepoint );
        else if( codepoint > 0x080 )
            pack( '\xc0', 2, codepoint );
        else
            utf8[0] = '\0';
    };
    operator const char * () const { return utf8; }
    char operator [] ( size_t index ) const { return utf8[index]; }
    char & operator [] ( size_t index ) { return utf8[index]; }

private:
    void pack_ascii( int codepoint )
    {
        utf8[0] = codepoint;
        utf8[1] = '\0';
    }

    void pack( char marker, size_t length, int codepoint )
    {
        utf8[length] = '\0';
        int scaled_codepoint = codepoint;
        for( int i = length-1; i >= 0; --i )
        {
            utf8[i] = scaled_codepoint & 0x3f;
            utf8[i] |= 0x80;
            scaled_codepoint >>= 6;
        }
        utf8[0] |= marker;
    }
};

class UnicodeCodepointReader
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
    UnicodeCodepointReader( ReadUTF8WithUnget & r_input_in )
        : m( r_input_in )
    {
        //           %x75 4HEXDIG )  ; uXXXX                U+XXXX
        // We have read "\u" already.  We may have a surrogate
        // -which requires reading 1234\u5678 and converting to UTF-8

        if( read_code_point_code() )
        {
            if( is_low_surrogate() )
                report_bad_unicode_escape();
            else if( is_high_surrogate() )
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

    bool is_high_surrogate()
    {
        return m.code_point >= 0xD800 && m.code_point <= 0xDBFF;
    }

    bool is_low_surrogate()
    {
        return m.code_point >= 0xDC00 && m.code_point <= 0xDFFF;
    }

    void combine_low_surrogate()
    {
        int high_surrogate = m.code_point;

        if( get() == '\\' && get() == 'u' && read_code_point_code() )
        {
            if( is_low_surrogate() )
                make_code_point( high_surrogate, m.code_point );
            else
                report_bad_unicode_escape();
        }
        else
        {
            report_bad_unicode_escape();
        }
    }

    void make_code_point( int high_surrogate, int low_surrogate )
    {
        m.code_point = ((high_surrogate & 0x3ff) << 10) + (low_surrogate & 0x3ff) + 0x10000;
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
        Event * p_event;
        Parser::ParserResult result;

        Members( ReadUTF8WithUnget & r_input_in, int c_in, Event * p_event_out )
            : r_input( r_input_in ), c( c_in ), p_event( p_event_out ),
                result( Parser::PR_OK )
        {}
    } m;

public:
    StringReader( ReadUTF8WithUnget & r_input_in, int c_in, Event * p_event_out )
        : m( r_input_in, c_in, p_event_out )
    {
        // string = quotation-mark *char quotation-mark

        m.p_event->type = Event::T_STRING;

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
        m.p_event->value += m.c;
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
            m.p_event->value += mapped_char_in;
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

        UnicodeCodepointReader codepoint_reader( m.r_input );

        // Allow for a successful operation, without overwriting the error code of a previous unsuccessful operation
        Parser::ParserResult current_result = codepoint_reader.result();
        get();

        record_first_error( current_result );

        if( current_result != Parser::PR_OK )
            return false;

        m.p_event->value += codepoint_reader.as_utf8();
        return true;
    }

    void record_first_error( Parser::ParserResult current_result )
    {
        if( m.result == Parser::PR_OK )
            m.result = current_result;
    }
};

//----------------------------------------------------------------------------
//                               class Parser
//----------------------------------------------------------------------------

Parser::ParserResult Parser::get( Event * p_event_out )
{
    m.p_event_out =  p_event_out;
    m.p_event_out->clear();

    get_non_ws();

    if( m.c == Reader::EOM )
    {
        if( context() == C_DONE )
            return PR_END_OF_MESSAGE;
        return PR_UNEXPECTED_END_OF_MESSAGE;
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
        return report_error( PR_READ_PAST_END_OF_MESSAGE );
    }

    assert( 0 );    // Shouldn't get here
    return report_error( PR_UNDOCUMENTED_FAIL );
}

Parser::ParserResult Parser::get_outer()
{
    // JSON-text = object / array

    m.context_stack.top() = Parser::C_DONE;

    if( m.c == '{' )
    {
        m.p_event_out->type = Event::T_OBJECT_START;
        m.context_stack.push( C_START_OBJECT );
        return PR_OK;
    }

    else if( m.c == '[' )
    {
        m.p_event_out->type = Event::T_ARRAY_START;
        m.context_stack.push( C_START_ARRAY );
        return PR_OK;
    }

    return report_error( PR_EXPECTED_OBJECT_OR_ARRAY );
}

Parser::ParserResult Parser::get_start_object()
{
    if( m.c == '}' )
    {
        m.p_event_out->type = Event::T_OBJECT_END;
        return context_update_for_object();
    }

    return get_for_object();
}

Parser::ParserResult Parser::get_in_object()
{
    if( m.c == '}' )
    {
        m.p_event_out->type = Event::T_OBJECT_END;
        return context_update_for_object();
    }

    if( m.c != ',' )
        return report_error( PR_EXPECTED_COMMA_OR_END_OF_ARRAY );

    get_non_ws();

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

    return get_for_array();
}

Parser::ParserResult Parser::get_in_array()
{
    if( m.c == ']' )
    {
        m.p_event_out->type = Event::T_ARRAY_END;
        return context_update_for_array();
    }

    if( m.c != ',' )
        return report_error( PR_EXPECTED_COMMA_OR_END_OF_ARRAY );

    get_non_ws();

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

    if( m.c == '}' )
    {
        m.p_event_out->type = Event::T_OBJECT_END;
        return PR_OK;
    }

    else if( m.c == ']' )
    {
        m.p_event_out->type = Event::T_ARRAY_END;
        return PR_OK;
    }

    else    // TODO: other name-value pairs
    {
        return report_error( PR_UNDOCUMENTED_FAIL );
    }

    return report_error( PR_UNDOCUMENTED_FAIL );
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

    else if( m.c == '}' )
        return PR_UNEXPECTED_OBJECT_CLOSE;

    else if( m.c == ']' )
        return PR_UNEXPECTED_ARRAY_CLOSE;

    read_to_non_quoted_value_end();

    return report_error( PR_UNRECOGNISED_VALUE_FORMAT );
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

Parser::ParserResult Parser::get_number()
{
    Parser::ParserResult result = NumberReader( m.input, m.c, m.p_event_out );

    if( result != PR_OK )
        return report_error( result );

    return PR_OK;
}

Parser::ParserResult Parser::get_string()
{
    Parser::ParserResult result = StringReader( m.input, m.c, m.p_event_out );

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

Parser::ParserResult Parser::context_update_for_object()
{
    if( m.p_event_out->type == Event::T_OBJECT_END )
        m.context_stack.pop();
    else if( m.p_event_out->type == Event::T_ARRAY_END )
        return report_error( PR_UNEXPECTED_ARRAY_CLOSE );
    else
        m.context_stack.top() = C_IN_OBJECT;

    context_update_if_nesting();

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

    context_update_if_nesting();

    return PR_OK;
}

void Parser::context_update_if_nesting()
{
    if( m.p_event_out->type == Event::T_ARRAY_START )
        m.context_stack.push( C_START_ARRAY );
    else if( m.p_event_out->type == Event::T_OBJECT_START )
        m.context_stack.push( C_START_OBJECT );
}

//----------------------------------------------------------------------------
//                               class SmartParser
//----------------------------------------------------------------------------

}   // End of namespace cljp
