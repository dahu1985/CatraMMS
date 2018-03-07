/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   UploadBinary.h
 * Author: giuliano
 *
 * Created on February 18, 2018, 1:27 AM
 */

#ifndef UploadBinary_h
#define UploadBinary_h

#include "APICommon.h"

class UploadBinary: public APICommon {
public:
    UploadBinary(const char* configurationPathName);
    
    ~UploadBinary();
    
    virtual void manageRequestAndResponse(
        string requestURI,
        string requestMethod,
        unordered_map<string, string> queryParameters,
        tuple<shared_ptr<Customer>,bool,bool>& customerAndFlags,
        unsigned long contentLength,
        string requestBody);

    virtual void getBinaryAndResponse(
            string requestURI,
            string requestMethod,
            string xCatraMMSResumeHeader,
            unordered_map<string, string> queryParameters,
            tuple<shared_ptr<Customer>,bool,bool>& customerAndFlags,
            unsigned long contentLength
    );
    
private:
    unsigned long       _binaryBufferLength;
    unsigned long       _progressUpdatePeriodInSeconds;
};

#endif