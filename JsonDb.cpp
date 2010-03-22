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

#include <boost/enable_shared_from_this.hpp>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/tokenizer.hpp>

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

class Value
	: public boost::enable_shared_from_this<Value>
{
protected:
	ValueKey key;

	static std::string Indent(unsigned int level) 
	{
		return std::string(level * 2, ' ');
	}

public:
	enum 
	{
		VALUE_NUMBER_INTEGER	=	0x10,
		VALUE_NUMBER_FLOAT		= 0x20,
		VALUE_NUMBER_BOOL			= 0x30,
		VALUE_STRING					= 0x40,
		VALUE_ARRAY						=	0x50,
		VALUE_OBJECT					= 0x60,
		VALUE_NULL						= 0x70
	} ValueTypeId;

	Value(ValueKey _key)
		: key(_key)
	{ }

	virtual ~Value()
	{ }

	// Get the key value
	ValueKey GetKey() const { return key; }

	// Set the key value
	void SetKey(ValueKey key) { this->key = key; }

	// Return the type identifier
	virtual char const *GetTypeString() const = 0;

	// Return as integer
	virtual int GetValueInt() const
	{
		throw std::runtime_error((boost::format("Failed to convert object to integer, item is of type '%s'") % GetTypeString()).str().c_str());
	}

	// Return as float
	virtual float GetValueFloat() const 
	{
		throw std::runtime_error((boost::format("Failed to convert object to float, item is of type '%s'") % GetTypeString()).str().c_str());
	}

	// Return as boolean
	virtual bool GetValueBoolean() const
	{
		throw std::runtime_error((boost::format("Failed to convert object to boolean, item is of type '%s'") % GetTypeString()).str().c_str());
	}

	// Return as string
	virtual std::string const &GetValueString() const 
	{
		throw std::runtime_error((boost::format("Failed to convert object to string, item is of type '%s'") % GetTypeString()).str().c_str());
	}	

	// Get entry with the specified path
	virtual ValuePointer Get(JsonDb::TransactionHandle &transaction, std::string const &complete_path, std::string const &path, NotExistsResolution not_exist_resolution) 
	{
		throw std::runtime_error((boost::format("Failed to get subelement '%s' from object '%s', item is of type '%s'") % path % complete_path % GetTypeString()).str().c_str());
	}

	// Get entry with the specified path from element at the specified index
	virtual ValuePointer Get(JsonDb::TransactionHandle &transaction, std::string const &complete_path, size_t index) 
	{
		throw std::runtime_error((boost::format("Failed to get element by index from object '%s', item is of type '%s'") % complete_path % GetTypeString()).str());
	}

	// Print to a stream
	virtual void Print(JsonDb::TransactionHandle &transaction, std::ostream &output, unsigned int indent_level = 0) const = 0;

	// Serialize to a stream
	virtual void Serialize(std::ostream &output) const 
	{
		throw std::runtime_error((boost::format("Failed to serialize object of this type: '%s'") % GetTypeString()).str().c_str());
	}

	// Delete this element
	virtual void Delete(JsonDb::TransactionHandle &transaction)
	{
		transaction->Delete(GetKey());
	}

	// Unserialize from a stream
	static ValuePointer Unserialize(ValueKey key, std::istream &input);
};

// The null element
class ValueNull
	: public Value
{
public:
	typedef int Type;

	ValueNull(ValueKey key)
		: Value(key)
	{ }

	void Serialize(std::ostream &output) const;
	void Print(JsonDb::TransactionHandle &transaction, std::ostream &output, unsigned int indent_level) const;

	char const *GetTypeString() const
	{
		return "Null";
	}
};

void ValueNull::Print(JsonDb::TransactionHandle &transaction, std::ostream &output, unsigned int indent_level) const
{
	output << "null";
}

void ValueNull::Serialize(std::ostream &output) const
{
	unsigned char type = VALUE_NULL;	
	output.write((char *)&type, sizeof(unsigned char));
}

class ValueNumberInteger
	: public Value
{
public:
	typedef int Type;

	ValueNumberInteger(ValueKey key, Type _value = 0)
		: Value(key)
		, value(_value)
	{ }

	void Serialize(std::ostream &output) const;
	void Print(JsonDb::TransactionHandle &transaction, std::ostream &output, unsigned int indent_level) const;

	// Allow reading as integer
	int GetValueInt() const
	{
		return value;
	}

	char const *GetTypeString() const
	{
		return "Integer";
	}

private:
	Type value;
};

void ValueNumberInteger::Serialize(std::ostream &output) const
{
	unsigned char type = VALUE_NUMBER_INTEGER;	
	output.write((char *)&type, sizeof(unsigned char));
	output.write((char *)&value, sizeof(Type));
}

