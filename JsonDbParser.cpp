/*
 		Copyright (C) 2010 Wouter van Kleunen

		This file is part of JsonDb.

    Foobar is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Foobar is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with JsonDb.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "JsonDb.h"
#include "JsonDbValues.h"
#include "JsonDbParser.h"

#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/spirit/core.hpp>
#include <boost/spirit/utility/confix.hpp>
#include <boost/spirit/utility/escape_char.hpp>
#include <boost/spirit/utility/lists.hpp>

using namespace boost;
using namespace boost::spirit;

std::string remove_trailing_quote(std::string const &s)
{
	assert((s[s.length() - 1] == '"') || (s[s.length() - 1] == '\''));
	return s.substr(0, s.length() - 1);
}

// this class's methods get called by the spirit parse resulting
// in the creation of a JSON object or array
//
class Semantic_actions
{
public:
	Semantic_actions(JsonDb::TransactionHandle &_transaction, ValuePointer root);

	void begin_obj   ( char c );
	void end_obj     ( char c );
	void begin_array ( char c );
	void end_array   ( char c );
	void new_c_esc_ch( char c );
	void new_name ( const char* str, const char* end );
	void new_str  ( const char* str, const char* end );
	void new_true ( const char* str, const char* end );
	void new_false( const char* str, const char* end );
	void new_null ( const char* str, const char* end );
	void new_int ( int i );
	void new_real( double d );

private:

	void add_to_current(ValuePointer value); 
	std::string get_current_str();

	JsonDb::TransactionHandle transaction;

	std::vector<ValuePointer> stack;    // previous child objects and arrays 
	ValuePointer current_value;         // the child object or array that is currently being constructed
	ValuePointer root;

	std::string name;             		  // of current name/value pair
	std::string current_str;        		// current name or string value 
};

Semantic_actions::Semantic_actions(JsonDb::TransactionHandle &_transaction, ValuePointer _root)
	: transaction(_transaction)
{ 
	root = _root;
	current_value = _root;
}

void Semantic_actions::begin_obj(char c)
{
	assert(c == '{');

	// Store the current value in the stack
	stack.push_back(current_value);

	// Add a new value
	ValuePointer new_value(new ValueObject(null_key));
	add_to_current(new_value);
	current_value = new_value; 
}

void Semantic_actions::end_obj(char c)
{
	assert(c == '}');
	transaction->Store(current_value->GetKey(), current_value);
	current_value = stack.back();
	stack.pop_back(); 
}

void Semantic_actions::begin_array(char c)
{
	assert(c == '[');

	// Store the current value in the stack
	stack.push_back(current_value);

	// Add a new value
	ValuePointer new_value(new ValueArray(null_key));
	add_to_current(new_value);
	current_value = new_value; 
}

void Semantic_actions::end_array(char c)
{
	assert(c == ']');

	transaction->Store(current_value->GetKey(), current_value);
	current_value = stack.back();
	stack.pop_back(); 
}

void Semantic_actions::new_c_esc_ch(char c)  // work around for c_escape_ch_p parser bug where
																						 // esc chars are not converted unless the semantic action
																						 // is attached directlry to the parser
{
	current_str += c;
}

void Semantic_actions::new_name(const char *str, const char *end)
{
	assert(current_value != NULL && current_value->GetType() == Value::VALUE_OBJECT);
	name = get_current_str();
}

void Semantic_actions::new_str(const char *str, const char *end)
{
	ValuePointer value(new ValueString(null_key, get_current_str()));
	add_to_current(value);
}

void Semantic_actions::new_true(const char *str, const char *end)
{
	assert(std::string(str, end) == "true");
	ValuePointer value(new ValueNumberBoolean(null_key, true));
	add_to_current(value);
}

void Semantic_actions::new_false(const char *str, const char *end)
{
	assert(std::string(str, end) == "false");
	ValuePointer value(new ValueNumberBoolean(null_key, false));
	add_to_current(value);
}

void Semantic_actions::new_null(const char *str, const char *end)
{
	assert(std::string(str, end) == "null");
	ValuePointer value(new ValueNull(null_key));
	add_to_current(value);
}

void Semantic_actions::new_int(int i)
{
	ValuePointer value(new ValueNumberInteger(null_key, i));
	add_to_current(value);
}

void Semantic_actions::new_real(double d)
{
	ValuePointer value(new ValueNumberFloat(null_key, (float)d));
	add_to_current(value);
}

void Semantic_actions::add_to_current(ValuePointer value)
{
	if(current_value == root)
	{
		// Replace the root with this value
		ValueKey key = current_value->GetKey();
		current_value->Delete(transaction);
		value->SetKey(key);
		transaction->Store(key, value);
	} else if(current_value->GetType() == Value::VALUE_OBJECT)
	{
		// Set the field
		ValuePointer old_value = current_value->Get(transaction, name, name, create);
		ValueKey key = old_value->GetKey();
		old_value->Delete(transaction);
		value->SetKey(key);
		transaction->Store(key, value);
	}	 else if(current_value->GetType() == Value::VALUE_ARRAY)
	{
		// Append to the list
		ValueKey key = transaction->GenerateKey();
		value->SetKey(key);
		current_value->Append(transaction, key);
		transaction->Store(key, value);
	} 
}

std::string Semantic_actions::get_current_str()
{
	std::string result = remove_trailing_quote(current_str);
	current_str = "";
	return result;
} 


// the spirit grammer 
//
class Json_grammer 
	: public grammar<Json_grammer>
{
public:

		Json_grammer(Semantic_actions& semantic_actions)
		:   actions(semantic_actions)
		{
		}

		template< typename ScannerT >
		struct definition
		{
				definition( const Json_grammer& self )
				{
						// need to convert the semantic action class methods to functors with the 
						// parameter signature expected by spirit

						typedef function< void( char )                     > Char_action;
						typedef function< void( const char*, const char* ) > Str_action;
						typedef function< void( double )                   > Real_action;
						typedef function< void( int )                      > Int_action;

						Char_action begin_obj     ( bind( &Semantic_actions::begin_obj,    &self.actions, _1 ) );
						Char_action end_obj       ( bind( &Semantic_actions::end_obj,      &self.actions, _1 ) );
						Char_action begin_array   ( bind( &Semantic_actions::begin_array,  &self.actions, _1 ) );
						Char_action end_array     ( bind( &Semantic_actions::end_array,    &self.actions, _1 ) );
						Char_action new_c_esc_ch  ( bind( &Semantic_actions::new_c_esc_ch, &self.actions, _1 ) );
						Str_action  new_name      ( bind( &Semantic_actions::new_name,     &self.actions, _1, _2 ) );
						Str_action  new_str       ( bind( &Semantic_actions::new_str,      &self.actions, _1, _2 ) );
						Str_action  new_true      ( bind( &Semantic_actions::new_true,     &self.actions, _1, _2 ) );
						Str_action  new_false     ( bind( &Semantic_actions::new_false,    &self.actions, _1, _2 ) );
						Str_action  new_null      ( bind( &Semantic_actions::new_null,     &self.actions, _1, _2 ) );
						Real_action new_real      ( bind( &Semantic_actions::new_real,     &self.actions, _1 ) );
						Int_action  new_int       ( bind( &Semantic_actions::new_int,      &self.actions, _1 ) );

						json
								= ( object | array ) >> end_p
								;

						object
								= confix_p
									( 
											ch_p('{')[ begin_obj ], 
											*members, 
											ch_p('}')[ end_obj ] 
									)
								;

						members
								= pair >> *( ',' >> pair )
								;

						pair
								= string[ new_name ] 
								>> ':' 
								>> value
								;

						value
								= string[ new_str ] 
								| number
								| object
								| array 
								| str_p( "true" ) [ new_true  ] 
								| str_p( "false" )[ new_false ] 
								| str_p( "null" ) [ new_null  ]
								;

						array
								= confix_p
									( 
											ch_p('[')[ begin_array ], 
											*list_p( elements, ',' ), 
											ch_p(']')[ end_array ] 
									)
								;

						elements
								= value >> *( ',' >> value )
								;

						string
								= lexeme_d // this causes white space inside a string to be retained
									[
											confix_p
											( 
													'"', *c_escape_ch_p[ new_c_esc_ch ],  '"' 
											) |
											confix_p
											( 
													'\'', *c_escape_ch_p[ new_c_esc_ch ],  '\'' 
											) 
									]
								;

						number
								= strict_real_p[ new_real ] 
								| int_p        [ new_int  ]
								;
				}

				rule< ScannerT > json, object, members, pair, array, elements, value, string, number;

				const rule< ScannerT >& start() const { return json; }
		};

		Semantic_actions& actions;
};

bool JsonDb_ParseJsonExpression(JsonDb::TransactionHandle &transaction, std::string const &expression, ValuePointer root)
{
	Semantic_actions semantic_actions(transaction, root);
	parse_info<> info = parse(expression.c_str(), Json_grammer(semantic_actions), space_p);
	return info.full;
}

