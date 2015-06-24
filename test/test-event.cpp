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

TFEATURE( "struct Event" )
{
    cljp::Event event;

    TTEST( event.name == "" );
    TTEST( event.value == "" );
    TTEST( event.type == cljp::Event::T_UNKNOWN );

    event.name = "name";
    event.value = "value";
    event.type = cljp::Event::T_STRING;

    TTEST( event.name == "name" );
    TTEST( event.value == "value" );
    TTEST( event.type == cljp::Event::T_STRING );

    event.clear();

    TTEST( event.name == "" );
    TTEST( event.value == "" );
    TTEST( event.type == cljp::Event::T_UNKNOWN );
}

TFEATURE( "Event simple / convenience is_XXX methods" )
{
    // Seems no reason to test all of these!
    cljp::Event event;

    event.type = cljp::Event::T_BOOLEAN;
    TTEST( event.is_boolean() );
    TTEST( event.is_bool() );

    event.type = cljp::Event::T_NUMBER;
    TTEST( event.is_number() );
}

TFEATURE( "Event::is_true and is_false" )
{
    // is_true() and is_false() don't do implicit type casting
    {
    TDOC( "Test Event::is_true()" );
    cljp::Event event;
    event.type = cljp::Event::T_BOOLEAN;

    event.value = "true";
    TTEST( event.is_true() == true );

    event.value = "false";
    TTEST( event.is_true() == false );

    event.value = "TRUE";
    TTEST( event.is_true() == false );

    event.type = cljp::Event::T_STRING;     // It's not BOOLEAN so it's not true
    event.value = "true";
    TTEST( event.is_true() == false );
    }

    {
    TDOC( "Test Event::is_false()" );
    cljp::Event event;
    event.type = cljp::Event::T_BOOLEAN;

    event.value = "false";
    TTEST( event.is_false() == true );

    event.value = "FALSE";
    TTEST( event.is_false() == false );

    event.type = cljp::Event::T_STRING;     // It's not BOOLEAN so it's not false
    event.value = "false";
    TTEST( event.is_false() == false );
    }
}

TFEATURE( "Event::is_int" )
{
    cljp::Event event;
    event.type = cljp::Event::T_NUMBER;

    event.value = "0";
    TTEST( event.is_int() == true );

    event.value = "1";
    TTEST( event.is_int() == true );

    event.value = "-11";
    TTEST( event.is_int() == true );

    event.value = "20";
    TTEST( event.is_int() == true );

    event.value = "1.0";
    TTEST( event.is_int() == false );

    event.value = "1e1";
    TTEST( event.is_int() == false );

    event.value = "1E1";
    TTEST( event.is_int() == false );

    event.type = cljp::Event::T_STRING; // Must be T_NUMBER to be is_int()
    event.value = "0";
    TTEST( event.is_int() == false );
}

TFEATURE( "Event::to_bool" )
{
    {
    TDOC( "Test Event::to_bool() for booleans" );
    cljp::Event event;
    event.type = cljp::Event::T_BOOLEAN;

    event.value = "false";
    TTEST( event.to_bool() == false );

    event.value = "true";
    TTEST( event.to_bool() == true );
    }

    {
    TDOC( "Test Event::to_bool() for strings" );
    cljp::Event event;
    event.type = cljp::Event::T_STRING;

    event.value = "false";
    TTEST( event.to_bool() == true );

    event.value = " ";
    TTEST( event.to_bool() == true );

    event.value = "";
    TTEST( event.to_bool() == false );
    }

    {
    TDOC( "Test Event::to_bool() for numbers" );
    cljp::Event event;
    event.type = cljp::Event::T_NUMBER;

    event.value = "0";
    TTEST( event.to_bool() == false );

    event.value = "0.0";
    TTEST( event.to_bool() == false );

    event.value = "0e1";
    TTEST( event.to_bool() == false );

    event.value = "1";
    TTEST( event.to_bool() == true );
    }

    {
    TDOC( "Test Event::to_bool() for null" );
    cljp::Event event;
    event.type = cljp::Event::T_NULL;

    event.value = "null";
    TTEST( event.to_bool() == false );
    }
}

