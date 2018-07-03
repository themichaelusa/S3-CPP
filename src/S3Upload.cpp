//
// Created by Michael Usachenko on 6/29/18.
//

using namespace std;

#include <aws/core/Aws.h>
#include <aws/s3/S3Client.h>
#include <aws/s3-encryption/S3EncryptionClient.h>

#include <aws/s3/S3Request.h>
#include <aws/s3/model/GetObjectRequest.h>
#include <aws/s3/model/PutObjectRequest.h>
#include <aws/s3/model/DeleteObjectRequest.h>
#include <aws/core/utils/Outcome.h>

#include <aws/s3-encryption/CryptoConfiguration.h>
#include <aws/s3-encryption/materials/KMSEncryptionMaterials.h>
#include <aws/core/auth/AWSCredentialsProviderChain.h>

#include <iostream>
#include <sstream>
#include <fstream>
#include "S3Upload.h"

// CONSTS DEFN

static const int S3Upload::_UPLOAD_FILE = 1;
static const int S3Upload::_UPLOAD_STREAM = 2;
static const int S3Upload::_DOWNLOAD = 3;
static const int S3Upload::_REMOVE = 4;

// CTORS && DTOR

S3Upload::S3Upload(string *MASTER_KEY_ID, Aws::SDKOptions options) {
    _encryptionClient = _configEncryptionClient(MASTER_KEY_ID);
    _MASTER_KEY_ID = MASTER_KEY_ID;
    _options = options;
    _BUCKET = nullptr;
}

S3Upload::S3Upload(string *MASTER_KEY_ID) {
    _encryptionClient = _configEncryptionClient(MASTER_KEY_ID);
    _MASTER_KEY_ID = MASTER_KEY_ID;
    Aws::SDKOptions options;
    _options = options;
    _BUCKET = nullptr;
}

S3Upload::~S3Upload() {
    if (_BUCKET){
        delete _BUCKET;
    }
}

// GETTERS/SETTERS
void S3Upload::setBucket(string *BUCKET_NAME) {
    _BUCKET = BUCKET_NAME;
}


// CONFIG
inline Aws::S3Encryption::S3EncryptionClient* S3Upload::_configEncryptionClient(const string *master) {
    // get kms materials, crypto details, and aws credentials
    auto kmsMaterials = Aws::MakeShared<Aws::S3Encryption::Materials::KMSEncryptionMaterials>("s3Encryption", master);
    Aws::S3Encryption::CryptoConfiguration cryptoConfiguration(Aws::S3Encryption::StorageMethod::INSTRUCTION_FILE,
                                            Aws::S3Encryption::CryptoMode::STRICT_AUTHENTICATED_ENCRYPTION);
    auto credentials = Aws::MakeShared<Aws::Auth::DefaultAWSCredentialsProviderChain>("s3Encryption");
    return new  Aws::S3Encryption::S3EncryptionClient(kmsMaterials, cryptoConfiguration, credentials);
}

// PUBLIC METHODS
bool S3Upload::upload(const string& filename, const string& key) {
    auto path_ptr = new string(filename);
    auto key_ptr = new string(key);
    bool result = _use_api(_UPLOAD_FILE, path_ptr, key_ptr);
    delete path_ptr;
    delete key_ptr;
    return result;
}

bool S3Upload::upload(const stringstream& stream, const string& key) {
    auto ss = new string(stream.str());
    auto key_ptr = new string(key);
    bool result = _use_api(_UPLOAD_STREAM, ss, key_ptr);
    delete ss;
    return result;
}

bool S3Upload::download(const string& key) {
    auto key_ptr = new string(key);
    bool result = _use_api(_DOWNLOAD, nullptr, key_ptr);
    delete key_ptr;
    return result;
}

bool S3Upload::remove(const string& key) {
    auto key_ptr = new string(key);
    bool result = _use_api(_REMOVE, nullptr, key_ptr);
    delete key_ptr;
    return result;
}

// PRIVATE S3 ACCESS HELPERS
bool S3Upload::_use_api(int op, const string* path_or_stream, const string* key){
    bool result;
    Aws::InitAPI(_options);
    {
        string* requestResult = nullptr;
        if (op == _UPLOAD_FILE){
            requestResult = _upload_file(path_or_stream, key);
        }
        else if (op == _UPLOAD_STREAM){
            requestResult = _upload_stream(path_or_stream, key);
        }
        else if (op == _DOWNLOAD) {
            requestResult = _download_file(key);
        }
        else if (op == _REMOVE) {
            requestResult = _remove_file(key);
        }

        if (_successful_s3_request(requestResult)) {
            result = true;
            cout<<"AWS S3 Request Successful"<<endl;
        } else {
            result = false;
            cout << *requestResult << endl;
        }

        //TODO: Log requests, inform mailing/text list if fail
        delete requestResult;
    }
    Aws::ShutdownAPI(_options);
    return result;
}

inline bool S3Upload::_successful_s3_request(const string *requestResult) {
    return strcmp(requestResult->c_str(), _SUCCESSFUL_REQUEST.c_str()) == 0;
}

inline string* S3Upload::_get_s3_request_errors(Aws::Utils::Outcome request_outcome) {
    if (request_outcome.IsSuccess()){
        return new string(_SUCCESSFUL_REQUEST);
    }
    else {
        stringstream ss;
        ss << request_outcome.GetError().GetExceptionName() << request_outcome.GetError().GetMessage();
        return new string(ss.str());
    }
}

template <class T>
inline T S3Upload::_init_s3_request(const string *key) {
    T req;
    req.WithBucket(_BUCKET->c_str());
    req.WithKey(key->c_str());
    return req;
}

string* S3Upload::_upload_file(const string* filename, const string* key) {
    auto request = _init_s3_request<Aws::S3::Model::PutObjectRequest>(key);

    // Note: Binary files must also have the std::ios_base::bin flag or'ed in
    auto input_data = Aws::MakeShared<Aws::FStream>("PutObjectInputStream",
            filename->c_str(), std::ios_base::in | std::ios_base::binary);

    auto io_data = Aws::MakeShared<Aws::IOStream>("PutObjectInputStreamIO",
            input_data);
    request.SetBody(io_data);

    Aws::S3::Model::PutObjectOutcome o = _encryptionClient->PutObject(request);
    return _get_s3_request_errors(o);
}

string* S3Upload::_upload_stream(const string* stream, const string* key) {
    auto request = _init_s3_request<Aws::S3::Model::PutObjectRequest>(key);
    auto io_data = Aws::MakeShared<Aws::IOStream>("PutObjectInputStreamIO", stream);
    request.SetBody(io_data);

    Aws::S3::Model::PutObjectOutcome o = _encryptionClient->PutObject(request);
    return _get_s3_request_errors(o);
}

string* S3Upload::_download_file(const string* key) {
    auto request = _init_s3_request<Aws::S3::Model::GetObjectRequest>(key);
    Aws::S3::Model::GetObjectOutcome o = _encryptionClient->GetObject(request);
    string* error_stream = _get_s3_request_errors(o);

    if (_successful_s3_request(error_stream)){
        Aws::OFStream local_file;
        local_file.open(key->c_str(), std::ios::out | std::ios::binary);
        local_file << o.GetResult().GetBody().rdbuf();
    }

    return error_stream;
}

string* S3Upload::_remove_file(const string* key) {
    auto request = _init_s3_request<Aws::S3::Model::DeleteObjectRequest>(key);
    Aws::S3::Model::DeleteObjectOutcome o = _encryptionClient->DeleteObject(request);
    return _get_s3_request_errors(o);
}




