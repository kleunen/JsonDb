#ifndef __json_db_h__
#define __json_db_h__

#include <boost/shared_ptr.hpp>
#include <string>
#include <stdexcept>

#include <depot.h>
#include <curia.h>
#include <vista.h>

#include <map>

class Value;

// Pointer to a value
typedef boost::shared_ptr<Value> ValuePointer;

// Type of the key in the database
typedef unsigned int ValueKey;

// Identifier of the null element
static const ValueKey null_key = 0;

// Identifier of the root element
static const ValueKey root_key = 100;

// Identifier of the next id element
static const ValueKey next_id_key = 101;

// First identifier for a user created id
static const ValueKey initial_next_id = 1000;

enum NotExistsResolution
{
	create,
	throw_exception,
	return_null
};

class JsonDb
{
public:
	class Transaction;

	// Pointer to a database transaction
	typedef boost::shared_ptr<Transaction> TransactionHandle;

	/* Our database transaction used to update the database */
	class Transaction
	{

	public:
		typedef boost::shared_ptr<VILLA> StorageDbPointer;

		Transaction(std::string const &filename, ValuePointer const &_null_element);

		~Transaction()
		{
			Commit();
		}

		// Store a entry in the database
		void Store(ValueKey key, ValuePointer value);

		// Retrieve a entry from the database
		ValuePointer Retrieve(ValueKey key);

		// Delete entry from database
		void Delete(ValueKey key);

		// Commit the transaction
		void Commit();

		// Get the database root entry
		ValuePointer GetRoot()
		{
			return Retrieve(root_key);
		}

		ValueKey GenerateKey()
		{
			return next_id++;
		}

		static TransactionHandle StartTransaction(std::string const &filename, ValuePointer const &null_element)
		{
			return TransactionHandle(new Transaction(filename, null_element));
		}

	private:

		// Id of next item to store in the database
		ValueKey next_id;

		// Value of next_id when we started the transaction
		ValueKey start_next_id;

		// Pointer to the database
		StorageDbPointer db;

		// Our null element
		ValuePointer null_element;
	};

	JsonDb(std::string const &_filename);

	// Set value in database
	void Set(TransactionHandle &transaction, std::string const &path, int value, bool create_if_not_exists = true);
	void Set(TransactionHandle &transaction, std::string const &path, std::string const &value, bool create_if_not_exists = true);
	void Set(TransactionHandle &transaction, std::string const &path, char const *value, bool create_if_not_exists = true)
	{
		std::string value_str(value);
		Set(transaction, path, value_str, create_if_not_exists);
	}

	void Set(TransactionHandle &transaction, std::string const &path, float value, bool create_if_not_exists = true);
	void Set(TransactionHandle &transaction, std::string const &path, bool value, bool create_if_not_exists = true);

	// Create an array with the specified number of elements at the path
	void SetArray(TransactionHandle &transaction, std::string const &path, size_t elements, bool create_if_not_exists = true);

	// Read values from the database
	std::string const &GetString(TransactionHandle &transaction, std::string const &path);
	int GetInt(TransactionHandle &transaction, std::string const &path);
	bool GetBool(TransactionHandle &transaction, std::string const &path);
	float GetFloat(TransactionHandle &transaction, std::string const &path);

	// Returns true if the specified path exists
	bool Exists(TransactionHandle &transaction, std::string const &path);

	// Delete a key from the database
	void Delete(TransactionHandle &transaction, std::string const &key);

	// Pretty-print the database to the specified output stream
	void Print(TransactionHandle &transaction, std::ostream &output);

	// Start a transaction
	TransactionHandle StartTransaction()
	{
		return Transaction::StartTransaction(filename, null_element);
	}

private:

	// Get raw element pointer from database
	ValuePointer Get(TransactionHandle &transaction, std::string const &path, NotExistsResolution not_exists_resolution);

	// Set raw element 
	void Set(TransactionHandle &transaction, std::string const &path, ValuePointer new_value, bool create_if_not_exists);

	// Delete raw element
	void Delete(TransactionHandle &transaction, ValuePointer value);

	// Our database filename
	std::string filename;

	// Our null element
	ValuePointer null_element;
};

#endif