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

#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/tokenizer.hpp>
#include <boost/filesystem/operations.hpp>

#include <iostream>
#include <sstream>

#include <map>
#include <deque>

class CharPtr
	: public boost::shared_ptr<char>
{
public:
	CharPtr(char *object = NULL)
		: boost::shared_ptr<char>(object, cbfree)
	{ }
};

static void CloseDatabase(VILLA *villa)
{
	if(villa != NULL)
		vlclose(villa);
}

JsonDb::Transaction::Transaction(std::string const &filename, ValuePointer const &_null_element)
{
	/* The null element */
	null_element = _null_element;

  /* open the database */
	db = StorageDbPointer(vlopen(filename.c_str(), VL_OWRITER | VL_OCREAT, VL_CMPINT), CloseDatabase);

	/* Start the transaction */
	vltranbegin(db.get());

	if(db.get() == NULL)
    throw std::runtime_error((boost::format("Failed to open database: %s") % dperrmsg(dpecode)).str().c_str());
	
	ValuePointer root = Retrieve(root_key);
	if(root.get() == NULL)
	{
		root = ValuePointer(new ValueObject(root_key));
		Store(root->GetKey(), root);
	}

	ValuePointer next_id_value = Retrieve(next_id_key);
	if(next_id_value.get() == NULL)
	{
		// Initialize values
		next_id = initial_next_id;
		start_next_id = next_id;
	} else
	{
		// Retrieve values from database
		next_id = next_id_value->GetValueInt();
		start_next_id = next_id;
	}

	// std::cout << "Start transaction, next id: " << next_id << std::endl;
}

void JsonDb::Transaction::Store(ValueKey key, ValuePointer value)
{
	if(key != null_key)
	{
		std::ostringstream output;
		value->Serialize(output);

		std::string output_string = output.str();
		vlput(db.get(), (char const *)&key, sizeof(ValueKey), &output_string[0], output_string.size(), VL_DOVER);
		// std::cout << "Store: key=" << key << std::endl;
	}
}

ValuePointer JsonDb::Transaction::Retrieve(ValueKey key)
{
	if(key == null_key)
		return null_element;

	// std::cout << "Retrieve: key=" << key << std::endl;

	// Then retrieve the actual data
	int value_size;
	CharPtr val = CharPtr(vlget(db.get(), (char const *)&key, sizeof(ValueKey), &value_size));

	if(val.get() == NULL)
		return ValuePointer();

	if(value_size <= 0)
		throw std::runtime_error((boost::format("Element has an invalid size: %d") % key).str().c_str());

	std::string val_str(val.get(), value_size);
	std::istringstream input(val_str);
	ValuePointer result = Value::Unserialize(key, input);	

	return result;
}

void JsonDb::Transaction::Delete(ValueKey key)
{
	vlout(db.get(), (char const *)&key, sizeof(ValueKey));

	// std::cout << "Delete: key=" << key << std::endl;
}

void JsonDb::Transaction::Commit()
{
	if(next_id != start_next_id)
	{
		Store(next_id_key, ValuePointer(new ValueNumberInteger(next_id_key, next_id)));

		// std::cout << "Commit transaction, next id: " << next_id << std::endl;
	} 

	vltrancommit(db.get());

	start_next_id = next_id;
}

std::set<ValueKey> JsonDb::Transaction::Walk()
{
	std::set<ValueKey> keys;

 	// initialize the iterator 
  if(!vlcurfirst(db.get()))
		throw std::runtime_error("Failed to initialize database iterator");

	CharPtr key;
	while((key = CharPtr(vlcurkey(db.get(), NULL))) != NULL)
	{
		keys.insert(*(ValueKey const *)key.get());
		vlcurnext(db.get());
	}

	return keys;
}

JsonDb::JsonDb(std::string const &_filename)
	: filename(_filename)
{ 
	null_element = ValuePointer(new ValueNull(null_key));
}

void JsonDb::Set(TransactionHandle &transaction, std::string const &path, ValuePointer new_value, bool create_if_not_exists)
{
	std::pair<ValuePointer, ValuePointer> old_value = Get(transaction, path, create_if_not_exists ? create : throw_exception);
	new_value->SetKey(old_value.second->GetKey());
	old_value.second->Delete(transaction);
	transaction->Store(new_value->GetKey(), new_value);
}

