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

#include <boost/format.hpp>
#include <boost/test/unit_test.hpp>

void JsonDb_CreateDatabase()
{
	JsonDb json_db("test.db");
	JsonDb::TransactionHandle transaction = json_db.StartTransaction();

	json_db.Set(transaction, "this.is.a.deep.test.path.int_value", 1);
	BOOST_CHECK(json_db.GetInt(transaction, "this.is.a.deep.test.path.int_value") == 1);
	BOOST_CHECK_THROW(json_db.GetFloat(transaction, "this.is.a.deep.test.path.int_value"), std::runtime_error); 
	BOOST_CHECK_THROW(json_db.GetString(transaction, "this.is.a.deep.test.path.int_value"), std::runtime_error); 
	BOOST_CHECK_THROW(json_db.GetBool(transaction, "this.is.a.deep.test.path.int_value"), std::runtime_error); 
	BOOST_CHECK_THROW(json_db.GetInt(transaction, "this.is.a.deep.test.path.int_value.sub_value"), std::runtime_error); 
	BOOST_CHECK_THROW(json_db.GetInt(transaction, "this.is.a.deep.test.path.int_value[0]"), std::runtime_error); 

	json_db.Set(transaction, "this.is.a.deep.test.path.float_value", 1.1f);
	BOOST_CHECK(json_db.GetFloat(transaction, "this.is.a.deep.test.path.float_value") == 1.1f);
	BOOST_CHECK_THROW(json_db.GetInt(transaction, "this.is.a.deep.test.path.float_value"), std::runtime_error); 
	BOOST_CHECK_THROW(json_db.GetString(transaction, "this.is.a.deep.test.path.float_value"), std::runtime_error); 
	BOOST_CHECK_THROW(json_db.GetBool(transaction, "this.is.a.deep.test.path.float_value"), std::runtime_error); 
	BOOST_CHECK_THROW(json_db.GetInt(transaction, "this.is.a.deep.test.path.float_value.sub_value"), std::runtime_error); 
	BOOST_CHECK_THROW(json_db.GetInt(transaction, "this.is.a.deep.test.path.float_value[0]"), std::runtime_error); 

	json_db.Set(transaction, "this.is.a.deep.test.path.string_value", "test");
	BOOST_CHECK(json_db.GetString(transaction, "this.is.a.deep.test.path.string_value") == "test");
	BOOST_CHECK_THROW(json_db.GetInt(transaction, "this.is.a.deep.test.path.string_value"), std::runtime_error); 
	BOOST_CHECK_THROW(json_db.GetFloat(transaction, "this.is.a.deep.test.path.string_value"), std::runtime_error); 
	BOOST_CHECK_THROW(json_db.GetBool(transaction, "this.is.a.deep.test.path.string_value"), std::runtime_error); 
	BOOST_CHECK_THROW(json_db.GetInt(transaction, "this.is.a.deep.test.path.string_value.sub_value"), std::runtime_error); 
	BOOST_CHECK_THROW(json_db.GetInt(transaction, "this.is.a.deep.test.path.string_value[0]"), std::runtime_error); 

	json_db.Set(transaction, "this.is.a.deep.test.path.bool_value", true);
	BOOST_CHECK(json_db.GetBool(transaction, "this.is.a.deep.test.path.bool_value") == true); 
	BOOST_CHECK_THROW(json_db.GetInt(transaction, "this.is.a.deep.test.path.bool_value"), std::runtime_error); 
	BOOST_CHECK_THROW(json_db.GetFloat(transaction, "this.is.a.deep.test.path.bool_value"), std::runtime_error); 
	BOOST_CHECK_THROW(json_db.GetString(transaction, "this.is.a.deep.test.path.bool_value"), std::runtime_error); 
	BOOST_CHECK_THROW(json_db.GetInt(transaction, "this.is.a.deep.test.path.bool_value.sub_value"), std::runtime_error); 
	BOOST_CHECK_THROW(json_db.GetInt(transaction, "this.is.a.deep.test.path.bool_value[0]"), std::runtime_error); 

	// Create an empty array
	json_db.SetArray(transaction, "this.is.a.deep.test.path.array_value", 5);

	// Set some values in the array
	for(int i = 0; i < 5; ++i)
		json_db.Set(transaction, (boost::format("this.is.a.deep.test.path.array_value[%d]") % i).str(), i * 10);

	// Get some values from the array
	for(int i = 0; i < 5; ++i)
		BOOST_CHECK(json_db.GetInt(transaction, (boost::format(("this.is.a.deep.test.path.array_value[%d]")) % i).str()) == i * 10);
	for(int i = 6; i < 10; ++i)
		BOOST_CHECK_THROW(json_db.GetInt(transaction, (boost::format(("this.is.a.deep.test.path.array_value[%d]")) % i).str()), std::runtime_error);

	// Create some multilevel arrays
	json_db.SetArray(transaction, "this.is.a.deep.test.path.multilevel_array_value", 5);
	for(int i = 0; i < 5; ++i)
		json_db.SetArray(transaction, (boost::format("this.is.a.deep.test.path.multilevel_array_value[%d]") % i).str(), i);

	// Validate values in multilevel array
	for(int i = 0; i < 5; ++i)
		for(int j = 0; j < i; ++j)
			json_db.Set(transaction, (boost::format("this.is.a.deep.test.path.multilevel_array_value[%d][%d]") % i % j).str(), i * j);
	
	// Check exists
	BOOST_CHECK(json_db.Exists(transaction, "this.is.a.deep.test.path") == true);
	BOOST_CHECK(json_db.Exists(transaction, "this.is.a.deep.test.path.array_value[0]") == true);
	BOOST_CHECK(json_db.Exists(transaction, "this.is.a.deep.test.path.array_value") == true);
	BOOST_CHECK(json_db.Exists(transaction, "this.is.a.deep.test.blaat") == false);

	// Test delete
	json_db.Set(transaction, "this.is.a.deep.test.path.delete_value", 1);
	BOOST_CHECK(json_db.Exists(transaction, "this.is.a.deep.test.path.delete_value") == true);
	json_db.Delete(transaction, "this.is.a.deep.test.path.delete_value");
	BOOST_CHECK(json_db.Exists(transaction, "this.is.a.deep.test.path.delete_value") == false);

	// Validate the integrity of the database
	BOOST_CHECK(json_db.Validate(transaction) == true);
}

