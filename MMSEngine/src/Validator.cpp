/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Validator.cpp
 * Author: giuliano
 * 
 * Created on March 29, 2018, 6:27 AM
 */

#include "Validator.h"

Validator::Validator(
        shared_ptr<spdlog::logger> logger, 
        shared_ptr<MMSEngineDBFacade> mmsEngineDBFacade
) 
{
    _logger             = logger;
    _mmsEngineDBFacade  = mmsEngineDBFacade;
}

Validator::Validator(const Validator& orig) {
}

Validator::~Validator() {
}

void Validator::validateProcessMetadata(Json::Value processRoot)
{
    // check Process metadata
    
    string field = "Task";
    if (!isMetadataPresent(processRoot, field))
    {
        string errorMessage = __FILEREF__ + "Field is not present or it is null"
                + ", Field: " + field;
        _logger->error(errorMessage);

        throw runtime_error(errorMessage);
    }
    Json::Value taskRoot = processRoot[field];  
    
    validateTaskMetadata(taskRoot);
}

pair<MMSEngineDBFacade::ContentType,vector<int64_t>> 
        Validator::validateTaskMetadata(Json::Value taskRoot)
{
    MMSEngineDBFacade::IngestionType    ingestionType;
    MMSEngineDBFacade::ContentType      contentType;
    vector<int64_t>                     dependencies;

    string field = "Type";
    if (!isMetadataPresent(taskRoot, field))
    {
        string errorMessage = __FILEREF__ + "Field is not present or it is null"
                + ", Field: " + field;
        _logger->error(errorMessage);

        throw runtime_error(errorMessage);
    }

    string type = taskRoot.get("Type", "XXX").asString();
    if (type == "ContentIngestion")
    {
        ingestionType = MMSEngineDBFacade::IngestionType::ContentIngestion;
        
        field = "Parameters";
        if (!isMetadataPresent(taskRoot, field))
        {
            string errorMessage = __FILEREF__ + "Field is not present or it is null"
                    + ", Field: " + field;
            _logger->error(errorMessage);

            throw runtime_error(errorMessage);
        }

        Json::Value contentIngestionRoot = taskRoot[field]; 
        
        contentType = validateContentIngestionMetadata(
                contentIngestionRoot);
    }
    else if (type == "Frame")
    {
        ingestionType = MMSEngineDBFacade::IngestionType::Frame;
        
        field = "Parameters";
        if (!isMetadataPresent(taskRoot, field))
        {
            string errorMessage = __FILEREF__ + "Field is not present or it is null"
                    + ", Field: " + field;
            _logger->error(errorMessage);

            throw runtime_error(errorMessage);
        }

        Json::Value frameRoot = taskRoot[field]; 
        contentType = validateFrameMetadata(frameRoot, dependencies);        
    }
    else if (type == "PeriodicalFrames")
    {
        ingestionType = MMSEngineDBFacade::IngestionType::Frame;
        
        field = "Parameters";
        if (!isMetadataPresent(taskRoot, field))
        {
            string errorMessage = __FILEREF__ + "Field is not present or it is null"
                    + ", Field: " + field;
            _logger->error(errorMessage);

            throw runtime_error(errorMessage);
        }

        Json::Value frameRoot = taskRoot[field]; 
        contentType = validatePeriodicalFramesMetadata(frameRoot, dependencies);        
    }
    else
    {
        string errorMessage = __FILEREF__ + "Field 'Type' is wrong"
                + ", Type: " + type;
        _logger->error(errorMessage);

        throw runtime_error(errorMessage);
    }
        
    return make_pair(contentType, dependencies);
}

