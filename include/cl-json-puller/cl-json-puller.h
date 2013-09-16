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

#ifndef CL_JSON_PULLER_H
#define CL_JSON_PULLER_H

#include "cl-json-puller-config.h"

#include <string>
#include <vector>
#include <cstdio>
#include <stack>

#include "../test/sdd.h"    // Temporary inclusion while being designed

namespace cljp {    // Codalogic JSON Puller

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
//                             class UTFConverter
//----------------------------------------------------------------------------

class UTFConverter
{
public:
    SDD_CLASS( "Utility class providing conversion from non-UTF8 to UTF8" )

    typedef char utf8_buffer_t[6+1];
    typedef char utf16_buffer_t[2+1];
    typedef char utf32_buffer_t[4+1];

    enum ConversionResult { CR_OK, CR_LOW_SURROGATE, CR_HIGH_SURROGATE, CR_FAIL };

    SDD_METHOD( from_utf16le, "( char * utf8_out, int utf16le_in ) -> ConversionResult" )
    //SDD_METHOD( from_utf16le, "( char * utf8_out, Reader & reader_in ) -> ConversionResult" )
};

//----------------------------------------------------------------------------
//                               class ReadUTF8
//----------------------------------------------------------------------------

class ReadUTF8
{
private:
    struct Members {
        Reader & r_reader;

        Members( Reader & r_reader_in )
            : r_reader( r_reader_in )
        {}
    } m;

public:
    SDD_CLASS( "Converts any Reader input into UTF8" )

    ReadUTF8( Reader & r_reader_in )
        : m( r_reader_in )
    {}

    int get();

    void rewind();
};

//----------------------------------------------------------------------------
//                           class ReadUTF8WithUnget
//----------------------------------------------------------------------------

class ReadUTF8WithUnget
{
private:
    struct Members {
        std::vector< char > unget_buffer;
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
};

//----------------------------------------------------------------------------
//                             class Event
//----------------------------------------------------------------------------

class Event
{
public:
    enum Type { T_UNKNOWN, T_STRING, T_NUMBER, T_OBJECT_START, T_OBJECT_END,
            T_ARRAY_START, T_ARRAY_END, T_BOOLEAN, T_NULL };

private:
    struct Members {
        std::string name;
        std::string value;
        Type type;

        Members() : type( T_UNKNOWN ) {}
    } m;

public:
    // Event() = default;
    // Event( const Event & ) = default;
    // Event & operator = ( const Event & ) = default;

    const std::string & name() const { return m.name; }
    void name( const std::string & r_name_in ) { m.name = r_name_in; }

    const std::string & value() const { return m.value; }
    void value( const std::string & r_value_in ) { m.value = r_value_in; }
    void value( const char * p_value_in ) { m.value = p_value_in; }
    void value_append( int c ) { m.value += c; }

    Type type() const { return m.type; }
    void type( Type type_in ) { m.type = type_in; }

    void clear() { m.name.clear(); m.value.clear(); m.type = T_UNKNOWN; }   // Don't do m = Members(); because we want to preserve any memory allocated by the strings
};

//----------------------------------------------------------------------------
//                               class Parser
//----------------------------------------------------------------------------

class Parser
{
private:
    enum Context {
			C_OUTER, C_DONE, C_START_OBJECT, C_IN_OBJECT, C_START_ARRAY, C_IN_ARRAY };

    struct Members {
        ReadUTF8WithUnget input;
        int c;
        std::stack< Context > context_stack;
        Event * p_event_out;

        Members( Reader & reader_in )
            : input( reader_in ), c( ' ' ), p_event_out( 0 )
        {
			context_stack.push( C_OUTER );
        }
    } m;

public:
    enum ParserResult {
			PR_OK,
			PR_UNDOCUMENTED_FAIL,
			PR_END_OF_MESSAGE,
			PR_READ_PAST_END_OF_MESSAGE,
			PR_EXPECTED_OBJECT_OR_ARRAY,
			PR_UNEXPECTED_OBJECT_CLOSE,
			PR_UNEXPECTED_ARRAY_CLOSE,
			PR_EXPECTED_COMMA_OR_END_OF_ARRAY,
			PR_UNRECOGNISED_VALUE_FORMAT,
			PR_BAD_FORMAT_STRING,
			PR_BAD_FORMAT_FALSE,
			PR_BAD_FORMAT_TRUE,
			PR_BAD_FORMAT_NULL,
			PR_BAD_FORMAT_NUMBER
			};

    Parser( Reader & reader_in )
        : m( reader_in )
    {}

    ParserResult get( Event * p_event_out );

private:
	#ifndef CLJP_PARSER_PRIVATE
	#define CLJP_PARSER_PRIVATE
	#endif
	CLJP_PARSER_PRIVATE

	ParserResult report_error( ParserResult error )
	{
		#if CLJP_THROW_ERRORS == 1
			throw( error );
		#endif;
		return error;
	}
};

//----------------------------------------------------------------------------
//                           class SmartParser
//----------------------------------------------------------------------------

class SmartParser : public Parser
{
public:
    SDD_CLASS( "Adds context to the basic parser to keep track of whether reading name/value pairs or just values" )
    SDD_EXTENDS( Parser )

    SDD_METHOD( get, "Reads either a name/value pair or just a value dependin on context" )
};

}   // End of namespace cljp

#endif  // CL_JSON_PULLER_H
