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

#ifndef CL_JSON_PULL_H
#define CL_JSON_PULL_H

#include "cl-json-pull-config.h"

#include <string>
#include <vector>
#include <cstdio>
#include <stack>
#include <cassert>

namespace cljp {    // Codalogic JSON Pull (Parser)

//----------------------------------------------------------------------------
//                             class Reader
//----------------------------------------------------------------------------

class Reader
{
public:
    static const int EOM /*= -1*/;  // End of message

    virtual ~Reader() {}

    virtual void close_on_destruct( bool is_close_on_destruct_required ) {}

    int get() { return do_get(); }  // Returns EOM when no more input
    void rewind() { return do_rewind(); }

private:
    virtual int do_get() = 0;
    virtual void do_rewind() = 0;
};

//----------------------------------------------------------------------------
//                             class ReaderMemory
//----------------------------------------------------------------------------

class ReaderMemory : public Reader
{
private:
    struct Members {
        const char * p_start;
        const char * p_now;
        const char * p_end; // Half closed end (i.e. one past last char in string)

        Members( const char * p_start_in, const char * p_end_in )
            : p_start( p_start_in ), p_now( p_start_in ), p_end( p_end_in )
        {}
    } m;

public:
    ReaderMemory( const char * p_start_in, const char * p_end_in );

private:
    virtual int do_get();
    virtual void do_rewind();
};

//----------------------------------------------------------------------------
//                             class ReaderString
//----------------------------------------------------------------------------

class ReaderString : public ReaderMemory
{
public:
    ReaderString( const std::string & r_in )
        : ReaderMemory( r_in.c_str(), r_in.c_str() + r_in.size() )
    {}
};

//----------------------------------------------------------------------------
//                             class ReaderFile
//----------------------------------------------------------------------------

class ReaderFile : public Reader
{
private:
    struct Members {
        FILE * h_fin;
        bool is_close_on_destruct_required;

        Members( FILE * h_fin_in )
            : h_fin( h_fin_in ), is_close_on_destruct_required( true )
        {}
    } m;

public:
    ReaderFile( const char * p_file_name_in );
    ReaderFile( FILE * h_fin_in );
    ~ReaderFile();

    bool is_open() const { return m.h_fin != 0; }

private:
    virtual int do_get();
    virtual void do_rewind();
    virtual void close_on_destruct( bool is_close_on_destruct_required );
};

//----------------------------------------------------------------------------
//                               class ReadUTF8
//----------------------------------------------------------------------------

class ReadUTF8
{
public:
    enum Modes { LEARNING, LEARNING_UTF8_OR_LE, UTF8, UTF16LE, UTF16BE, UTF32LE, UTF32BE, ERRORED };
    typedef int utf8_buffer_t[6+1];

private:
    struct Members {
        Reader & r_reader;
        Modes mode;
        utf8_buffer_t utf8_buffer;
        int * p_utf8_buffer;    // NULL or pointing to \0 indicates no utf-8 chars stored

        Members( Reader & r_reader_in )
            :
            r_reader( r_reader_in ),
            mode( LEARNING ),
            p_utf8_buffer( 0 )
        {}
    } m;

public:
    ReadUTF8( Reader & r_reader_in )
        : m( r_reader_in )
    {}

    Modes mode() const { return m.mode; }

    int get();

    void rewind();

    Reader & reader() const { return m.r_reader; }

private:
    struct CharPair
    {
        int c1; int c2;
        bool is_eom() const { return c1 == cljp::Reader::EOM || c2 == cljp::Reader::EOM; }
        int to_little_endian_code_point() const { return c1 + c2 * 256; }
        int to_big_endian_code_point() const { return c1 * 256 + c2; }
    };
    CharPair get_pair();
    struct CharQuad
    {
        int c1; int c2; int c3; int c4;
        bool is_eom() const
        {
            return c1 == cljp::Reader::EOM || c2 == cljp::Reader::EOM ||
                    c3 == cljp::Reader::EOM || c4 == cljp::Reader::EOM;
        }
        int to_little_endian_code_point() const { return c1 + 256 * (c2 + 256 * (c3 + 256 * c4)); }
        int to_big_endian_code_point() const { return ((c1 * 256 + c2) * 256 + c3) * 256 + c4; }
    };
    CharQuad get_quad();