TFEATURE( "Event to_float, to_int" )
{
    {
    cljp::Event event;
    event.type = cljp::Event::T_NUMBER;

    event.value = "1";
    TTEST( event.to_float() == 1.0 );

    event.value = "1.5";
    TTEST( event.to_float() == 1.5 );

    TTEST( event.to_int() == 1 );

    TTEST( event.to_long() == 1 );

    event.value = "-11.5";
    TTEST( event.to_float() == -11.5 );

    TTEST( event.to_int() == -11 );

    TTEST( event.to_long() == -11 );
    }

    {
    TDOC( "Event to_float, to_int for string to bool then to int/float conversions" );
    cljp::Event event;
    event.type = cljp::Event::T_STRING;

    event.value = "1";
    TTEST( event.to_float() == 1.0 );

    event.value = "0";
    TTEST( event.to_float() == 1.0 );   // The string "0" maps to true (as it's not empty) which maps to 1

    event.value = "";
    TTEST( event.to_float() == 0.0 );

    event.value = "1";
    TTEST( event.to_int() == 1 );

    event.value = "0";
    TTEST( event.to_int() == 1 );   // The string "0" maps to true (as it's not empty) which maps to 1

    event.value = "";
    TTEST( event.to_int() == 0 );
    }
}

TFEATURE( "Event to_string, to_wstring" )
{
    {
    cljp::Event event;
    event.type = cljp::Event::T_STRING;

    event.value = "MyString";
    TTEST( event.to_string() == "MyString" );
    }

    {
    cljp::Event event;
    event.type = cljp::Event::T_STRING;

    // From rfc3629
    event.value = "\xE6\x97\xA5\xE6\x9C\xAC\xE8\xAA\x9E";
    TTEST( event.to_string() == "\xE6\x97\xA5\xE6\x9C\xAC\xE8\xAA\x9E" );
    }

    {
    cljp::Event event;
    event.type = cljp::Event::T_STRING;

    event.value = "MyString";
    TTEST( event.to_wstring() == L"MyString" );
    }

    {
    cljp::Event event;
    event.type = cljp::Event::T_STRING;

    // From rfc3629
    event.value = "\xE6\x97\xA5\xE6\x9C\xAC\xE8\xAA\x9E";
    TTEST( event.to_wstring() == L"\x65E5\x672C\x8A9E" );
    }

    {
    cljp::Event event;
    event.type = cljp::Event::T_STRING;

    // From rfc3629
    event.value = "with\xE6\x97\xA5\xE6\x9C\xAC\xE8\xAA\x9EHold";
    TTEST( event.to_wstring() == L"with\x65E5\x672C\x8A9EHold" );
    }

    {
    cljp::Event event;
    event.type = cljp::Event::T_STRING;

    // From rfc2781 (surrogates)
    event.value = "\xF0\x92\x8D\x85=Ra";
    #if defined( _MSC_VER )
        { TTEST( event.to_wstring() == L"\xD808\xDF45=Ra" ); }
    #elif defined( __GNUC__ )
        { TTEST( event.to_wstring() == L"\x12345=Ra" ); }
    #else
        TTODO( "Event::to_wstring() with surrogate range needs fully tested" );
        TTEST( false );
    #endif
    }

    {
    cljp::Event event;
    event.type = cljp::Event::T_STRING;

    // From rfc2781 (surrogates)
    event.value = "with\xF0\x92\x8D\x85=Ra";
    #if defined( _MSC_VER )
        { TTEST( event.to_wstring() == L"with\xD808\xDF45=Ra" ); }
    #elif defined( __GNUC__ )
        { TTEST( event.to_wstring() == L"with\x12345=Ra" ); }
    #else
        TTODO( "Event::to_wstring() with surrogate range needs fully tested" );
        TTEST( false );
    #endif
    }
}
