/*
 * Copyright (c) 2014 Francisco Zamora-Martinez (pakozm@gmail.com)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include <client/redef_macros.h>
#include <client/dbclient.h>
#include <client/gridfs.h>
#include <fcntl.h>
#include <fstream>
#include <utility>
#include <algorithm>
#include "mongo_cxx_extension.h"

namespace mongo_cxx_extension {
  
  using namespace mongo;

  using std::ios;
  using std::ofstream;
  using std::ostream;
  using std::string;
  using std::min;

  GridFileBuilder::GridFileBuilder(DBClientBase *client,
                                   const string &dbName,
                                   unsigned int chunkSize,
                                   const string& prefix) :
    _client(client), _dbName(dbName), _prefix(prefix), _chunkSize(chunkSize),
    _current_chunk(0), _pending_data(NULL), _pending_data_size(0),
    _file_length(0) {
    _chunkNS = _dbName + "." + _prefix + ".chunks";
    _filesNS = _dbName + "." + _prefix + ".files";
    OID id;
    id.init();
    _file_id = BSON("_id" << id);
    // forces to build the gridFS collections
    GridFS aux(*client, dbName, prefix);
  }
  
  GridFileBuilder::~GridFileBuilder() {
    delete[] _pending_data;
  }

  const char *GridFileBuilder::privateAppendChunk(const char *data,
                                                  size_t length,
                                                  bool pending_insert) {
    size_t chunk_len;
    char const * const end = data + length;
    while (data < end) {
      chunk_len = min(_chunkSize, (size_t)(end-data));
      // the last chunk needs to be stored as pending_data
      if (chunk_len < _chunkSize && !pending_insert) break;
      /* from gridfs.cpp at https://github.com/mongodb/mongo-cxx-driver/blob/legacy/src/mongo/client/gridfs.cpp */
      BSONObjBuilder b;
      b.appendAs( _file_id["_id"] , "files_id" );
      b.append( "n" , _current_chunk );
      b.appendBinData( "data" , chunk_len, BinDataGeneral, data );
      BSONObj chunk_data = b.obj();
      /************************************************************************/
      ++_current_chunk;
      _client->insert( _chunkNS.c_str(), chunk_data );
      data += chunk_len;
      _file_length += chunk_len;
    }
    return data;
  }
  
  void GridFileBuilder::privateAppendPendingData() {
    if (_pending_data_size > 0) {
      privateAppendChunk(_pending_data, _pending_data_size, true);
      delete[] _pending_data;
      _pending_data_size = 0;
      _pending_data = NULL;
    }
  }

  void GridFileBuilder::appendChunk(const char *data, size_t length) {
    if (length == 0) return;
    // check if there are pending data
    if (_pending_data != NULL) {
      size_t total_size = _pending_data_size + length;
      size_t size = min(_chunkSize, total_size) - _pending_data_size;
      memcpy(_pending_data + _pending_data_size, data, size*sizeof(char));
      _pending_data_size += size;
      // CHECK _pending_data_size <= _chunkSize
      if (_pending_data_size == _chunkSize) {
        privateAppendPendingData();
        appendChunk(data + size, length - size);
      }
    }
    else {
      char const * const end = data + length;
      // split data in _chunkSize blocks, and store as pending the last block if
      // necessary
      data = privateAppendChunk(data, length);
      // process pending data if necessary
      if (data != end) {
        if (_pending_data_size == 0) _pending_data = new char[_chunkSize];
        size_t size = (size_t)(end-data);
        memcpy(_pending_data+_pending_data_size, data, size*sizeof(char));
        _pending_data_size += size;
      }
    }
  }
  
  BSONObj GridFileBuilder::buildFile(const string &name,
                                     const string& content_type) {
    privateAppendPendingData();
    
    /* from gridfs.cpp at https://github.com/mongodb/mongo-cxx-driver/blob/legacy/src/mongo/client/gridfs.cpp */
    
    // Wait for any pending writebacks to finish
    BSONObj errObj = _client->getLastErrorDetailed();
    uassert( 16428,
             str::stream() << "Error storing GridFS chunk for file: " << name
             << ", error: " << errObj,
             DBClientWithCommands::getLastErrorString(errObj) == "" );
    
    BSONObj res;
    if ( ! _client->runCommand( _dbName.c_str() ,
                                BSON( "filemd5" << _file_id << "root" << _prefix ) ,
                                res ) )
      throw UserException( 9008 , "filemd5 failed" );
    
    BSONObjBuilder file;
    file << "_id" << _file_id["_id"]
         << "filename" << name
         << "chunkSize" << (unsigned int)_chunkSize
         << "uploadDate" << DATENOW
         << "md5" << res["md5"]
      ;
    
    if (_file_length < 1024*1024*1024) { // 2^30
      file << "length" << (int) _file_length;
    }
    else {
      file << "length" << (long long) _file_length;
    }
    
    if (!content_type.empty())
      file << "contentType" << content_type;
    
    BSONObj ret = file.obj();
    _client->insert(_filesNS.c_str(), ret);
    
    // resets the object
    _current_chunk     = 0;
    _pending_data      = NULL;
    _pending_data_size = 0;
    _file_length       = 0;
    OID id;
    id.init();
    _file_id = BSON("_id" << id);
    
    return ret;
  }
  
}