    int state_learning();
    int state_learning_utf8_or_le();
    int state_expecting_utf8_with_bom();
    int state_expecting_utf16le_or_utf32le_with_bom();
    int state_learning_utf16be_or_utf32be();
    int state_expecting_utf16be_with_bom();
    int state_learning_utf32be_possibly_with_bom();
    int state_utf8_reading_non_ascii( int c );
    int state_utf16le();
    int construct_utf8_from_utf16le( CharPair pair );
    int state_utf16be();
    int construct_utf8_from_utf16be( CharPair pair );
    int state_utf32le();
    int state_utf32be();

    int construct_utf8( int code_point );

    int in_error() { m.mode = ERRORED; return cljp::Reader::EOM; }
};

//----------------------------------------------------------------------------
//                           class UngetBuffer
//----------------------------------------------------------------------------

template< typename Tchar >
class UngetBuffer
{
private:
    struct Members {
        enum { max_size = 10 };
        Tchar buffer[max_size];
        size_t size;

        Members() : size(0) {}
    } m;

public:
    UngetBuffer() {}
    void clear() { m.size = 0; }
    bool empty() const { return m.size == 0; }
    size_t size() const { return m.size; }
    Tchar top() const { assert( m.size > 0 ); return m.buffer[m.size-1]; }
    void push( Tchar c ) { assert( m.size < m.max_size ); m.buffer[m.size++] = c; }
    void pop() { assert( m.size > 0 ); --m.size; }
};

//----------------------------------------------------------------------------
//                           class ReadUTF8WithUnget
//----------------------------------------------------------------------------

class ReadUTF8WithUnget
{
private:
    struct Members {
        UngetBuffer< char > unget_buffer;
        ReadUTF8 read_utf8;

        Members( Reader & reader_in )
            : read_utf8( reader_in )
        {}
    } m;

public:
    ReadUTF8WithUnget( Reader & reader_in )
            : m( reader_in )
        {}

    int get();
    int get_non_ws();

    void unget( int c );

    void rewind();

    Reader & reader() const { return m.read_utf8.reader(); }
};

//----------------------------------------------------------------------------
//                             class Event
//----------------------------------------------------------------------------

struct Event
{
    enum Type { T_UNKNOWN, T_STRING, T_NUMBER, T_BOOLEAN, T_NULL,
            T_OBJECT_START, T_OBJECT_END, T_ARRAY_START, T_ARRAY_END };

    std::string name;
    std::string value;
    Type type;

    Event() : type( T_UNKNOWN ) {}
    // Event( const Event & ) = default;
    // Event & operator = ( const Event & ) = default;

    void clear() { name.clear(); value.clear(); type = T_UNKNOWN; }

    // Convenience methods
    bool is_unknown() const { return type == T_UNKNOWN; }   // For completeness
    bool is_string() const { return type == T_STRING; }
    bool is_number() const { return type == T_NUMBER; }
    bool is_boolean() const { return type == T_BOOLEAN; }
    bool is_bool() const { return is_boolean(); }           // For convenience
    bool is_null() const { return type == T_NULL; }
    bool is_object_start() const { return type == T_OBJECT_START; }
    bool is_object_end() const { return type == T_OBJECT_END; }
    bool is_array_start() const { return type == T_ARRAY_START; }
    bool is_array_end() const { return type == T_ARRAY_END; }

    // is_true() and is_false() don't do implicit type casting
    bool is_true() const { return type == T_BOOLEAN && value == "true"; }
    bool is_false() const { return type == T_BOOLEAN && value == "false"; }
    bool is_int() const;
    bool is_float() const { return is_number(); }           // For convenience

