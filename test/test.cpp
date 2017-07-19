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

#include "cl-json-pull/cl-json-pull.h"

#define CLUNIT_HOME
#include "clunit.h"

int main()
{
    TRUNALL();
}

int example_check()
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
