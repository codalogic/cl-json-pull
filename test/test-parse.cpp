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

#include "cl-json-pull.h"   // Put file under test first to verify dependencies

#include "clunit.h"

#include <string>

#include "test-harness.h"

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

    TTEST( h.parser.get( &h.event ) == cljp::Parser::PR_END_OF_MESSAGE );
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

    TTEST( h.parser.get( &h.event ) == cljp::Parser::PR_END_OF_MESSAGE );
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

    {
    Harness h( "12" );

    TTEST( h.parser.get( &h.event ) == cljp::Parser::PR_OK );
    TTEST( h.event.type == cljp::Event::T_NUMBER );
    TTEST( h.event.value == "12" );

    TTEST( h.parser.get( &h.event ) == cljp::Parser::PR_END_OF_MESSAGE );
    }

    {
    Harness h( "\"String\"" );

    TTEST( h.parser.get( &h.event ) == cljp::Parser::PR_OK );
    TTEST( h.event.type == cljp::Event::T_STRING );
    TTEST( h.event.value == "String" );

    TTEST( h.parser.get( &h.event ) == cljp::Parser::PR_END_OF_MESSAGE );
    }

    {
    Harness h( "false" );

    TTEST( h.parser.get( &h.event ) == cljp::Parser::PR_OK );
    TTEST( h.event.type == cljp::Event::T_BOOLEAN );
    TTEST( h.event.value == "false" );

    TTEST( h.parser.get( &h.event ) == cljp::Parser::PR_END_OF_MESSAGE );
    }

    {
    Harness h( "?" );

    TTEST( h.parser.get( &h.event ) == cljp::Parser::PR_UNRECOGNISED_VALUE_FORMAT );

    TTEST( h.parser.get( &h.event ) == cljp::Parser::PR_UNABLE_TO_CONTINUE_DUE_TO_ERRORS );
    }
}

TFEATURE( "Repeated reads at end of message return 'End of message'" )
{
    {
    Harness h( "12" );

    TTEST( h.parser.get( &h.event ) == cljp::Parser::PR_OK );
    TTEST( h.event.type == cljp::Event::T_NUMBER );
    TTEST( h.event.value == "12" );

    TTEST( h.parser.get( &h.event ) == cljp::Parser::PR_END_OF_MESSAGE );

    // Repeated reads at end of message return 'End of message'
    TTEST( h.parser.get( &h.event ) == cljp::Parser::PR_END_OF_MESSAGE );
    TTEST( h.parser.get( &h.event ) == cljp::Parser::PR_END_OF_MESSAGE );
    }
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
    number_ok_test( __LINE__, "1" );
    number_ok_test( __LINE__, "12" );
    number_ok_test( __LINE__, "12,", "12" );
    number_ok_test( __LINE__, "12 ", "12" );

    number_ok_test( __LINE__, "-1" );
    number_ok_test( __LINE__, "-12" );
    number_fail_test( __LINE__, "-" );
    number_fail_test( __LINE__, "+1" );

    number_ok_test( __LINE__, "0" );
    number_ok_test( __LINE__, "-0" );
    number_fail_test( __LINE__, "00" );
    number_fail_test( __LINE__, "01" );
    number_fail_test( __LINE__, "-01" );

    number_ok_test( __LINE__, "1.1" );
    number_ok_test( __LINE__, "12.12" );
    number_ok_test( __LINE__, "-12.12" );
    number_fail_test( __LINE__, "." );
    number_fail_test( __LINE__, "1." );
    number_fail_test( __LINE__, ".1" );
    number_fail_test( __LINE__, "-." );
    number_fail_test( __LINE__, "-1." );
    number_fail_test( __LINE__, "-.1" );

    number_ok_test( __LINE__, "1e1" );
    number_ok_test( __LINE__, "1E1" );
    number_ok_test( __LINE__, "1e+12" );
    number_ok_test( __LINE__, "12e+12" );
    number_ok_test( __LINE__, "1e-12" );
    number_ok_test( __LINE__, "12e-12" );
    number_ok_test( __LINE__, "1.1e+12" );
    number_ok_test( __LINE__, "12.12e-12" );
    number_ok_test( __LINE__, "-12.12e-12" );
    number_ok_test( __LINE__, "12.12e12" );
    number_ok_test( __LINE__, "-12.12e12" );
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

void string_fail_test(
        int test_line,
        const char * p_input,
        cljp::Parser::ParserResult expected_error_code )
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

    TTEST( h.parser.get( &h.event ) == expected_error_code );
}