pair<MMSEngineDBFacade::ContentType,vector<int64_t>> 
        Validator::validateTaskMetadata(
        MMSEngineDBFacade::IngestionType ingestionType, Json::Value typeRoot)
{
    MMSEngineDBFacade::ContentType      contentType;
    vector<int64_t>                     dependencies;

    if (ingestionType == MMSEngineDBFacade::IngestionType::ContentIngestion)
    {

        contentType = validateContentIngestionMetadata(
                typeRoot);
    }
    else if (ingestionType == MMSEngineDBFacade::IngestionType::Frame)
    {
        MMSEngineDBFacade::ContentType contentType =
            validateFrameMetadata(typeRoot, dependencies);        
    }
    else if (ingestionType == MMSEngineDBFacade::IngestionType::PeriodicalFrames)
    {
        MMSEngineDBFacade::ContentType contentType =
            validatePeriodicalFramesMetadata(typeRoot, dependencies);        
    }
    else
    {
        string errorMessage = __FILEREF__ + "Unknown IngestionType"
                + ", ingestionType: " + MMSEngineDBFacade::toString(ingestionType);
        _logger->error(errorMessage);

        throw runtime_error(errorMessage);
    }
    
    
    return make_pair(contentType, dependencies);
}

MMSEngineDBFacade::ContentType Validator::validateContentIngestionMetadata(
    Json::Value contentIngestionRoot)
{
    // see sample in directory samples
    
    MMSEngineDBFacade::ContentType         contentType;
    
    vector<string> mandatoryFields = {
        // "SourceURL",     it is optional in case of push
        "SourceFileName",
        "ContentType"
    };
    for (string mandatoryField: mandatoryFields)
    {
        if (!isMetadataPresent(contentIngestionRoot, mandatoryField))
        {
            string errorMessage = __FILEREF__ + "Field is not present or it is null"
                    + ", Field: " + mandatoryField;
            _logger->error(errorMessage);

            throw runtime_error(errorMessage);
        }
    }

    string sContentType = contentIngestionRoot.get("ContentType", "XXX").asString();
    try
    {
        contentType = MMSEngineDBFacade::toContentType(sContentType);
    }
    catch(exception e)
    {
        string errorMessage = __FILEREF__ + "Field 'ContentType' is wrong"
                + ", sContentType: " + sContentType;
        _logger->error(errorMessage);

        throw runtime_error(errorMessage);
    }

    string field;
    if (contentType == MMSEngineDBFacade::ContentType::Video 
            || contentType == MMSEngineDBFacade::ContentType::Audio)
    {
        field = "EncodingPriority";
        if (isMetadataPresent(contentIngestionRoot, field))
        {
            string encodingPriority = contentIngestionRoot.get(field, "XXX").asString();
            try
            {
                MMSEngineDBFacade::toEncodingPriority(encodingPriority);    // it generate an exception in case of wrong string
            }
            catch(exception e)
            {
                string errorMessage = __FILEREF__ + "Field 'EncodingPriority' is wrong"
                        + ", EncodingPriority: " + encodingPriority;
                _logger->error(errorMessage);

                throw runtime_error(errorMessage);
            }
        }
    }

    /*
    // Territories
    {
        field = "Territories";
        if (isMetadataPresent(contentIngestionRoot, field))
        {
            const Json::Value territories = contentIngestionRoot[field];
            
            for( Json::ValueIterator itr = territories.begin() ; itr != territories.end() ; itr++ ) 
            {
                Json::Value territory = territories[territoryIndex];
            }
        
    }
    */
            
    return contentType;
}

