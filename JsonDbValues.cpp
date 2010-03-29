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

void ValueNull::Print(JsonDb::TransactionHandle &transaction, std::ostream &output, unsigned int indent_level) const
{
	output << "null";
}

void ValueNull::Serialize(std::ostream &output) const
{
	unsigned char type = VALUE_NULL;	
	output.write((char *)&type, sizeof(unsigned char));
}


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


void ValueNumberReal::Serialize(std::ostream &output) const
{
	unsigned char type = VALUE_NUMBER_REAL;	
	output.write((char *)&type, sizeof(unsigned char));
	output.write((char *)&value, sizeof(Type));
}

void ValueNumberReal::Print(JsonDb::TransactionHandle &transaction, std::ostream &output, unsigned int indent_level) const
{
	output << (boost::format("%.1f") % value);
}


void ValueNumberBoolean::Serialize(std::ostream &output) const
{
	unsigned char type = VALUE_NUMBER_BOOL;	
	output.write((char *)&type, sizeof(unsigned char));
	output.write((char *)&value, sizeof(Type));
}

void ValueNumberBoolean::Print(JsonDb::TransactionHandle &transaction, std::ostream &output, unsigned int indent_level) const
{
	output << (value ? "true" : "false");
}


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


void ValueArray::Serialize(std::ostream &output) const
{
	unsigned char type = VALUE_ARRAY;	
	output.write((char *)&type, sizeof(unsigned char));
	unsigned int entries = values.size();
	output.write((char *)&entries, sizeof(unsigned int));

	if(entries > 0)
		output.write((char *)&values[0], sizeof(ValueKey) * entries);
}

void ValueArray::Print(JsonDb::TransactionHandle &transaction, std::ostream &output, unsigned int indent_level) const
{
	output << "[";
	for(Type::const_iterator i = values.begin(); i != values.end(); ++i)
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

void ValueArray::Append(JsonDb::TransactionHandle &transaction, ValueKey key)
{
	values.push_back(key);
	transaction->Store(GetKey(), shared_from_this());
}

void ValueArray::Delete(JsonDb::TransactionHandle &transaction)
{
	// Delete this element
	transaction->Delete(GetKey());

	// Delete all sub elements
	for(Type::const_iterator i = values.begin(); i != values.end(); ++i)
		transaction->Retrieve(*i)->Delete(transaction);
}

void ValueArray::Walk(JsonDb::TransactionHandle &transaction, std::set<ValueKey> &keys)
{
	keys.insert(GetKey());
	for(Type::const_iterator i = values.begin(); i != values.end(); ++i)
	 	transaction->Retrieve(*i)->Walk(transaction, keys);
}


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
		transaction->Retrieve(i->second)->Delete(transaction);
}

void ValueObject::Delete(JsonDb::TransactionHandle &transaction, ValuePointer const &element)
{
	// Delete all sub elements
	for(Type::iterator i = values.begin(); i != values.end(); ++i)
	{
		if(i->second == element->GetKey())
		{
			// Delete the element
			transaction->Retrieve(i->second)->Delete(transaction);

			// Remove element from list
			values.erase(i);

			// Store the current element
			transaction->Store(GetKey(), shared_from_this());

			// Done
			return;
		}
	}
}

void ValueObject::Walk(JsonDb::TransactionHandle &transaction, std::set<ValueKey> &keys)
{
	keys.insert(GetKey());
	for(Type::const_iterator i = values.begin(); i != values.end(); ++i)
	 	transaction->Retrieve(i->second)->Walk(transaction, keys);
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

		case Value::VALUE_NUMBER_REAL:
		{
			ValueNumberReal::Type value;
			input.read((char *)&value, sizeof(value));
			return ValuePointer(new ValueNumberReal(key, value));
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
				input.read((char *)&value[0], len);
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
			if(entries > 0)
				input.read((char *)&values[0], sizeof(ValueKey) * entries);
			
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