void JsonDb::Set(TransactionHandle &transaction, std::string const &path, int value, bool create_if_not_exists)
{
	Set(transaction, path, ValuePointer(new ValueNumberInteger(null_key, value)), create_if_not_exists);
}

void JsonDb::Set(TransactionHandle &transaction, std::string const &path, std::string const &value, bool create_if_not_exists)
{
	Set(transaction, path, ValuePointer(new ValueString(null_key, value)), create_if_not_exists);
}

void JsonDb::Set(TransactionHandle &transaction, std::string const &path, float value, bool create_if_not_exists)
{
	Set(transaction, path, ValuePointer(new ValueNumberFloat(null_key, value)), create_if_not_exists);
}

void JsonDb::Set(TransactionHandle &transaction, std::string const &path, bool value, bool create_if_not_exists)
{
	Set(transaction, path, ValuePointer(new ValueNumberBoolean(null_key, value)), create_if_not_exists);
}

void JsonDb::SetArray(TransactionHandle &transaction, std::string const &path, size_t total_elements, bool create_if_not_exists)
{
	ValueArray::Type elements(total_elements);
	for(ValueArray::Type::iterator i = elements.begin(); i != elements.end(); ++i)
	{
		ValuePointer new_element(new ValueNull(transaction->GenerateKey()));
		*i = new_element->GetKey();
		transaction->Store(new_element->GetKey(), new_element);
	}

	Set(transaction, path, ValuePointer(new ValueArray(null_key, elements)), create_if_not_exists);
}

void JsonDb::SetJson(TransactionHandle &transaction, std::string const &path, std::string const &value, bool create_if_not_exists)
{
	std::pair<ValuePointer, ValuePointer> old_value = Get(transaction, path, create_if_not_exists ? create : throw_exception);
	JsonDb_ParseJsonExpression(transaction, value, old_value.second);
	//Set(transaction, path, ValuePointer(new ValueNumberBoolean(null_key, value)), create_if_not_exists);
}

std::pair<ValuePointer, ValuePointer> JsonDb::Get(TransactionHandle &transaction, std::string const &path, NotExistsResolution not_exists_resolution)
{
	ValuePointer parent = transaction->GetRoot();
	ValuePointer element = transaction->GetRoot();
	std::string current_path;

	if(path.empty())
			throw std::runtime_error((boost::format("Invalid path specified: %s, path should not be empty") % path).str());

	boost::tokenizer< boost::char_separator<char> > tokens(path,  boost::char_separator<char>("."));
	for(boost::tokenizer< boost::char_separator<char> >::iterator i = tokens.begin(); i != tokens.end(); ++i)
	{
		if(i->empty())
			throw std::runtime_error((boost::format("Invalid path specified: %s") % path).str());

		std::string const &name = *i;

		size_t index_start_pos = name.find('[');
		if(index_start_pos != std::string::npos)
		{
			std::string element_name_str = name.substr(0, index_start_pos);

			parent = element;
			element = element->Get(transaction, current_path, element_name_str, not_exists_resolution);
			current_path += (i != tokens.begin() ? "." : "") + element_name_str;

			if(element == NULL)
				return std::make_pair<ValuePointer, ValuePointer>(parent, element);

			while(true)
			{
				size_t index_end_pos = name.find(']', index_start_pos);
				if(index_end_pos == std::string::npos)
					throw std::runtime_error((boost::format("Invalid path specified: %s") % path).str());
				std::string element_index_str = name.substr(index_start_pos + 1, index_end_pos - 1 - index_start_pos);

				size_t index;
				try 
				{
					index = boost::lexical_cast<size_t>(element_index_str);
				} catch(boost::bad_lexical_cast &e)
				{
					throw std::runtime_error((boost::format("Invalid path specified: %s") % path).str());
				}

				parent = element;
				element = element->Get(transaction, current_path, index);
				current_path += element_index_str;

				index_start_pos = index_end_pos + 1;
				if(index_start_pos == name.size())
					break;
				if(name[index_start_pos] != '[')
					throw std::runtime_error((boost::format("Invalid path specified: %s") % path).str());
			}

		} else
		{
			parent = element;
			element = element->Get(transaction, path, name, not_exists_resolution);
			current_path += (i != tokens.begin() ? "." : "") + name;

			if(element == NULL)
				return std::make_pair<ValuePointer, ValuePointer>(parent, element);
		}
	}
	
	return std::make_pair<ValuePointer, ValuePointer>(parent, element);
}