TFEATURE( "Parser Reading string values" )
{
    TDOC( "Parser::get_string()" );

    string_ok_test( __LINE__, "Fred" );
        //           %x22 /          ; "    quotation mark  U+0022
        //           %x5C /          ; \    reverse solidus U+005C
        //           %x2F /          ; /    solidus         U+002F
        //           %x62 /          ; b    backspace       U+0008
        //           %x66 /          ; f    form feed       U+000C
        //           %x6E /          ; n    line feed       U+000A
        //           %x72 /          ; r    carriage return U+000D
        //           %x74 /          ; t    tab             U+0009
    string_ok_test( __LINE__, "Say \\\"Fred\\\"", "Say \"Fred\"" );
    string_ok_test( __LINE__, "Say \\nFred\\n", "Say \nFred\n" );

    string_ok_test( __LINE__, "Say \\\\Fred\\/", "Say \\Fred/" );
    string_ok_test( __LINE__, "Say \\bFred\\f", "Say \bFred\f" );
    string_ok_test( __LINE__, "Say \\nFred\\r", "Say \nFred\r" );
    string_ok_test( __LINE__, "Say \\tFred\\t", "Say \tFred\t" );

    string_fail_test( __LINE__, "Say \\qFred", cljp::Parser::PR_BAD_FORMAT_STRING );

    TDOC( "Parser::get_string() - char outside unescaped = %x20-21 / %x23-5B / %x5D-10FFFF fails" );
    string_fail_test( __LINE__, "Say \x01 Fred", cljp::Parser::PR_BAD_FORMAT_STRING );
}

TFEATURE( "Parser Reading string Unicode escapes" )
{
    TCALL( string_ok_test( __LINE__, "Say \\u002fFred", "Say /Fred" ) );

    // The following converted using http://rishida.net/tools/conversion/
    TCALL( string_ok_test( __LINE__, "\\uD800\\uDC02", "\xF0\x90\x80\x82" ) );  // \uD800\uDC02 -> u+10002
    TCALL( string_ok_test( __LINE__, "\\ud800\\udc02", "\xF0\x90\x80\x82" ) );  // \ud800\udc02 -> u+10002
    TCALL( string_ok_test( __LINE__, "\\u0802", "\xE0\xA0\x82" ) );
    TCALL( string_ok_test( __LINE__, "\\uFFFC", "\xEF\xBF\xBC" ) );
    TCALL( string_ok_test( __LINE__, "\\ufffc", "\xEF\xBF\xBC" ) );
    TCALL( string_ok_test( __LINE__, "\\u0082", "\xC2\x82" ) );
    TCALL( string_ok_test( __LINE__, "\\u07FC", "\xDF\xBC" ) );

    // Check conversions also work within a string
    TCALL( string_ok_test( __LINE__, "X\\uD800\\uDC02A", "X\xF0\x90\x80\x82""A" ) );    // \uD800\uDC02 -> u+10002
    TCALL( string_ok_test( __LINE__, "X\\u0802A", "X\xE0\xA0\x82""A" ) );
    TCALL( string_ok_test( __LINE__, "X\\u0082A", "X\xC2\x82""A" ) );

    TDOC( "Parser::get_string() - truncated BMP unicode escape fails" );
    TCALL( string_fail_test( __LINE__, "Say \\u002 Fred", cljp::Parser::PR_BAD_UNICODE_ESCAPE ) );
    TCALL( string_fail_test( __LINE__, "Say \\u002QFred", cljp::Parser::PR_BAD_UNICODE_ESCAPE ) );

    // From rfc3629
    TCALL( string_ok_test( __LINE__, "\\u65E5\\u672C\\u8A9E", "\xE6\x97\xA5\xE6\x9C\xAC\xE8\xAA\x9E" ) );

    // From rfc2781
    TDOC( "Parser::get_string() - Surrogates unicode escape" );
    TCALL( string_ok_test( __LINE__, "\\uD808\\uDF45=Ra", "\xF0\x92\x8D\x85=Ra" ) );

    TDOC( "Parser::get_string() - High surrogate without following low surrogate fails" );
    TCALL( string_fail_test( __LINE__, "\\uD808Fred", cljp::Parser::PR_BAD_UNICODE_ESCAPE ) );
    TCALL( string_fail_test( __LINE__, "\\uD808\\u0022", cljp::Parser::PR_BAD_UNICODE_ESCAPE ) );

    TDOC( "Parser::get_string() - Low surrogate without preceeding high surrogate fails" );
    TCALL( string_fail_test( __LINE__, "\\uDF45Fred", cljp::Parser::PR_BAD_UNICODE_ESCAPE ) );
    TCALL( string_fail_test( __LINE__, "\\uDF45\\u0022", cljp::Parser::PR_BAD_UNICODE_ESCAPE ) );
}

