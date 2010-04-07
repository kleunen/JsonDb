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
#include "JsonDbPathParser.h"

#include <boost/config/warning_disable.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>

#include <string>

template <typename Iterator>
struct JsonPathGrammar
	: boost::spirit::qi::grammar<Iterator>
{
	void PushNameChar(char c)
	{
		name += c;
	}

	void PushNameEscapeChar(char c)
	{
		switch(c)
		{
 			case 'b': name += '\b';     break;
			case 't': name += '\t';     break;
			case 'n': name += '\n';     break;
			case 'f': name += '\f';     break;
			case 'r': name += '\r';     break;
 			case '"': name += '"';      break;
 			case '\'': name += '\'';      break;
			case '\\': name += '\\';    break; 
		}
	}

	void HandleName()
	{
		if(parent && child)
		{
			parent = child;
			child = parent->Get(transaction, name, not_exists_resolution);
		}

		name = "";
	}

	void HandleNumber(int index)
	{
		if(parent && child)
		{
			parent = child;
			child = parent->Get(transaction, index);
		}
	}

	JsonPathGrammar(JsonDb::TransactionHandle &_transaction, ValuePointer root, NotExistsResolution _not_exists_resolution)
		: JsonPathGrammar::base_type(start)
		, parent(root), child(root)
		, transaction(_transaction)
		, not_exists_resolution(_not_exists_resolution)
	{
    namespace qi = boost::spirit::qi;
    namespace ascii = boost::spirit::ascii;
		using qi::int_;

		number = (int_) [ boost::bind(&JsonPathGrammar<Iterator>::HandleNumber, this, _1) ];

		name_char_no_singlequote  = 
			((ascii::char_ - '\'' - '\\') [ boost::bind(&JsonPathGrammar<Iterator>::PushNameChar, this, _1) ] || 
			 ('\\' >> ascii::char_[ boost::bind(&JsonPathGrammar<Iterator>::PushNameEscapeChar, this, _1) ]));

		single_quote_escaped_name = 
			'\'' >> 
				(+name_char_no_singlequote) [ boost::bind(&JsonPathGrammar<Iterator>::HandleName, this) ] >>
			'\'';
		
		name_char_no_doublequote = 
			((ascii::char_ - '\"' - '\\') [ boost::bind(&JsonPathGrammar<Iterator>::PushNameChar, this, _1) ] || 
			 ('\\' >> ascii::char_[ boost::bind(&JsonPathGrammar<Iterator>::PushNameEscapeChar, this, _1) ]));

		double_quote_escaped_name = 
			'"' >> 
			(+name_char_no_doublequote) [ boost::bind(&JsonPathGrammar<Iterator>::HandleName, this) ] >>
			'"';

		alpha_char_name = (ascii::alpha) [ boost::bind(&JsonPathGrammar<Iterator>::PushNameChar, this, _1) ];
		alnum_char_name = 
			(ascii::alnum [ boost::bind(&JsonPathGrammar<Iterator>::PushNameChar, this, _1) ] ) || 
			(ascii::char_('_') [ boost::bind(&JsonPathGrammar<Iterator>::PushNameChar, this, _1) ] );

		unquoted_name = 
			(alpha_char_name >> *alnum_char_name)  [ boost::bind(&JsonPathGrammar<Iterator>::HandleName, this) ];

		start = 
			"$" >> 
					*( 
						('.' >> unquoted_name)|| 
						('[' >> 
						 ( number || single_quote_escaped_name || double_quote_escaped_name ) >> 
						 ']')
					);
		
	}

	JsonDb::TransactionHandle &transaction;
	NotExistsResolution not_exists_resolution;

	ValuePointer parent;
 	ValuePointer child;

	ValuePointer GetParent() { return parent; }
	ValuePointer GetChild() { return child; }

	boost::spirit::qi::rule<Iterator> number;
	boost::spirit::qi::rule<Iterator> unquoted_name;
	boost::spirit::qi::rule<Iterator> alpha_char_name;
	boost::spirit::qi::rule<Iterator> alnum_char_name;
	boost::spirit::qi::rule<Iterator> single_quote_escaped_name;
	boost::spirit::qi::rule<Iterator> name_char_no_singlequote;
	boost::spirit::qi::rule<Iterator> double_quote_escaped_name;
	boost::spirit::qi::rule<Iterator> name_char_no_doublequote;

	boost::spirit::qi::rule<Iterator> start;

	std::string name;
	std::string current_path;
};

std::pair<ValuePointer, ValuePointer> JsonDb_ParseJsonPathExpression(JsonDb::TransactionHandle &transaction, std::string const &expression, ValuePointer root, NotExistsResolution not_exists_resolution)
{
	JsonPathGrammar<std::string::const_iterator> grammar(transaction, root, not_exists_resolution);

	std::string::const_iterator iter = expression.begin();
	std::string::const_iterator end = expression.end();

	bool result;
	try
	{
		result = parse(iter, end, grammar) && (iter == end);
	} catch(std::runtime_error &e)
	{
		throw std::runtime_error((boost::format("Parser error at for path: '%s', message: '%s'") % expression % e.what()).str());
	}

	if(result)
		return std::make_pair<ValuePointer, ValuePointer>(grammar.GetParent(), grammar.GetChild());
	else
		throw std::runtime_error((boost::format("Invalid path specified: %s") % expression).str());
}