int JsonDb::GetInt(TransactionHandle &transaction, std::string const &path)
{
	return Get(transaction, path, throw_exception).second->GetValueInt();
}

std::string JsonDb::GetString(TransactionHandle &transaction, std::string const &path)
{
	return Get(transaction, path, throw_exception).second->GetValueString();
}

bool JsonDb::GetBool(TransactionHandle &transaction, std::string const &path)
{
	return Get(transaction, path, throw_exception).second->GetValueBoolean();
}

float JsonDb::GetFloat(TransactionHandle &transaction, std::string const &path)
{
	return Get(transaction, path, throw_exception).second->GetValueFloat();
}

bool JsonDb::Exists(TransactionHandle &transaction, std::string const &path) 
{
	return Get(transaction, path, return_null).second != NULL;
}

void JsonDb::Delete(TransactionHandle &transaction, ValuePointer value)
{
	if(value != NULL)
		value->Delete(transaction);
}

void JsonDb::Delete(TransactionHandle &transaction, std::string const &path)
{
	std::pair<ValuePointer, ValuePointer> element = Get(transaction, path, return_null);
	element.first->Delete(transaction, element.second);
}

void JsonDb::Print(TransactionHandle &transaction, std::ostream &output)
{
	transaction->GetRoot()->Print(transaction, output, 1);
}

std::set<ValueKey> JsonDb::WalkTree(TransactionHandle &transaction)
{
	std::set<ValueKey> result;
	result.insert(transaction->GetRoot()->GetKey());
	transaction->GetRoot()->Walk(transaction, result);
	return result;
}

void JsonDb::Delete()
{
	// Remove the database directory and all it's subdirectories
	boost::filesystem::remove_all(boost::filesystem::path(filename));	
}

bool JsonDb::Validate(TransactionHandle &transaction)
{
	std::set<ValueKey> tree_keys = WalkTree(transaction);
	std::set<ValueKey> db_keys = transaction->Walk();

	// Item storing next id is a valid item
	if(db_keys.find(next_id_key) != db_keys.end())
		tree_keys.insert(next_id_key);

	std::set<ValueKey> db_missing_keys;
	std::set_difference(
			tree_keys.begin(), tree_keys.end(),
			db_keys.begin(), db_keys.end(),
			std::inserter(db_missing_keys, db_missing_keys.begin()));

	std::set<ValueKey> db_stale_keys;
	std::set_difference(
			db_keys.begin(), db_keys.end(),
			tree_keys.begin(), tree_keys.end(),
			std::inserter(db_stale_keys, db_stale_keys.begin()));

	std::cout << "Keys found in tree: " << std::endl;
	for(std::set<ValueKey>::const_iterator i = tree_keys.begin(); i != tree_keys.end(); ++i)
		std::cout << (i != tree_keys.begin() ? ", " : "") << *i;
	std::cout << std::endl;

	std::cout << "Keys found in database: " << std::endl;
	for(std::set<ValueKey>::const_iterator i = db_keys.begin(); i != db_keys.end(); ++i)
		std::cout << (i != db_keys.begin() ? ", " : "") << *i;
	std::cout << std::endl;

	if(!db_missing_keys.empty())
	{
		std::cout << "Following keys are missing from the database: " << std::endl;

		for(std::set<ValueKey>::const_iterator i = db_missing_keys.begin(); i != db_missing_keys.end(); ++i)
			std::cout << (i != db_missing_keys.begin() ? ", " : "") << *i;

		std::cout << std::endl;
	}

	if(!db_stale_keys.empty())
	{
		std::cout << "Following stale keys where found in the database: " << std::endl;

		for(std::set<ValueKey>::const_iterator i = db_stale_keys.begin(); i != db_stale_keys.end(); ++i)
			std::cout << (i != db_stale_keys.begin() ? ", " : "") << *i;

		std::cout << std::endl;
	}

	return (db_missing_keys.empty() && db_stale_keys.empty());
}

