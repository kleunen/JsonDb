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

int main()
{
	try
	{
		JsonDb json_db("test.db");
		JsonDb::TransactionHandle transaction = json_db.StartTransaction();
		//JsonDb::ValuePointer value = json_db.GetRoot();

		// JsonDb::ValuePointer test = JsonDb::ValuePointer(new JsonDb::ValueNumberInteger(json_db, 10));
		// json_db.Set("test", test);
		json_db.Set(transaction, "this.is.a.deep.test.path.int_value", 1);
		json_db.Set(transaction, "this.is.a.deep.test.path.float_value", 1.1f);
		json_db.Set(transaction, "this.is.a.deep.test.path.string_value", "test");
		json_db.Set(transaction, "this.is.a.deep.test.path.bool_value", true);

		// Create an empty array
		json_db.SetArray(transaction, "this.is.a.deep.test.path.array_value", 5);

		// Set some values in the array
		for(int i = 0; i < 5; ++i)
			json_db.Set(transaction, (boost::format("this.is.a.deep.test.path.array_value[%d]") % i).str(), i * 10);

		// Get the values
		std::cout << "Int value: " << json_db.GetInt(transaction, "this.is.a.deep.test.path.int_value") << std::endl;
		std::cout << "Float value: " << json_db.GetFloat(transaction, "this.is.a.deep.test.path.float_value") << std::endl;
		std::cout << "String value: " << json_db.GetString(transaction, "this.is.a.deep.test.path.string_value") << std::endl;
		std::cout << "Bool value: " << json_db.GetBool(transaction, "this.is.a.deep.test.path.bool_value") << std::endl; 

		// Get some values from the array
		for(int i = 0; i < 5; ++i)
			std::cout << "Array element value: " << json_db.GetInt(transaction, (boost::format(("this.is.a.deep.test.path.array_value[%d]")) % i).str()) << std::endl;

		std::cout << "Exists: " << json_db.Exists(transaction, "this.is.a.deep.test.path") << std::endl;
		std::cout << "Exists: " << json_db.Exists(transaction, "this.is.a.deep.test.path.array_value[0]") << std::endl;
		std::cout << "Exists: " << json_db.Exists(transaction, "this.is.a.deep.test.path.array_value") << std::endl;
		std::cout << "Exists: " << json_db.Exists(transaction, "this.is.a.deep.test.blaat") << std::endl;

	//	json_db.Delete(transaction, "this.is.a.deep.test.path.array_value");

		std::ostringstream output;
		json_db.Print(transaction, output);
		std::cout << output.str() << std::endl;

		transaction->Commit();
	} catch(std::exception &e)
	{
		std::cerr << "Internal error: " << e.what() << std::endl;
	}

	return 0;
}