void string_fail_disallows_follow_on_pulls_test(
        int test_line,
        const char * p_input )
{
    char c_doc[256];
    sprintf( c_doc, "Line: %d, input: %s", test_line, p_input );
    TDOC( c_doc );

    std::string composed_input( "[\"" );
    composed_input.append( p_input );
    composed_input.append( "Fred\", \"Bill\"" );

    Harness h( composed_input );

    TTEST( h.parser.get( &h.event ) == cljp::Parser::PR_OK );
    TTEST( h.event.type == cljp::Event::T_ARRAY_START );

    TTEST( h.parser.get( &h.event ) != cljp::Parser::PR_OK );
    TTEST( h.event.type == cljp::Event::T_STRING );
    TTEST( h.event.value.find( "Fred" ) != std::string::npos );

    TTEST( h.parser.get( &h.event ) == cljp::Parser::PR_UNABLE_TO_CONTINUE_DUE_TO_ERRORS );
}

TFEATURE( "Parser Reading string with bad Unicode escapes, check parsing terminated mid-string" )
{
    string_fail_disallows_follow_on_pulls_test( __LINE__, " \\u002 " );
}

TFEATURE( "Parser::get_string() - String end quote in middle of unicode escape code" )
{
    Harness h( "[ \" Fred\\u00\", \"Bill\"" );

    TTEST( h.parser.get( &h.event ) == cljp::Parser::PR_OK );
    TTEST( h.event.type == cljp::Event::T_ARRAY_START );

    TTEST( h.parser.get( &h.event ) != cljp::Parser::PR_OK );
    TTEST( h.event.type == cljp::Event::T_STRING );
    TTEST( h.event.value.find( "Fred" ) != std::string::npos );

    TTEST( h.parser.get( &h.event ) == cljp::Parser::PR_UNABLE_TO_CONTINUE_DUE_TO_ERRORS );
}

TFEATURE( "Parser Reading string unexpected EOF" )
{
    Harness h( "[\"Fred" );

    TTEST( h.parser.get( &h.event ) == cljp::Parser::PR_OK );
    TTEST( h.event.type == cljp::Event::T_ARRAY_START );

    TTEST( h.parser.get( &h.event ) == cljp::Parser::PR_UNEXPECTED_END_OF_MESSAGE );
}

