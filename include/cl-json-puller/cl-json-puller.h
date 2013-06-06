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

#include <string>

namespace cljp {	// Codalogic JSON Puller

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
	Type type() const { return m.type; }
	void type( Type type_in ) { m.type = type_in; }

	void clear() { m = Members(); }
};

}	// End of namespace cljp

#endif	// CL_JSON_PULLER_H
