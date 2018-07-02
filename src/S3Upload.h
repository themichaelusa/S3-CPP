//
// Created by Michael Usachenko on 6/29/18.
//

#ifndef S3UPLOAD_S3UPLOAD_H
#define S3UPLOAD_S3UPLOAD_H

// IMPORTS
#include <aws/s3-encryption/S3EncryptionClient.h>
using namespace Aws::S3Encryption;

#include <string>
using std::string;

// CONSTANTS

//#define SUCCESSFUL_REQUEST "";

class S3Upload {
    public:
        S3Upload(string* MASTER_KEY_ID);
        S3Upload(string* MASTER_KEY_ID, Aws::SDKOptions options);
        ~S3Upload();

        void setBucket(string* BUCKET_NAME);
        bool upload_file(const string* filename, const string* key);
        bool download_file(const string* filename, const string* key);
        bool remove_file(const string* key);


    private:
        string* _BUCKET;
        string* _MASTER_KEY_ID;
        static const string SUCCESSFUL_REQUEST;

        Aws::SDKOptions _options;
        S3EncryptionClient* _encryptionClient;

        S3EncryptionClient* configEncryptionClient(const string *master);
        bool _use_api(string* (*file_op) (const string*, const string*),
                      const string* filename, const string* key);
        string* _upload_file(const string* filename, const string* key);
        string* _download_file(const string* filename, const string* key);
};


#endif //S3UPLOAD_S3UPLOAD_H
