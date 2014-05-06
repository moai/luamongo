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

#ifndef MONGO_CXX_EXTENSION
#define MONGO_CXX_EXTENSION

#include <client/dbclient.h>
#include <client/gridfs.h>

namespace mongo_cxx_extension {
  
  const unsigned int DEFAULT_CHUNK_SIZE = 256*1024;

  // FIXME: client pointer is keep, review Lua binding in order to avoid
  // unexpected garbage collection of DBClientBase
  class GridFileBuilder {
  public:
    /**
     * @param client - db connection
     * @param dbName - root database name
     * @param chunkSize - size of chunks
     * @param prefix - if you want your data somewhere besides <dbname>.fs
     */
    GridFileBuilder(mongo::DBClientBase *client, const std::string &dbName,
                    unsigned int chunkSize = DEFAULT_CHUNK_SIZE,
                    const std::string& prefix = "fs");
    ~GridFileBuilder();
    // chunks are splitted in as many as necessary chunkSize blocks; sizes not
    // multiple of chunkSize will copy remaining data at pending_data pointer
    void appendChunk(const char *data, size_t length);
    // buildFile will destroy this builder, not allowing to insert more data
    mongo::BSONObj buildFile(const std::string &name,
                             const std::string& contentType="");
    
  private:
    mongo::DBClientBase *_client;
    std::string _dbName;
    std::string _prefix;
    std::string _chunkNS;
    std::string _filesNS;
    size_t _chunkSize;
    unsigned int _current_chunk;
    mongo::BSONObj _file_id;
    char *_pending_data; // NULL or pointer with _chunkSize space
    size_t _pending_data_size;
    mongo::gridfs_offset _file_length;
    
    const char *privateAppendChunk(const char *data, size_t length,
                                   bool pending_insert = false);
    void privateAppendPendingData();
  };
}

#endif // MONGO_CXX_EXTENSION