void JsonDb_ValidateDatabase()
{
	JsonDb json_db("test.db");
	JsonDb::TransactionHandle transaction = json_db.StartTransaction();

	BOOST_CHECK(json_db.GetInt(transaction, "this.is.a.deep.test.path.int_value") == 1);
	BOOST_CHECK(json_db.GetFloat(transaction, "this.is.a.deep.test.path.float_value") == 1.1f);
	BOOST_CHECK(json_db.GetString(transaction, "this.is.a.deep.test.path.string_value") == "test");
	BOOST_CHECK(json_db.GetBool(transaction, "this.is.a.deep.test.path.bool_value") == true); 

	// Get some values from the array
	for(int i = 0; i < 5; ++i)
		BOOST_CHECK(json_db.GetInt(transaction, (boost::format(("this.is.a.deep.test.path.array_value[%d]")) % i).str()) == i * 10);

	// Validate values in multilevel array
	for(int i = 0; i < 5; ++i)
		for(int j = 0; j < i; ++j)
			json_db.Set(transaction, (boost::format("this.is.a.deep.test.path.multilevel_array_value[%d][%d]") % i % j).str(), i * j);

	BOOST_CHECK(json_db.Exists(transaction, "this.is.a.deep.test.path") == true);
	BOOST_CHECK(json_db.Exists(transaction, "this.is.a.deep.test.path.array_value[0]") == true);
	BOOST_CHECK(json_db.Exists(transaction, "this.is.a.deep.test.path.array_value") == true);

	std::ostringstream output;
	json_db.Print(transaction, output);
	std::cout << output.str() << std::endl;
}

void JsonDb_Test()
{
	JsonDb_CreateDatabase();
	JsonDb_ValidateDatabase();
}

boost::unit_test::test_suite *init_unit_test_suite(int argc, char **argv) 
{
	boost::unit_test::test_suite *test(BOOST_TEST_SUITE("JsonDb unit test"));
	test->add(BOOST_TEST_CASE(&JsonDb_Test));
	return test; 
}

