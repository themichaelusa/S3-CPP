//
// Created by Michael Usachenko on 6/29/18.
//

using namespace Aws;
using namespace Aws::S3;
using namespace Aws::S3::Model;
using namespace Aws::S3Encryption;
using namespace Aws::S3Encryption::Materials;
using namespace std;

#include <aws/core/Aws.h>
#include <aws/s3/model/DeleteBucketRequest.h>
#include <aws/s3-encryption/CryptoConfiguration.h>
#include <aws/s3-encryption/materials/KMSEncryptionMaterials.h>
#include <aws/core/auth/AWSCredentialsProviderChain.h>

#include <list>
#include <iostream>
#include <sstream>
#include <fstream>
#include "S3Upload.h"

S3Upload::S3Upload(string *MASTER_KEY_ID, Aws::SDKOptions options) {
    _encryptionClient = configEncryptionClient(MASTER_KEY_ID);
    _MASTER_KEY_ID = MASTER_KEY_ID;
    _options = options;
    _BUCKET = nullptr;
}

S3Upload::S3Upload(string *MASTER_KEY_ID) {
    _encryptionClient = configEncryptionClient(MASTER_KEY_ID);
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

// CONFIG
S3EncryptionClient* S3Upload::configEncryptionClient(const string *master){
    // get kms materials, crypto details, and aws credentials
    auto kmsMaterials = Aws::MakeShared<KMSEncryptionMaterials>("s3Encryption", master);
    CryptoConfiguration cryptoConfiguration(StorageMethod::INSTRUCTION_FILE,
                                            CryptoMode::STRICT_AUTHENTICATED_ENCRYPTION);
    auto credentials = Aws::MakeShared<Aws::Auth::DefaultAWSCredentialsProviderChain>("s3Encryption");
    return new S3EncryptionClient(kmsMaterials, cryptoConfiguration, credentials);
}

void S3Upload::configFnPtrMap(){
    _FN_PTRS = new map<string, void*>();
    _FN_PTRS->insert(make_pair("upload", &_upload_file));
    _FN_PTRS->insert(make_pair("download", &_download_file));
    _FN_PTRS->insert(make_pair("remove", &_download_file));

}

bool S3Upload::_use_api(string op, const string* filename, const string* key){
    Aws::InitAPI(_options);
    {
        string* requestResult;
        if (op.compare("UL")){
            requestResult = _upload_file(filename, key);
        }
        else if (op.compare("DL")) {
            requestResult = _download_file(key);
        }
        else if (op.compare("RM")) {
            requestResult = _remove_file(key);
        }

        //string* requestResult = _FN_PTRS->at(op)(filename, key);
        if (strcmp(requestResult->c_str(), SUCCESSFUL_REQUEST.c_str()) == 0) {
            cout << *requestResult << endl;
        } else {
            cout<<"AWS S3 Request Successful"<<endl;
        }

        //TODO: Log requests, inform mailing/text list if fail
    }
    Aws::ShutdownAPI(_options);
}

bool S3Upload::download_file(const string* filename, const string* key) {
    return _use_api(&_download_file, filename, key);
}

bool S3Upload::upload_file(const string* filename, const string* key) {
    return _use_api(&_upload_file, filename, key);
}

string* S3Upload::_upload_file(const string* filename, const string* key) {
    Aws::S3::Model::PutObjectRequest r;
    r.WithBucket(_BUCKET->c_str());
    r.WithKey(key->c_str());

    // Note: Binary files must also have the std::ios_base::bin flag or'ed in
    auto input_data = Aws::MakeShared<Aws::FStream>("PutObjectInputStream",
            filename->c_str(), std::ios_base::in | std::ios_base::binary);

    auto io_data = Aws::MakeShared<Aws::IOStream>("PutObjectInputStreamIO",
            input_data);
    r.SetBody(io_data);

    PutObjectOutcome o = _encryptionClient->PutObject(r);
    delete filename;

    if (o.IsSuccess()){
        return new string("");
    }
    else {
        stringstream ss;
        ss << o.GetError().GetExceptionName() << o.GetError().GetMessage();
        return new string(ss.str());
    }
}

string* S3Upload::_download_file(const string* key) {
    Aws::S3::Model::GetObjectRequest r;
    r.WithBucket(_BUCKET->c_str());
    r.WithKey(key->c_str());
    GetObjectOutcome o = _encryptionClient->GetObject(r);

    if (o.IsSuccess()){
        Aws::OFStream local_file;
        local_file.open(key->c_str(), std::ios::out | std::ios::binary);
        local_file << o.GetResult().GetBody().rdbuf();
        return new string(SUCCESSFUL_REQUEST);
    }
    else {
        stringstream ss;
        ss << o.GetError().GetExceptionName() << o.GetError().GetMessage();
        return new string(ss.str());
    }
}

string* S3Upload::_remove_file(const string* key) {
    Aws::S3::Model::DeleteObjectRequest r;
    r.WithBucket(_BUCKET->c_str());
    r.WithKey(key->c_str());
    DeleteObjectOutcome o = _encryptionClient->GetObject(r);

    if (o.IsSuccess()){
        Aws::OFStream local_file;
        local_file.open(key->c_str(), std::ios::out | std::ios::binary);
        local_file << o.GetResult().GetBody().rdbuf();
        return new string(SUCCESSFUL_REQUEST);
    }
    else {
        stringstream ss;
        ss << o.GetError().GetExceptionName() << o.GetError().GetMessage();
        return new string(ss.str());
    }
}

void S3Upload::setBucket(string *BUCKET_NAME) {
    _BUCKET = BUCKET_NAME;
}