    bool to_bool() const;
    double to_float() const;
    int to_int() const;
    long to_long() const;
    const std::string & to_string() const { return value; } // For completeness
    std::wstring to_wstring() const;                        // For convenience
};

//----------------------------------------------------------------------------
//                               class Parser
//----------------------------------------------------------------------------

class Parser
{
public:
    enum ParserResult {
            PR_OK,
            PR_END_OF_MESSAGE,
            PR_UNABLE_TO_CONTINUE_DUE_TO_ERRORS,
            PR_UNEXPECTED_END_OF_MESSAGE,
            PR_EXPECTED_COLON_NAME_SEPARATOR,
            PR_UNEXPECTED_OBJECT_CLOSE,
            PR_UNEXPECTED_ARRAY_CLOSE,
            PR_EXPECTED_COMMA_OR_END_OF_ARRAY,
            PR_EXPECTED_COMMA_OR_END_OF_OBJECT,
            PR_UNRECOGNISED_VALUE_FORMAT,
            PR_BAD_FORMAT_STRING,
            PR_BAD_FORMAT_FALSE,
            PR_BAD_FORMAT_TRUE,
            PR_BAD_FORMAT_NULL,
            PR_BAD_FORMAT_NUMBER,
            PR_BAD_UNICODE_ESCAPE,
            PR_EXPECTED_MEMBER_NAME,
            PR_UNDOCUMENTED_FAIL = 100
            };

private:
    enum Context {
            C_OUTER, C_DONE, C_START_OBJECT, C_IN_OBJECT, C_START_ARRAY, C_IN_ARRAY };

    struct Members {
        ReadUTF8WithUnget input;
        typedef std::stack< Context > context_stack_t;
        context_stack_t context_stack;
        int c;
        Event * p_event_out;
        ParserResult last_result;

        Members( Reader & reader_in )
            : input( reader_in )
        {
            new_message();
        }
        void new_message()
        {
            context_stack = context_stack_t();
            context_stack.push( C_OUTER );
            c = ' ';
            p_event_out = 0;
            last_result = PR_OK;
        }
    } m;

public:
    Parser( Reader & reader_in )
        : m( reader_in )
    {}

    ParserResult get( Event * p_event_out );
    void new_message();

private:
    int get() { m.c = m.input.get(); return m.c; }
    int get_non_ws() { m.c = m.input.get_non_ws(); return m.c; }
    int c() { return m.c; }
    void unget( int c ) { m.input.unget( c ); }
    void unget() { m.input.unget( m.c ); }
    Context context() const { return m.context_stack.top(); }
    ParserResult get_outer();
    ParserResult get_start_object();
    ParserResult get_in_object();
    ParserResult get_for_object();
    ParserResult get_start_array();
    ParserResult get_in_array();
    ParserResult get_for_array();
    ParserResult get_member();
    ParserResult get_name();
    ParserResult skip_name_separator();
    ParserResult get_value();
    ParserResult get_false();
    ParserResult get_true();
    ParserResult get_null();
    ParserResult get_constant_string(
                            const char * const p_chars_start,
                            Event::Type on_success_type,
                            ParserResult on_error_code );
    bool is_number_start_char();
    bool is_invalid_json_number_start_char();
    ParserResult get_number();
    ParserResult get_string();
    void read_to_non_quoted_value_end();
    bool is_separator();
    bool is_unexpected_object_close();
    ParserResult unexpected_object_close_error();
    bool is_unexpected_array_close();
    ParserResult unexpected_array_close_error();
    bool is_unexpected_close();
    ParserResult unexpected_close_error();
    ParserResult context_update_for_object();
    ParserResult context_update_for_array();
    void conditional_context_update_for_nesting_increase();

    ParserResult report_error( ParserResult error );
};

//----------------------------------------------------------------------------
//                           class ParserException
//----------------------------------------------------------------------------

class ParserException : public std::exception
{
private:
    struct Members {
        Parser::ParserResult error;

        Members( Parser::ParserResult error_in ) : error( error_in ) {}
    } m;

public:
    ParserException( Parser::ParserResult error_in )
        : m( error_in )
    {}
    Parser::ParserResult error() const { return m.error; }
    const char * what() const throw()
    {
        return "cljp::ParserException";
    }
};

}   // End of namespace cljp

#endif  // CL_JSON_PULL_H
