#ifndef __json_db_values_h__
#define __json_db_values_h__

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

#include <boost/enable_shared_from_this.hpp>
#include <boost/format.hpp>

#include <deque>
#include <iostream>

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
	enum ValueTypeId
	{
		VALUE_NUMBER_INTEGER	=	0x10,
		VALUE_NUMBER_FLOAT		= 0x20,
		VALUE_NUMBER_BOOL			= 0x30,
		VALUE_STRING					= 0x40,
		VALUE_ARRAY						=	0x50,
		VALUE_OBJECT					= 0x60,
		VALUE_NULL						= 0x70
	};

	Value(ValueKey _key)
		: key(_key)
	{ }

	virtual ~Value()
	{ }

	// Get the key value
	ValueKey GetKey() const { return key; }

	// Set the key value
	void SetKey(ValueKey key) { this->key = key; }

	// Return the type identifier as a string
	virtual char const *GetTypeString() const = 0;

	// Return the type identifier as a string
	virtual ValueTypeId GetType() const = 0;
	
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
	virtual std::string GetValueString() const 
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
		throw std::runtime_error((boost::format("Failed to get element by index from object '%s', item is of type '%s'") % complete_path % GetTypeString()).str().c_str());
	}

	// Append an item to a list
	virtual ValuePointer Append(JsonDb::TransactionHandle &transaction, ValueKey key) 
	{
		throw std::runtime_error((boost::format("Failed to append element, item is of type '%s'") % GetTypeString()).str().c_str());
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

	// Delete subelement element
	virtual void Delete(JsonDb::TransactionHandle &transaction, ValuePointer const &element)
	{
		throw std::runtime_error((boost::format("Failed to delement subelement from object, item is of type '%s'") % GetTypeString()).str().c_str());
	}

	// Unserialize from a stream
	static ValuePointer Unserialize(ValueKey key, std::istream &input);

	// Walk through the database and retrieve all keys
	virtual void Walk(JsonDb::TransactionHandle &transaction, std::set<ValueKey> &keys)
	{
		keys.insert(GetKey());
	}
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

	ValueTypeId GetType() const 
	{
	 	return VALUE_NULL;
	}
};

// An integer
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

	ValueTypeId GetType() const 
	{
	 	return VALUE_NUMBER_INTEGER;
	}

private:
	Type value;
};

// Floating pointer number
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

	ValueTypeId GetType() const 
	{
	 	return VALUE_NUMBER_FLOAT;
	}
private:
	Type value;
};

// A boolean
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

	ValueTypeId GetType() const 
	{
	 	return VALUE_NUMBER_BOOL;
	}

private:
	Type value;
};

// A string
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
	std::string GetValueString() const
	{
		return value;
	}

	char const *GetTypeString() const
	{
		return "String";
	}

	ValueTypeId GetType() const 
	{
	 	return VALUE_STRING;
	}

private:
	Type value;
};

// An array
class ValueArray
	: public Value
{
public:
	typedef std::vector<ValueKey> Type;

	ValueArray(ValueKey key, Type _values = Type())
		: Value(key)
		, values(_values)
	{ }

	// Allow serialize and printing
	void Serialize(std::ostream &output) const;
	void Print(JsonDb::TransactionHandle &transaction, std::ostream &output, unsigned int indent_level) const;

	// Allow path functions
	ValuePointer Get(JsonDb::TransactionHandle &transaction, std::string const &complete_path, size_t index);

	// Append an item to a list
	ValuePointer Append(JsonDb::TransactionHandle &transaction, ValueKey key);

	// Delete this element and all subelements
	void Delete(JsonDb::TransactionHandle &transaction);

	char const *GetTypeString() const
	{
		return "Array";
	}

	ValueTypeId GetType() const 
	{
	 	return VALUE_ARRAY;
	}

	// Walk through the database and retrieve all keys
	void Walk(JsonDb::TransactionHandle &transaction, std::set<ValueKey> &keys);

private:
	Type values;
};

// An object
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

	// Delete a element from this object
	void Delete(JsonDb::TransactionHandle &transaction, ValuePointer const &element);

	void Walk(JsonDb::TransactionHandle &transaction, std::set<ValueKey> &keys);

	ValueTypeId GetType() const 
	{
	 	return VALUE_OBJECT;
	}

	char const *GetTypeString() const
	{
		return "Object";
	}

private:
	Type values;
};

#endif