void ValueNumberInteger::Print(JsonDb::TransactionHandle &transaction, std::ostream &output, unsigned int indent_level) const
{
	output << value;
}

class ValueNumberFloat
	: public Value
{

public:
	typedef float Type;

	ValueNumberFloat(ValueKey key, Type _value = 0.0f)
		: Value(key)
		, value(_value)
	{ }

	void Serialize(std::ostream &output) const;
	void Print(JsonDb::TransactionHandle &transaction, std::ostream &output, unsigned int indent_level) const;

	// Allow reading as float
	float GetValueFloat() const
	{
		return value;
	}

	char const *GetTypeString() const
	{
		return "Float";
	}

private:
	Type value;
};

void ValueNumberFloat::Serialize(std::ostream &output) const
{
	unsigned char type = VALUE_NUMBER_FLOAT;	
	output.write((char *)&type, sizeof(unsigned char));
	output.write((char *)&value, sizeof(Type));
}

void ValueNumberFloat::Print(JsonDb::TransactionHandle &transaction, std::ostream &output, unsigned int indent_level) const
{
	output << value;
}

class ValueNumberBoolean
	: public Value
{
public:
	typedef bool Type;

	ValueNumberBoolean(ValueKey key, Type _value = false)
		: Value(key)
		, value(_value)
	{ }

	void Serialize(std::ostream &output) const;
	void Print(JsonDb::TransactionHandle &transaction, std::ostream &output, unsigned int indent_level) const;

	// Allow reading as boolean
	bool GetValueBoolean() const
	{
		return value;
	}

	char const *GetTypeString() const
	{
		return "Boolean";
	}

private:
	Type value;
};

void ValueNumberBoolean::Serialize(std::ostream &output) const
{
	unsigned char type = VALUE_NUMBER_BOOL;	
	output.write((char *)&type, sizeof(unsigned char));
	output.write((char *)&value, sizeof(Type));
}

void ValueNumberBoolean::Print(JsonDb::TransactionHandle &transaction, std::ostream &output, unsigned int indent_level) const
{
	output << (value ? "True" : "False");
}

class ValueString
	: public Value
{
public:
	typedef std::string Type;

	ValueString(ValueKey key, Type _value = Type())
		: Value(key)
		, value(_value)
	{ }

	// Allow serialize and print
	void Serialize(std::ostream &output) const;
	void Print(JsonDb::TransactionHandle &transaction, std::ostream &output, unsigned int indent_level) const;

	// Allow reading as string
	std::string const &GetValueString() const
	{
		return value;
	}

	char const *GetTypeString() const
	{
		return "String";
	}

private:
	Type value;
};

void ValueString::Serialize(std::ostream &output) const
{
	unsigned char type = VALUE_STRING;	
	output.write((char *)&type, sizeof(unsigned char));
	size_t len = value.size();
	output.write((char *)&len, sizeof(size_t));
	output.write(value.c_str(), len);
}

void ValueString::Print(JsonDb::TransactionHandle &transaction, std::ostream &output, unsigned int indent_level) const
{
	output << "\"" << value << "\"";
}

class ValueArray
	: public Value
{
public:
	typedef std::deque<ValueKey> Type;

	ValueArray(ValueKey key, Type _values = Type())
		: Value(key)
		, values(_values)
	{ }

	// Allow serialize and printing
	void Serialize(std::ostream &output) const;
	void Print(JsonDb::TransactionHandle &transaction, std::ostream &output, unsigned int indent_level) const;

	// Allow path functions
	ValuePointer Get(JsonDb::TransactionHandle &transaction, std::string const &complete_path, size_t index);

	// Delete this element and all subelements
	void Delete(JsonDb::TransactionHandle &transaction);

	char const *GetTypeString() const
	{
		return "Array";
	}

private:
	Type values;
};

void ValueArray::Serialize(std::ostream &output) const
{
	unsigned char type = VALUE_ARRAY;	
	output.write((char *)&type, sizeof(unsigned char));
	unsigned int entries = values.size();
	output.write((char *)&entries, sizeof(unsigned int));

	for(std::deque<ValueKey>::const_iterator i = values.begin(); i != values.end(); ++i)
	{
		output.write((char *)&(*i), sizeof(ValueKey));
	}
}

void ValueArray::Print(JsonDb::TransactionHandle &transaction, std::ostream &output, unsigned int indent_level) const
{
	output << "[";
	for(std::deque<ValueKey>::const_iterator i = values.begin(); i != values.end(); ++i)
	{
		if(i != values.begin())
			output << ",";
	 	transaction->Retrieve(*i)->Print(transaction, output, indent_level);
	}
	output << "]";
}

