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
    UploadBinary(Json::Value configuration, 
        shared_ptr<MMSEngineDBFacade> mmsEngineDBFacade,
        shared_ptr<MMSStorage> mmsStorage,
        mutex* fcgiAcceptMutex,
        shared_ptr<spdlog::logger> logger);
    
    ~UploadBinary();
    
    virtual void manageRequestAndResponse(
            FCGX_Request& request,
            string requestURI,
            string requestMethod,
            unordered_map<string, string> queryParameters,
            tuple<int64_t,shared_ptr<Workspace>,bool,bool,bool>& userKeyWorkspaceAndFlags,
			string apiKey,
            unsigned long contentLength,
            string requestBody,
            string xCatraMMSResumeHeader,
            unordered_map<string, string>& requestDetails
    );

    virtual void getBinaryAndResponse(
            string requestURI,
            string requestMethod,
            string xCatraMMSResumeHeader,
            unordered_map<string, string> queryParameters,
            tuple<int64_t,shared_ptr<Workspace>,bool,bool,bool>& userKeyWorkspaceAndFlags,
            unsigned long contentLength
    );
    
private:
    unsigned long       _binaryBufferLength;
    unsigned long       _progressUpdatePeriodInSeconds;
};

#endif