MMSEngineDBFacade::ContentType Validator::validateFrameMetadata(
    Json::Value frameRoot, vector<int64_t>& dependencies)
{
    // see sample in directory samples
        
    vector<string> mandatoryFields = {
        "SourceFileName"
    };
    for (string mandatoryField: mandatoryFields)
    {
        if (!isMetadataPresent(frameRoot, mandatoryField))
        {
            string errorMessage = __FILEREF__ + "Field is not present or it is null"
                    + ", Field: " + mandatoryField;
            _logger->error(errorMessage);

            throw runtime_error(errorMessage);
        }
    }

    int64_t referenceMediaItemKey = -1;
    int64_t referenceIngestionJobKey = -1;
    string referenceUniqueName = "";
    string field = "ReferenceMediaItemKey";
    if (!isMetadataPresent(frameRoot, field))
    {
        field = "ReferenceIngestionJobKey";
        if (!isMetadataPresent(frameRoot, field))
        {
            field = "ReferenceUniqueName";
            if (!isMetadataPresent(frameRoot, field))
            {
                string errorMessage = __FILEREF__ + "Field is not present or it is null"
                        + ", Field: " + "Reference...";
                _logger->error(errorMessage);

                throw runtime_error(errorMessage);
            }
            else
            {
                referenceUniqueName = frameRoot.get(field, "XXX").asString();
            }        
        }
        else
        {
            referenceIngestionJobKey = frameRoot.get(field, "XXX").asInt64();
        }        
    }
    else
    {
        referenceMediaItemKey = frameRoot.get(field, "XXX").asInt64();    
    }
    
    MMSEngineDBFacade::ContentType      referenceContentType;
    try
    {
        bool warningIfMissing = true;
        if (referenceMediaItemKey != -1)
        {
            referenceContentType = _mmsEngineDBFacade->getMediaItemKeyDetails(
                referenceMediaItemKey, warningIfMissing); 
        }
        else if (referenceIngestionJobKey != -1)
        {
            pair<int64_t,MMSEngineDBFacade::ContentType> mediaItemKeyAndContentType = 
                    _mmsEngineDBFacade->getMediaItemKeyDetailsByIngestionJobKey(
                    referenceIngestionJobKey, warningIfMissing);  
            
            referenceMediaItemKey = mediaItemKeyAndContentType.first;
            referenceContentType = mediaItemKeyAndContentType.second;
        }
        else // if (referenceUniqueName != "")
        {
            pair<int64_t,MMSEngineDBFacade::ContentType> mediaItemKeyAndContentType = 
                    _mmsEngineDBFacade->getMediaItemKeyDetailsByUniqueName(
                    referenceUniqueName, warningIfMissing);  
            
            referenceMediaItemKey = mediaItemKeyAndContentType.first;
            referenceContentType = mediaItemKeyAndContentType.second;
        }
    }
    catch(runtime_error e)
    {
        string errorMessage = __FILEREF__ + "Reference... was not found"
                + ", referenceIngestionJobKey: " + to_string(referenceIngestionJobKey)
                ;
        _logger->warn(errorMessage);

        throw runtime_error(errorMessage);
    }
    catch(exception e)
    {
        string errorMessage = __FILEREF__ + "_mmsEngineDBFacade->getMediaItemKeyDetails failed"
                + ", referenceMediaItemKey: " + to_string(referenceMediaItemKey)
                + ", referenceIngestionJobKey: " + to_string(referenceIngestionJobKey)
                ;
        _logger->error(errorMessage);

        throw runtime_error(errorMessage);
    }
    
    MMSEngineDBFacade::ContentType      frameContentType = MMSEngineDBFacade::ContentType::Image;

    if (referenceContentType != MMSEngineDBFacade::ContentType::Video)
    {
        string errorMessage = __FILEREF__ + "Reference... does not refer a video content"
            + ", referenceMediaItemKey: " + to_string(referenceMediaItemKey)
            + ", referenceIngestionJobKey: " + to_string(referenceIngestionJobKey)
            + ", referenceUniqueName: " + referenceUniqueName
            + ", referenceContentType: " + MMSEngineDBFacade::toString(referenceContentType)
                ;
        _logger->error(errorMessage);

        throw runtime_error(errorMessage);
    }

    dependencies.push_back(referenceMediaItemKey);

    /*
    // Territories
    {
        field = "Territories";
        if (isMetadataPresent(contentIngestion, field))
        {
            const Json::Value territories = contentIngestion[field];
            
            for( Json::ValueIterator itr = territories.begin() ; itr != territories.end() ; itr++ ) 
            {
                Json::Value territory = territories[territoryIndex];
            }
        
    }
    */
            
    return frameContentType;
}

