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

#include <string>

struct Harness
{
    std::string json;
    cljp::ReaderString reader;
    cljp::Parser parser;
    cljp::Event event;

    Harness( const std::string & r_json_in )
        :
        json( r_json_in ),
        reader( json ),
        parser( reader )
    {}
};

TFEATURE( "Basic Parser" )
{
    TDOC( "Testing outer parsing handling" );

    {
    Harness h( "{" );

    TTEST( h.parser.get( &h.event ) == cljp::Parser::PR_OK );
    TTEST( h.event.type == cljp::Event::T_OBJECT_START );
    }

    {
    Harness h( " [" );

    TTEST( h.parser.get( &h.event ) == cljp::Parser::PR_OK );
    TTEST( h.event.type == cljp::Event::T_ARRAY_START );
    }

    {
    Harness h( "[ ]" );

    TTEST( h.parser.get( &h.event ) == cljp::Parser::PR_OK );
    TTEST( h.event.type == cljp::Event::T_ARRAY_START );

    TTEST( h.parser.get( &h.event ) == cljp::Parser::PR_OK );
    TTEST( h.event.type == cljp::Event::T_ARRAY_END );
    }

    {
    Harness h( "[ ]" );

    TTEST( h.parser.get( &h.event ) == cljp::Parser::PR_OK );
    TTEST( h.event.type == cljp::Event::T_ARRAY_START );

    TTEST( h.parser.get( &h.event ) == cljp::Parser::PR_OK );
    TTEST( h.event.type == cljp::Event::T_ARRAY_END );

    TTEST( h.parser.get( &h.event ) == cljp::Parser::PR_END_OF_MESSAGE );
    }

    {
    Harness h( "[ ]]" );    // Error case

    TTEST( h.parser.get( &h.event ) == cljp::Parser::PR_OK );
    TTEST( h.event.type == cljp::Event::T_ARRAY_START );

    TTEST( h.parser.get( &h.event ) == cljp::Parser::PR_OK );
    TTEST( h.event.type == cljp::Event::T_ARRAY_END );

    TTEST( h.parser.get( &h.event ) == cljp::Parser::PR_READ_PAST_END_OF_MESSAGE );
    }

    {
    Harness h( "[ }" ); // Error case

    TTEST( h.parser.get( &h.event ) == cljp::Parser::PR_OK );
    TTEST( h.event.type == cljp::Event::T_ARRAY_START );

    TTEST( h.parser.get( &h.event ) == cljp::Parser::PR_UNEXPECTED_OBJECT_CLOSE );
    }

    {
    Harness h( " {]" ); // Error case

    TTEST( h.parser.get( &h.event ) == cljp::Parser::PR_OK );
    TTEST( h.event.type == cljp::Event::T_OBJECT_START );

    TTEST( h.parser.get( &h.event ) == cljp::Parser::PR_UNEXPECTED_ARRAY_CLOSE );
    }

    TDOC( "Parser::get_in_array();" );
    {
    Harness h( "[{}]" );

    TTEST( h.parser.get( &h.event ) == cljp::Parser::PR_OK );
    TTEST( h.event.type == cljp::Event::T_ARRAY_START );

    TTEST( h.parser.get( &h.event ) == cljp::Parser::PR_OK );
    TTEST( h.event.type == cljp::Event::T_OBJECT_START );

    TTEST( h.parser.get( &h.event ) == cljp::Parser::PR_OK );
    TTEST( h.event.type == cljp::Event::T_OBJECT_END );

    TTEST( h.parser.get( &h.event ) == cljp::Parser::PR_OK );
    TTEST( h.event.type == cljp::Event::T_ARRAY_END );
    }

    {
    Harness h( "[{},{}]" );

    TTEST( h.parser.get( &h.event ) == cljp::Parser::PR_OK );
    TTEST( h.event.type == cljp::Event::T_ARRAY_START );

    TTEST( h.parser.get( &h.event ) == cljp::Parser::PR_OK );
    TTEST( h.event.type == cljp::Event::T_OBJECT_START );

    TTEST( h.parser.get( &h.event ) == cljp::Parser::PR_OK );
    TTEST( h.event.type == cljp::Event::T_OBJECT_END );

    TTEST( h.parser.get( &h.event ) == cljp::Parser::PR_OK );
    TTEST( h.event.type == cljp::Event::T_OBJECT_START );

    TTEST( h.parser.get( &h.event ) == cljp::Parser::PR_OK );
    TTEST( h.event.type == cljp::Event::T_OBJECT_END );

    TTEST( h.parser.get( &h.event ) == cljp::Parser::PR_OK );
    TTEST( h.event.type == cljp::Event::T_ARRAY_END );
    }

    {
    Harness h( "[{},]" );   // Error case

    TTEST( h.parser.get( &h.event ) == cljp::Parser::PR_OK );
    TTEST( h.event.type == cljp::Event::T_ARRAY_START );

    TTEST( h.parser.get( &h.event ) == cljp::Parser::PR_OK );
    TTEST( h.event.type == cljp::Event::T_OBJECT_START );

    TTEST( h.parser.get( &h.event ) == cljp::Parser::PR_OK );
    TTEST( h.event.type == cljp::Event::T_OBJECT_END );

    TTEST( h.parser.get( &h.event ) == cljp::Parser::PR_UNEXPECTED_ARRAY_CLOSE );
    }

    TTODO( "Parser::get_in_object();" );
}

