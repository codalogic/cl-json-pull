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
    ParserResult get_string(); \
    ParserResult get_false(); \
    ParserResult get_true(); \
    ParserResult get_null(); \
    ParserResult get_constant_string( \
                            const char * const p_chars_start, \
                            Event::Type on_success_type, \
                            ParserResult on_error_code ); \
    bool is_number_start_char(); \
    ParserResult get_number(); \
    void read_to_non_quoted_value_end(); \
    bool is_separator(); \
    ParserResult context_update_for_object(); \
    ParserResult context_update_for_array(); \
    void context_update_if_nesting(); \


#include "cl-json-pull.h"

#include <cstdio>
#include <cassert>

namespace cljpp {    // Codalogic JSON Pull Parser

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
//                               class Parser
//----------------------------------------------------------------------------

Parser::ParserResult Parser::get( Event * p_event_out )
{
    m.p_event_out =  p_event_out;
    m.p_event_out->clear();

    get_non_ws();

    if( m.c == Reader::EOM )
        return PR_END_OF_MESSAGE;

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
    ParserResult result = get_member();
    if( result != PR_OK )
        return result;

    return context_update_for_object();
}

Parser::ParserResult Parser::get_start_array()
{
    if( m.c == ']' )
    {
        m.p_event_out->type = Event::T_ARRAY_END;
        return context_update_for_array();
    }

    ParserResult result = get_value();
    if( result != PR_OK )
        return result;

    return context_update_for_array();
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
    ParserResult result = get_value();
    if( result != PR_OK )
        return result;

    return context_update_for_array();
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

Parser::ParserResult Parser::get_string()
{
    return report_error( PR_BAD_FORMAT_STRING );
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

Parser::ParserResult Parser::get_number()
{
    return report_error( PR_BAD_FORMAT_NUMBER );
}

void Parser::read_to_non_quoted_value_end()
{
    m.p_event_out->value += m.c;
    while( get(), ! is_separator() )
        m.p_event_out->value += m.c;
    unget();
}

bool Parser::is_separator()
{
    return isspace( m.c ) || m.c == ',' || m.c == ']' || m.c == '}' || m.c == Reader::EOM;
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