MMSEngineDBFacade::ContentType Validator::validatePeriodicalFramesMetadata(
    Json::Value frameRoot, vector<int64_t>& dependencies)
{
    // see sample in directory samples
        
    vector<string> mandatoryFields = {
        "SourceFileName",
        "PeriodInSeconds"
    };
    for (string mandatoryField: mandatoryFields)
    {
        if (!isMetadataPresent(frameRoot, mandatoryField))
        {
            string errorMessage = __FILEREF__ + "Field is not present or it is null"
                    + ", Field: " + mandatoryField;
            _logger->error(errorMessage);

            throw runtime_error(errorMessage);
        }
    }

    int64_t referenceMediaItemKey = -1;
    int64_t referenceIngestionJobKey = -1;
    string referenceUniqueName = "";
    string field = "ReferenceMediaItemKey";
    if (!isMetadataPresent(frameRoot, field))
    {
        field = "ReferenceIngestionJobKey";
        if (!isMetadataPresent(frameRoot, field))
        {
            field = "ReferenceUniqueName";
            if (!isMetadataPresent(frameRoot, field))
            {
                string errorMessage = __FILEREF__ + "Field is not present or it is null"
                        + ", Field: " + "Reference...";
                _logger->error(errorMessage);

                throw runtime_error(errorMessage);
            }
            else
            {
                referenceUniqueName = frameRoot.get(field, "XXX").asString();
            }        
        }
        else
        {
            referenceIngestionJobKey = frameRoot.get(field, "XXX").asInt64();
        }        
    }
    else
    {
        referenceMediaItemKey = frameRoot.get(field, "XXX").asInt64();    
    }
    
    MMSEngineDBFacade::ContentType      referenceContentType;
    try
    {
        bool warningIfMissing = true;
        if (referenceMediaItemKey != -1)
        {
            referenceContentType = _mmsEngineDBFacade->getMediaItemKeyDetails(
                referenceMediaItemKey, warningIfMissing); 
        }
        else if (referenceIngestionJobKey != -1)
        {
            pair<int64_t,MMSEngineDBFacade::ContentType> mediaItemKeyAndContentType = 
                    _mmsEngineDBFacade->getMediaItemKeyDetailsByIngestionJobKey(
                    referenceIngestionJobKey, warningIfMissing);  
            
            referenceMediaItemKey = mediaItemKeyAndContentType.first;
            referenceContentType = mediaItemKeyAndContentType.second;
        }
        else // if (referenceUniqueName != "")
        {
            pair<int64_t,MMSEngineDBFacade::ContentType> mediaItemKeyAndContentType = 
                    _mmsEngineDBFacade->getMediaItemKeyDetailsByUniqueName(
                    referenceUniqueName, warningIfMissing);  
            
            referenceMediaItemKey = mediaItemKeyAndContentType.first;
            referenceContentType = mediaItemKeyAndContentType.second;
        }
    }
    catch(runtime_error e)
    {
        string errorMessage = __FILEREF__ + "Reference... was not found"
                + ", referenceIngestionJobKey: " + to_string(referenceIngestionJobKey)
                ;
        _logger->warn(errorMessage);

        throw runtime_error(errorMessage);
    }
    catch(exception e)
    {
        string errorMessage = __FILEREF__ + "_mmsEngineDBFacade->getMediaItemKeyDetails failed"
                + ", referenceMediaItemKey: " + to_string(referenceMediaItemKey)
                + ", referenceIngestionJobKey: " + to_string(referenceIngestionJobKey)
                ;
        _logger->error(errorMessage);

        throw runtime_error(errorMessage);
    }
    
    MMSEngineDBFacade::ContentType      frameContentType = MMSEngineDBFacade::ContentType::Image;

    if (referenceContentType != MMSEngineDBFacade::ContentType::Video)
    {
        string errorMessage = __FILEREF__ + "Reference... does not refer a video content"
            + ", referenceMediaItemKey: " + to_string(referenceMediaItemKey)
            + ", referenceIngestionJobKey: " + to_string(referenceIngestionJobKey)
            + ", referenceUniqueName: " + referenceUniqueName
            + ", referenceContentType: " + MMSEngineDBFacade::toString(referenceContentType)
                ;
        _logger->error(errorMessage);

        throw runtime_error(errorMessage);
    }

    dependencies.push_back(referenceMediaItemKey);

    /*
    // Territories
    {
        field = "Territories";
        if (isMetadataPresent(contentIngestion, field))
        {
            const Json::Value territories = contentIngestion[field];
            
            for( Json::ValueIterator itr = territories.begin() ; itr != territories.end() ; itr++ ) 
            {
                Json::Value territory = territories[territoryIndex];
            }
        
    }
    */
            
    return frameContentType;
}

bool Validator::isMetadataPresent(Json::Value root, string field)
{
    if (root.isObject() && root.isMember(field) && !root[field].isNull())
        return true;
    else
        return false;
}