TFEATURE( "Parser truncated input" )
{
    TTODO( "Parser truncated input" );  // Put in a sample of test progressively long bits
}

void value_test(
        int test_line,
        const char * p_input,
        cljp::Parser::ParserResult expected_result,
        cljp::Event::Type expected_type,
        const char * p_expected_value )
{
    char c_doc[256];
    sprintf( c_doc, "Line: %d, input: %s", test_line, p_input );
    TDOC( c_doc );

    std::string composed_input( "[" );
    composed_input.append( p_input );

    Harness h( composed_input );

    TTEST( h.parser.get( &h.event ) == cljp::Parser::PR_OK );
    TTEST( h.event.type == cljp::Event::T_ARRAY_START );

    TTEST( h.parser.get( &h.event ) == expected_result );
    if( expected_result == cljp::Parser::PR_OK )
    {
        TTEST( h.event.type == expected_type );
        TTEST( h.event.value == p_expected_value );
    }
}

TFEATURE( "Parser Reading constant values" )
{
    TDOC( "Parser::get_false()" );
    value_test( __LINE__, "false", cljp::Parser::PR_OK, cljp::Event::T_BOOLEAN, "false" );
    value_test( __LINE__, " false", cljp::Parser::PR_OK, cljp::Event::T_BOOLEAN, "false" );
    value_test( __LINE__, "false,", cljp::Parser::PR_OK, cljp::Event::T_BOOLEAN, "false" );
    value_test( __LINE__, "false", cljp::Parser::PR_OK, cljp::Event::T_BOOLEAN, "false" );
    value_test( __LINE__, "false ", cljp::Parser::PR_OK, cljp::Event::T_BOOLEAN, "false" );
    value_test( __LINE__, "false]", cljp::Parser::PR_OK, cljp::Event::T_BOOLEAN, "false" );
    value_test( __LINE__, "false}", cljp::Parser::PR_OK, cljp::Event::T_BOOLEAN, "false" );

    // Error cases
    value_test( __LINE__, "f", cljp::Parser::PR_BAD_FORMAT_FALSE, cljp::Event::T_BOOLEAN, "false" );
    value_test( __LINE__, "fal", cljp::Parser::PR_BAD_FORMAT_FALSE, cljp::Event::T_BOOLEAN, "false" );
    value_test( __LINE__, "falsey", cljp::Parser::PR_BAD_FORMAT_FALSE, cljp::Event::T_BOOLEAN, "false" );
    value_test( __LINE__, "false:", cljp::Parser::PR_BAD_FORMAT_FALSE, cljp::Event::T_BOOLEAN, "false" );

    TDOC( "Parser::get_true()" );
    value_test( __LINE__, "true", cljp::Parser::PR_OK, cljp::Event::T_BOOLEAN, "true" );
    value_test( __LINE__, " true", cljp::Parser::PR_OK, cljp::Event::T_BOOLEAN, "true" );
    value_test( __LINE__, "true,", cljp::Parser::PR_OK, cljp::Event::T_BOOLEAN, "true" );
    value_test( __LINE__, "true", cljp::Parser::PR_OK, cljp::Event::T_BOOLEAN, "true" );
    value_test( __LINE__, "true ", cljp::Parser::PR_OK, cljp::Event::T_BOOLEAN, "true" );
    value_test( __LINE__, "true]", cljp::Parser::PR_OK, cljp::Event::T_BOOLEAN, "true" );
    value_test( __LINE__, "true}", cljp::Parser::PR_OK, cljp::Event::T_BOOLEAN, "true" );

    // Error cases
    value_test( __LINE__, "t", cljp::Parser::PR_BAD_FORMAT_TRUE, cljp::Event::T_BOOLEAN, "true" );
    value_test( __LINE__, "tru", cljp::Parser::PR_BAD_FORMAT_TRUE, cljp::Event::T_BOOLEAN, "true" );
    value_test( __LINE__, "truey", cljp::Parser::PR_BAD_FORMAT_TRUE, cljp::Event::T_BOOLEAN, "true" );
    value_test( __LINE__, "true:", cljp::Parser::PR_BAD_FORMAT_TRUE, cljp::Event::T_BOOLEAN, "true" );

    TDOC( "Parser::get_null()" );
    value_test( __LINE__, "null", cljp::Parser::PR_OK, cljp::Event::T_NULL, "null" );
    value_test( __LINE__, " null", cljp::Parser::PR_OK, cljp::Event::T_NULL, "null" );
    value_test( __LINE__, "null,", cljp::Parser::PR_OK, cljp::Event::T_NULL, "null" );
    value_test( __LINE__, "null", cljp::Parser::PR_OK, cljp::Event::T_NULL, "null" );
    value_test( __LINE__, "null ", cljp::Parser::PR_OK, cljp::Event::T_NULL, "null" );
    value_test( __LINE__, "null]", cljp::Parser::PR_OK, cljp::Event::T_NULL, "null" );
    value_test( __LINE__, "null}", cljp::Parser::PR_OK, cljp::Event::T_NULL, "null" );

    // Error cases
    value_test( __LINE__, "n", cljp::Parser::PR_BAD_FORMAT_NULL, cljp::Event::T_NULL, "null" );
    value_test( __LINE__, "nul", cljp::Parser::PR_BAD_FORMAT_NULL, cljp::Event::T_NULL, "null" );
    value_test( __LINE__, "nully", cljp::Parser::PR_BAD_FORMAT_NULL, cljp::Event::T_NULL, "null" );
    value_test( __LINE__, "null:", cljp::Parser::PR_BAD_FORMAT_NULL, cljp::Event::T_NULL, "null" );
}

