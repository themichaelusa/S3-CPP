//
// Created by Michael Usachenko on 6/29/18.
//

#ifndef S3UPLOAD_S3UPLOAD_H
#define S3UPLOAD_S3UPLOAD_H

// IMPORTS
#include <aws/s3-encryption/S3EncryptionClient.h>
using namespace Aws::S3Encryption;

#include <string>
#include <sstream>
#include <map>

using std::stringstream;
using std::string;
using std::map;

// CONSTANTS

//#define SUCCESSFUL_REQUEST "";

class S3Upload {
    public:
        S3Upload(string* MASTER_KEY_ID);
        S3Upload(string* MASTER_KEY_ID, Aws::SDKOptions options);
        ~S3Upload();

        void setBucket(string* BUCKET_NAME);
        bool upload(const string* filename, const string* key);
        bool upload(const stringstream* stream, const string* key);

        bool download(const string* key);
        bool remove(const string* key);



    private:
        map<string, void*>* _FN_PTRS;
        string* _BUCKET;
        string* _MASTER_KEY_ID;
        static const string SUCCESSFUL_REQUEST;
        //static const int UPLOAD_OP = 1;
        //static const int DOWNLOAD_OP = 2;



        Aws::SDKOptions _options;
        S3EncryptionClient* _encryptionClient;
        S3EncryptionClient* configEncryptionClient(const string *master);
        void configFnPtrMap();
        bool _use_api(const string* filename, const string* key);

        // private w
        string* _upload_file(const string* filename, const string* key);
        string* _download_file(const string* key);
        string* _remove_file(const string* key);
};


#endif //S3UPLOAD_S3UPLOAD_H
