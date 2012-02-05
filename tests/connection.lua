#!/usr/bin/lua
-- Tests DB connection bindings
--
-- Configuration can be set with the following environment variables:
--    TEST_SERVER   ('localhost')
--    TEST_DB       ('test')
--    TEST_USER     (nil, no auth will be done)
--    TEST_PASS     ('')

require 'mongo'
require 'os'

require 'tests/lunity'
module( 'LUAMONGO_TEST_CONNECTION', lunity )

function setup()
    test_server = os.getenv('TEST_SERVER') or 'localhost'
    test_user = os.getenv('TEST_USER') or nil
    test_password = os.getenv('TEST_PASS') or ''
    test_db = os.getenv('TEST_DB') or 'test'
    test_ns = test_db .. '.conn_values'
end

function teardown()
end

function test_ReplicaSet()
    -- Create a Connection object
    local db = mongo.Connection.New()
    assertNotNil( db, 'unable to create mongo.Connection' )
    assert( db:connect(test_server), 'unable to forcefully connect to mongo instance' )

    -- test data
    local data = { a = 10, b = 'str1' }
    local data_as_json = "{'a': 10, 'b': 'str1'}"

    -- auth if we need to
    if test_user then
        assertTrue( db:auth{dbname=test_db, username=test_user, password=test_password}, "unable to auth to db" )
    end

    -- drop existing data
    assertTrue( db:drop_collection(test_ns) )

    -- insert a value into the namespace 'test.values'
    assertTrue( db:insert(test_ns, data), 'unable to insert table-based data' )
   
    -- insert the same using a JSON string
    assertTrue( db:insert(test_ns, data_as_json), 'unable to insert JSON-based data' )

    -- check the data
    assertEqual( 2, db:count(test_ns) )

    -- query all the values in the namespace, ensuring the values are equal to the inserted values
    local q = db:query( test_ns, {} )
    assertNotNil( q, 'unable to create Query object' )
    for result in q:results() do
        assertEqual( result.a, data.a )
        assertEqual( result.b, data.b )
    end
	
	-- query for a single result from the namespace
	local result = db:find_one( test_ns, {} )
	assertNotNil( result, 'could not find result' )
	assertEqual( result.a, data.a )
	assertEqual( result.b, data.b )
end


runTests()