void number_ok_test(
        int test_line,
        const char * p_input,
        const char * p_expected_value )
{
    value_test( test_line, p_input, cljp::Parser::PR_OK, cljp::Event::T_NUMBER, p_expected_value );
}

void number_ok_test(
        int test_line,
        const char * p_input )
{
    number_ok_test( test_line, p_input, p_input );
}

void number_fail_test(
        int test_line,
        const char * p_input )
{
    value_test( test_line, p_input, cljp::Parser::PR_BAD_FORMAT_NUMBER, cljp::Event::T_NUMBER, p_input );
}

TFEATURE( "Parser Reading number values" )
{
    TTODO( "Parser::get_number()" );

    number_ok_test( __LINE__, "1" );
    number_ok_test( __LINE__, "12" );
    number_ok_test( __LINE__, "12,", "12" );
    number_ok_test( __LINE__, "12 ", "12" );

    number_ok_test( __LINE__, "-1" );
    number_ok_test( __LINE__, "-12" );
    number_fail_test( __LINE__, "-" );
    value_test( __LINE__, "+1", cljp::Parser::PR_UNRECOGNISED_VALUE_FORMAT, cljp::Event::T_NUMBER, "+1" );

    number_ok_test( __LINE__, "0" );
    number_ok_test( __LINE__, "-0" );
    number_fail_test( __LINE__, "00" );

    number_ok_test( __LINE__, "1.1" );
    number_ok_test( __LINE__, "12.12" );
    number_ok_test( __LINE__, "-12.12" );
    number_fail_test( __LINE__, "1." );

    number_ok_test( __LINE__, "1e1" );
    number_ok_test( __LINE__, "1e+12" );
    number_ok_test( __LINE__, "12e+12" );
    number_ok_test( __LINE__, "1e-12" );
    number_ok_test( __LINE__, "12e-12" );
    number_ok_test( __LINE__, "1.1e+12" );
    number_ok_test( __LINE__, "12.12e-12" );
    number_ok_test( __LINE__, "-12.12e-12" );
    number_fail_test( __LINE__, "1.e" );
    number_fail_test( __LINE__, "1e" );
    number_fail_test( __LINE__, "1e+ " );
}

