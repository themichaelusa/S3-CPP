//
// Created by Michael Usachenko on 6/29/18.
//

#ifndef S3UPLOAD_S3UPLOAD_H
#define S3UPLOAD_S3UPLOAD_H

// IMPORTS
#include <aws/s3-encryption/S3EncryptionClient.h>
#include <string>
#include <sstream>

using namespace Aws::S3Encryption;
using std::stringstream;
using std::string;

class S3Upload {
    public:
        S3Upload(string* MASTER_KEY_ID);
        S3Upload(string* MASTER_KEY_ID, Aws::SDKOptions options);
        ~S3Upload();

        void setBucket(string* BUCKET_NAME);

        bool upload(const string& filename, const string& key);
        bool upload(const stringstream& stream, const string& key);
        bool download(const string& key);
        bool remove(const string& key);

    private:
        // AUTH
        string* _BUCKET;
        string* _MASTER_KEY_ID;
        Aws::SDKOptions _options;
        S3EncryptionClient* _encryptionClient;

        // CLASS CONSTANTS
        static const string _SUCCESSFUL_REQUEST;
        static const int _UPLOAD_FILE;
        static const int _UPLOAD_STREAM;
        static const int _DOWNLOAD;
        static const int _REMOVE;

        //class config
        inline S3EncryptionClient* _configEncryptionClient(const string *master);

        //s3 api access helpers
        bool _use_api(int op, const string* filename, const string* key);
        string* _upload_file(const string* filename, const string* key);
        string* _upload_stream(const string* stream, const string* key);
        string* _download_file(const string* key);
        string* _remove_file(const string* key);
};


#endif //S3UPLOAD_S3UPLOAD_H
