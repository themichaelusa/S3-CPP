#include <iostream>
#include <string>
#include "S3Upload.h"

using namespace std;

int main() {

    auto test_master = new string("test-mk");
    auto bucket_name = new string("test-b");

    S3Upload* s3 = new S3Upload(test_master);
    s3->setBucket(bucket_name);
    s3->upload("path", "key");

}