TFEATURE( "Parser Back-to-back numbers" )
{
    {
    Harness h( "[12.3, 4.2e+6]" );

    TTEST( h.parser.get( &h.event ) == cljp::Parser::PR_OK );
    TTEST( h.event.type == cljp::Event::T_ARRAY_START );

    TTEST( h.parser.get( &h.event ) == cljp::Parser::PR_OK );
    TTEST( h.event.type == cljp::Event::T_NUMBER );
    TTEST( h.event.value == "12.3" );

    TTEST( h.parser.get( &h.event ) == cljp::Parser::PR_OK );
    TTEST( h.event.type == cljp::Event::T_NUMBER );
    TTEST( h.event.value == "4.2e+6" );

    TTEST( h.parser.get( &h.event ) == cljp::Parser::PR_OK );
    TTEST( h.event.type == cljp::Event::T_ARRAY_END );
    }

    {
    Harness h( "[ 12.3 , 4.2e+6 ]" );

    TTEST( h.parser.get( &h.event ) == cljp::Parser::PR_OK );
    TTEST( h.event.type == cljp::Event::T_ARRAY_START );

    TTEST( h.parser.get( &h.event ) == cljp::Parser::PR_OK );
    TTEST( h.event.type == cljp::Event::T_NUMBER );
    TTEST( h.event.value == "12.3" );

    TTEST( h.parser.get( &h.event ) == cljp::Parser::PR_OK );
    TTEST( h.event.type == cljp::Event::T_NUMBER );
    TTEST( h.event.value == "4.2e+6" );

    TTEST( h.parser.get( &h.event ) == cljp::Parser::PR_OK );
    TTEST( h.event.type == cljp::Event::T_ARRAY_END );
    }
}

void string_ok_test(
        int test_line,
        const char * p_input,
        const char * p_expected_value )
{
    char c_doc[256];
    sprintf( c_doc, "Line: %d, input: %s", test_line, p_input );
    TDOC( c_doc );

    std::string composed_input( "[\"" );
    composed_input.append( p_input );
    composed_input.append( "\"" );

    Harness h( composed_input );

    TTEST( h.parser.get( &h.event ) == cljp::Parser::PR_OK );
    TTEST( h.event.type == cljp::Event::T_ARRAY_START );

    TTEST( h.parser.get( &h.event ) == cljp::Parser::PR_OK );
    TTEST( h.event.type == cljp::Event::T_STRING );
    TTEST( h.event.value == p_expected_value );
}

void string_ok_test(
        int test_line,
        const char * p_input )
{
	string_ok_test( test_line, p_input, p_input );
}

TFEATURE( "Parser Reading string values" )
{
    TTODO( "Parser::get_string()" );
    
    string_ok_test( __LINE__, "Fred" );
}
