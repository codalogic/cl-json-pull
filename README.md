cl-json-pull
============

cl-json-pull is Codalogic's C++ JSON Pull Parser.
It's intended to be a lower layer component on top of which more functional
layers can be built.

Currently it is a strict ECMA-404 / RFC 7159 implementation with no extensions.
Some configurable extensions may be added in future.

It can accept input in any of UTF-8, UTF-16 (LE or BE) and UTF-32 (LE or BE),
with or without a BOM.

The main class is `Parser` in the `cljp` namespace.

The primary method in this is
`Parser::Status Parser::get( Event * p_event_out )`.  Each call of this method
retrieves another event from the JSON input until the end of the message
is encountered or an error is detected.  The same 'Event' object can be
used in multiple `Parser::get()` calls, or different ones can be used.

The returned `Parser::Status` value indicates the success or otherwise of the
get operation.  `PS_OK` indicates that the get operation was successful, and
the `Event` object pointed to by `p_event_out` has been populated.
`PS_END_OF_MESSAGE` indicates that the end of the message has been reached.
Other values of `Parser::Status` indicate various error conditions.

On a succesful `get()` operation, the retrieved `Event` object indicates the
`type` of event that was retrieved, the member `name` if applicable, and the
`value` if applicable.  The indicated types include `T_STRING`, `T_NUMBER`,
`T_BOOLEAN`, `T_NULL`, `T_OBJECT_START`, `T_OBJECT_END`, `T_ARRAY_START`,
and `T_ARRAY_END`.

The `Event` object also includes helper methods to determine the type of the
event, including `is_string()`, `is_number()`, `is_boolean()`, `is_bool()`,
`is_null()`, `is_object_start()`, `is_object_end()`, `is_array_start()` and
`is_array_end()`.  Higher order methods that derive more details about the event
include `is_true()`, `is_false()`, `is_int()` and `is_float()`.  (Note that the
`is_true()` and `is_false()` methods require the underlying JSON type to be
Boolean in order to return a `true` value.  In other words, `is_true()` will
return `false` if the underlying JSON type is a string with the value "true".)

The `Event` object also contains conversion methods that allows the stored `value`
field to be converted to other useful types, for example, `to_bool()`, `to_float()`
`to_int()`, `to_long()`, `to_string()` and `to_wstring()`.  Unlike `is_true()` and
`is_false()` mentiond above, `to_bool()` will 'cast' non-Boolean values into
such a value.  For example, empty strings will yield `false`, and non-empty strings
`true`.  Numerical values equal to `0` will yield `false` and non-zero values
will yield `true`.

The `Parser::skip()` method skips the rest of an object or array.  It is used to
easily ignore the contents of objects or arrays you are not interested in.


To create a `Parser` object on which `Parser::get()` can be called, it is necessary
to create an object that derives from the `Reader` class.  The supplied derivations
are `ReaderMemory`, `ReaderString` and `ReaderFile`, which read from memory, a
std::string, or a file respectively.  Other derivations of `Reader` can be created
to read input from other sources, such as a socket.

Putting it all together, a trivial (albeit useless!) program would look like:

```cpp
#include "cl-json-pull.h"

#include <iostream>

int main()
{
    cljp::ReaderFile reader( "myfile.json" );

    if( reader.is_open() )
    {
        cljp::Parser parser( reader );
        cljp::Event event;

        while( parser.get( &event ) == cljp::Parser::PS_OK )
        {
            // TODO: Do something with event, e.g.:
            switch( event.type )
            {
            case cljp::Event::T_OBJECT_START:
                std::cout << "Object start\n";
                if( event.is( "Wanted1" ) )
                {
                    // record something about Wanted1
                }
                else if( event.is( "Wanted2" ) )
                {
                    // record something about Wanted2
                }
                else
                {
                    // Skip contents of unwanted object
                    if( parser.skip() != cljp::Parser::PS_OK )
                        return 0;
                }
            break;

            case cljp::Event::T_OBJECT_END:
                std::cout << "Object end\n";
            break;

            case cljp::Event::T_ARRAY_START:
                std::cout << "Array start\n";
            break;

            case cljp::Event::T_ARRAY_END:
                std::cout << "Array end\n";
            break;

            default:
                std::cout << "Some other event\n";
            }
        }
    }
    return 0;
}
```

The `test-messages.cpp` test file gives some examples of expected event sequences.

Future Work
===========

Possible future work includes allowing member names to not be quoted, and JavaScript
style comments.

Consider adding an Event::on( "name", type, handler ) method to allow easier
handling of events.  For example:

```cpp
    Event event;
    parser.get( &event );
    event.on( "foo", T_STRING, handle_foo ).
          on( "bar", T_NUMBER, handle_bar );
```

License
=======

Copyright (c) 2012, Codalogic Ltd (http://www.codalogic.com)
All rights reserved.

The license for this file is based on the BSD-3-Clause license
(http://www.opensource.org/licenses/BSD-3-Clause).

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

- Redistributions of source code must retain the above copyright notice,
  this list of conditions and the following disclaimer.
- Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.
- Neither the name Codalogic nor the names of its contributors may be used
  to endorse or promote products derived from this software without
  specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