ValuePointer ValueArray::Get(JsonDb::TransactionHandle &transaction, std::string const &complete_path, size_t index)
{
	if(index >= values.size())
		throw std::runtime_error((boost::format("Index out of bound: %d, in array: '%s'") % index % complete_path).str());

	return transaction->Retrieve(values[index]);
}

void ValueArray::Delete(JsonDb::TransactionHandle &transaction)
{
	// Delete this element
	transaction->Delete(GetKey());

	// Delete all sub elements
	for(Type::const_iterator i = values.begin(); i != values.end(); ++i)
		transaction->Delete(*i);
}

class ValueObject
	: public Value
{

public:
	typedef std::map<std::string, ValueKey> Type;

	ValueObject(ValueKey key, Type _values = Type())
		: Value(key)
		, values(_values)
	{ }

	// Allow serialize and printing
	void Serialize(std::ostream &output) const;
	void Print(JsonDb::TransactionHandle &transaction, std::ostream &output, unsigned int indent_level) const;

	// Allow path functions
	ValuePointer Get(JsonDb::TransactionHandle &transaction, std::string const &complete_path, std::string const &path, NotExistsResolution not_exists_resolution);

	// Delete this element and all subelements
	void Delete(JsonDb::TransactionHandle &transaction);

	char const *GetTypeString() const
	{
		return "Object";
	}

private:
	Type values;
};

void ValueObject::Serialize(std::ostream &output) const
{
	unsigned char type = VALUE_OBJECT;	
	output.write((char *)&type, sizeof(unsigned char));
	unsigned int entries = values.size();
	output.write((char *)&entries, sizeof(unsigned int));

	for(std::map<std::string, ValueKey>::const_iterator i = values.begin(); i != values.end(); ++i)
	{
		unsigned int name_length = i->first.size();
		output.write((char *)&name_length, sizeof(name_length));
		output.write(&i->first[0], i->first.size());
		output.write((char *)&i->second, sizeof(ValueKey));
	}
}

void ValueObject::Print(JsonDb::TransactionHandle &transaction, std::ostream &output, unsigned int indent_level) const
{
	output << std::endl << Indent(indent_level - 1) << "{";
	for(std::map<std::string, ValueKey>::const_iterator i = values.begin(); i != values.end(); ++i)
	{
		if(i != values.begin())
			output << "," << std::endl;
		else 
			output << std::endl;

		output << Indent(indent_level) << "\"" << i->first << "\" = ";
	 	transaction->Retrieve(i->second)->Print(transaction, output, indent_level + 1);
	}
	output << std::endl << Indent(indent_level - 1) << "}";
}

ValuePointer ValueObject::Get(JsonDb::TransactionHandle &transaction, std::string const &complete_path, std::string const &path, NotExistsResolution not_exists_resolution)
{
	ValuePointer element_pointer;

	Type::const_iterator i = values.find(path);
	if(i != values.end())
		return transaction->Retrieve(i->second);

	if(not_exists_resolution == throw_exception)
	{
		throw std::runtime_error((boost::format("Path could not be found: %s") % complete_path).str());
	} else if(not_exists_resolution == return_null)
	{
		return ValuePointer();
	}
	
	ValueKey key = transaction->GenerateKey();
	element_pointer = ValuePointer(new ValueObject(key));

	values[path] = element_pointer->GetKey();

	transaction->Store(element_pointer->GetKey(), element_pointer);
	transaction->Store(GetKey(), shared_from_this());

	return element_pointer;
}

void ValueObject::Delete(JsonDb::TransactionHandle &transaction)
{
	// Delete this element
	transaction->Delete(GetKey());

	// Delete all sub elements
	for(Type::const_iterator i = values.begin(); i != values.end(); ++i)
		transaction->Delete(i->second);
}

