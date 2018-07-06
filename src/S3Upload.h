//
// Created by Michael Usachenko on 6/29/18.
//

#ifndef S3UPLOAD_S3UPLOAD_H
#define S3UPLOAD_S3UPLOAD_H

// IMPORTS
#include <aws/s3-encryption/S3EncryptionClient.h>
#include <aws/core/utils/Outcome.h>
#include <aws/s3/S3Request.h>

#include <string>
#include <sstream>

using std::stringstream;
using std::string;

class S3Upload {
public:
    S3Upload(string* MASTER_KEY_ID);
    S3Upload(string* MASTER_KEY_ID, Aws::SDKOptions options);
    ~S3Upload();

    // getters/setters
    void setBucket(string* BUCKET_NAME);

    // public user methods
    bool upload(const string& filename, const string& key);
    bool upload(const stringstream& stream, const string& key);
    bool download(const string& key, const string& path);
    bool remove(const string& key);

private:
    // AUTH
    string* _BUCKET;
    string* _MASTER_KEY_ID;
    Aws::SDKOptions _options;
    Aws::S3Encryption::S3EncryptionClient* _encryptionClient;

    // CLASS CONSTANTS
    static const string _SUCCESSFUL_REQUEST;
    static const int _UPLOAD_FILE;
    static const int _UPLOAD_STREAM;
    static const int _DOWNLOAD;
    static const int _REMOVE;

    //class config
    inline Aws::S3Encryption::S3EncryptionClient* _configEncryptionClient(const string *master);

    //s3 api access helpers
    bool _use_api(int op, const string* filename, const string* key);
    inline bool _successful_s3_request(const string* requestResult);
    template <typename T> inline string* _get_s3_request_errors(T& request_outcome);
    template <typename T> inline T _init_s3_request(const string *key);

    string* _upload_file(const string* filename, const string* key);
    string* _upload_stream(const string* stream, const string* key);
    string* _download_file(const string* key, const string* path);
    string* _remove_file(const string* key);
};


#endif //S3UPLOAD_S3UPLOAD_H