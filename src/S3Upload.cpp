//
// Created by Michael Usachenko on 6/29/18.
//

using namespace Aws::S3;
using namespace Aws::S3::Model;
using namespace Aws::S3Encryption;
using namespace Aws::S3Encryption::Materials;
using namespace std;

#include <aws/s3-encryption/CryptoConfiguration.h>
#include <aws/s3-encryption/materials/KMSEncryptionMaterials.h>
#include <aws/core/auth/AWSCredentialsProviderChain.h>

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

S3EncryptionClient* S3Upload::configEncryptionClient(const string *master){
    // get kms materials, crypto details, and aws credentials
    auto kmsMaterials = Aws::MakeShared<KMSEncryptionMaterials>("s3Encryption", master);
    CryptoConfiguration cryptoConfiguration(StorageMethod::INSTRUCTION_FILE,
                                            CryptoMode::STRICT_AUTHENTICATED_ENCRYPTION);
    auto credentials = Aws::MakeShared<Aws::Auth::DefaultAWSCredentialsProviderChain>("s3Encryption");
    return new S3EncryptionClient(kmsMaterials, cryptoConfiguration, credentials);
}

bool S3Upload::use_api(string* (*file_op) (const string*, string), const string* filename, const string key){
    Aws::InitAPI(_options);
    {
        string* requestResult = file_op(filename, key);
        if (strcmp(requestResult->c_str(), "") == 0) {
            cout << *requestResult << endl;
        } else {
            cout<<"AWS S3 Request Successful"<<endl;
        }

        //TODO: Log requests, inform mailing/text list if fail
    }
    Aws::ShutdownAPI(_options);
}

bool S3Upload::download_file(const string* filename, const string key) {
    return use_api(&_download_file, filename, key);
}

bool S3Upload::upload_file(const string* filename, const string key) {
    return use_api(&_upload_file, filename, key);
}

string* S3Upload::_upload_file(const string *filename, string key) {
    Aws::S3::Model::PutObjectRequest r;
    r.WithBucket(_BUCKET->c_str()).WithKey(key);

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

string* S3Upload::_download_file(const string *filename, string key) {
    Aws::S3::Model::GetObjectRequest r;
    r.WithBucket(_BUCKET->c_str()).WithKey(key);
    GetObjectOutcome o = _encryptionClient->GetObject(r);
    delete filename;

    if (o.IsSuccess()){
        Aws::OFStream local_file;
        local_file.open(key.c_str(), std::ios::out | std::ios::binary);
        local_file << o.GetResult().GetBody().rdbuf();
        return new string("");
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