TFEATURE( "Parser Read member" )
{
    {
    Harness h( "{ \"Field\" : 12 }" );

    TTEST( h.parser.get( &h.event ) == cljp::Parser::PR_OK );
    TTEST( h.event.type == cljp::Event::T_OBJECT_START );

    TTEST( h.parser.get( &h.event ) == cljp::Parser::PR_OK );
    TTEST( h.event.name == "Field" );
    TTEST( h.event.type == cljp::Event::T_NUMBER );
    TTEST( h.event.value == "12" );

    TTEST( h.parser.get( &h.event ) == cljp::Parser::PR_OK );
    TTEST( h.event.type == cljp::Event::T_OBJECT_END );
    }

    {
    Harness h( "{ \"Field\" : 12, \"Jam\":\"High\" }" );

    TTEST( h.parser.get( &h.event ) == cljp::Parser::PR_OK );
    TTEST( h.event.type == cljp::Event::T_OBJECT_START );

    TTEST( h.parser.get( &h.event ) == cljp::Parser::PR_OK );
    TTEST( h.event.name == "Field" );
    TTEST( h.event.type == cljp::Event::T_NUMBER );
    TTEST( h.event.value == "12" );

    TTEST( h.parser.get( &h.event ) == cljp::Parser::PR_OK );
    TTEST( h.event.name == "Jam" );
    TTEST( h.event.type == cljp::Event::T_STRING );
    TTEST( h.event.value == "High" );

    TTEST( h.parser.get( &h.event ) == cljp::Parser::PR_OK );
    TTEST( h.event.type == cljp::Event::T_OBJECT_END );
    }

    // Error cases

    {
    Harness h( "{ \"Field\" : 12, }" ); // Comma after first member, but no second member

    TTEST( h.parser.get( &h.event ) == cljp::Parser::PR_OK );
    TTEST( h.event.type == cljp::Event::T_OBJECT_START );

    TTEST( h.parser.get( &h.event ) == cljp::Parser::PR_OK );
    TTEST( h.event.name == "Field" );
    TTEST( h.event.type == cljp::Event::T_NUMBER );
    TTEST( h.event.value == "12" );

    TTEST( h.parser.get( &h.event ) == cljp::Parser::PR_UNEXPECTED_OBJECT_CLOSE );

    TTEST( h.parser.get( &h.event ) == cljp::Parser::PR_UNABLE_TO_CONTINUE_DUE_TO_ERRORS );
    }

    {
    Harness h( "{ \"Field\" : 12, \"Jam\" }" );

    TTEST( h.parser.get( &h.event ) == cljp::Parser::PR_OK );
    TTEST( h.event.type == cljp::Event::T_OBJECT_START );

    TTEST( h.parser.get( &h.event ) == cljp::Parser::PR_OK );
    TTEST( h.event.name == "Field" );
    TTEST( h.event.type == cljp::Event::T_NUMBER );
    TTEST( h.event.value == "12" );

    TTEST( h.parser.get( &h.event ) == cljp::Parser::PR_EXPECTED_COLON_NAME_SEPARATOR );

    TTEST( h.parser.get( &h.event ) == cljp::Parser::PR_UNABLE_TO_CONTINUE_DUE_TO_ERRORS );
    }

    {
    Harness h( "{ \"Field\" : 12, 15 }" );

    TTEST( h.parser.get( &h.event ) == cljp::Parser::PR_OK );
    TTEST( h.event.type == cljp::Event::T_OBJECT_START );

    TTEST( h.parser.get( &h.event ) == cljp::Parser::PR_OK );
    TTEST( h.event.name == "Field" );
    TTEST( h.event.type == cljp::Event::T_NUMBER );
    TTEST( h.event.value == "12" );

    TTEST( h.parser.get( &h.event ) == cljp::Parser::PR_EXPECTED_MEMBER_NAME );

    TTEST( h.parser.get( &h.event ) == cljp::Parser::PR_UNABLE_TO_CONTINUE_DUE_TO_ERRORS );
    }

    {
    Harness h( "{ \"Field\" : 12, Jam }" );

    TTEST( h.parser.get( &h.event ) == cljp::Parser::PR_OK );
    TTEST( h.event.type == cljp::Event::T_OBJECT_START );

    TTEST( h.parser.get( &h.event ) == cljp::Parser::PR_OK );
    TTEST( h.event.name == "Field" );
    TTEST( h.event.type == cljp::Event::T_NUMBER );
    TTEST( h.event.value == "12" );

    TTEST( h.parser.get( &h.event ) == cljp::Parser::PR_EXPECTED_MEMBER_NAME );

    TTEST( h.parser.get( &h.event ) == cljp::Parser::PR_UNABLE_TO_CONTINUE_DUE_TO_ERRORS );
    }
}