ValuePointer Value::Unserialize(ValueKey key, std::istream &input) 
{
	unsigned char type;
	input.read((char *)&type, sizeof(unsigned char));
	
	// std::cout << "Type: " << (unsigned int)type << ", object: " << Value::VALUE_OBJECT << std::endl;
	switch(type)
	{
		case Value::VALUE_NUMBER_INTEGER:
		{
			ValueNumberInteger::Type value;
			input.read((char *)&value, sizeof(value));
			return ValuePointer(new ValueNumberInteger(key, value));
		}

		case Value::VALUE_NUMBER_FLOAT:
		{
			ValueNumberFloat::Type value;
			input.read((char *)&value, sizeof(value));
			return ValuePointer(new ValueNumberFloat(key, value));
		}

		case Value::VALUE_NUMBER_BOOL:
		{
			ValueNumberBoolean::Type value;
			input.read((char *)&value, sizeof(value));
			return ValuePointer(new ValueNumberBoolean(key, value));
		}

		case Value::VALUE_STRING:
		{
			size_t len;
			input.read((char *)&len, sizeof(size_t));
			ValueString::Type value(len, (char)0);
			if(len > 0)
				input.read((char *)&value[0], sizeof(value));
			return ValuePointer(new ValueString(key, value));
		}

		case Value::VALUE_NULL:
		{
			return ValuePointer(new ValueNull(key));
		}

		case Value::VALUE_ARRAY:
		{
			unsigned int entries;
			input.read((char *)&entries, sizeof(entries));
			
			ValueArray::Type values(entries);
			for(unsigned int i = 0; i < entries; ++i)
			{
				ValueKey value;
				input.read((char *)&value, sizeof(value));
				values[i] = value;
			}
			
			return ValuePointer(new ValueArray(key, values));
		}

		case Value::VALUE_OBJECT:
		{
			unsigned int entries;
			input.read((char *)&entries, sizeof(entries));
			
			ValueObject::Type values;
			for(unsigned int i = 0; i < entries; ++i)
			{
				unsigned int name_length;
				input.read((char *)&name_length, sizeof(name_length));

				std::string name((size_t)name_length, (char)0);
				input.read(&name[0], name_length);

				ValueKey value;
				input.read((char *)&value, sizeof(value));

				values[name] = value;
			}

			return ValuePointer(new ValueObject(key, values));
		}
	}; 

	throw std::runtime_error("Failed to unserialize database entry");
}

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

JsonDb::JsonDb(std::string const &_filename)
	: filename(_filename)
{ 
	null_element = ValuePointer(new ValueNull(null_key));
}

void JsonDb::Set(TransactionHandle &transaction, std::string const &path, ValuePointer new_value, bool create_if_not_exists)
{
	ValuePointer old_value = Get(transaction, path, create_if_not_exists ? create : throw_exception);
	Delete(transaction, old_value);
	new_value->SetKey(old_value->GetKey());
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

ValuePointer JsonDb::Get(TransactionHandle &transaction, std::string const &path, NotExistsResolution not_exists_resolution)
{
	ValuePointer element = transaction->GetRoot();
	std::string current_path;

	boost::tokenizer< boost::char_separator<char> > tokens(path,  boost::char_separator<char>("."));
	for(boost::tokenizer< boost::char_separator<char> >::iterator i = tokens.begin(); i != tokens.end(); ++i)
	{
		if(i->empty())
			throw std::runtime_error((boost::format("Invalid path specified: %s") % path).str());

		std::string name = *i;

		size_t index_start_pos = i->find('[');
		if(index_start_pos != std::string::npos)
		{
			if(name[name.size() - 1] != ']')
				throw std::runtime_error((boost::format("Invalid path specified: %s") % path).str());
			
			std::string element_name_str = name.substr(0, index_start_pos);

			std::string element_index_str = name.substr(index_start_pos + 1, name.size() - 2 - index_start_pos);

			size_t index;
			try 
			{
				index = boost::lexical_cast<size_t>(element_index_str);
			} catch(boost::bad_lexical_cast &e)
			{
				throw std::runtime_error((boost::format("Invalid path specified: %s") % path).str());
			}

			element = element->Get(transaction, current_path, element_name_str, not_exists_resolution);
			current_path += (i != tokens.begin() ? "." : "") + element_name_str;

			if(element == NULL)
				return element;

			element = element->Get(transaction, current_path, index);
			current_path += element_index_str;
		} else
		{
			element = element->Get(transaction, path, name, not_exists_resolution);
			current_path += (i != tokens.begin() ? "." : "") + name;

			if(element == NULL)
				return element;
		}
	}
	
	return element;
}

int JsonDb::GetInt(TransactionHandle &transaction, std::string const &path)
{
	return Get(transaction, path, throw_exception)->GetValueInt();
}

std::string const &JsonDb::GetString(TransactionHandle &transaction, std::string const &path)
{
	return Get(transaction, path, throw_exception)->GetValueString();
}

bool JsonDb::GetBool(TransactionHandle &transaction, std::string const &path)
{
	return Get(transaction, path, throw_exception)->GetValueBoolean();
}

float JsonDb::GetFloat(TransactionHandle &transaction, std::string const &path)
{
	return Get(transaction, path, throw_exception)->GetValueFloat();
}

bool JsonDb::Exists(TransactionHandle &transaction, std::string const &path) 
{
	return Get(transaction, path, return_null) != NULL;
}

void JsonDb::Delete(TransactionHandle &transaction, ValuePointer value)
{
	if(value != NULL)
		value->Delete(transaction);
}

void JsonDb::Delete(TransactionHandle &transaction, std::string const &path)
{
//	Delete(transaction, Get(transaction, path, return_null));
}

void JsonDb::Print(TransactionHandle &transaction, std::ostream &output)
{
	output << "root:";
	transaction->GetRoot()->Print(transaction, output, 1);
}

