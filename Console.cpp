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

#include <sstream>
#include <iostream>
#include <readline/readline.h>
#include <readline/history.h>
#include <boost/tokenizer.hpp>

bool quit = false;

/* Read a string, and return a pointer to it.
   Returns NULL on EOF. */
std::string rl_gets (char const *prompt)
{
  /* Get a line from the user. */
  char *line_read = readline (prompt);
	if(line_read == NULL)
		quit = true;

  /* If the line has any text in it,
     save it on the history. */
  if (line_read && *line_read)
	 	add_history (line_read);

	std::string result;

	if(line_read)
	{
		result = std::string(line_read);
		free(line_read);
	}

  return result;
}

void Help()
{
	std::cout << "Valid commands are:" << std::endl;
	std::cout << "get <path>            - Print value of the specified path" << std::endl;
	std::cout << "put <path> <value>    - Set path to the specified json value" << std::endl;
	std::cout << "delete <path>         - Delete specified value from database" << std::endl;
	std::cout << "append <path> <value> - Append value to array" << std::endl;
	std::cout << "quit                  - Exit" << std::endl;
	std::cout << std::endl;
	std::cout << "Examples: " << std::endl;
	std::cout << "put $.a.b.c.d { 'e':'Hello world', 'f':10 }" << std::endl;
	std::cout << "get $.a.b.c.d" << std::endl;
	std::cout << "delete $.a.b.c.d" << std::endl;
	std::cout << "put $.a.b.c.array [10, 20, 30]" << std::endl;
	std::cout << "append $.a.b.c.array 40" << std::endl;
	std::cout << "quit" << std::endl;
}

int main(int argc, char **argv)
{
	std::cout << "JsonDbConsole" << std::endl;

	if(argc != 2)
	{
		std::cout << "Usage: jsondb_console <dbname>" << std::endl;
		return 0;
	}

	std::cout << "Opening database: " << argv[1] << std::endl;
	JsonDb json_db(argv[1]);

	while(!quit)
	{
		std::string line = rl_gets("> ");
	
		if(line.empty())
			continue;

		try
		{
			boost::char_separator<char> separator(" ");
			boost::tokenizer<boost::char_separator<char> > tokenizer(line, separator);

			std::vector<std::string> tokens;
			std::copy(tokenizer.begin(), tokenizer.end(), std::back_inserter(tokens));
			size_t tokens_count = tokens.size();

			if(tokens_count == 1 && tokens[0] == "quit")
			{
				quit = true;
			} else if(tokens_count == 2 && tokens[0] == "get")
			{
				JsonDb::TransactionHandle transaction = json_db.StartTransaction();
				json_db.Print(transaction, tokens[1], std::cout);
				std::cout << std::endl;
			} else if(tokens_count == 2 && tokens[0] == "delete")
			{
				JsonDb::TransactionHandle transaction = json_db.StartTransaction();
				json_db.Delete(transaction, tokens[1]);
			} else if(tokens_count >= 3 && tokens[0] == "put")
			{
				std::string value;
				for(std::vector<std::string>::const_iterator i = tokens.begin() + 2; i != tokens.end(); ++i)
					value += " " + *i;

				JsonDb::TransactionHandle transaction = json_db.StartTransaction();
				json_db.SetJson(transaction, tokens[1], value);
			} else if(tokens_count >= 3 && tokens[0] == "append")
			{
				std::string value;
				for(std::vector<std::string>::const_iterator i = tokens.begin() + 2; i != tokens.end(); ++i)
					value += " " + *i;

				JsonDb::TransactionHandle transaction = json_db.StartTransaction();
				json_db.AppendArrayJson(transaction, tokens[1], value);
			} else if(tokens_count == 1 && tokens[0] == "help")
			{
				Help();
			} else
			{
				std::cout << "An invalid command was specified" << std::endl << std::endl;
				Help();
			}
		} catch(std::runtime_error &e)
		{
			std::cout << "Error occurred while handling request: " << e.what() << std::endl;
		}
	}

	return 0;
}

