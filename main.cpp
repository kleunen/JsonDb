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

void JsonDb_CreateDatabase(JsonDb &json_db)
{
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

	json_db.Set(transaction, "this.is.a.deep.test.path.long_string_value", "this is a very long string");
	BOOST_CHECK(json_db.GetString(transaction, "this.is.a.deep.test.path.long_string_value") == "this is a very long string");

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

void JsonDb_ValidateDatabase(JsonDb &json_db)
{
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

	json_db.Print(transaction, std::cout);
	std::cout << std::endl;
}

void JsonDb_EmptyDatabase(JsonDb &json_db)
{
	JsonDb::TransactionHandle transaction = json_db.StartTransaction();
	json_db.Delete(transaction, "this");

	BOOST_CHECK(json_db.Exists(transaction, "this.is.a.deep.test") == false);
	BOOST_CHECK(json_db.Exists(transaction, "this") == false);

	// Validate the integrity of the database
	BOOST_CHECK(json_db.Validate(transaction) == true);
}

void JsonDb_Test()
{
	JsonDb json_db("test.db");

	JsonDb_CreateDatabase(json_db);
	JsonDb_ValidateDatabase(json_db);
	JsonDb_EmptyDatabase(json_db);

	// Delete the complete database
	json_db.Delete();
}

void JsonDb_ParserTest()
{
	char const *json_value = 
		"{"
		"		'name' 	: 'Wouter van Kleunen',"
		"		'email' : 'wouter.van@kleunen.nl',"
		"		'float_value' : 1.0,"
		"		'int_value' 	: 1,"
		"		'bool_true_value' : true,"
		" 	'bool_false_value' : false,"
		"		'null_value' : null,"
		"		'array_value' : [10, 'test', false],"
		"		'sub_object' : { 'a' : 10, 'b' : 'test', 'c' : false }"
		"}";

	JsonDb json_db("test.db");
	JsonDb::TransactionHandle transaction = json_db.StartTransaction();

	json_db.SetJson(transaction, "json_test", json_value);
	BOOST_CHECK(json_db.GetString(transaction, "json_test.name") == "Wouter van Kleunen");
	BOOST_CHECK(json_db.GetString(transaction, "json_test.email") == "wouter.van@kleunen.nl");
	BOOST_CHECK(json_db.GetFloat(transaction, "json_test.float_value") == 1.0);
	BOOST_CHECK(json_db.GetInt(transaction, "json_test.int_value") == 1);
	BOOST_CHECK(json_db.GetBool(transaction, "json_test.bool_true_value") == true);
	BOOST_CHECK(json_db.GetBool(transaction, "json_test.bool_false_value") == false);
	BOOST_CHECK(json_db.GetInt(transaction, "json_test.array_value[0]") == 10);
	BOOST_CHECK(json_db.GetString(transaction, "json_test.array_value[1]") == "test");
	BOOST_CHECK(json_db.GetBool(transaction, "json_test.array_value[2]") == false);
	BOOST_CHECK(json_db.GetInt(transaction, "json_test.sub_object.a") == 10);
	BOOST_CHECK(json_db.GetString(transaction, "json_test.sub_object.b") == "test");
	BOOST_CHECK(json_db.GetBool(transaction, "json_test.sub_object.c") == false);

	json_db.Print(transaction, std::cout);
	std::cout << std::endl;
}

boost::unit_test::test_suite *init_unit_test_suite(int argc, char **argv) 
{
	boost::unit_test::test_suite *test(BOOST_TEST_SUITE("JsonDb unit test"));
	test->add(BOOST_TEST_CASE(&JsonDb_Test));
	test->add(BOOST_TEST_CASE(&JsonDb_ParserTest));
	return test; 
}

