/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   EnodingsManager.cpp
 * Author: giuliano
 * 
 * Created on February 4, 2018, 7:18 PM
 */

#include <fstream>
#include <sstream>
#ifdef __LOCALENCODER__
#else
    #include <curlpp/cURLpp.hpp>
    #include <curlpp/Easy.hpp>
    #include <curlpp/Options.hpp>
    #include <curlpp/Exception.hpp>
    #include <curlpp/Infos.hpp>
#endif
#include "catralibraries/ProcessUtility.h"
#include "LocalAssetIngestionEvent.h"
#include "MultiLocalAssetIngestionEvent.h"
#include "catralibraries/Convert.h"
#include "Validator.h"
#include "FFMpeg.h"
#include "EncoderVideoAudioProxy.h"
#include "opencv2/objdetect.hpp"
#include "opencv2/face.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/imgcodecs.hpp"


EncoderVideoAudioProxy::EncoderVideoAudioProxy()
{
}

EncoderVideoAudioProxy::~EncoderVideoAudioProxy() 
{
}

void EncoderVideoAudioProxy::init(
        int proxyIdentifier,
        mutex* mtEncodingJobs,
        Json::Value configuration,
        shared_ptr<MultiEventsSet> multiEventsSet,
        shared_ptr<MMSEngineDBFacade> mmsEngineDBFacade,
        shared_ptr<MMSStorage> mmsStorage,
        shared_ptr<EncodersLoadBalancer> encodersLoadBalancer,
        #ifdef __LOCALENCODER__
            int* pRunningEncodingsNumber,
        #endif
        shared_ptr<spdlog::logger> logger
)
{
    _proxyIdentifier        = proxyIdentifier;
    
    _mtEncodingJobs         = mtEncodingJobs;
    
    _logger                 = logger;
    _configuration          = configuration;
    
    _multiEventsSet         = multiEventsSet;
    _mmsEngineDBFacade      = mmsEngineDBFacade;
    _mmsStorage             = mmsStorage;
    _encodersLoadBalancer   = encodersLoadBalancer;
    
    _mp4Encoder             = _configuration["encoding"].get("mp4Encoder", "").asString();
    _logger->info(__FILEREF__ + "Configuration item"
        + ", encoding->mp4Encoder: " + _mp4Encoder
    );
    _mpeg2TSEncoder         = _configuration["encoding"].get("mpeg2TSEncoder", "").asString();
    _logger->info(__FILEREF__ + "Configuration item"
        + ", encoding->mpeg2TSEncoder: " + _mpeg2TSEncoder
    );
    
    _intervalInSecondsToCheckEncodingFinished         = _configuration["encoding"].get("intervalInSecondsToCheckEncodingFinished", "").asInt();
    _logger->info(__FILEREF__ + "Configuration item"
        + ", encoding->intervalInSecondsToCheckEncodingFinished: " + to_string(_intervalInSecondsToCheckEncodingFinished)
    );        
    _secondsToWaitNFSBuffers						= _configuration["encoding"].get("secondsToWaitNFSBuffers", "").asInt();
    _logger->info(__FILEREF__ + "Configuration item"
        + ", encoding->secondsToWaitNFSBuffers: " + to_string(_secondsToWaitNFSBuffers)
    );        
    
    _ffmpegEncoderProtocol = _configuration["ffmpeg"].get("encoderProtocol", "").asString();
    _logger->info(__FILEREF__ + "Configuration item"
        + ", ffmpeg->encoderProtocol: " + _ffmpegEncoderProtocol
    );
    _ffmpegEncoderPort = _configuration["ffmpeg"].get("encoderPort", "").asInt();
    _logger->info(__FILEREF__ + "Configuration item"
        + ", ffmpeg->encoderPort: " + to_string(_ffmpegEncoderPort)
    );
    _ffmpegEncoderUser = _configuration["ffmpeg"].get("encoderUser", "").asString();
    _logger->info(__FILEREF__ + "Configuration item"
        + ", ffmpeg->encoderUser: " + _ffmpegEncoderUser
    );
    _ffmpegEncoderPassword = _configuration["ffmpeg"].get("encoderPassword", "").asString();
    _logger->info(__FILEREF__ + "Configuration item"
        + ", ffmpeg->encoderPassword: " + "..."
    );
    _ffmpegEncoderProgressURI = _configuration["ffmpeg"].get("encoderProgressURI", "").asString();
    _logger->info(__FILEREF__ + "Configuration item"
        + ", ffmpeg->encoderProgressURI: " + _ffmpegEncoderProgressURI
    );
    _ffmpegEncoderStatusURI = _configuration["ffmpeg"].get("encoderStatusURI", "").asString();
    _logger->info(__FILEREF__ + "Configuration item"
        + ", ffmpeg->encoderStatusURI: " + _ffmpegEncoderStatusURI
    );
    _ffmpegEncodeURI = _configuration["ffmpeg"].get("encodeURI", "").asString();
    _logger->info(__FILEREF__ + "Configuration item"
        + ", ffmpeg->encodeURI: " + _ffmpegEncodeURI
    );
    _ffmpegOverlayImageOnVideoURI = _configuration["ffmpeg"].get("overlayImageOnVideoURI", "").asString();
    _logger->info(__FILEREF__ + "Configuration item"
        + ", ffmpeg->overlayImageOnVideoURI: " + _ffmpegOverlayImageOnVideoURI
    );
    _ffmpegOverlayTextOnVideoURI = _configuration["ffmpeg"].get("overlayTextOnVideoURI", "").asString();
    _logger->info(__FILEREF__ + "Configuration item"
        + ", ffmpeg->overlayTextOnVideoURI: " + _ffmpegOverlayTextOnVideoURI
    );
    _ffmpegGenerateFramesURI = _configuration["ffmpeg"].get("generateFramesURI", "").asString();
    _logger->info(__FILEREF__ + "Configuration item"
        + ", ffmpeg->generateFramesURI: " + _ffmpegGenerateFramesURI
    );
    _ffmpegSlideShowURI = _configuration["ffmpeg"].get("slideShowURI", "").asString();
    _logger->info(__FILEREF__ + "Configuration item"
        + ", ffmpeg->slideShowURI: " + _ffmpegSlideShowURI
    );
    _ffmpegLiveRecorderURI = _configuration["ffmpeg"].get("liveRecorderURI", "").asString();
    _logger->info(__FILEREF__ + "Configuration item"
        + ", ffmpeg->liveRecorderURI: " + _ffmpegLiveRecorderURI
    );
            
    _computerVisionCascadePath             = configuration["computerVision"].get("cascadePath", "XXX").asString();
    _logger->info(__FILEREF__ + "Configuration item"
        + ", computerVision->cascadePath: " + _computerVisionCascadePath
    );
	if (_computerVisionCascadePath.back() == '/')
		_computerVisionCascadePath.pop_back();
    _computerVisionDefaultScale				= configuration["computerVision"].get("defaultScale", 1.1).asDouble();
    _logger->info(__FILEREF__ + "Configuration item"
        + ", computerVision->defaultScale: " + to_string(_computerVisionDefaultScale)
    );
    _computerVisionDefaultMinNeighbors		= configuration["computerVision"].get("defaultMinNeighbors", 2).asInt();
    _logger->info(__FILEREF__ + "Configuration item"
        + ", computerVision->defaultMinNeighbors: " + to_string(_computerVisionDefaultMinNeighbors)
    );
    _computerVisionDefaultTryFlip		= configuration["computerVision"].get("defaultTryFlip", 2).asBool();
    _logger->info(__FILEREF__ + "Configuration item"
        + ", computerVision->defaultTryFlip: " + to_string(_computerVisionDefaultTryFlip)
    );

    #ifdef __LOCALENCODER__
        _ffmpegMaxCapacity      = 1;
        
        _pRunningEncodingsNumber  = pRunningEncodingsNumber;
        
        _ffmpeg = make_shared<FFMpeg>(configuration, logger);
    #endif
}

void EncoderVideoAudioProxy::setEncodingData(
        EncodingJobStatus* status,
        shared_ptr<MMSEngineDBFacade::EncodingItem> encodingItem
)
{
    _status                 = status;
    
    _encodingItem           = encodingItem;            
}

void EncoderVideoAudioProxy::operator()()
{
    
    _logger->info(__FILEREF__ + "Running EncoderVideoAudioProxy..."
        + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
        + ", _encodingItem->_encodingJobKey: " + to_string(_encodingItem->_encodingJobKey)
        + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey)
        + ", _encodingItem->_encodingType: " + MMSEngineDBFacade::toString(_encodingItem->_encodingType)
        + ", _encodingItem->_encodingParameters: " + _encodingItem->_encodingParameters
    );

    string stagingEncodedAssetPathName;
    try
    {
        if (_encodingItem->_encodingType == MMSEngineDBFacade::EncodingType::EncodeVideoAudio)
        {
            stagingEncodedAssetPathName = encodeContentVideoAudio();
        }
        else if (_encodingItem->_encodingType == MMSEngineDBFacade::EncodingType::OverlayImageOnVideo)
        {
            stagingEncodedAssetPathName = overlayImageOnVideo();
        }
        else if (_encodingItem->_encodingType == MMSEngineDBFacade::EncodingType::OverlayTextOnVideo)
        {
            stagingEncodedAssetPathName = overlayTextOnVideo();
        }
        else if (_encodingItem->_encodingType == MMSEngineDBFacade::EncodingType::GenerateFrames)
        {
            generateFrames();
        }
        else if (_encodingItem->_encodingType == MMSEngineDBFacade::EncodingType::SlideShow)
        {
            stagingEncodedAssetPathName = slideShow();
        }
        else if (_encodingItem->_encodingType == MMSEngineDBFacade::EncodingType::FaceRecognition)
        {
            stagingEncodedAssetPathName = faceRecognition();
        }
        else if (_encodingItem->_encodingType == MMSEngineDBFacade::EncodingType::FaceIdentification)
        {
            stagingEncodedAssetPathName = faceIdentification();
        }
        else if (_encodingItem->_encodingType == MMSEngineDBFacade::EncodingType::LiveRecorder)
        {
            stagingEncodedAssetPathName = liveRecorder();
        }
        else
        {
            string errorMessage = string("Wrong EncodingType")
                    + ", EncodingType: " + MMSEngineDBFacade::toString(_encodingItem->_encodingType)
                    ;
            
            _logger->error(__FILEREF__ + errorMessage);
            
            throw runtime_error(errorMessage);
        }
    }
    catch(MaxConcurrentJobsReached e)
    {
        if (_encodingItem->_encodingType == MMSEngineDBFacade::EncodingType::EncodeVideoAudio)
        {
            _logger->warn(__FILEREF__ + "encodeContentVideoAudio: " + e.what()
                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
            );
        }
        else if (_encodingItem->_encodingType == MMSEngineDBFacade::EncodingType::OverlayImageOnVideo)
        {
            _logger->warn(__FILEREF__ + "overlayImageOnVideo: " + e.what()
                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
            );
        }
        else if (_encodingItem->_encodingType == MMSEngineDBFacade::EncodingType::OverlayTextOnVideo)
        {
            _logger->warn(__FILEREF__ + "overlayTextOnVideo: " + e.what()
                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
            );
        }
        else if (_encodingItem->_encodingType == MMSEngineDBFacade::EncodingType::GenerateFrames)
        {
            _logger->warn(__FILEREF__ + "generateFrames: " + e.what()
                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
            );
        }
        else if (_encodingItem->_encodingType == MMSEngineDBFacade::EncodingType::SlideShow)
        {
            _logger->warn(__FILEREF__ + "slideShow: " + e.what()
                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
            );
        }
        else if (_encodingItem->_encodingType == MMSEngineDBFacade::EncodingType::FaceRecognition)
        {
            _logger->warn(__FILEREF__ + "faceRecognition: " + e.what()
                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
            );
        }
        else if (_encodingItem->_encodingType == MMSEngineDBFacade::EncodingType::FaceIdentification)
        {
            _logger->warn(__FILEREF__ + "faceIdentification: " + e.what()
                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
            );
        }
        else if (_encodingItem->_encodingType == MMSEngineDBFacade::EncodingType::LiveRecorder)
        {
            _logger->warn(__FILEREF__ + "liveRecorder: " + e.what()
                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
            );
        }

        _logger->info(__FILEREF__ + "_mmsEngineDBFacade->updateEncodingJob MaxCapacityReached"
            + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
            + ", _encodingItem->_encodingJobKey: " + to_string(_encodingItem->_encodingJobKey)
            + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey)
            + ", _encodingItem->_encodingType: " + MMSEngineDBFacade::toString(_encodingItem->_encodingType)
            + ", _encodingItem->_encodingParameters: " + _encodingItem->_encodingParameters
        );

        int64_t mediaItemKey = -1;
        int64_t encodedPhysicalPathKey = -1;
        _mmsEngineDBFacade->updateEncodingJob (_encodingItem->_encodingJobKey, 
                MMSEngineDBFacade::EncodingError::MaxCapacityReached, 
                mediaItemKey, encodedPhysicalPathKey,
                _encodingItem->_ingestionJobKey);

        {
            lock_guard<mutex> locker(*_mtEncodingJobs);

            *_status = EncodingJobStatus::Free;
        }

        _logger->info(__FILEREF__ + "EncoderVideoAudioProxy finished"
            + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
            + ", _encodingItem->_encodingJobKey: " + to_string(_encodingItem->_encodingJobKey)
            + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey)
            + ", _encodingItem->_encodingType: " + MMSEngineDBFacade::toString(_encodingItem->_encodingType)
            + ", _encodingItem->_encodingParameters: " + _encodingItem->_encodingParameters
        );

        // throw e;
        return;
    }
    catch(EncoderError e)
    {
        if (_encodingItem->_encodingType == MMSEngineDBFacade::EncodingType::EncodeVideoAudio)
        {
            _logger->error(__FILEREF__ + "encodeContentVideoAudio: " + e.what()
                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
            );
        }
        else if (_encodingItem->_encodingType == MMSEngineDBFacade::EncodingType::OverlayImageOnVideo)
        {
            _logger->error(__FILEREF__ + "overlayImageOnVideo: " + e.what()
                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
            );
        }
        else if (_encodingItem->_encodingType == MMSEngineDBFacade::EncodingType::OverlayTextOnVideo)
        {
            _logger->error(__FILEREF__ + "overlayTextOnVideo: " + e.what()
                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
            );
        }
        else if (_encodingItem->_encodingType == MMSEngineDBFacade::EncodingType::GenerateFrames)
        {
            _logger->error(__FILEREF__ + "generateFrames: " + e.what()
                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
            );
        }
        else if (_encodingItem->_encodingType == MMSEngineDBFacade::EncodingType::SlideShow)
        {
            _logger->error(__FILEREF__ + "slideShow: " + e.what()
                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
            );
        }
        else if (_encodingItem->_encodingType == MMSEngineDBFacade::EncodingType::FaceRecognition)
        {
            _logger->error(__FILEREF__ + "faceRecognition: " + e.what()
                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
            );
        }
        else if (_encodingItem->_encodingType == MMSEngineDBFacade::EncodingType::FaceIdentification)
        {
            _logger->warn(__FILEREF__ + "faceIdentification: " + e.what()
                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
            );
        }
        else if (_encodingItem->_encodingType == MMSEngineDBFacade::EncodingType::LiveRecorder)
        {
            _logger->warn(__FILEREF__ + "liveRecorder: " + e.what()
                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
            );
        }

        _logger->info(__FILEREF__ + "_mmsEngineDBFacade->updateEncodingJob PunctualError"
            + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
            + ", _encodingItem->_encodingJobKey: " + to_string(_encodingItem->_encodingJobKey)
            + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey)
            + ", _encodingItem->_encodingType: " + MMSEngineDBFacade::toString(_encodingItem->_encodingType)
            + ", _encodingItem->_encodingParameters: " + _encodingItem->_encodingParameters
        );

        int64_t mediaItemKey = -1;
        int64_t encodedPhysicalPathKey = -1;
        int encodingFailureNumber = _mmsEngineDBFacade->updateEncodingJob (_encodingItem->_encodingJobKey, 
                MMSEngineDBFacade::EncodingError::PunctualError, 
                mediaItemKey, encodedPhysicalPathKey,
                _encodingItem->_ingestionJobKey);

        _logger->info(__FILEREF__ + "_mmsEngineDBFacade->updateEncodingJob PunctualError"
            + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
            + ", _encodingItem->_encodingJobKey: " + to_string(_encodingItem->_encodingJobKey)
            + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey)
            + ", encodingFailureNumber: " + to_string(encodingFailureNumber)
            + ", _encodingItem->_encodingType: " + MMSEngineDBFacade::toString(_encodingItem->_encodingType)
            + ", _encodingItem->_encodingParameters: " + _encodingItem->_encodingParameters
        );

        {
            lock_guard<mutex> locker(*_mtEncodingJobs);

            *_status = EncodingJobStatus::Free;
        }

        _logger->info(__FILEREF__ + "EncoderVideoAudioProxy finished"
            + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
            + ", _encodingItem->_encodingJobKey: " + to_string(_encodingItem->_encodingJobKey)
            + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey)
            + ", _encodingItem->_encodingType: " + MMSEngineDBFacade::toString(_encodingItem->_encodingType)
            + ", _encodingItem->_encodingParameters: " + _encodingItem->_encodingParameters
        );

        // throw e;
        return;
    }
    catch(runtime_error e)
    {
        if (_encodingItem->_encodingType == MMSEngineDBFacade::EncodingType::EncodeVideoAudio)
        {
            _logger->error(__FILEREF__ + "encodeContentVideoAudio: " + e.what()
                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
            );
        }
        else if (_encodingItem->_encodingType == MMSEngineDBFacade::EncodingType::OverlayImageOnVideo)
        {
            _logger->error(__FILEREF__ + "overlayImageOnVideo: " + e.what()
                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
            );
        }
        else if (_encodingItem->_encodingType == MMSEngineDBFacade::EncodingType::OverlayTextOnVideo)
        {
            _logger->error(__FILEREF__ + "overlayTextOnVideo: " + e.what()
                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
            );
        }
        else if (_encodingItem->_encodingType == MMSEngineDBFacade::EncodingType::GenerateFrames)
        {
            _logger->error(__FILEREF__ + "generateFrames: " + e.what()
                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
            );
        }
        else if (_encodingItem->_encodingType == MMSEngineDBFacade::EncodingType::SlideShow)
        {
            _logger->error(__FILEREF__ + "slideShow: " + e.what()
                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
            );
        }
        else if (_encodingItem->_encodingType == MMSEngineDBFacade::EncodingType::FaceRecognition)
        {
            _logger->error(__FILEREF__ + "faceRecognition: " + e.what()
                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
            );
        }
        else if (_encodingItem->_encodingType == MMSEngineDBFacade::EncodingType::FaceIdentification)
        {
            _logger->warn(__FILEREF__ + "faceIdentification: " + e.what()
                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
            );
        }
        else if (_encodingItem->_encodingType == MMSEngineDBFacade::EncodingType::LiveRecorder)
        {
            _logger->warn(__FILEREF__ + "liveRecorder: " + e.what()
                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
            );
        }

        _logger->info(__FILEREF__ + "_mmsEngineDBFacade->updateEncodingJob PunctualError"
            + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
            + ", _encodingItem->_encodingJobKey: " + to_string(_encodingItem->_encodingJobKey)
            + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey)
            + ", _encodingItem->_encodingType: " + MMSEngineDBFacade::toString(_encodingItem->_encodingType)
            + ", _encodingItem->_encodingParameters: " + _encodingItem->_encodingParameters
        );

        int64_t mediaItemKey = -1;
        int64_t encodedPhysicalPathKey = -1;
        // PunctualError is used because, in case it always happens, the encoding will never reach a final state
        int encodingFailureNumber = _mmsEngineDBFacade->updateEncodingJob (
                _encodingItem->_encodingJobKey, 
                MMSEngineDBFacade::EncodingError::PunctualError,    // ErrorBeforeEncoding, 
                mediaItemKey, encodedPhysicalPathKey,
                _encodingItem->_ingestionJobKey);

        _logger->info(__FILEREF__ + "_mmsEngineDBFacade->updateEncodingJob PunctualError"
            + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
            + ", _encodingItem->_encodingJobKey: " + to_string(_encodingItem->_encodingJobKey)
            + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey)
            + ", _encodingItem->_encodingType: " + MMSEngineDBFacade::toString(_encodingItem->_encodingType)
            + ", _encodingItem->_encodingParameters: " + _encodingItem->_encodingParameters
            + ", encodingFailureNumber: " + to_string(encodingFailureNumber)
        );

        {
            lock_guard<mutex> locker(*_mtEncodingJobs);

            *_status = EncodingJobStatus::Free;
        }

        _logger->info(__FILEREF__ + "EncoderVideoAudioProxy finished"
            + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
            + ", _encodingItem->_encodingJobKey: " + to_string(_encodingItem->_encodingJobKey)
            + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey)
            + ", _encodingItem->_encodingType: " + MMSEngineDBFacade::toString(_encodingItem->_encodingType)
            + ", _encodingItem->_encodingParameters: " + _encodingItem->_encodingParameters
        );

        // throw e;
        return;
    }
    catch(exception e)
    {
        if (_encodingItem->_encodingType == MMSEngineDBFacade::EncodingType::EncodeVideoAudio)
        {
            _logger->error(__FILEREF__ + "encodeContentVideoAudio: " + e.what()
                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
            );
        }
        else if (_encodingItem->_encodingType == MMSEngineDBFacade::EncodingType::OverlayImageOnVideo)
        {
            _logger->error(__FILEREF__ + "overlayImageOnVideo: " + e.what()
                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
            );
        }
        else if (_encodingItem->_encodingType == MMSEngineDBFacade::EncodingType::OverlayTextOnVideo)
        {
            _logger->error(__FILEREF__ + "overlayTextOnVideo: " + e.what()
                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
            );
        }
        else if (_encodingItem->_encodingType == MMSEngineDBFacade::EncodingType::GenerateFrames)
        {
            _logger->error(__FILEREF__ + "generateFrames: " + e.what()
                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
            );
        }
        else if (_encodingItem->_encodingType == MMSEngineDBFacade::EncodingType::SlideShow)
        {
            _logger->error(__FILEREF__ + "slideShow: " + e.what()
                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
            );
        }
        else if (_encodingItem->_encodingType == MMSEngineDBFacade::EncodingType::FaceRecognition)
        {
            _logger->error(__FILEREF__ + "faceRecognition: " + e.what()
                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
            );
        }
        else if (_encodingItem->_encodingType == MMSEngineDBFacade::EncodingType::FaceIdentification)
        {
            _logger->warn(__FILEREF__ + "faceIdentification: " + e.what()
                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
            );
        }
        else if (_encodingItem->_encodingType == MMSEngineDBFacade::EncodingType::LiveRecorder)
        {
            _logger->warn(__FILEREF__ + "liveRecorder: " + e.what()
                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
            );
        }

        _logger->info(__FILEREF__ + "_mmsEngineDBFacade->updateEncodingJob PunctualError"
            + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
            + ", _encodingItem->_encodingJobKey: " + to_string(_encodingItem->_encodingJobKey)
            + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey)
            + ", _encodingItem->_encodingType: " + MMSEngineDBFacade::toString(_encodingItem->_encodingType)
            + ", _encodingItem->_encodingParameters: " + _encodingItem->_encodingParameters
        );

        int64_t mediaItemKey = -1;
        int64_t encodedPhysicalPathKey = -1;
        // PunctualError is used because, in case it always happens, the encoding will never reach a final state
        int encodingFailureNumber = _mmsEngineDBFacade->updateEncodingJob (
                _encodingItem->_encodingJobKey, 
                MMSEngineDBFacade::EncodingError::PunctualError,    // ErrorBeforeEncoding, 
                mediaItemKey, encodedPhysicalPathKey,
                _encodingItem->_ingestionJobKey);

        _logger->info(__FILEREF__ + "_mmsEngineDBFacade->updateEncodingJob PunctualError"
            + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
            + ", _encodingItem->_encodingJobKey: " + to_string(_encodingItem->_encodingJobKey)
            + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey)
            + ", _encodingItem->_encodingType: " + MMSEngineDBFacade::toString(_encodingItem->_encodingType)
            + ", _encodingItem->_encodingParameters: " + _encodingItem->_encodingParameters
            + ", encodingFailureNumber: " + to_string(encodingFailureNumber)
        );

        {
            lock_guard<mutex> locker(*_mtEncodingJobs);

            *_status = EncodingJobStatus::Free;
        }

        _logger->info(__FILEREF__ + "EncoderVideoAudioProxy finished"
            + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
            + ", _encodingItem->_encodingJobKey: " + to_string(_encodingItem->_encodingJobKey)
            + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey)
            + ", _encodingItem->_encodingType: " + MMSEngineDBFacade::toString(_encodingItem->_encodingType)
            + ", _encodingItem->_encodingParameters: " + _encodingItem->_encodingParameters
        );

        // throw e;
        return;
    }

    int64_t mediaItemKey;
    int64_t encodedPhysicalPathKey;

    try
    {
        if (_encodingItem->_encodingType == MMSEngineDBFacade::EncodingType::EncodeVideoAudio)
        {
            mediaItemKey = _encodingItem->_encodeData->_mediaItemKey;

            encodedPhysicalPathKey = processEncodedContentVideoAudio(
                stagingEncodedAssetPathName);
        }
        else if (_encodingItem->_encodingType == MMSEngineDBFacade::EncodingType::OverlayImageOnVideo)
        {
//            pair<int64_t,int64_t> mediaItemKeyAndPhysicalPathKey = processOverlayedImageOnVideo(
//                stagingEncodedAssetPathName);

            processOverlayedImageOnVideo(stagingEncodedAssetPathName);
            
            mediaItemKey = -1;
            encodedPhysicalPathKey = -1;
            
//            mediaItemKey = mediaItemKeyAndPhysicalPathKey.first;
//            encodedPhysicalPathKey = mediaItemKeyAndPhysicalPathKey.second;
        }
        else if (_encodingItem->_encodingType == MMSEngineDBFacade::EncodingType::OverlayTextOnVideo)
        {
            /*
            pair<int64_t,int64_t> mediaItemKeyAndPhysicalPathKey = processOverlayedTextOnVideo(
                stagingEncodedAssetPathName);
            
            mediaItemKey = mediaItemKeyAndPhysicalPathKey.first;
            encodedPhysicalPathKey = mediaItemKeyAndPhysicalPathKey.second;
             */
            processOverlayedTextOnVideo(stagingEncodedAssetPathName);     
            
            mediaItemKey = -1;
            encodedPhysicalPathKey = -1;
        }
        else if (_encodingItem->_encodingType == MMSEngineDBFacade::EncodingType::GenerateFrames)
        {
            processGeneratedFrames();     
            
            mediaItemKey = -1;
            encodedPhysicalPathKey = -1;
        }
        else if (_encodingItem->_encodingType == MMSEngineDBFacade::EncodingType::SlideShow)
        {
            processSlideShow(stagingEncodedAssetPathName);
            
            mediaItemKey = -1;
            encodedPhysicalPathKey = -1;            
        }
        else if (_encodingItem->_encodingType == MMSEngineDBFacade::EncodingType::FaceRecognition)
        {
            processFaceRecognition(stagingEncodedAssetPathName);
            
            mediaItemKey = -1;
            encodedPhysicalPathKey = -1;            
        }
        else if (_encodingItem->_encodingType == MMSEngineDBFacade::EncodingType::FaceIdentification)
        {
            processFaceIdentification(stagingEncodedAssetPathName);
            
            mediaItemKey = -1;
            encodedPhysicalPathKey = -1;            
        }
        else if (_encodingItem->_encodingType == MMSEngineDBFacade::EncodingType::LiveRecorder)
        {
            processLiveRecorder(stagingEncodedAssetPathName);
            
            mediaItemKey = -1;
            encodedPhysicalPathKey = -1;            
        }
        else
        {
            string errorMessage = string("Wrong EncodingType")
                    + ", EncodingType: " + MMSEngineDBFacade::toString(_encodingItem->_encodingType)
                    ;
            
            _logger->error(__FILEREF__ + errorMessage);
            
            throw runtime_error(errorMessage);
        }
    }
    catch(runtime_error e)
    {
        if (_encodingItem->_encodingType == MMSEngineDBFacade::EncodingType::EncodeVideoAudio)
        {
            _logger->error(__FILEREF__ + "processEncodedContentVideoAudio failed: " + e.what()
                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
            );
        }
        else if (_encodingItem->_encodingType == MMSEngineDBFacade::EncodingType::OverlayImageOnVideo)
        {
            _logger->error(__FILEREF__ + "processOverlayedImageOnVideo failed: " + e.what()
                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
            );
        }
        else if (_encodingItem->_encodingType == MMSEngineDBFacade::EncodingType::OverlayTextOnVideo)
        {
            _logger->error(__FILEREF__ + "processOverlayedTextOnVideo failed: " + e.what()
                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
            );
        }
        else if (_encodingItem->_encodingType == MMSEngineDBFacade::EncodingType::GenerateFrames)
        {
            _logger->error(__FILEREF__ + "processGeneratedFrames failed: " + e.what()
                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
            );
        }
        else if (_encodingItem->_encodingType == MMSEngineDBFacade::EncodingType::SlideShow)
        {
            _logger->error(__FILEREF__ + "processSlideShow failed: " + e.what()
                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
            );
        }
        else if (_encodingItem->_encodingType == MMSEngineDBFacade::EncodingType::FaceRecognition)
        {
            _logger->error(__FILEREF__ + "processFaceRecognition failed: " + e.what()
                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
            );
        }
        else if (_encodingItem->_encodingType == MMSEngineDBFacade::EncodingType::FaceIdentification)
        {
            _logger->error(__FILEREF__ + "processFaceIdentification failed: " + e.what()
                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
            );
        }
        else if (_encodingItem->_encodingType == MMSEngineDBFacade::EncodingType::LiveRecorder)
        {
            _logger->error(__FILEREF__ + "processLiveRecorder failed: " + e.what()
                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
            );
        }

        if (FileIO::fileExisting(stagingEncodedAssetPathName) || 
                FileIO::directoryExisting(stagingEncodedAssetPathName))
        {
            FileIO::DirectoryEntryType_t detSourceFileType = FileIO::getDirectoryEntryType(stagingEncodedAssetPathName);

            _logger->error(__FILEREF__ + "Remove"
                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                + ", stagingEncodedAssetPathName: " + stagingEncodedAssetPathName
            );

            // file in case of .3gp content OR directory in case of IPhone content
            if (detSourceFileType == FileIO::TOOLS_FILEIO_DIRECTORY)
            {
                Boolean_t bRemoveRecursively = true;
                _logger->info(__FILEREF__ + "removeDirectory"
                    + ", stagingEncodedAssetPathName: " + stagingEncodedAssetPathName
                );
                FileIO::removeDirectory(stagingEncodedAssetPathName, bRemoveRecursively);
            }
            else if (detSourceFileType == FileIO::TOOLS_FILEIO_REGULARFILE) 
            {
                _logger->info(__FILEREF__ + "remove"
                    + ", stagingEncodedAssetPathName: " + stagingEncodedAssetPathName
                );
                FileIO::remove(stagingEncodedAssetPathName);
            }
        }

        _logger->info(__FILEREF__ + "_mmsEngineDBFacade->updateEncodingJob PunctualError"
            + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
            + ", _encodingItem->_encodingJobKey: " + to_string(_encodingItem->_encodingJobKey)
            + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey)
            + ", _encodingItem->_encodingType: " + MMSEngineDBFacade::toString(_encodingItem->_encodingType)
            + ", _encodingItem->_encodingParameters: " + _encodingItem->_encodingParameters
        );

        int64_t mediaItemKey = -1;
        encodedPhysicalPathKey = -1;
        // PunctualError is used because, in case it always happens, the encoding will never reach a final state
        int encodingFailureNumber = _mmsEngineDBFacade->updateEncodingJob (
                _encodingItem->_encodingJobKey, 
                MMSEngineDBFacade::EncodingError::PunctualError,    // ErrorBeforeEncoding, 
                mediaItemKey, encodedPhysicalPathKey,
                _encodingItem->_ingestionJobKey);

        _logger->info(__FILEREF__ + "_mmsEngineDBFacade->updateEncodingJob PunctualError"
            + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
            + ", _encodingItem->_encodingJobKey: " + to_string(_encodingItem->_encodingJobKey)
            + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey)
            + ", _encodingItem->_encodingType: " + MMSEngineDBFacade::toString(_encodingItem->_encodingType)
            + ", _encodingItem->_encodingParameters: " + _encodingItem->_encodingParameters
            + ", encodingFailureNumber: " + to_string(encodingFailureNumber)
        );

        {
            lock_guard<mutex> locker(*_mtEncodingJobs);

            *_status = EncodingJobStatus::Free;
        }

        _logger->info(__FILEREF__ + "EncoderVideoAudioProxy finished"
            + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
            + ", _encodingItem->_encodingJobKey: " + to_string(_encodingItem->_encodingJobKey)
            + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey)
            + ", _encodingItem->_encodingType: " + MMSEngineDBFacade::toString(_encodingItem->_encodingType)
            + ", _encodingItem->_encodingParameters: " + _encodingItem->_encodingParameters
        );

        // throw e;
        return;
    }
    catch(exception e)
    {
        if (_encodingItem->_encodingType == MMSEngineDBFacade::EncodingType::EncodeVideoAudio)
        {
            _logger->error(__FILEREF__ + "processEncodedContentVideoAudio failed: " + e.what()
                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
            );
        }
        else if (_encodingItem->_encodingType == MMSEngineDBFacade::EncodingType::OverlayImageOnVideo)
        {
            _logger->error(__FILEREF__ + "processOverlayedImageOnVideo failed: " + e.what()
                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
            );
        }
        else if (_encodingItem->_encodingType == MMSEngineDBFacade::EncodingType::OverlayTextOnVideo)
        {
            _logger->error(__FILEREF__ + "processOverlayedTextOnVideo failed: " + e.what()
                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
            );
        }
        else if (_encodingItem->_encodingType == MMSEngineDBFacade::EncodingType::GenerateFrames)
        {
            _logger->error(__FILEREF__ + "processGeneratedFrames failed: " + e.what()
                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
            );
        }
        else if (_encodingItem->_encodingType == MMSEngineDBFacade::EncodingType::SlideShow)
        {
            _logger->error(__FILEREF__ + "processSlideShow failed: " + e.what()
                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
            );
        }
        else if (_encodingItem->_encodingType == MMSEngineDBFacade::EncodingType::FaceRecognition)
        {
            _logger->error(__FILEREF__ + "processFaceRecognition failed: " + e.what()
                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
            );
        }
        else if (_encodingItem->_encodingType == MMSEngineDBFacade::EncodingType::FaceIdentification)
        {
            _logger->error(__FILEREF__ + "processFaceIdentification failed: " + e.what()
                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
            );
        }
        else if (_encodingItem->_encodingType == MMSEngineDBFacade::EncodingType::LiveRecorder)
        {
            _logger->error(__FILEREF__ + "processLiveRecorder failed: " + e.what()
                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
            );
        }

        if (FileIO::fileExisting(stagingEncodedAssetPathName)
                || FileIO::directoryExisting(stagingEncodedAssetPathName))
        {
            FileIO::DirectoryEntryType_t detSourceFileType = FileIO::getDirectoryEntryType(stagingEncodedAssetPathName);

            _logger->error(__FILEREF__ + "Remove"
                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                + ", stagingEncodedAssetPathName: " + stagingEncodedAssetPathName
            );

            // file in case of .3gp content OR directory in case of IPhone content
            if (detSourceFileType == FileIO::TOOLS_FILEIO_DIRECTORY)
            {
                _logger->info(__FILEREF__ + "removeDirectory"
                    + ", stagingEncodedAssetPathName: " + stagingEncodedAssetPathName
                );
                Boolean_t bRemoveRecursively = true;
                FileIO::removeDirectory(stagingEncodedAssetPathName, bRemoveRecursively);
            }
            else if (detSourceFileType == FileIO::TOOLS_FILEIO_REGULARFILE) 
            {
                _logger->info(__FILEREF__ + "remove"
                    + ", stagingEncodedAssetPathName: " + stagingEncodedAssetPathName
                );
                FileIO::remove(stagingEncodedAssetPathName);
            }
        }

        _logger->info(__FILEREF__ + "_mmsEngineDBFacade->updateEncodingJob PunctualError"
            + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
            + ", _encodingItem->_encodingJobKey: " + to_string(_encodingItem->_encodingJobKey)
            + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey)
            + ", _encodingItem->_encodingType: " + MMSEngineDBFacade::toString(_encodingItem->_encodingType)
            + ", _encodingItem->_encodingParameters: " + _encodingItem->_encodingParameters
        );

        int64_t mediaItemKey = -1;
        int64_t encodedPhysicalPathKey = -1;
        // PunctualError is used because, in case it always happens, the encoding will never reach a final state
        int encodingFailureNumber = _mmsEngineDBFacade->updateEncodingJob (
                _encodingItem->_encodingJobKey, 
                MMSEngineDBFacade::EncodingError::PunctualError,    // ErrorBeforeEncoding, 
                mediaItemKey, encodedPhysicalPathKey,
                _encodingItem->_ingestionJobKey);

        _logger->info(__FILEREF__ + "_mmsEngineDBFacade->updateEncodingJob PunctualError"
            + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
            + ", _encodingItem->_encodingJobKey: " + to_string(_encodingItem->_encodingJobKey)
            + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey)
            + ", _encodingItem->_encodingType: " + MMSEngineDBFacade::toString(_encodingItem->_encodingType)
            + ", _encodingItem->_encodingParameters: " + _encodingItem->_encodingParameters
            + ", encodingFailureNumber: " + to_string(encodingFailureNumber)
        );

        {
            lock_guard<mutex> locker(*_mtEncodingJobs);

            *_status = EncodingJobStatus::Free;
        }

        _logger->info(__FILEREF__ + "EncoderVideoAudioProxy finished"
            + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
            + ", _encodingItem->_encodingJobKey: " + to_string(_encodingItem->_encodingJobKey)
            + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey)
            + ", _encodingItem->_encodingType: " + MMSEngineDBFacade::toString(_encodingItem->_encodingType)
            + ", _encodingItem->_encodingParameters: " + _encodingItem->_encodingParameters
        );

        // throw e;
        return;
    }

    try
    {
        _logger->info(__FILEREF__ + "_mmsEngineDBFacade->updateEncodingJob NoError"
            + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
            + ", _encodingItem->_encodingJobKey: " + to_string(_encodingItem->_encodingJobKey)
            + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey)
            + ", _encodingItem->_encodingType: " + MMSEngineDBFacade::toString(_encodingItem->_encodingType)
            + ", _encodingItem->_encodingParameters: " + _encodingItem->_encodingParameters
        );

        _mmsEngineDBFacade->updateEncodingJob (
            _encodingItem->_encodingJobKey, 
            MMSEngineDBFacade::EncodingError::NoError,
            mediaItemKey, encodedPhysicalPathKey,
            _encodingItem->_ingestionJobKey);
    }
    catch(exception e)
    {
        _logger->error(__FILEREF__ + "_mmsEngineDBFacade->updateEncodingJob failed: " + e.what()
            + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
        );

        {
            lock_guard<mutex> locker(*_mtEncodingJobs);

            *_status = EncodingJobStatus::Free;
        }

        _logger->info(__FILEREF__ + "EncoderVideoAudioProxy finished"
            + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
            + ", _encodingItem->_encodingJobKey: " + to_string(_encodingItem->_encodingJobKey)
            + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey)
            + ", _encodingItem->_encodingType: " + MMSEngineDBFacade::toString(_encodingItem->_encodingType)
            + ", _encodingItem->_encodingParameters: " + _encodingItem->_encodingParameters
        );

        // throw e;
        return;
    }
    
    {
        lock_guard<mutex> locker(*_mtEncodingJobs);

        *_status = EncodingJobStatus::Free;
    }        
    
    _logger->info(__FILEREF__ + "EncoderVideoAudioProxy finished"
            + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
        + ", _encodingItem->_encodingJobKey: " + to_string(_encodingItem->_encodingJobKey)
        + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey)
        + ", _encodingItem->_encodingType: " + MMSEngineDBFacade::toString(_encodingItem->_encodingType)
        + ", _encodingItem->_encodingParameters: " + _encodingItem->_encodingParameters
    );
        
}

string EncoderVideoAudioProxy::encodeContentVideoAudio()
{
    string stagingEncodedAssetPathName;
    
    _logger->info(__FILEREF__ + "Creating encoderVideoAudioProxy thread"
            + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
        + ", _encodingItem->_encodeData->_encodingProfileTechnology" + to_string(static_cast<int>(_encodingItem->_encodeData->_encodingProfileTechnology))
        + ", _mp4Encoder: " + _mp4Encoder
    );

    if (
        (_encodingItem->_encodeData->_encodingProfileTechnology == MMSEngineDBFacade::EncodingTechnology::MP4 &&
            _mp4Encoder == "FFMPEG") ||
        (_encodingItem->_encodeData->_encodingProfileTechnology == MMSEngineDBFacade::EncodingTechnology::MPEG2_TS &&
            _mpeg2TSEncoder == "FFMPEG") ||
        _encodingItem->_encodeData->_encodingProfileTechnology == MMSEngineDBFacade::EncodingTechnology::WEBM ||
        (_encodingItem->_encodeData->_encodingProfileTechnology == MMSEngineDBFacade::EncodingTechnology::Adobe &&
            _mpeg2TSEncoder == "FFMPEG")
    )
    {
        stagingEncodedAssetPathName = encodeContent_VideoAudio_through_ffmpeg();
    }
    else if (_encodingItem->_encodeData->_encodingProfileTechnology == MMSEngineDBFacade::EncodingTechnology::WindowsMedia)
    {
        string errorMessage = __FILEREF__ + "No Encoder available to encode WindowsMedia technology"
            + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                ;
        _logger->error(errorMessage);
        
        throw runtime_error(errorMessage);
    }
    else
    {
        string errorMessage = __FILEREF__ + "Unknown technology and no Encoder available to encode"
            + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                ;
        _logger->error(errorMessage);
        
        throw runtime_error(errorMessage);
    }
    
    return stagingEncodedAssetPathName;
}

string EncoderVideoAudioProxy::encodeContent_VideoAudio_through_ffmpeg()
{
    
    int64_t sourcePhysicalPathKey;
    int64_t encodingProfileKey;    

    {
        string field = "sourcePhysicalPathKey";
        sourcePhysicalPathKey = _encodingItem->_parametersRoot.get(field, 0).asInt64();

        field = "encodingProfileKey";
        encodingProfileKey = _encodingItem->_parametersRoot.get(field, 0).asInt64();
    }
    
    string stagingEncodedAssetPathName;
    string encodedFileName;
    string mmsSourceAssetPathName;

    
    #ifdef __LOCALENCODER__
        if (*_pRunningEncodingsNumber > _ffmpegMaxCapacity)
        {
            _logger->info("Max ffmpeg encoder capacity is reached"
                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
            );

            throw MaxConcurrentJobsReached();
        }
    #endif

    // stagingEncodedAssetPathName preparation
    {        
        mmsSourceAssetPathName = _mmsStorage->getMMSAssetPathName(
            _encodingItem->_encodeData->_mmsPartitionNumber,
            _encodingItem->_workspace->_directoryName,
            _encodingItem->_encodeData->_relativePath,
            _encodingItem->_encodeData->_fileName);

        size_t extensionIndex = _encodingItem->_encodeData->_fileName.find_last_of(".");
        if (extensionIndex == string::npos)
        {
            string errorMessage = __FILEREF__ + "No extension find in the asset file name"
                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey) 
                + ", _encodingItem->_encodeData->_fileName: " + _encodingItem->_encodeData->_fileName;
            _logger->error(errorMessage);

            throw runtime_error(errorMessage);
        }

        encodedFileName =
                to_string(_encodingItem->_ingestionJobKey)
                + "_"
                + to_string(_encodingItem->_encodingJobKey)
                + "_" 
                + to_string(encodingProfileKey);
        if (_encodingItem->_encodeData->_encodingProfileTechnology == MMSEngineDBFacade::EncodingTechnology::MP4)
            encodedFileName.append(".mp4");
        else if (_encodingItem->_encodeData->_encodingProfileTechnology == MMSEngineDBFacade::EncodingTechnology::MPEG2_TS ||
                _encodingItem->_encodeData->_encodingProfileTechnology == MMSEngineDBFacade::EncodingTechnology::Adobe)
            ;
        else if (_encodingItem->_encodeData->_encodingProfileTechnology == MMSEngineDBFacade::EncodingTechnology::WEBM)
            encodedFileName.append(".webm");
        else
        {
            string errorMessage = __FILEREF__ + "Unknown technology"
                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                    ;
            _logger->error(errorMessage);

            throw runtime_error(errorMessage);
        }

        bool removeLinuxPathIfExist = true;
        stagingEncodedAssetPathName = _mmsStorage->getStagingAssetPathName(
            _encodingItem->_workspace->_directoryName,
            to_string(_encodingItem->_encodingJobKey),
            "/",    // _encodingItem->_relativePath,
            encodedFileName,
            -1, // _encodingItem->_mediaItemKey, not used because encodedFileName is not ""
            -1, // _encodingItem->_physicalPathKey, not used because encodedFileName is not ""
            removeLinuxPathIfExist);

        if (_encodingItem->_encodeData->_encodingProfileTechnology == MMSEngineDBFacade::EncodingTechnology::MPEG2_TS ||
            _encodingItem->_encodeData->_encodingProfileTechnology == MMSEngineDBFacade::EncodingTechnology::Adobe)
        {
            // In this case, the path is a directory where to place the segments

            if (!FileIO::directoryExisting(stagingEncodedAssetPathName)) 
            {
                _logger->info(__FILEREF__ + "Create directory"
                    + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                    + ", stagingEncodedAssetPathName: " + stagingEncodedAssetPathName
                );

                bool noErrorIfExists = true;
                bool recursive = true;
                FileIO::createDirectory(
                        stagingEncodedAssetPathName,
                        S_IRUSR | S_IWUSR | S_IXUSR |
                        S_IRGRP | S_IXGRP |
                        S_IROTH | S_IXOTH, noErrorIfExists, recursive);
            }        
        }
        
    }

    #ifdef __LOCALENCODER__
        (*_pRunningEncodingsNumber)++;

        try
        {
            _ffmpeg->encodeContent(
                mmsSourceAssetPathName,
                _encodingItem->_encodeData->_durationInMilliSeconds,
                encodedFileName,
                stagingEncodedAssetPathName,
                _encodingItem->_details,
                _encodingItem->_contentType == MMSEngineDBFacade::ContentType::Video,
                _encodingItem->_physicalPathKey,
                _encodingItem->_workspace->_directoryName,
                _encodingItem->_relativePath,
                _encodingItem->_encodingJobKey,
                _encodingItem->_ingestionJobKey);

            (*_pRunningEncodingsNumber)++;
        }
        catch(runtime_error e)
        {
            _logger->error(__FILEREF__ + "_ffmpeg->encodeContent failed"
                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                + ", mmsSourceAssetPathName: " + mmsSourceAssetPathName
                + ", encodedFileName: " + encodedFileName
                + ", stagingEncodedAssetPathName: " + stagingEncodedAssetPathName
                + ", _encodingItem->_details: " + _encodingItem->_details
                + ", _encodingItem->_contentType: " + MMSEngineDBFacade::toString(_encodingItem->_contentType)
                + ", _encodingItem->_physicalPathKey: " + to_string(_encodingItem->_physicalPathKey)
                + ", _encodingItem->_encodingJobKey: " + to_string(_encodingItem->_encodingJobKey)
                + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey)
            );
            
            (*_pRunningEncodingsNumber)++;
            
            throw e;
        }
        catch(exception e)
        {
            _logger->error(__FILEREF__ + "_ffmpeg->encodeContent failed"
                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                + ", mmsSourceAssetPathName: " + mmsSourceAssetPathName
                + ", encodedFileName: " + encodedFileName
                + ", stagingEncodedAssetPathName: " + stagingEncodedAssetPathName
                + ", _encodingItem->_details: " + _encodingItem->_details
                + ", _encodingItem->_contentType: " + MMSEngineDBFacade::toString(_encodingItem->_contentType)
                + ", _encodingItem->_physicalPathKey: " + to_string(_encodingItem->_physicalPathKey)
                + ", _encodingItem->_encodingJobKey: " + to_string(_encodingItem->_encodingJobKey)
                + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey)
            );
            
            (*_pRunningEncodingsNumber)++;
            
            throw e;
        }
    #else
        string ffmpegEncoderURL;
        ostringstream response;
        try
        {
            _currentUsedFFMpegEncoderHost = _encodersLoadBalancer->getEncoderHost(_encodingItem->_workspace);
            _logger->info(__FILEREF__ + "Configuration item"
                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                + ", _currentUsedFFMpegEncoderHost: " + _currentUsedFFMpegEncoderHost
            );
            ffmpegEncoderURL = 
                    _ffmpegEncoderProtocol
                    + "://"
                    + _currentUsedFFMpegEncoderHost + ":"
                    + to_string(_ffmpegEncoderPort)
                    + _ffmpegEncodeURI
                    + "/" + to_string(_encodingItem->_encodingJobKey)
            ;
            string body;
            {
                Json::Value encodingMedatada;
                
                encodingMedatada["mmsSourceAssetPathName"] = mmsSourceAssetPathName;
                encodingMedatada["durationInMilliSeconds"] = (Json::LargestUInt) (_encodingItem->_encodeData->_durationInMilliSeconds);
                // encodingMedatada["encodedFileName"] = encodedFileName;
                encodingMedatada["stagingEncodedAssetPathName"] = stagingEncodedAssetPathName;
                Json::Value encodingDetails;
                {
                    try
                    {
                        Json::CharReaderBuilder builder;
                        Json::CharReader* reader = builder.newCharReader();
                        string errors;

                        bool parsingSuccessful = reader->parse(_encodingItem->_encodeData->_jsonProfile.c_str(),
                                _encodingItem->_encodeData->_jsonProfile.c_str() + _encodingItem->_encodeData->_jsonProfile.size(), 
                                &encodingDetails, &errors);
                        delete reader;

                        if (!parsingSuccessful)
                        {
                            string errorMessage = __FILEREF__ + "failed to parse the _encodingItem->_jsonProfile"
                                    + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                                    + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey) 
                                    + ", errors: " + errors
                                    + ", _encodingItem->_encodeData->_jsonProfile: " + _encodingItem->_encodeData->_jsonProfile
                                    ;
                            _logger->error(errorMessage);

                            throw runtime_error(errorMessage);
                        }
                    }
                    catch(...)
                    {
                        string errorMessage = string("_encodingItem->_jsonProfile json is not well format")
                                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                                + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey) 
                                + ", _encodingItem->_encodeData->_jsonProfile: " + _encodingItem->_encodeData->_jsonProfile
                                ;
                        _logger->error(__FILEREF__ + errorMessage);

                        throw runtime_error(errorMessage);
                    }
                }
                encodingMedatada["encodingProfileDetails"] = encodingDetails;
                encodingMedatada["contentType"] = MMSEngineDBFacade::toString(_encodingItem->_encodeData->_contentType);
                encodingMedatada["physicalPathKey"] = (Json::LargestUInt) (sourcePhysicalPathKey);
                encodingMedatada["workspaceDirectoryName"] = _encodingItem->_workspace->_directoryName;
                encodingMedatada["relativePath"] = _encodingItem->_encodeData->_relativePath;
                encodingMedatada["encodingJobKey"] = (Json::LargestUInt) (_encodingItem->_encodingJobKey);
                encodingMedatada["ingestionJobKey"] = (Json::LargestUInt) (_encodingItem->_ingestionJobKey);

                {
                    Json::StreamWriterBuilder wbuilder;
                    
                    body = Json::writeString(wbuilder, encodingMedatada);
                }
            }
            
            list<string> header;

            header.push_back("Content-Type: application/json");
            {
                string userPasswordEncoded = Convert::base64_encode(_ffmpegEncoderUser + ":" + _ffmpegEncoderPassword);
                string basicAuthorization = string("Authorization: Basic ") + userPasswordEncoded;

                header.push_back(basicAuthorization);
            }
            
            curlpp::Cleanup cleaner;
            curlpp::Easy request;

            // Setting the URL to retrive.
            request.setOpt(new curlpp::options::Url(ffmpegEncoderURL));

            if (_ffmpegEncoderProtocol == "https")
            {
                /*
                    typedef curlpp::OptionTrait<std::string, CURLOPT_SSLCERTPASSWD> SslCertPasswd;                            
                    typedef curlpp::OptionTrait<std::string, CURLOPT_SSLKEY> SslKey;                                          
                    typedef curlpp::OptionTrait<std::string, CURLOPT_SSLKEYTYPE> SslKeyType;                                  
                    typedef curlpp::OptionTrait<std::string, CURLOPT_SSLKEYPASSWD> SslKeyPasswd;                              
                    typedef curlpp::OptionTrait<std::string, CURLOPT_SSLENGINE> SslEngine;                                    
                    typedef curlpp::NoValueOptionTrait<CURLOPT_SSLENGINE_DEFAULT> SslEngineDefault;                           
                    typedef curlpp::OptionTrait<long, CURLOPT_SSLVERSION> SslVersion;                                         
                    typedef curlpp::OptionTrait<std::string, CURLOPT_CAINFO> CaInfo;                                          
                    typedef curlpp::OptionTrait<std::string, CURLOPT_CAPATH> CaPath;                                          
                    typedef curlpp::OptionTrait<std::string, CURLOPT_RANDOM_FILE> RandomFile;                                 
                    typedef curlpp::OptionTrait<std::string, CURLOPT_EGDSOCKET> EgdSocket;                                    
                    typedef curlpp::OptionTrait<std::string, CURLOPT_SSL_CIPHER_LIST> SslCipherList;                          
                    typedef curlpp::OptionTrait<std::string, CURLOPT_KRB4LEVEL> Krb4Level;                                    
                 */
                                                                                                  
                
                /*
                // cert is stored PEM coded in file... 
                // since PEM is default, we needn't set it for PEM 
                // curl_easy_setopt(curl, CURLOPT_SSLCERTTYPE, "PEM");
                curlpp::OptionTrait<string, CURLOPT_SSLCERTTYPE> sslCertType("PEM");
                equest.setOpt(sslCertType);

                // set the cert for client authentication
                // "testcert.pem"
                // curl_easy_setopt(curl, CURLOPT_SSLCERT, pCertFile);
                curlpp::OptionTrait<string, CURLOPT_SSLCERT> sslCert("cert.pem");
                request.setOpt(sslCert);
                 */

                /*
                // sorry, for engine we must set the passphrase
                //   (if the key has one...)
                // const char *pPassphrase = NULL;
                if(pPassphrase)
                  curl_easy_setopt(curl, CURLOPT_KEYPASSWD, pPassphrase);

                // if we use a key stored in a crypto engine,
                //   we must set the key type to "ENG"
                // pKeyType  = "PEM";
                curl_easy_setopt(curl, CURLOPT_SSLKEYTYPE, pKeyType);

                // set the private key (file or ID in engine)
                // pKeyName  = "testkey.pem";
                curl_easy_setopt(curl, CURLOPT_SSLKEY, pKeyName);

                // set the file with the certs vaildating the server
                // *pCACertFile = "cacert.pem";
                curl_easy_setopt(curl, CURLOPT_CAINFO, pCACertFile);
                */
                
                // disconnect if we can't validate server's cert
                bool bSslVerifyPeer = false;
                curlpp::OptionTrait<bool, CURLOPT_SSL_VERIFYPEER> sslVerifyPeer(bSslVerifyPeer);
                request.setOpt(sslVerifyPeer);
                
                curlpp::OptionTrait<bool, CURLOPT_SSL_VERIFYHOST> sslVerifyHost(0L);
                request.setOpt(sslVerifyHost);
                
                // request.setOpt(new curlpp::options::SslEngineDefault());                                              

            }
            request.setOpt(new curlpp::options::HttpHeader(header));
            request.setOpt(new curlpp::options::PostFields(body));
            request.setOpt(new curlpp::options::PostFieldSize(body.length()));

            request.setOpt(new curlpp::options::WriteStream(&response));

            chrono::system_clock::time_point startEncoding = chrono::system_clock::now();

            _logger->info(__FILEREF__ + "Encoding media file"
                    + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                    + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey) 
                    + ", ffmpegEncoderURL: " + ffmpegEncoderURL
                    + ", body: " + body
            );
            request.perform();

            string sResponse = response.str();
            // LF and CR create problems to the json parser...
            while (sResponse.back() == 10 || sResponse.back() == 13)
                sResponse.pop_back();

            Json::Value encodeContentResponse;
            try
            {                
                Json::CharReaderBuilder builder;
                Json::CharReader* reader = builder.newCharReader();
                string errors;

                bool parsingSuccessful = reader->parse(sResponse.c_str(),
                        sResponse.c_str() + sResponse.size(), 
                        &encodeContentResponse, &errors);
                delete reader;

                if (!parsingSuccessful)
                {
                    string errorMessage = __FILEREF__ + "failed to parse the response body"
                            + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                            + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey) 
                            + ", errors: " + errors
                            + ", sResponse: " + sResponse
                            ;
                    _logger->error(errorMessage);

                    throw runtime_error(errorMessage);
                }               
            }
            catch(runtime_error e)
            {
                string errorMessage = string("response Body json is not well format")
                        + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                        + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey) 
                        + ", sResponse: " + sResponse
                        + ", e.what(): " + e.what()
                        ;
                _logger->error(__FILEREF__ + errorMessage);

                throw e;
            }
            catch(...)
            {
                string errorMessage = string("response Body json is not well format")
                        + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                        + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey) 
                        + ", sResponse: " + sResponse
                        ;
                _logger->error(__FILEREF__ + errorMessage);

                throw runtime_error(errorMessage);
            }

            {
                // same string declared in FFMPEGEncoder.cpp
                string noEncodingAvailableMessage("__NO-ENCODING-AVAILABLE__");
            
                string field = "error";
                if (Validator::isMetadataPresent(encodeContentResponse, field))
                {
                    // remove the staging directory created just for this encoding
                    {
                        size_t directoryEndIndex = stagingEncodedAssetPathName.find_last_of("/");
                        if (directoryEndIndex == string::npos)
                        {
                            string errorMessage = __FILEREF__ + "No directory found in the staging asset path name"
                                    + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                                    + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey) 
                                    + ", stagingEncodedAssetPathName: " + stagingEncodedAssetPathName;
                            _logger->error(errorMessage);

                            // throw runtime_error(errorMessage);
                        }
                        else
                        {
                            string stagingDirectory = stagingEncodedAssetPathName.substr(0, directoryEndIndex);
                            
                            try
                            {
                                _logger->info(__FILEREF__ + "removeDirectory"
                                    + ", stagingDirectory: " + stagingDirectory
                                );
                                Boolean_t bRemoveRecursively = true;
                                FileIO::removeDirectory(stagingDirectory, bRemoveRecursively);
                            }
                            catch (runtime_error e)
                            {
                                _logger->warn(__FILEREF__ + "FileIO::removeDirectory failed (runtime_error)"
                                    + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                                    + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey) 
                                    + ", stagingDirectory: " + stagingDirectory
                                    + ", exception: " + e.what()
                                    + ", response.str(): " + response.str()
                                );
                            }
                        }
                    }
                    
                    string error = encodeContentResponse.get(field, "XXX").asString();
                    
                    if (error.find(noEncodingAvailableMessage) != string::npos)
                    {
                        string errorMessage = string("No Encodings available")
                                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                                + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey) 
                                + ", sResponse: " + sResponse
                                ;
                        _logger->warn(__FILEREF__ + errorMessage);

                        throw MaxConcurrentJobsReached();
                    }
                    else
                    {
                        string errorMessage = string("FFMPEGEncoder error")
                                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                                + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey) 
                                + ", sResponse: " + sResponse
                                ;
                        _logger->error(__FILEREF__ + errorMessage);

                        throw runtime_error(errorMessage);
                    }                        
                }
                /*
                else
                {
                    string field = "ffmpegEncoderHost";
                    if (Validator::isMetadataPresent(encodeContentResponse, field))
                    {
                        _currentUsedFFMpegEncoderHost = encodeContentResponse.get("ffmpegEncoderHost", "XXX").asString();
                        
                        _logger->info(__FILEREF__ + "Retrieving ffmpegEncoderHost"
                            + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                            + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey) 
                            + "_currentUsedFFMpegEncoderHost: " + _currentUsedFFMpegEncoderHost
                                );                                        
                    }
                    else
                    {
                        string errorMessage = string("Unexpected FFMPEGEncoder response")
                                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                                + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey) 
                                + ", sResponse: " + sResponse
                                ;
                        _logger->error(__FILEREF__ + errorMessage);

                        throw runtime_error(errorMessage);
                    }
                }
                */                        
            }
            
            // loop waiting the end of the encoding
            bool encodingFinished = false;
            int maxEncodingStatusFailures = 1;
            int encodingStatusFailures = 0;
            while(!(encodingFinished || encodingStatusFailures >= maxEncodingStatusFailures))
            {
                this_thread::sleep_for(chrono::seconds(_intervalInSecondsToCheckEncodingFinished));
                
                try
                {
                    encodingFinished = getEncodingStatus(/* _encodingItem->_encodingJobKey */);
                }
                catch(...)
                {
                    _logger->error(__FILEREF__ + "getEncodingStatus failed");
                    
                    encodingStatusFailures++;
                }
            }
            
            chrono::system_clock::time_point endEncoding = chrono::system_clock::now();
            
            _logger->info(__FILEREF__ + "Encoded media file"
                    + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                    + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey) 
                    + ", ffmpegEncoderURL: " + ffmpegEncoderURL
                    + ", body: " + body
                    + ", sResponse: " + sResponse
                    + ", encodingDuration (secs): " + to_string(chrono::duration_cast<chrono::seconds>(endEncoding - startEncoding).count())
                    + ", _intervalInSecondsToCheckEncodingFinished: " + to_string(_intervalInSecondsToCheckEncodingFinished)
            );
        }
        catch(MaxConcurrentJobsReached e)
        {
            string errorMessage = string("MaxConcurrentJobsReached")
                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey) 
                + ", response.str(): " + response.str()
                + ", e.what(): " + e.what()
                ;
            _logger->warn(__FILEREF__ + errorMessage);

            throw e;
        }
        catch (curlpp::LogicError & e) 
        {
            _logger->error(__FILEREF__ + "Encoding URL failed (LogicError)"
                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey) 
                + ", ffmpegEncoderURL: " + ffmpegEncoderURL 
                + ", exception: " + e.what()
                + ", response.str(): " + response.str()
            );
            
            throw e;
        }
        catch (curlpp::RuntimeError & e) 
        {
            _logger->error(__FILEREF__ + "Encoding URL failed (RuntimeError)"
                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey) 
                + ", ffmpegEncoderURL: " + ffmpegEncoderURL 
                + ", exception: " + e.what()
                + ", response.str(): " + response.str()
            );

            throw e;
        }
        catch (runtime_error e)
        {
            _logger->error(__FILEREF__ + "Encoding URL failed (runtime_error)"
                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey) 
                + ", ffmpegEncoderURL: " + ffmpegEncoderURL 
                + ", exception: " + e.what()
                + ", response.str(): " + response.str()
            );

            throw e;
        }
        catch (exception e)
        {
            _logger->error(__FILEREF__ + "Encoding URL failed (exception)"
                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey) 
                + ", ffmpegEncoderURL: " + ffmpegEncoderURL 
                + ", exception: " + e.what()
                + ", response.str(): " + response.str()
            );

            throw e;
        }
    #endif

    return stagingEncodedAssetPathName;
}

int64_t EncoderVideoAudioProxy::processEncodedContentVideoAudio(string stagingEncodedAssetPathName)
{
    int64_t sourcePhysicalPathKey;
    int64_t encodingProfileKey;    

    {
        string field = "sourcePhysicalPathKey";
        sourcePhysicalPathKey = _encodingItem->_parametersRoot.get(field, 0).asInt64();

        field = "encodingProfileKey";
        encodingProfileKey = _encodingItem->_parametersRoot.get(field, 0).asInt64();
    }
    
    int64_t durationInMilliSeconds = -1;
    long bitRate = -1;
    string videoCodecName;
    string videoProfile;
    int videoWidth = -1;
    int videoHeight = -1;
    string videoAvgFrameRate;
    long videoBitRate = -1;
    string audioCodecName;
    long audioSampleRate = -1;
    int audioChannels = -1;
    long audioBitRate = -1;

    int imageWidth = -1;
    int imageHeight = -1;
    string imageFormat;
    int imageQuality = -1;
    try
    {
        FFMpeg ffmpeg (_configuration, _logger);
        tuple<int64_t,long,string,string,int,int,string,long,string,long,int,long> mediaInfo =
            ffmpeg.getMediaInfo(stagingEncodedAssetPathName);

        tie(durationInMilliSeconds, bitRate, 
            videoCodecName, videoProfile, videoWidth, videoHeight, videoAvgFrameRate, videoBitRate,
            audioCodecName, audioSampleRate, audioChannels, audioBitRate) = mediaInfo;
    }
    catch(runtime_error e)
    {
        _logger->error(__FILEREF__ + "EncoderVideoAudioProxy::getMediaInfo failed"
            + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
            + ", _encodingItem->_encodingJobKey: " + to_string(_encodingItem->_encodingJobKey)
            + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey)
            + ", _encodingItem->_encodingParameters: " + _encodingItem->_encodingParameters
            + ", stagingEncodedAssetPathName: " + stagingEncodedAssetPathName
            + ", _encodingItem->_workspace->_directoryName: " + _encodingItem->_workspace->_directoryName
            + ", _encodingItem->_encodeData->_relativePath: " + _encodingItem->_encodeData->_relativePath
            + ", e.what(): " + e.what()
        );

        throw e;
    }
    catch(exception e)
    {
        _logger->error(__FILEREF__ + "EncoderVideoAudioProxy::getMediaInfo failed"
            + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
            + ", _encodingItem->_encodingJobKey: " + to_string(_encodingItem->_encodingJobKey)
            + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey)
            + ", _encodingItem->_encodingParameters: " + _encodingItem->_encodingParameters
            + ", stagingEncodedAssetPathName: " + stagingEncodedAssetPathName
            + ", _encodingItem->_workspace->_directoryName: " + _encodingItem->_workspace->_directoryName
            + ", _encodingItem->_encodeData->_relativePath: " + _encodingItem->_encodeData->_relativePath
        );

        throw e;
    }        
    
    
    int64_t encodedPhysicalPathKey;
    string encodedFileName;
    string mmsAssetPathName;
    unsigned long mmsPartitionIndexUsed;
    try
    {
        size_t fileNameIndex = stagingEncodedAssetPathName.find_last_of("/");
        if (fileNameIndex == string::npos)
        {
            string errorMessage = __FILEREF__ + "No fileName find in the asset path name"
                    + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                    + ", stagingEncodedAssetPathName: " + stagingEncodedAssetPathName;
            _logger->error(errorMessage);

            throw runtime_error(errorMessage);
        }

        encodedFileName = stagingEncodedAssetPathName.substr(fileNameIndex + 1);

        /*
        encodedFileName = _encodingItem->_fileName
                + "_" 
                + to_string(_encodingItem->_encodingProfileKey);
        if (_encodingItem->_encodingProfileTechnology == MMSEngineDBFacade::EncodingTechnology::MP4)
            encodedFileName.append(".mp4");
        else if (_encodingItem->_encodingProfileTechnology == MMSEngineDBFacade::EncodingTechnology::MPEG2_TS ||
                _encodingItem->_encodingProfileTechnology == MMSEngineDBFacade::EncodingTechnology::Adobe)
            ;
        else if (_encodingItem->_encodingProfileTechnology == MMSEngineDBFacade::EncodingTechnology::WEBM)
            encodedFileName.append(".webm");
        else
        {
            string errorMessage = __FILEREF__ + "Unknown technology"
                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                    ;
            _logger->error(errorMessage);

            throw runtime_error(errorMessage);
        }
        */        

        bool partitionIndexToBeCalculated = true;
        bool deliveryRepositoriesToo = true;

        mmsAssetPathName = _mmsStorage->moveAssetInMMSRepository(
            stagingEncodedAssetPathName,
            _encodingItem->_workspace->_directoryName,
            encodedFileName,
            _encodingItem->_encodeData->_relativePath,

            partitionIndexToBeCalculated,
            &mmsPartitionIndexUsed, // OUT if bIsPartitionIndexToBeCalculated is true, IN is bIsPartitionIndexToBeCalculated is false

            deliveryRepositoriesToo,
            _encodingItem->_workspace->_territories
        );
    }
    catch(runtime_error e)
    {
        _logger->error(__FILEREF__ + "_mmsStorage->moveAssetInMMSRepository failed"
            + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
            + ", _encodingItem->_encodingJobKey: " + to_string(_encodingItem->_encodingJobKey)
            + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey)
            + ", _encodingItem->_encodingParameters: " + _encodingItem->_encodingParameters
            + ", stagingEncodedAssetPathName: " + stagingEncodedAssetPathName
            + ", _encodingItem->_workspace->_directoryName: " + _encodingItem->_workspace->_directoryName
            + ", _encodingItem->_encodeData->_relativePath: " + _encodingItem->_encodeData->_relativePath
            + ", e.what(): " + e.what()
        );

        throw e;
    }
    catch(exception e)
    {
        _logger->error(__FILEREF__ + "_mmsStorage->moveAssetInMMSRepository failed"
            + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
            + ", _encodingItem->_encodingJobKey: " + to_string(_encodingItem->_encodingJobKey)
            + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey)
            + ", _encodingItem->_encodingParameters: " + _encodingItem->_encodingParameters
            + ", stagingEncodedAssetPathName: " + stagingEncodedAssetPathName
            + ", _encodingItem->_workspace->_directoryName: " + _encodingItem->_workspace->_directoryName
            + ", _encodingItem->_encodeData->_relativePath: " + _encodingItem->_encodeData->_relativePath
        );

        throw e;
    }

    try
    {
        unsigned long long mmsAssetSizeInBytes;
        {
            FileIO::DirectoryEntryType_t detSourceFileType = 
                    FileIO::getDirectoryEntryType(mmsAssetPathName);

            // file in case of .3gp content OR directory in case of IPhone content
            if (detSourceFileType != FileIO::TOOLS_FILEIO_DIRECTORY &&
                    detSourceFileType != FileIO::TOOLS_FILEIO_REGULARFILE) 
            {
                string errorMessage = __FILEREF__ + "Wrong directory entry type"
                        + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                        + ", mmsAssetPathName: " + mmsAssetPathName
                        ;

                _logger->error(errorMessage);
                throw runtime_error(errorMessage);
            }

            if (detSourceFileType == FileIO::TOOLS_FILEIO_DIRECTORY)
            {
                mmsAssetSizeInBytes = FileIO::getDirectorySizeInBytes(mmsAssetPathName);   
            }
            else
            {
                bool inCaseOfLinkHasItToBeRead = false;
                mmsAssetSizeInBytes = FileIO::getFileSizeInBytes(mmsAssetPathName,
                        inCaseOfLinkHasItToBeRead);   
            }
        }


        encodedPhysicalPathKey = _mmsEngineDBFacade->saveEncodedContentMetadata(
            _encodingItem->_workspace->_workspaceKey,
            _encodingItem->_encodeData->_mediaItemKey,
            encodedFileName,
            _encodingItem->_encodeData->_relativePath,
            mmsPartitionIndexUsed,
            mmsAssetSizeInBytes,
            encodingProfileKey,
                
            durationInMilliSeconds,
            bitRate,
            videoCodecName,
            videoProfile,
            videoWidth,
            videoHeight,
            videoAvgFrameRate,
            videoBitRate,
            audioCodecName,
            audioSampleRate,
            audioChannels,
            audioBitRate,

            imageWidth,
            imageHeight,
            imageFormat,
            imageQuality
                );
        
        _logger->info(__FILEREF__ + "Saved the Encoded content"
            + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
            + ", _encodingItem->_encodingJobKey: " + to_string(_encodingItem->_encodingJobKey)
            + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey)
            + ", _encodingItem->_encodingParameters: " + _encodingItem->_encodingParameters
            + ", encodedPhysicalPathKey: " + to_string(encodedPhysicalPathKey)
        );
    }
    catch(exception e)
    {
        _logger->error(__FILEREF__ + "_mmsEngineDBFacade->saveEncodedContentMetadata failed"
            + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
            + ", _encodingItem->_encodingJobKey: " + to_string(_encodingItem->_encodingJobKey)
            + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey)
            + ", _encodingItem->_encodingParameters: " + _encodingItem->_encodingParameters
            + ", stagingEncodedAssetPathName: " + stagingEncodedAssetPathName
        );

        if (FileIO::fileExisting(mmsAssetPathName)
                || FileIO::directoryExisting(mmsAssetPathName))
        {
            FileIO::DirectoryEntryType_t detSourceFileType = FileIO::getDirectoryEntryType(mmsAssetPathName);

            _logger->info(__FILEREF__ + "Remove"
                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                + ", mmsAssetPathName: " + mmsAssetPathName
            );

            // file in case of .3gp content OR directory in case of IPhone content
            if (detSourceFileType == FileIO::TOOLS_FILEIO_DIRECTORY)
            {
                _logger->info(__FILEREF__ + "removeDirectory"
                    + ", mmsAssetPathName: " + mmsAssetPathName
                );
                Boolean_t bRemoveRecursively = true;
                FileIO::removeDirectory(mmsAssetPathName, bRemoveRecursively);
            }
            else if (detSourceFileType == FileIO::TOOLS_FILEIO_REGULARFILE) 
            {
                _logger->info(__FILEREF__ + "remove"
                    + ", mmsAssetPathName: " + mmsAssetPathName
                );
                FileIO::remove(mmsAssetPathName);
            }
        }

        throw e;
    }
    
    return encodedPhysicalPathKey;
}

string EncoderVideoAudioProxy::overlayImageOnVideo()
{
    string stagingEncodedAssetPathName;
    
    /*
    _logger->info(__FILEREF__ + "Creating encoderVideoAudioProxy thread"
            + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
        + ", _encodingItem->_encodingProfileTechnology" + to_string(static_cast<int>(_encodingItem->_encodingProfileTechnology))
        + ", _mp4Encoder: " + _mp4Encoder
    );

    if (
        (_encodingItem->_encodingProfileTechnology == MMSEngineDBFacade::EncodingTechnology::MP4 &&
            _mp4Encoder == "FFMPEG") ||
        (_encodingItem->_encodingProfileTechnology == MMSEngineDBFacade::EncodingTechnology::MPEG2_TS &&
            _mpeg2TSEncoder == "FFMPEG") ||
        _encodingItem->_encodingProfileTechnology == MMSEngineDBFacade::EncodingTechnology::WEBM ||
        (_encodingItem->_encodingProfileTechnology == MMSEngineDBFacade::EncodingTechnology::Adobe &&
            _mpeg2TSEncoder == "FFMPEG")
    )
    {
    */
        stagingEncodedAssetPathName = overlayImageOnVideo_through_ffmpeg();
    /*
    }
    else if (_encodingItem->_encodingProfileTechnology == MMSEngineDBFacade::EncodingTechnology::WindowsMedia)
    {
        string errorMessage = __FILEREF__ + "No Encoder available to encode WindowsMedia technology"
            + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                ;
        _logger->error(errorMessage);
        
        throw runtime_error(errorMessage);
    }
    else
    {
        string errorMessage = __FILEREF__ + "Unknown technology and no Encoder available to encode"
            + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                ;
        _logger->error(errorMessage);
        
        throw runtime_error(errorMessage);
    }
    */
    
    return stagingEncodedAssetPathName;
}

string EncoderVideoAudioProxy::overlayImageOnVideo_through_ffmpeg()
{
    
    int64_t sourceVideoPhysicalPathKey;
    int64_t sourceImagePhysicalPathKey;  
    string imagePosition_X_InPixel;
    string imagePosition_Y_InPixel;

    // _encodingItem->_parametersRoot filled in MMSEngineDBFacade::addOverlayImageOnVideoJob
    {
        string field = "sourceVideoPhysicalPathKey";
        sourceVideoPhysicalPathKey = _encodingItem->_parametersRoot.get(field, 0).asInt64();

        field = "sourceImagePhysicalPathKey";
        sourceImagePhysicalPathKey = _encodingItem->_parametersRoot.get(field, 0).asInt64();

        field = "imagePosition_X_InPixel";
        imagePosition_X_InPixel = _encodingItem->_parametersRoot.get(field, "XXX").asString();

        field = "imagePosition_Y_InPixel";
        imagePosition_Y_InPixel = _encodingItem->_parametersRoot.get(field, "XXX").asString();
    }
    
    string stagingEncodedAssetPathName;
    // string encodedFileName;
    string mmsSourceVideoAssetPathName;
    string mmsSourceImageAssetPathName;

    
    #ifdef __LOCALENCODER__
        if (*_pRunningEncodingsNumber > _ffmpegMaxCapacity)
        {
            _logger->info("Max ffmpeg encoder capacity is reached"
                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
            );

            throw MaxConcurrentJobsReached();
        }
    #endif

    // stagingEncodedAssetPathName preparation
    {        
        mmsSourceVideoAssetPathName = _mmsStorage->getMMSAssetPathName(
            _encodingItem->_overlayImageOnVideoData->_mmsVideoPartitionNumber,
            _encodingItem->_workspace->_directoryName,
            _encodingItem->_overlayImageOnVideoData->_videoRelativePath,
            _encodingItem->_overlayImageOnVideoData->_videoFileName);

        mmsSourceImageAssetPathName = _mmsStorage->getMMSAssetPathName(
            _encodingItem->_overlayImageOnVideoData->_mmsImagePartitionNumber,
            _encodingItem->_workspace->_directoryName,
            _encodingItem->_overlayImageOnVideoData->_imageRelativePath,
            _encodingItem->_overlayImageOnVideoData->_imageFileName);

        size_t extensionIndex = _encodingItem->_overlayImageOnVideoData->_videoFileName.find_last_of(".");
        if (extensionIndex == string::npos)
        {
            string errorMessage = __FILEREF__ + "No extension find in the asset file name"
                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey) 
                + ", _encodingItem->_overlayImageOnVideoData->_videoFileName: " + _encodingItem->_overlayImageOnVideoData->_videoFileName;
            _logger->error(errorMessage);

            throw runtime_error(errorMessage);
        }

        /*
        encodedFileName =
                to_string(_encodingItem->_ingestionJobKey)
                + "_"
                + to_string(_encodingItem->_encodingJobKey)
                + _encodingItem->_overlayImageOnVideoData->_videoFileName.substr(extensionIndex)
                ;
        */

        string workspaceIngestionRepository = _mmsStorage->getWorkspaceIngestionRepository(
                _encodingItem->_workspace);
        stagingEncodedAssetPathName = 
                workspaceIngestionRepository + "/" 
                + to_string(_encodingItem->_ingestionJobKey)
                + "_overlayedimage"
                + _encodingItem->_overlayImageOnVideoData->_videoFileName.substr(extensionIndex)
                ;
        /*
        bool removeLinuxPathIfExist = true;
        stagingEncodedAssetPathName = _mmsStorage->getStagingAssetPathName(
            _encodingItem->_workspace->_directoryName,
            "/",    // _encodingItem->_relativePath,
            encodedFileName,
            -1, // _encodingItem->_mediaItemKey, not used because encodedFileName is not ""
            -1, // _encodingItem->_physicalPathKey, not used because encodedFileName is not ""
            removeLinuxPathIfExist);        
         */
    }

    #ifdef __LOCALENCODER__
        (*_pRunningEncodingsNumber)++;

        /*
        try
        {
            _ffmpeg->encodeContent(
                mmsSourceAssetPathName,
                _encodingItem->_durationInMilliSeconds,
                encodedFileName,
                stagingEncodedAssetPathName,
                _encodingItem->_details,
                _encodingItem->_contentType == MMSEngineDBFacade::ContentType::Video,
                _encodingItem->_physicalPathKey,
                _encodingItem->_workspace->_directoryName,
                _encodingItem->_relativePath,
                _encodingItem->_encodingJobKey,
                _encodingItem->_ingestionJobKey);

            (*_pRunningEncodingsNumber)++;
        }
        catch(runtime_error e)
        {
            _logger->error(__FILEREF__ + "_ffmpeg->encodeContent failed"
                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                + ", mmsSourceAssetPathName: " + mmsSourceAssetPathName
                + ", encodedFileName: " + encodedFileName
                + ", stagingEncodedAssetPathName: " + stagingEncodedAssetPathName
                + ", _encodingItem->_details: " + _encodingItem->_details
                + ", _encodingItem->_contentType: " + MMSEngineDBFacade::toString(_encodingItem->_contentType)
                + ", _encodingItem->_physicalPathKey: " + to_string(_encodingItem->_physicalPathKey)
                + ", _encodingItem->_encodingJobKey: " + to_string(_encodingItem->_encodingJobKey)
                + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey)
            );
            
            (*_pRunningEncodingsNumber)++;
            
            throw e;
        }
        catch(exception e)
        {
            _logger->error(__FILEREF__ + "_ffmpeg->encodeContent failed"
                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                + ", mmsSourceAssetPathName: " + mmsSourceAssetPathName
                + ", encodedFileName: " + encodedFileName
                + ", stagingEncodedAssetPathName: " + stagingEncodedAssetPathName
                + ", _encodingItem->_details: " + _encodingItem->_details
                + ", _encodingItem->_contentType: " + MMSEngineDBFacade::toString(_encodingItem->_contentType)
                + ", _encodingItem->_physicalPathKey: " + to_string(_encodingItem->_physicalPathKey)
                + ", _encodingItem->_encodingJobKey: " + to_string(_encodingItem->_encodingJobKey)
                + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey)
            );
            
            (*_pRunningEncodingsNumber)++;
            
            throw e;
        }
        */
    #else
        string ffmpegEncoderURL;
        ostringstream response;
        try
        {
            _currentUsedFFMpegEncoderHost = _encodersLoadBalancer->getEncoderHost(_encodingItem->_workspace);
            _logger->info(__FILEREF__ + "Configuration item"
                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                + ", _currentUsedFFMpegEncoderHost: " + _currentUsedFFMpegEncoderHost
            );
            ffmpegEncoderURL = 
                    _ffmpegEncoderProtocol
                    + "://"
                    + _currentUsedFFMpegEncoderHost + ":"
                    + to_string(_ffmpegEncoderPort)
                    + _ffmpegOverlayImageOnVideoURI
                    + "/" + to_string(_encodingItem->_encodingJobKey)
            ;
            string body;
            {
                Json::Value overlayMedatada;
                
                overlayMedatada["mmsSourceVideoAssetPathName"] = mmsSourceVideoAssetPathName;
                overlayMedatada["videoDurationInMilliSeconds"] = (Json::LargestUInt) (_encodingItem->_overlayImageOnVideoData->_videoDurationInMilliSeconds);
                overlayMedatada["mmsSourceImageAssetPathName"] = mmsSourceImageAssetPathName;
                overlayMedatada["imagePosition_X_InPixel"] = imagePosition_X_InPixel;
                overlayMedatada["imagePosition_Y_InPixel"] = imagePosition_Y_InPixel;
                // overlayMedatada["encodedFileName"] = encodedFileName;
                overlayMedatada["stagingEncodedAssetPathName"] = stagingEncodedAssetPathName;
                overlayMedatada["encodingJobKey"] = (Json::LargestUInt) (_encodingItem->_encodingJobKey);
                overlayMedatada["ingestionJobKey"] = (Json::LargestUInt) (_encodingItem->_ingestionJobKey);

                {
                    Json::StreamWriterBuilder wbuilder;
                    
                    body = Json::writeString(wbuilder, overlayMedatada);
                }
            }
            
            list<string> header;

            header.push_back("Content-Type: application/json");
            {
                string userPasswordEncoded = Convert::base64_encode(_ffmpegEncoderUser + ":" + _ffmpegEncoderPassword);
                string basicAuthorization = string("Authorization: Basic ") + userPasswordEncoded;

                header.push_back(basicAuthorization);
            }
            
            curlpp::Cleanup cleaner;
            curlpp::Easy request;

            // Setting the URL to retrive.
            request.setOpt(new curlpp::options::Url(ffmpegEncoderURL));

            if (_ffmpegEncoderProtocol == "https")
            {
                /*
                    typedef curlpp::OptionTrait<std::string, CURLOPT_SSLCERTPASSWD> SslCertPasswd;                            
                    typedef curlpp::OptionTrait<std::string, CURLOPT_SSLKEY> SslKey;                                          
                    typedef curlpp::OptionTrait<std::string, CURLOPT_SSLKEYTYPE> SslKeyType;                                  
                    typedef curlpp::OptionTrait<std::string, CURLOPT_SSLKEYPASSWD> SslKeyPasswd;                              
                    typedef curlpp::OptionTrait<std::string, CURLOPT_SSLENGINE> SslEngine;                                    
                    typedef curlpp::NoValueOptionTrait<CURLOPT_SSLENGINE_DEFAULT> SslEngineDefault;                           
                    typedef curlpp::OptionTrait<long, CURLOPT_SSLVERSION> SslVersion;                                         
                    typedef curlpp::OptionTrait<std::string, CURLOPT_CAINFO> CaInfo;                                          
                    typedef curlpp::OptionTrait<std::string, CURLOPT_CAPATH> CaPath;                                          
                    typedef curlpp::OptionTrait<std::string, CURLOPT_RANDOM_FILE> RandomFile;                                 
                    typedef curlpp::OptionTrait<std::string, CURLOPT_EGDSOCKET> EgdSocket;                                    
                    typedef curlpp::OptionTrait<std::string, CURLOPT_SSL_CIPHER_LIST> SslCipherList;                          
                    typedef curlpp::OptionTrait<std::string, CURLOPT_KRB4LEVEL> Krb4Level;                                    
                 */
                                                                                                  
                
                /*
                // cert is stored PEM coded in file... 
                // since PEM is default, we needn't set it for PEM 
                // curl_easy_setopt(curl, CURLOPT_SSLCERTTYPE, "PEM");
                curlpp::OptionTrait<string, CURLOPT_SSLCERTTYPE> sslCertType("PEM");
                equest.setOpt(sslCertType);

                // set the cert for client authentication
                // "testcert.pem"
                // curl_easy_setopt(curl, CURLOPT_SSLCERT, pCertFile);
                curlpp::OptionTrait<string, CURLOPT_SSLCERT> sslCert("cert.pem");
                request.setOpt(sslCert);
                 */

                /*
                // sorry, for engine we must set the passphrase
                //   (if the key has one...)
                // const char *pPassphrase = NULL;
                if(pPassphrase)
                  curl_easy_setopt(curl, CURLOPT_KEYPASSWD, pPassphrase);

                // if we use a key stored in a crypto engine,
                //   we must set the key type to "ENG"
                // pKeyType  = "PEM";
                curl_easy_setopt(curl, CURLOPT_SSLKEYTYPE, pKeyType);

                // set the private key (file or ID in engine)
                // pKeyName  = "testkey.pem";
                curl_easy_setopt(curl, CURLOPT_SSLKEY, pKeyName);

                // set the file with the certs vaildating the server
                // *pCACertFile = "cacert.pem";
                curl_easy_setopt(curl, CURLOPT_CAINFO, pCACertFile);
                */
                
                // disconnect if we can't validate server's cert
                bool bSslVerifyPeer = false;
                curlpp::OptionTrait<bool, CURLOPT_SSL_VERIFYPEER> sslVerifyPeer(bSslVerifyPeer);
                request.setOpt(sslVerifyPeer);
                
                curlpp::OptionTrait<bool, CURLOPT_SSL_VERIFYHOST> sslVerifyHost(0L);
                request.setOpt(sslVerifyHost);
                
                // request.setOpt(new curlpp::options::SslEngineDefault());                                              

            }
            request.setOpt(new curlpp::options::HttpHeader(header));
            request.setOpt(new curlpp::options::PostFields(body));
            request.setOpt(new curlpp::options::PostFieldSize(body.length()));

            request.setOpt(new curlpp::options::WriteStream(&response));

            chrono::system_clock::time_point startEncoding = chrono::system_clock::now();

            _logger->info(__FILEREF__ + "Overlaying media file"
                    + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                    + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey) 
                    + ", ffmpegEncoderURL: " + ffmpegEncoderURL
                    + ", body: " + body
            );
            request.perform();

            string sResponse = response.str();
            // LF and CR create problems to the json parser...
            while (sResponse.back() == 10 || sResponse.back() == 13)
                sResponse.pop_back();

            Json::Value overlayContentResponse;
            try
            {                
                Json::CharReaderBuilder builder;
                Json::CharReader* reader = builder.newCharReader();
                string errors;

                bool parsingSuccessful = reader->parse(sResponse.c_str(),
                        sResponse.c_str() + sResponse.size(), 
                        &overlayContentResponse, &errors);
                delete reader;

                if (!parsingSuccessful)
                {
                    string errorMessage = __FILEREF__ + "failed to parse the response body"
                            + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                            + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey) 
                            + ", errors: " + errors
                            + ", sResponse: " + sResponse
                            ;
                    _logger->error(errorMessage);

                    throw runtime_error(errorMessage);
                }               
            }
            catch(runtime_error e)
            {
                string errorMessage = string("response Body json is not well format")
                        + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                        + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey) 
                        + ", sResponse: " + sResponse
                        + ", e.what(): " + e.what()
                        ;
                _logger->error(__FILEREF__ + errorMessage);

                throw e;
            }
            catch(...)
            {
                string errorMessage = string("response Body json is not well format")
                        + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                        + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey) 
                        + ", sResponse: " + sResponse
                        ;
                _logger->error(__FILEREF__ + errorMessage);

                throw runtime_error(errorMessage);
            }

            {
                // same string declared in FFMPEGEncoder.cpp
                string noEncodingAvailableMessage("__NO-ENCODING-AVAILABLE__");
            
                string field = "error";
                if (Validator::isMetadataPresent(overlayContentResponse, field))
                {
                    string error = overlayContentResponse.get(field, "XXX").asString();
                    
                    if (error.find(noEncodingAvailableMessage) != string::npos)
                    {
                        string errorMessage = string("No Encodings available")
                                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                                + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey) 
                                + ", sResponse: " + sResponse
                                ;
                        _logger->warn(__FILEREF__ + errorMessage);

                        throw MaxConcurrentJobsReached();
                    }
                    else
                    {
                        string errorMessage = string("FFMPEGEncoder error")
                                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                                + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey) 
                                + ", sResponse: " + sResponse
                                ;
                        _logger->error(__FILEREF__ + errorMessage);

                        throw runtime_error(errorMessage);
                    }                        
                }
                /*
                else
                {
                    string field = "ffmpegEncoderHost";
                    if (Validator::isMetadataPresent(encodeContentResponse, field))
                    {
                        _currentUsedFFMpegEncoderHost = encodeContentResponse.get("ffmpegEncoderHost", "XXX").asString();
                        
                        _logger->info(__FILEREF__ + "Retrieving ffmpegEncoderHost"
                            + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                            + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey) 
                            + "_currentUsedFFMpegEncoderHost: " + _currentUsedFFMpegEncoderHost
                                );                                        
                    }
                    else
                    {
                        string errorMessage = string("Unexpected FFMPEGEncoder response")
                                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                                + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey) 
                                + ", sResponse: " + sResponse
                                ;
                        _logger->error(__FILEREF__ + errorMessage);

                        throw runtime_error(errorMessage);
                    }
                }
                */                        
            }
            
            // loop waiting the end of the encoding
            bool encodingFinished = false;
            int maxEncodingStatusFailures = 1;
            int encodingStatusFailures = 0;
            while(!(encodingFinished || encodingStatusFailures >= maxEncodingStatusFailures))
            {
                this_thread::sleep_for(chrono::seconds(_intervalInSecondsToCheckEncodingFinished));
                
                try
                {
                    encodingFinished = getEncodingStatus(/* _encodingItem->_encodingJobKey */);
                }
                catch(...)
                {
                    _logger->error(__FILEREF__ + "getEncodingStatus failed");
                    
                    encodingStatusFailures++;
                }
            }
            
            chrono::system_clock::time_point endEncoding = chrono::system_clock::now();
            
            _logger->info(__FILEREF__ + "Overlayed media file"
                    + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                    + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey) 
                    + ", ffmpegEncoderURL: " + ffmpegEncoderURL
                    + ", body: " + body
                    + ", sResponse: " + sResponse
                    + ", encodingDuration (secs): " + to_string(chrono::duration_cast<chrono::seconds>(endEncoding - startEncoding).count())
                    + ", _intervalInSecondsToCheckEncodingFinished: " + to_string(_intervalInSecondsToCheckEncodingFinished)
            );
        }
        catch(MaxConcurrentJobsReached e)
        {
            string errorMessage = string("MaxConcurrentJobsReached")
                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey) 
                + ", response.str(): " + response.str()
                + ", e.what(): " + e.what()
                ;
            _logger->warn(__FILEREF__ + errorMessage);

            throw e;
        }
        catch (curlpp::LogicError & e) 
        {
            _logger->error(__FILEREF__ + "Encoding URL failed (LogicError)"
                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey) 
                + ", ffmpegEncoderURL: " + ffmpegEncoderURL 
                + ", exception: " + e.what()
                + ", response.str(): " + response.str()
            );
            
            throw e;
        }
        catch (curlpp::RuntimeError & e) 
        {
            _logger->error(__FILEREF__ + "Encoding URL failed (RuntimeError)"
                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey) 
                + ", ffmpegEncoderURL: " + ffmpegEncoderURL 
                + ", exception: " + e.what()
                + ", response.str(): " + response.str()
            );

            throw e;
        }
        catch (runtime_error e)
        {
            _logger->error(__FILEREF__ + "Encoding URL failed (runtime_error)"
                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey) 
                + ", ffmpegEncoderURL: " + ffmpegEncoderURL 
                + ", exception: " + e.what()
                + ", response.str(): " + response.str()
            );

            throw e;
        }
        catch (exception e)
        {
            _logger->error(__FILEREF__ + "Encoding URL failed (exception)"
                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey) 
                + ", ffmpegEncoderURL: " + ffmpegEncoderURL 
                + ", exception: " + e.what()
                + ", response.str(): " + response.str()
            );

            throw e;
        }
    #endif

    return stagingEncodedAssetPathName;
}

void EncoderVideoAudioProxy::processOverlayedImageOnVideo(string stagingEncodedAssetPathName)
{
    try
    {
        size_t extensionIndex = stagingEncodedAssetPathName.find_last_of(".");
        if (extensionIndex == string::npos)
        {
            string errorMessage = __FILEREF__ + "No extention find in the asset file name"
                    + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                    + ", stagingEncodedAssetPathName: " + stagingEncodedAssetPathName;
            _logger->error(errorMessage);

            throw runtime_error(errorMessage);
        }
        string fileFormat = stagingEncodedAssetPathName.substr(extensionIndex + 1);

        size_t fileNameIndex = stagingEncodedAssetPathName.find_last_of("/");
        if (fileNameIndex == string::npos)
        {
            string errorMessage = __FILEREF__ + "No fileName find in the asset path name"
                    + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                    + ", stagingEncodedAssetPathName: " + stagingEncodedAssetPathName;
            _logger->error(errorMessage);

            throw runtime_error(errorMessage);
        }
        string sourceFileName = stagingEncodedAssetPathName.substr(fileNameIndex + 1);

        
        string mediaMetaDataContent = generateMediaMetadataToIngest(_encodingItem->_ingestionJobKey,
            fileFormat, _encodingItem->_overlayImageOnVideoData->_overlayParametersRoot);
    
        shared_ptr<LocalAssetIngestionEvent>    localAssetIngestionEvent = _multiEventsSet->getEventsFactory()
                ->getFreeEvent<LocalAssetIngestionEvent>(MMSENGINE_EVENTTYPEIDENTIFIER_LOCALASSETINGESTIONEVENT);

        localAssetIngestionEvent->setSource(ENCODERVIDEOAUDIOPROXY);
        localAssetIngestionEvent->setDestination(MMSENGINEPROCESSORNAME);
        localAssetIngestionEvent->setExpirationTimePoint(chrono::system_clock::now());

        localAssetIngestionEvent->setIngestionJobKey(_encodingItem->_ingestionJobKey);
        localAssetIngestionEvent->setIngestionSourceFileName(sourceFileName);
        localAssetIngestionEvent->setMMSSourceFileName(sourceFileName);
        localAssetIngestionEvent->setWorkspace(_encodingItem->_workspace);
        localAssetIngestionEvent->setIngestionType(MMSEngineDBFacade::IngestionType::AddContent);
        localAssetIngestionEvent->setIngestionRowToBeUpdatedAsSuccess(true);

        localAssetIngestionEvent->setMetadataContent(mediaMetaDataContent);

        shared_ptr<Event2>    event = dynamic_pointer_cast<Event2>(localAssetIngestionEvent);
        _multiEventsSet->addEvent(event);

        _logger->info(__FILEREF__ + "addEvent: EVENT_TYPE (INGESTASSETEVENT)"
            + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
            + ", ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey)
            + ", sourceFileName: " + sourceFileName
            + ", getEventKey().first: " + to_string(event->getEventKey().first)
            + ", getEventKey().second: " + to_string(event->getEventKey().second));
    }
    catch(runtime_error e)
    {
        _logger->error(__FILEREF__ + "processOverlayedImageOnVideo failed"
            + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
            + ", _encodingItem->_encodingJobKey: " + to_string(_encodingItem->_encodingJobKey)
            + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey)
            + ", _encodingItem->_encodingParameters: " + _encodingItem->_encodingParameters
            + ", stagingEncodedAssetPathName: " + stagingEncodedAssetPathName
            + ", _encodingItem->_workspace->_directoryName: " + _encodingItem->_workspace->_directoryName
            + ", e.what(): " + e.what()
        );
                
        throw e;
    }
    catch(exception e)
    {
        _logger->error(__FILEREF__ + "processOverlayedImageOnVideo failed"
            + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
            + ", _encodingItem->_encodingJobKey: " + to_string(_encodingItem->_encodingJobKey)
            + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey)
            + ", _encodingItem->_encodingParameters: " + _encodingItem->_encodingParameters
            + ", stagingEncodedAssetPathName: " + stagingEncodedAssetPathName
            + ", _encodingItem->_workspace->_directoryName: " + _encodingItem->_workspace->_directoryName
        );
                
        throw e;
    }
    
    /*
    pair<int64_t,int64_t> mediaItemKeyAndPhysicalPathKey;
    
    string encodedFileName;
    string relativePathToBeUsed;
    unsigned long mmsPartitionIndexUsed;
    string mmsAssetPathName;
    try
    {
        size_t fileNameIndex = stagingEncodedAssetPathName.find_last_of("/");
        if (fileNameIndex == string::npos)
        {
            string errorMessage = __FILEREF__ + "No fileName find in the asset path name"
                    + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                    + ", stagingEncodedAssetPathName: " + stagingEncodedAssetPathName;
            _logger->error(errorMessage);

            throw runtime_error(errorMessage);
        }
        encodedFileName = stagingEncodedAssetPathName.substr(fileNameIndex + 1);

        relativePathToBeUsed = _mmsEngineDBFacade->nextRelativePathToBeUsed (
                _encodingItem->_workspace->_workspaceKey);
        
        bool partitionIndexToBeCalculated   = true;
        bool deliveryRepositoriesToo        = true;
        mmsAssetPathName = _mmsStorage->moveAssetInMMSRepository(
            stagingEncodedAssetPathName,
            _encodingItem->_workspace->_directoryName,
            encodedFileName,
            relativePathToBeUsed,
            partitionIndexToBeCalculated,
            &mmsPartitionIndexUsed,
            deliveryRepositoriesToo,
            _encodingItem->_workspace->_territories
            );
    }
    catch(runtime_error e)
    {
        _logger->error(__FILEREF__ + "_mmsStorage->moveAssetInMMSRepository failed"
            + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
            + ", _encodingItem->_encodingJobKey: " + to_string(_encodingItem->_encodingJobKey)
            + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey)
            + ", _encodingItem->_encodingParameters: " + _encodingItem->_encodingParameters
            + ", stagingEncodedAssetPathName: " + stagingEncodedAssetPathName
            + ", _encodingItem->_workspace->_directoryName: " + _encodingItem->_workspace->_directoryName
            + ", e.what(): " + e.what()
        );
                
        throw e;
    }
    catch(exception e)
    {
        _logger->error(__FILEREF__ + "_mmsStorage->moveAssetInMMSRepository failed"
            + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
            + ", _encodingItem->_encodingJobKey: " + to_string(_encodingItem->_encodingJobKey)
            + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey)
            + ", _encodingItem->_encodingParameters: " + _encodingItem->_encodingParameters
            + ", stagingEncodedAssetPathName: " + stagingEncodedAssetPathName
            + ", _encodingItem->_workspace->_directoryName: " + _encodingItem->_workspace->_directoryName
        );
                
        throw e;
    }

    MMSEngineDBFacade::ContentType contentType;
    
    int64_t durationInMilliSeconds = -1;
    long bitRate = -1;
    string videoCodecName;
    string videoProfile;
    int videoWidth = -1;
    int videoHeight = -1;
    string videoAvgFrameRate;
    long videoBitRate = -1;
    string audioCodecName;
    long audioSampleRate = -1;
    int audioChannels = -1;
    long audioBitRate = -1;

    int imageWidth = -1;
    int imageHeight = -1;
    string imageFormat;
    int imageQuality = -1;
    try
    {
        FFMpeg ffmpeg (_configuration, _logger);
        tuple<int64_t,long,string,string,int,int,string,long,string,long,int,long> mediaInfo =
            ffmpeg.getMediaInfo(mmsAssetPathName);

        tie(durationInMilliSeconds, bitRate, 
            videoCodecName, videoProfile, videoWidth, videoHeight, videoAvgFrameRate, videoBitRate,
            audioCodecName, audioSampleRate, audioChannels, audioBitRate) = mediaInfo;

        contentType = MMSEngineDBFacade::ContentType::Video;
    }
    catch(runtime_error e)
    {
        _logger->error(__FILEREF__ + "ffmpeg.getMediaInfo failed"
            + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
            + ", _encodingItem->_encodingJobKey: " + to_string(_encodingItem->_encodingJobKey)
            + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey)
            + ", _encodingItem->_encodingParameters: " + _encodingItem->_encodingParameters
            + ", stagingEncodedAssetPathName: " + stagingEncodedAssetPathName
            + ", _encodingItem->_workspace->_directoryName: " + _encodingItem->_workspace->_directoryName
            + ", e.what(): " + e.what()
        );
                
        throw e;
    }
    catch(exception e)
    {
        _logger->error(__FILEREF__ + "ffmpeg.getMediaInfo failed"
            + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
            + ", _encodingItem->_encodingJobKey: " + to_string(_encodingItem->_encodingJobKey)
            + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey)
            + ", _encodingItem->_encodingParameters: " + _encodingItem->_encodingParameters
            + ", stagingEncodedAssetPathName: " + stagingEncodedAssetPathName
            + ", _encodingItem->_workspace->_directoryName: " + _encodingItem->_workspace->_directoryName
        );
                
        throw e;
    }        

    try
    {
        bool inCaseOfLinkHasItToBeRead = false;
        unsigned long sizeInBytes = FileIO::getFileSizeInBytes(mmsAssetPathName,
                inCaseOfLinkHasItToBeRead);   

        _logger->info(__FILEREF__ + "_mmsEngineDBFacade->saveIngestedContentMetadata..."
            + ", ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey)
            + ", contentType: " + MMSEngineDBFacade::toString(contentType)
            + ", relativePathToBeUsed: " + relativePathToBeUsed
            + ", encodedFileName: " + encodedFileName
            + ", mmsPartitionIndexUsed: " + to_string(mmsPartitionIndexUsed)
            + ", sizeInBytes: " + to_string(sizeInBytes)

            + ", durationInMilliSeconds: " + to_string(durationInMilliSeconds)
            + ", bitRate: " + to_string(bitRate)
            + ", videoCodecName: " + videoCodecName
            + ", videoProfile: " + videoProfile
            + ", videoWidth: " + to_string(videoWidth)
            + ", videoHeight: " + to_string(videoHeight)
            + ", videoAvgFrameRate: " + videoAvgFrameRate
            + ", videoBitRate: " + to_string(videoBitRate)
            + ", audioCodecName: " + audioCodecName
            + ", audioSampleRate: " + to_string(audioSampleRate)
            + ", audioChannels: " + to_string(audioChannels)
            + ", audioBitRate: " + to_string(audioBitRate)

            + ", imageWidth: " + to_string(imageWidth)
            + ", imageHeight: " + to_string(imageHeight)
            + ", imageFormat: " + imageFormat
            + ", imageQuality: " + to_string(imageQuality)
        );

        mediaItemKeyAndPhysicalPathKey = _mmsEngineDBFacade->saveIngestedContentMetadata (
                    _encodingItem->_workspace,
                    _encodingItem->_ingestionJobKey,
                    true, // ingestionRowToBeUpdatedAsSuccess
                    contentType,
                    _encodingItem->_overlayImageOnVideoData->_overlayParametersRoot,
                    relativePathToBeUsed,
                    encodedFileName,
                    mmsPartitionIndexUsed,
                    sizeInBytes,
                
                    // video-audio
                    durationInMilliSeconds,
                    bitRate,
                    videoCodecName,
                    videoProfile,
                    videoWidth,
                    videoHeight,
                    videoAvgFrameRate,
                    videoBitRate,
                    audioCodecName,
                    audioSampleRate,
                    audioChannels,
                    audioBitRate,

                    // image
                    imageWidth,
                    imageHeight,
                    imageFormat,
                    imageQuality
        );

        _logger->info(__FILEREF__ + "Added a new ingested content"
            + ", ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey)
            + ", mediaItemKey: " + to_string(mediaItemKeyAndPhysicalPathKey.first)
            + ", physicalPathKey: " + to_string(mediaItemKeyAndPhysicalPathKey.second)
        );
    }
    catch(runtime_error e)
    {
        _logger->error(__FILEREF__ + "_mmsEngineDBFacade->saveIngestedContentMetadata failed"
            + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
            + ", _encodingItem->_encodingJobKey: " + to_string(_encodingItem->_encodingJobKey)
            + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey)
            + ", _encodingItem->_encodingParameters: " + _encodingItem->_encodingParameters
            + ", stagingEncodedAssetPathName: " + stagingEncodedAssetPathName
            + ", _encodingItem->_workspace->_directoryName: " + _encodingItem->_workspace->_directoryName
            + ", e.what(): " + e.what()
        );
                
        throw e;
    }
    catch(exception e)
    {
        _logger->error(__FILEREF__ + "_mmsEngineDBFacade->saveIngestedContentMetadata failed"
            + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
            + ", _encodingItem->_encodingJobKey: " + to_string(_encodingItem->_encodingJobKey)
            + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey)
            + ", _encodingItem->_encodingParameters: " + _encodingItem->_encodingParameters
            + ", stagingEncodedAssetPathName: " + stagingEncodedAssetPathName
            + ", _encodingItem->_workspace->_directoryName: " + _encodingItem->_workspace->_directoryName
        );
                
        throw e;
    }    

    
    return mediaItemKeyAndPhysicalPathKey;
    */
}

string EncoderVideoAudioProxy::overlayTextOnVideo()
{
    string stagingEncodedAssetPathName;
    
    stagingEncodedAssetPathName = overlayTextOnVideo_through_ffmpeg();
    
    return stagingEncodedAssetPathName;
}

string EncoderVideoAudioProxy::overlayTextOnVideo_through_ffmpeg()
{
    
    int64_t sourceVideoPhysicalPathKey;
    string text;
    string textPosition_X_InPixel;
    string textPosition_Y_InPixel;
    string fontType;
    int fontSize;
    string fontColor;
    int textPercentageOpacity;
    bool boxEnable;
    string boxColor;
    int boxPercentageOpacity;

    // _encodingItem->_parametersRoot filled in MMSEngineDBFacade::addOverlayTextOnVideoJob
    {
        string field = "sourceVideoPhysicalPathKey";
        sourceVideoPhysicalPathKey = _encodingItem->_parametersRoot.get(field, 0).asInt64();

        field = "text";
        text = _encodingItem->_parametersRoot.get(field, "XXX").asString();

        field = "textPosition_X_InPixel";
        textPosition_X_InPixel = _encodingItem->_parametersRoot.get(field, "XXX").asString();

        field = "textPosition_Y_InPixel";
        textPosition_Y_InPixel = _encodingItem->_parametersRoot.get(field, "XXX").asString();

        field = "fontType";
        fontType = _encodingItem->_parametersRoot.get(field, "XXX").asString();

        field = "fontSize";
        fontSize = _encodingItem->_parametersRoot.get(field, 0).asInt();

        field = "fontColor";
        fontColor = _encodingItem->_parametersRoot.get(field, "XXX").asString();

        field = "textPercentageOpacity";
        textPercentageOpacity = _encodingItem->_parametersRoot.get(field, 0).asInt();

        field = "boxEnable";
        boxEnable = _encodingItem->_parametersRoot.get(field, 0).asBool();

        field = "boxColor";
        boxColor = _encodingItem->_parametersRoot.get(field, "XXX").asString();

        field = "boxPercentageOpacity";
        boxPercentageOpacity = _encodingItem->_parametersRoot.get(field, 0).asInt();
    }
    
    string stagingEncodedAssetPathName;
    // string encodedFileName;
    string mmsSourceVideoAssetPathName;

    
    #ifdef __LOCALENCODER__
        if (*_pRunningEncodingsNumber > _ffmpegMaxCapacity)
        {
            _logger->info("Max ffmpeg encoder capacity is reached"
                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
            );

            throw MaxConcurrentJobsReached();
        }
    #endif

    // stagingEncodedAssetPathName preparation
    {        
        mmsSourceVideoAssetPathName = _mmsStorage->getMMSAssetPathName(
            _encodingItem->_overlayTextOnVideoData->_mmsVideoPartitionNumber,
            _encodingItem->_workspace->_directoryName,
            _encodingItem->_overlayTextOnVideoData->_videoRelativePath,
            _encodingItem->_overlayTextOnVideoData->_videoFileName);

        size_t extensionIndex = _encodingItem->_overlayTextOnVideoData->_videoFileName.find_last_of(".");
        if (extensionIndex == string::npos)
        {
            string errorMessage = __FILEREF__ + "No extension find in the asset file name"
                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey) 
                + ", _encodingItem->_overlayTextOnVideoData->_videoFileName: " + _encodingItem->_overlayTextOnVideoData->_videoFileName;
            _logger->error(errorMessage);

            throw runtime_error(errorMessage);
        }

        /*
        encodedFileName =
                to_string(_encodingItem->_ingestionJobKey)
                + "_"
                + to_string(_encodingItem->_encodingJobKey)
                + _encodingItem->_overlayTextOnVideoData->_videoFileName.substr(extensionIndex)
                ;
        */
        string workspaceIngestionRepository = _mmsStorage->getWorkspaceIngestionRepository(
                _encodingItem->_workspace);
        stagingEncodedAssetPathName = 
                workspaceIngestionRepository + "/" 
                + to_string(_encodingItem->_ingestionJobKey)
                + "_overlayedtext"
                + _encodingItem->_overlayTextOnVideoData->_videoFileName.substr(extensionIndex)
                ;
        /*
        bool removeLinuxPathIfExist = true;
        stagingEncodedAssetPathName = _mmsStorage->getStagingAssetPathName(
            _encodingItem->_workspace->_directoryName,
            "/",    // _encodingItem->_relativePath,
            encodedFileName,
            -1, // _encodingItem->_mediaItemKey, not used because encodedFileName is not ""
            -1, // _encodingItem->_physicalPathKey, not used because encodedFileName is not ""
            removeLinuxPathIfExist);        
        */
    }

    #ifdef __LOCALENCODER__
        (*_pRunningEncodingsNumber)++;

    #else
        string ffmpegEncoderURL;
        ostringstream response;
        try
        {
            _currentUsedFFMpegEncoderHost = _encodersLoadBalancer->getEncoderHost(_encodingItem->_workspace);
            _logger->info(__FILEREF__ + "Configuration item"
                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                + ", _currentUsedFFMpegEncoderHost: " + _currentUsedFFMpegEncoderHost
            );
            ffmpegEncoderURL = 
                    _ffmpegEncoderProtocol
                    + "://"
                    + _currentUsedFFMpegEncoderHost + ":"
                    + to_string(_ffmpegEncoderPort)
                    + _ffmpegOverlayTextOnVideoURI
                    + "/" + to_string(_encodingItem->_encodingJobKey)
            ;
            string body;
            {
                Json::Value overlayTextMedatada;
                
                overlayTextMedatada["mmsSourceVideoAssetPathName"] = mmsSourceVideoAssetPathName;
                overlayTextMedatada["videoDurationInMilliSeconds"] = (Json::LargestUInt) (_encodingItem->_overlayTextOnVideoData->_videoDurationInMilliSeconds);

                overlayTextMedatada["text"] = text;
                overlayTextMedatada["textPosition_X_InPixel"] = textPosition_X_InPixel;
                overlayTextMedatada["textPosition_Y_InPixel"] = textPosition_Y_InPixel;
                overlayTextMedatada["fontType"] = fontType;
                overlayTextMedatada["fontSize"] = fontSize;
                overlayTextMedatada["fontColor"] = fontColor;
                overlayTextMedatada["textPercentageOpacity"] = textPercentageOpacity;
                overlayTextMedatada["boxEnable"] = boxEnable;
                overlayTextMedatada["boxColor"] = boxColor;
                overlayTextMedatada["boxPercentageOpacity"] = boxPercentageOpacity;
                
                // overlayTextMedatada["encodedFileName"] = encodedFileName;
                overlayTextMedatada["stagingEncodedAssetPathName"] = stagingEncodedAssetPathName;
                overlayTextMedatada["encodingJobKey"] = (Json::LargestUInt) (_encodingItem->_encodingJobKey);
                overlayTextMedatada["ingestionJobKey"] = (Json::LargestUInt) (_encodingItem->_ingestionJobKey);

                {
                    Json::StreamWriterBuilder wbuilder;
                    
                    body = Json::writeString(wbuilder, overlayTextMedatada);
                }
            }
            
            list<string> header;

            header.push_back("Content-Type: application/json");
            {
                string userPasswordEncoded = Convert::base64_encode(_ffmpegEncoderUser + ":" + _ffmpegEncoderPassword);
                string basicAuthorization = string("Authorization: Basic ") + userPasswordEncoded;

                header.push_back(basicAuthorization);
            }
            
            curlpp::Cleanup cleaner;
            curlpp::Easy request;

            // Setting the URL to retrive.
            request.setOpt(new curlpp::options::Url(ffmpegEncoderURL));

            if (_ffmpegEncoderProtocol == "https")
            {
                /*
                    typedef curlpp::OptionTrait<std::string, CURLOPT_SSLCERTPASSWD> SslCertPasswd;                            
                    typedef curlpp::OptionTrait<std::string, CURLOPT_SSLKEY> SslKey;                                          
                    typedef curlpp::OptionTrait<std::string, CURLOPT_SSLKEYTYPE> SslKeyType;                                  
                    typedef curlpp::OptionTrait<std::string, CURLOPT_SSLKEYPASSWD> SslKeyPasswd;                              
                    typedef curlpp::OptionTrait<std::string, CURLOPT_SSLENGINE> SslEngine;                                    
                    typedef curlpp::NoValueOptionTrait<CURLOPT_SSLENGINE_DEFAULT> SslEngineDefault;                           
                    typedef curlpp::OptionTrait<long, CURLOPT_SSLVERSION> SslVersion;                                         
                    typedef curlpp::OptionTrait<std::string, CURLOPT_CAINFO> CaInfo;                                          
                    typedef curlpp::OptionTrait<std::string, CURLOPT_CAPATH> CaPath;                                          
                    typedef curlpp::OptionTrait<std::string, CURLOPT_RANDOM_FILE> RandomFile;                                 
                    typedef curlpp::OptionTrait<std::string, CURLOPT_EGDSOCKET> EgdSocket;                                    
                    typedef curlpp::OptionTrait<std::string, CURLOPT_SSL_CIPHER_LIST> SslCipherList;                          
                    typedef curlpp::OptionTrait<std::string, CURLOPT_KRB4LEVEL> Krb4Level;                                    
                 */
                                                                                                  
                
                /*
                // cert is stored PEM coded in file... 
                // since PEM is default, we needn't set it for PEM 
                // curl_easy_setopt(curl, CURLOPT_SSLCERTTYPE, "PEM");
                curlpp::OptionTrait<string, CURLOPT_SSLCERTTYPE> sslCertType("PEM");
                equest.setOpt(sslCertType);

                // set the cert for client authentication
                // "testcert.pem"
                // curl_easy_setopt(curl, CURLOPT_SSLCERT, pCertFile);
                curlpp::OptionTrait<string, CURLOPT_SSLCERT> sslCert("cert.pem");
                request.setOpt(sslCert);
                 */

                /*
                // sorry, for engine we must set the passphrase
                //   (if the key has one...)
                // const char *pPassphrase = NULL;
                if(pPassphrase)
                  curl_easy_setopt(curl, CURLOPT_KEYPASSWD, pPassphrase);

                // if we use a key stored in a crypto engine,
                //   we must set the key type to "ENG"
                // pKeyType  = "PEM";
                curl_easy_setopt(curl, CURLOPT_SSLKEYTYPE, pKeyType);

                // set the private key (file or ID in engine)
                // pKeyName  = "testkey.pem";
                curl_easy_setopt(curl, CURLOPT_SSLKEY, pKeyName);

                // set the file with the certs vaildating the server
                // *pCACertFile = "cacert.pem";
                curl_easy_setopt(curl, CURLOPT_CAINFO, pCACertFile);
                */
                
                // disconnect if we can't validate server's cert
                bool bSslVerifyPeer = false;
                curlpp::OptionTrait<bool, CURLOPT_SSL_VERIFYPEER> sslVerifyPeer(bSslVerifyPeer);
                request.setOpt(sslVerifyPeer);
                
                curlpp::OptionTrait<bool, CURLOPT_SSL_VERIFYHOST> sslVerifyHost(0L);
                request.setOpt(sslVerifyHost);
                
                // request.setOpt(new curlpp::options::SslEngineDefault());                                              

            }
            request.setOpt(new curlpp::options::HttpHeader(header));
            request.setOpt(new curlpp::options::PostFields(body));
            request.setOpt(new curlpp::options::PostFieldSize(body.length()));

            request.setOpt(new curlpp::options::WriteStream(&response));

            chrono::system_clock::time_point startEncoding = chrono::system_clock::now();

            _logger->info(__FILEREF__ + "OverlayText media file"
                    + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                    + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey) 
                    + ", ffmpegEncoderURL: " + ffmpegEncoderURL
                    + ", body: " + body
            );
            request.perform();

            string sResponse = response.str();
            // LF and CR create problems to the json parser...
            while (sResponse.back() == 10 || sResponse.back() == 13)
                sResponse.pop_back();

            Json::Value overlayTextContentResponse;
            try
            {                
                Json::CharReaderBuilder builder;
                Json::CharReader* reader = builder.newCharReader();
                string errors;

                bool parsingSuccessful = reader->parse(sResponse.c_str(),
                        sResponse.c_str() + sResponse.size(), 
                        &overlayTextContentResponse, &errors);
                delete reader;

                if (!parsingSuccessful)
                {
                    string errorMessage = __FILEREF__ + "failed to parse the response body"
                            + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                            + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey) 
                            + ", errors: " + errors
                            + ", sResponse: " + sResponse
                            ;
                    _logger->error(errorMessage);

                    throw runtime_error(errorMessage);
                }               
            }
            catch(runtime_error e)
            {
                string errorMessage = string("response Body json is not well format")
                        + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                        + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey) 
                        + ", sResponse: " + sResponse
                        + ", e.what(): " + e.what()
                        ;
                _logger->error(__FILEREF__ + errorMessage);

                throw e;
            }
            catch(...)
            {
                string errorMessage = string("response Body json is not well format")
                        + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                        + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey) 
                        + ", sResponse: " + sResponse
                        ;
                _logger->error(__FILEREF__ + errorMessage);

                throw runtime_error(errorMessage);
            }

            {
                // same string declared in FFMPEGEncoder.cpp
                string noEncodingAvailableMessage("__NO-ENCODING-AVAILABLE__");
            
                string field = "error";
                if (Validator::isMetadataPresent(overlayTextContentResponse, field))
                {
                    string error = overlayTextContentResponse.get(field, "XXX").asString();
                    
                    if (error.find(noEncodingAvailableMessage) != string::npos)
                    {
                        string errorMessage = string("No Encodings available")
                                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                                + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey) 
                                + ", sResponse: " + sResponse
                                ;
                        _logger->warn(__FILEREF__ + errorMessage);

                        throw MaxConcurrentJobsReached();
                    }
                    else
                    {
                        string errorMessage = string("FFMPEGEncoder error")
                                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                                + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey) 
                                + ", sResponse: " + sResponse
                                ;
                        _logger->error(__FILEREF__ + errorMessage);

                        throw runtime_error(errorMessage);
                    }                        
                }
                /*
                else
                {
                    string field = "ffmpegEncoderHost";
                    if (Validator::isMetadataPresent(encodeContentResponse, field))
                    {
                        _currentUsedFFMpegEncoderHost = encodeContentResponse.get("ffmpegEncoderHost", "XXX").asString();
                        
                        _logger->info(__FILEREF__ + "Retrieving ffmpegEncoderHost"
                            + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                            + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey) 
                            + "_currentUsedFFMpegEncoderHost: " + _currentUsedFFMpegEncoderHost
                                );                                        
                    }
                    else
                    {
                        string errorMessage = string("Unexpected FFMPEGEncoder response")
                                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                                + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey) 
                                + ", sResponse: " + sResponse
                                ;
                        _logger->error(__FILEREF__ + errorMessage);

                        throw runtime_error(errorMessage);
                    }
                }
                */                        
            }
            
            // loop waiting the end of the encoding
            bool encodingFinished = false;
            int maxEncodingStatusFailures = 1;
            int encodingStatusFailures = 0;
            while(!(encodingFinished || encodingStatusFailures >= maxEncodingStatusFailures))
            {
                this_thread::sleep_for(chrono::seconds(_intervalInSecondsToCheckEncodingFinished));
                
                try
                {
                    encodingFinished = getEncodingStatus(/* _encodingItem->_encodingJobKey */);
                }
                catch(...)
                {                    
                    encodingStatusFailures++;
                    
                    _logger->error(__FILEREF__ + "getEncodingStatus failed"
                        + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey) 
                        + ", encodingStatusFailures: " + to_string(encodingStatusFailures)
                    );
                }
            }
            
            // here we do not know if the encoding was successful or not
            // we can just check the encoded file because we know the ffmpeg methods
            // will remove the encoded file in case of failure
            if (!FileIO::fileExisting(stagingEncodedAssetPathName)
                && !FileIO::directoryExisting(stagingEncodedAssetPathName))
            {
                string errorMessage = string("Encoded file was not generated!!!")
                        + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                        + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey) 
                        + ", encodingStatusFailures: " + to_string(encodingStatusFailures)
                        + ", maxEncodingStatusFailures: " + to_string(maxEncodingStatusFailures)
                        ;
                _logger->error(__FILEREF__ + errorMessage);

                throw runtime_error(errorMessage);
            }
            
            chrono::system_clock::time_point endEncoding = chrono::system_clock::now();
            
            _logger->info(__FILEREF__ + "OverlayedText media file"
                    + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                    + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey) 
                    + ", ffmpegEncoderURL: " + ffmpegEncoderURL
                    + ", body: " + body
                    + ", sResponse: " + sResponse
                    + ", encodingDuration (secs): " + to_string(chrono::duration_cast<chrono::seconds>(endEncoding - startEncoding).count())
                    + ", _intervalInSecondsToCheckEncodingFinished: " + to_string(_intervalInSecondsToCheckEncodingFinished)
            );
        }
        catch(MaxConcurrentJobsReached e)
        {
            string errorMessage = string("MaxConcurrentJobsReached")
                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey) 
                + ", response.str(): " + response.str()
                + ", e.what(): " + e.what()
                ;
            _logger->warn(__FILEREF__ + errorMessage);

            throw e;
        }
        catch (curlpp::LogicError & e) 
        {
            _logger->error(__FILEREF__ + "Encoding URL failed (LogicError)"
                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey) 
                + ", ffmpegEncoderURL: " + ffmpegEncoderURL 
                + ", exception: " + e.what()
                + ", response.str(): " + response.str()
            );
            
            throw e;
        }
        catch (curlpp::RuntimeError & e) 
        {
            _logger->error(__FILEREF__ + "Encoding URL failed (RuntimeError)"
                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey) 
                + ", ffmpegEncoderURL: " + ffmpegEncoderURL 
                + ", exception: " + e.what()
                + ", response.str(): " + response.str()
            );

            throw e;
        }
        catch (runtime_error e)
        {
            _logger->error(__FILEREF__ + "Encoding URL failed (runtime_error)"
                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey) 
                + ", ffmpegEncoderURL: " + ffmpegEncoderURL 
                + ", exception: " + e.what()
                + ", response.str(): " + response.str()
            );

            throw e;
        }
        catch (exception e)
        {
            _logger->error(__FILEREF__ + "Encoding URL failed (exception)"
                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey) 
                + ", ffmpegEncoderURL: " + ffmpegEncoderURL 
                + ", exception: " + e.what()
                + ", response.str(): " + response.str()
            );

            throw e;
        }
    #endif

    return stagingEncodedAssetPathName;
}

void EncoderVideoAudioProxy::processOverlayedTextOnVideo(string stagingEncodedAssetPathName)
{
    try
    {
        size_t extensionIndex = stagingEncodedAssetPathName.find_last_of(".");
        if (extensionIndex == string::npos)
        {
            string errorMessage = __FILEREF__ + "No extention find in the asset file name"
                    + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                    + ", stagingEncodedAssetPathName: " + stagingEncodedAssetPathName;
            _logger->error(errorMessage);

            throw runtime_error(errorMessage);
        }
        string fileFormat = stagingEncodedAssetPathName.substr(extensionIndex + 1);

        size_t fileNameIndex = stagingEncodedAssetPathName.find_last_of("/");
        if (fileNameIndex == string::npos)
        {
            string errorMessage = __FILEREF__ + "No fileName find in the asset path name"
                    + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                    + ", stagingEncodedAssetPathName: " + stagingEncodedAssetPathName;
            _logger->error(errorMessage);

            throw runtime_error(errorMessage);
        }
        string sourceFileName = stagingEncodedAssetPathName.substr(fileNameIndex + 1);

        
        string mediaMetaDataContent = generateMediaMetadataToIngest(_encodingItem->_ingestionJobKey,
            fileFormat, _encodingItem->_overlayTextOnVideoData->_overlayTextParametersRoot);
    
        shared_ptr<LocalAssetIngestionEvent>    localAssetIngestionEvent = _multiEventsSet->getEventsFactory()
                ->getFreeEvent<LocalAssetIngestionEvent>(MMSENGINE_EVENTTYPEIDENTIFIER_LOCALASSETINGESTIONEVENT);

        localAssetIngestionEvent->setSource(ENCODERVIDEOAUDIOPROXY);
        localAssetIngestionEvent->setDestination(MMSENGINEPROCESSORNAME);
        localAssetIngestionEvent->setExpirationTimePoint(chrono::system_clock::now());

        localAssetIngestionEvent->setIngestionJobKey(_encodingItem->_ingestionJobKey);
        localAssetIngestionEvent->setIngestionSourceFileName(sourceFileName);
        localAssetIngestionEvent->setMMSSourceFileName(sourceFileName);
        localAssetIngestionEvent->setWorkspace(_encodingItem->_workspace);
        localAssetIngestionEvent->setIngestionType(MMSEngineDBFacade::IngestionType::AddContent);
        localAssetIngestionEvent->setIngestionRowToBeUpdatedAsSuccess(true);

        localAssetIngestionEvent->setMetadataContent(mediaMetaDataContent);

        shared_ptr<Event2>    event = dynamic_pointer_cast<Event2>(localAssetIngestionEvent);
        _multiEventsSet->addEvent(event);

        _logger->info(__FILEREF__ + "addEvent: EVENT_TYPE (INGESTASSETEVENT)"
            + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
            + ", ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey)
            + ", sourceFileName: " + sourceFileName
            + ", getEventKey().first: " + to_string(event->getEventKey().first)
            + ", getEventKey().second: " + to_string(event->getEventKey().second));
    }
    catch(runtime_error e)
    {
        _logger->error(__FILEREF__ + "processOverlayedImageOnVideo failed"
            + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
            + ", _encodingItem->_encodingJobKey: " + to_string(_encodingItem->_encodingJobKey)
            + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey)
            + ", _encodingItem->_encodingParameters: " + _encodingItem->_encodingParameters
            + ", stagingEncodedAssetPathName: " + stagingEncodedAssetPathName
            + ", _encodingItem->_workspace->_directoryName: " + _encodingItem->_workspace->_directoryName
            + ", e.what(): " + e.what()
        );
                
        throw e;
    }
    catch(exception e)
    {
        _logger->error(__FILEREF__ + "processOverlayedImageOnVideo failed"
            + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
            + ", _encodingItem->_encodingJobKey: " + to_string(_encodingItem->_encodingJobKey)
            + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey)
            + ", _encodingItem->_encodingParameters: " + _encodingItem->_encodingParameters
            + ", stagingEncodedAssetPathName: " + stagingEncodedAssetPathName
            + ", _encodingItem->_workspace->_directoryName: " + _encodingItem->_workspace->_directoryName
        );
                
        throw e;
    }
    /*
        pair<int64_t,int64_t> mediaItemKeyAndPhysicalPathKey;

        string encodedFileName;
        string relativePathToBeUsed;
        unsigned long mmsPartitionIndexUsed;
        string mmsAssetPathName;
        try
        {
            size_t fileNameIndex = stagingEncodedAssetPathName.find_last_of("/");
            if (fileNameIndex == string::npos)
            {
                string errorMessage = __FILEREF__ + "No fileName find in the asset path name"
                        + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                        + ", stagingEncodedAssetPathName: " + stagingEncodedAssetPathName;
                _logger->error(errorMessage);

                throw runtime_error(errorMessage);
            }
            encodedFileName = stagingEncodedAssetPathName.substr(fileNameIndex + 1);

            relativePathToBeUsed = _mmsEngineDBFacade->nextRelativePathToBeUsed (
                    _encodingItem->_workspace->_workspaceKey);

            bool partitionIndexToBeCalculated   = true;
            bool deliveryRepositoriesToo        = true;
            mmsAssetPathName = _mmsStorage->moveAssetInMMSRepository(
                stagingEncodedAssetPathName,
                _encodingItem->_workspace->_directoryName,
                encodedFileName,
                relativePathToBeUsed,
                partitionIndexToBeCalculated,
                &mmsPartitionIndexUsed,
                deliveryRepositoriesToo,
                _encodingItem->_workspace->_territories
                );
        }
        catch(runtime_error e)
        {
            _logger->error(__FILEREF__ + "_mmsStorage->moveAssetInMMSRepository failed"
                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                + ", _encodingItem->_encodingJobKey: " + to_string(_encodingItem->_encodingJobKey)
                + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey)
                + ", _encodingItem->_encodingParameters: " + _encodingItem->_encodingParameters
                + ", stagingEncodedAssetPathName: " + stagingEncodedAssetPathName
                + ", _encodingItem->_workspace->_directoryName: " + _encodingItem->_workspace->_directoryName
                + ", e.what(): " + e.what()
            );

            throw e;
        }
        catch(exception e)
        {
            _logger->error(__FILEREF__ + "_mmsStorage->moveAssetInMMSRepository failed"
                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                + ", _encodingItem->_encodingJobKey: " + to_string(_encodingItem->_encodingJobKey)
                + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey)
                + ", _encodingItem->_encodingParameters: " + _encodingItem->_encodingParameters
                + ", stagingEncodedAssetPathName: " + stagingEncodedAssetPathName
                + ", _encodingItem->_workspace->_directoryName: " + _encodingItem->_workspace->_directoryName
            );

            throw e;
        }

        MMSEngineDBFacade::ContentType contentType;

        int64_t durationInMilliSeconds = -1;
        long bitRate = -1;
        string videoCodecName;
        string videoProfile;
        int videoWidth = -1;
        int videoHeight = -1;
        string videoAvgFrameRate;
        long videoBitRate = -1;
        string audioCodecName;
        long audioSampleRate = -1;
        int audioChannels = -1;
        long audioBitRate = -1;

        int imageWidth = -1;
        int imageHeight = -1;
        string imageFormat;
        int imageQuality = -1;
        try
        {
            FFMpeg ffmpeg (_configuration, _logger);
            tuple<int64_t,long,string,string,int,int,string,long,string,long,int,long> mediaInfo =
                ffmpeg.getMediaInfo(mmsAssetPathName);

            tie(durationInMilliSeconds, bitRate, 
                videoCodecName, videoProfile, videoWidth, videoHeight, videoAvgFrameRate, videoBitRate,
                audioCodecName, audioSampleRate, audioChannels, audioBitRate) = mediaInfo;

            contentType = MMSEngineDBFacade::ContentType::Video;
        }
        catch(runtime_error e)
        {
            _logger->error(__FILEREF__ + "ffmpeg.getMediaInfo failed"
                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                + ", _encodingItem->_encodingJobKey: " + to_string(_encodingItem->_encodingJobKey)
                + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey)
                + ", _encodingItem->_encodingParameters: " + _encodingItem->_encodingParameters
                + ", stagingEncodedAssetPathName: " + stagingEncodedAssetPathName
                + ", _encodingItem->_workspace->_directoryName: " + _encodingItem->_workspace->_directoryName
                + ", e.what(): " + e.what()
            );

            throw e;
        }
        catch(exception e)
        {
            _logger->error(__FILEREF__ + "ffmpeg.getMediaInfo failed"
                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                + ", _encodingItem->_encodingJobKey: " + to_string(_encodingItem->_encodingJobKey)
                + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey)
                + ", _encodingItem->_encodingParameters: " + _encodingItem->_encodingParameters
                + ", stagingEncodedAssetPathName: " + stagingEncodedAssetPathName
                + ", _encodingItem->_workspace->_directoryName: " + _encodingItem->_workspace->_directoryName
            );

            throw e;
        }        

        try
        {
            bool inCaseOfLinkHasItToBeRead = false;
            unsigned long sizeInBytes = FileIO::getFileSizeInBytes(mmsAssetPathName,
                    inCaseOfLinkHasItToBeRead);   

            _logger->info(__FILEREF__ + "_mmsEngineDBFacade->saveIngestedContentMetadata..."
                + ", ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey)
                + ", contentType: " + MMSEngineDBFacade::toString(contentType)
                + ", relativePathToBeUsed: " + relativePathToBeUsed
                + ", encodedFileName: " + encodedFileName
                + ", mmsPartitionIndexUsed: " + to_string(mmsPartitionIndexUsed)
                + ", sizeInBytes: " + to_string(sizeInBytes)

                + ", durationInMilliSeconds: " + to_string(durationInMilliSeconds)
                + ", bitRate: " + to_string(bitRate)
                + ", videoCodecName: " + videoCodecName
                + ", videoProfile: " + videoProfile
                + ", videoWidth: " + to_string(videoWidth)
                + ", videoHeight: " + to_string(videoHeight)
                + ", videoAvgFrameRate: " + videoAvgFrameRate
                + ", videoBitRate: " + to_string(videoBitRate)
                + ", audioCodecName: " + audioCodecName
                + ", audioSampleRate: " + to_string(audioSampleRate)
                + ", audioChannels: " + to_string(audioChannels)
                + ", audioBitRate: " + to_string(audioBitRate)

                + ", imageWidth: " + to_string(imageWidth)
                + ", imageHeight: " + to_string(imageHeight)
                + ", imageFormat: " + imageFormat
                + ", imageQuality: " + to_string(imageQuality)
            );

            mediaItemKeyAndPhysicalPathKey = _mmsEngineDBFacade->saveIngestedContentMetadata (
                        _encodingItem->_workspace,
                        _encodingItem->_ingestionJobKey,
                        true, // ingestionRowToBeUpdatedAsSuccess
                        contentType,
                        _encodingItem->_overlayTextOnVideoData->_overlayTextParametersRoot,
                        relativePathToBeUsed,
                        encodedFileName,
                        mmsPartitionIndexUsed,
                        sizeInBytes,

                        // video-audio
                        durationInMilliSeconds,
                        bitRate,
                        videoCodecName,
                        videoProfile,
                        videoWidth,
                        videoHeight,
                        videoAvgFrameRate,
                        videoBitRate,
                        audioCodecName,
                        audioSampleRate,
                        audioChannels,
                        audioBitRate,

                        // image
                        imageWidth,
                        imageHeight,
                        imageFormat,
                        imageQuality
            );

            _logger->info(__FILEREF__ + "Added a new ingested content"
                + ", ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey)
                + ", mediaItemKey: " + to_string(mediaItemKeyAndPhysicalPathKey.first)
                + ", physicalPathKey: " + to_string(mediaItemKeyAndPhysicalPathKey.second)
            );
        }
        catch(runtime_error e)
        {
            _logger->error(__FILEREF__ + "_mmsEngineDBFacade->saveIngestedContentMetadata failed"
                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                + ", _encodingItem->_encodingJobKey: " + to_string(_encodingItem->_encodingJobKey)
                + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey)
                + ", _encodingItem->_encodingParameters: " + _encodingItem->_encodingParameters
                + ", stagingEncodedAssetPathName: " + stagingEncodedAssetPathName
                + ", _encodingItem->_workspace->_directoryName: " + _encodingItem->_workspace->_directoryName
                + ", e.what(): " + e.what()
            );

            throw e;
        }
        catch(exception e)
        {
            _logger->error(__FILEREF__ + "_mmsEngineDBFacade->saveIngestedContentMetadata failed"
                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                + ", _encodingItem->_encodingJobKey: " + to_string(_encodingItem->_encodingJobKey)
                + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey)
                + ", _encodingItem->_encodingParameters: " + _encodingItem->_encodingParameters
                + ", stagingEncodedAssetPathName: " + stagingEncodedAssetPathName
                + ", _encodingItem->_workspace->_directoryName: " + _encodingItem->_workspace->_directoryName
            );

            throw e;
        }    


        return mediaItemKeyAndPhysicalPathKey;
     */
}

void EncoderVideoAudioProxy::generateFrames()
{    
    /*
    _logger->info(__FILEREF__ + "Creating encoderVideoAudioProxy thread"
            + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
        + ", _encodingItem->_encodingProfileTechnology" + to_string(static_cast<int>(_encodingItem->_encodingProfileTechnology))
        + ", _mp4Encoder: " + _mp4Encoder
    );

    if (
        (_encodingItem->_encodingProfileTechnology == MMSEngineDBFacade::EncodingTechnology::MP4 &&
            _mp4Encoder == "FFMPEG") ||
        (_encodingItem->_encodingProfileTechnology == MMSEngineDBFacade::EncodingTechnology::MPEG2_TS &&
            _mpeg2TSEncoder == "FFMPEG") ||
        _encodingItem->_encodingProfileTechnology == MMSEngineDBFacade::EncodingTechnology::WEBM ||
        (_encodingItem->_encodingProfileTechnology == MMSEngineDBFacade::EncodingTechnology::Adobe &&
            _mpeg2TSEncoder == "FFMPEG")
    )
    {
    */
        generateFrames_through_ffmpeg();
    /*
    }
    else if (_encodingItem->_encodingProfileTechnology == MMSEngineDBFacade::EncodingTechnology::WindowsMedia)
    {
        string errorMessage = __FILEREF__ + "No Encoder available to encode WindowsMedia technology"
            + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                ;
        _logger->error(errorMessage);
        
        throw runtime_error(errorMessage);
    }
    else
    {
        string errorMessage = __FILEREF__ + "Unknown technology and no Encoder available to encode"
            + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                ;
        _logger->error(errorMessage);
        
        throw runtime_error(errorMessage);
    }
    */    
}

void EncoderVideoAudioProxy::generateFrames_through_ffmpeg()
{
    
    string imageDirectory;
    double startTimeInSeconds;
    int maxFramesNumber;  
    string videoFilter;
    int periodInSeconds;
    bool mjpeg;
    int imageWidth;
    int imageHeight;
    int64_t ingestionJobKey;
    int64_t videoDurationInMilliSeconds;

    // _encodingItem->_parametersRoot filled in MMSEngineDBFacade::addOverlayImageOnVideoJob
    {
        string field = "imageDirectory";
        imageDirectory = _encodingItem->_parametersRoot.get(field, "XXX").asString();

        field = "startTimeInSeconds";
        startTimeInSeconds = _encodingItem->_parametersRoot.get(field, 0).asDouble();

        field = "maxFramesNumber";
        maxFramesNumber = _encodingItem->_parametersRoot.get(field, 0).asInt();

        field = "videoFilter";
        videoFilter = _encodingItem->_parametersRoot.get(field, "XXX").asString();

        field = "periodInSeconds";
        periodInSeconds = _encodingItem->_parametersRoot.get(field, 0).asInt();

        field = "mjpeg";
        mjpeg = _encodingItem->_parametersRoot.get(field, 0).asBool();

        field = "imageWidth";
        imageWidth = _encodingItem->_parametersRoot.get(field, 0).asInt();

        field = "imageHeight";
        imageHeight = _encodingItem->_parametersRoot.get(field, 0).asInt();

        field = "ingestionJobKey";
        ingestionJobKey = _encodingItem->_parametersRoot.get(field, 0).asInt64();

        field = "videoDurationInMilliSeconds";
        videoDurationInMilliSeconds = _encodingItem->_parametersRoot.get(field, 0).asInt64();
    }
    
    string mmsSourceVideoAssetPathName;

    
    #ifdef __LOCALENCODER__
        if (*_pRunningEncodingsNumber > _ffmpegMaxCapacity)
        {
            _logger->info("Max ffmpeg encoder capacity is reached"
                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
            );

            throw MaxConcurrentJobsReached();
        }
    #endif

    // stagingEncodedAssetPathName preparation
    {        
        mmsSourceVideoAssetPathName = _mmsStorage->getMMSAssetPathName(
            _encodingItem->_generateFramesData->_mmsVideoPartitionNumber,
            _encodingItem->_workspace->_directoryName,
            _encodingItem->_generateFramesData->_videoRelativePath,
            _encodingItem->_generateFramesData->_videoFileName);
    }

    #ifdef __LOCALENCODER__
        (*_pRunningEncodingsNumber)++;

    #else
        string ffmpegEncoderURL;
        ostringstream response;
        try
        {
            _currentUsedFFMpegEncoderHost = _encodersLoadBalancer->getEncoderHost(_encodingItem->_workspace);
            _logger->info(__FILEREF__ + "Configuration item"
                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                + ", _currentUsedFFMpegEncoderHost: " + _currentUsedFFMpegEncoderHost
            );
            ffmpegEncoderURL = 
                    _ffmpegEncoderProtocol
                    + "://"
                    + _currentUsedFFMpegEncoderHost + ":"
                    + to_string(_ffmpegEncoderPort)
                    + _ffmpegGenerateFramesURI
                    + "/" + to_string(_encodingItem->_encodingJobKey)
            ;
            string body;
            {
                Json::Value generateFramesMedatada;
                
                generateFramesMedatada["imageDirectory"] = imageDirectory;
                generateFramesMedatada["startTimeInSeconds"] = startTimeInSeconds;
                generateFramesMedatada["maxFramesNumber"] = maxFramesNumber;
                generateFramesMedatada["videoFilter"] = videoFilter;
                generateFramesMedatada["periodInSeconds"] = periodInSeconds;
                generateFramesMedatada["mjpeg"] = mjpeg;
                generateFramesMedatada["imageWidth"] = imageWidth;
                generateFramesMedatada["imageHeight"] = imageHeight;
                generateFramesMedatada["ingestionJobKey"] = (Json::LargestUInt) (_encodingItem->_ingestionJobKey);
                generateFramesMedatada["mmsSourceVideoAssetPathName"] = mmsSourceVideoAssetPathName;
                generateFramesMedatada["videoDurationInMilliSeconds"] = (Json::LargestUInt) (videoDurationInMilliSeconds);

                {
                    Json::StreamWriterBuilder wbuilder;
                    
                    body = Json::writeString(wbuilder, generateFramesMedatada);
                }
            }
            
            list<string> header;

            header.push_back("Content-Type: application/json");
            {
                string userPasswordEncoded = Convert::base64_encode(_ffmpegEncoderUser + ":" + _ffmpegEncoderPassword);
                string basicAuthorization = string("Authorization: Basic ") + userPasswordEncoded;

                header.push_back(basicAuthorization);
            }
            
            curlpp::Cleanup cleaner;
            curlpp::Easy request;

            // Setting the URL to retrive.
            request.setOpt(new curlpp::options::Url(ffmpegEncoderURL));

            if (_ffmpegEncoderProtocol == "https")
            {
                /*
                    typedef curlpp::OptionTrait<std::string, CURLOPT_SSLCERTPASSWD> SslCertPasswd;                            
                    typedef curlpp::OptionTrait<std::string, CURLOPT_SSLKEY> SslKey;                                          
                    typedef curlpp::OptionTrait<std::string, CURLOPT_SSLKEYTYPE> SslKeyType;                                  
                    typedef curlpp::OptionTrait<std::string, CURLOPT_SSLKEYPASSWD> SslKeyPasswd;                              
                    typedef curlpp::OptionTrait<std::string, CURLOPT_SSLENGINE> SslEngine;                                    
                    typedef curlpp::NoValueOptionTrait<CURLOPT_SSLENGINE_DEFAULT> SslEngineDefault;                           
                    typedef curlpp::OptionTrait<long, CURLOPT_SSLVERSION> SslVersion;                                         
                    typedef curlpp::OptionTrait<std::string, CURLOPT_CAINFO> CaInfo;                                          
                    typedef curlpp::OptionTrait<std::string, CURLOPT_CAPATH> CaPath;                                          
                    typedef curlpp::OptionTrait<std::string, CURLOPT_RANDOM_FILE> RandomFile;                                 
                    typedef curlpp::OptionTrait<std::string, CURLOPT_EGDSOCKET> EgdSocket;                                    
                    typedef curlpp::OptionTrait<std::string, CURLOPT_SSL_CIPHER_LIST> SslCipherList;                          
                    typedef curlpp::OptionTrait<std::string, CURLOPT_KRB4LEVEL> Krb4Level;                                    
                 */
                                                                                                  
                
                /*
                // cert is stored PEM coded in file... 
                // since PEM is default, we needn't set it for PEM 
                // curl_easy_setopt(curl, CURLOPT_SSLCERTTYPE, "PEM");
                curlpp::OptionTrait<string, CURLOPT_SSLCERTTYPE> sslCertType("PEM");
                equest.setOpt(sslCertType);

                // set the cert for client authentication
                // "testcert.pem"
                // curl_easy_setopt(curl, CURLOPT_SSLCERT, pCertFile);
                curlpp::OptionTrait<string, CURLOPT_SSLCERT> sslCert("cert.pem");
                request.setOpt(sslCert);
                 */

                /*
                // sorry, for engine we must set the passphrase
                //   (if the key has one...)
                // const char *pPassphrase = NULL;
                if(pPassphrase)
                  curl_easy_setopt(curl, CURLOPT_KEYPASSWD, pPassphrase);

                // if we use a key stored in a crypto engine,
                //   we must set the key type to "ENG"
                // pKeyType  = "PEM";
                curl_easy_setopt(curl, CURLOPT_SSLKEYTYPE, pKeyType);

                // set the private key (file or ID in engine)
                // pKeyName  = "testkey.pem";
                curl_easy_setopt(curl, CURLOPT_SSLKEY, pKeyName);

                // set the file with the certs vaildating the server
                // *pCACertFile = "cacert.pem";
                curl_easy_setopt(curl, CURLOPT_CAINFO, pCACertFile);
                */
                
                // disconnect if we can't validate server's cert
                bool bSslVerifyPeer = false;
                curlpp::OptionTrait<bool, CURLOPT_SSL_VERIFYPEER> sslVerifyPeer(bSslVerifyPeer);
                request.setOpt(sslVerifyPeer);
                
                curlpp::OptionTrait<bool, CURLOPT_SSL_VERIFYHOST> sslVerifyHost(0L);
                request.setOpt(sslVerifyHost);
                
                // request.setOpt(new curlpp::options::SslEngineDefault());                                              

            }
            request.setOpt(new curlpp::options::HttpHeader(header));
            request.setOpt(new curlpp::options::PostFields(body));
            request.setOpt(new curlpp::options::PostFieldSize(body.length()));

            request.setOpt(new curlpp::options::WriteStream(&response));

            chrono::system_clock::time_point startEncoding = chrono::system_clock::now();

            _logger->info(__FILEREF__ + "Generating Frames"
                    + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                    + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey) 
                    + ", ffmpegEncoderURL: " + ffmpegEncoderURL
                    + ", body: " + body
            );
            request.perform();

            string sResponse = response.str();
            // LF and CR create problems to the json parser...
            while (sResponse.back() == 10 || sResponse.back() == 13)
                sResponse.pop_back();

            Json::Value generateFramesContentResponse;
            try
            {                
                Json::CharReaderBuilder builder;
                Json::CharReader* reader = builder.newCharReader();
                string errors;

                bool parsingSuccessful = reader->parse(sResponse.c_str(),
                        sResponse.c_str() + sResponse.size(), 
                        &generateFramesContentResponse, &errors);
                delete reader;

                if (!parsingSuccessful)
                {
                    string errorMessage = __FILEREF__ + "failed to parse the response body"
                            + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                            + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey) 
                            + ", errors: " + errors
                            + ", sResponse: " + sResponse
                            ;
                    _logger->error(errorMessage);

                    throw runtime_error(errorMessage);
                }               
            }
            catch(runtime_error e)
            {
                string errorMessage = string("response Body json is not well format")
                        + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                        + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey) 
                        + ", sResponse: " + sResponse
                        + ", e.what(): " + e.what()
                        ;
                _logger->error(__FILEREF__ + errorMessage);

                throw e;
            }
            catch(...)
            {
                string errorMessage = string("response Body json is not well format")
                        + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                        + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey) 
                        + ", sResponse: " + sResponse
                        ;
                _logger->error(__FILEREF__ + errorMessage);

                throw runtime_error(errorMessage);
            }

            {
                // same string declared in FFMPEGEncoder.cpp
                string noEncodingAvailableMessage("__NO-ENCODING-AVAILABLE__");
            
                string field = "error";
                if (Validator::isMetadataPresent(generateFramesContentResponse, field))
                {
                    string error = generateFramesContentResponse.get(field, "XXX").asString();
                    
                    if (error.find(noEncodingAvailableMessage) != string::npos)
                    {
                        string errorMessage = string("No Encodings available")
                                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                                + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey) 
                                + ", sResponse: " + sResponse
                                ;
                        _logger->warn(__FILEREF__ + errorMessage);

                        throw MaxConcurrentJobsReached();
                    }
                    else
                    {
                        string errorMessage = string("FFMPEGEncoder error")
                                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                                + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey) 
                                + ", sResponse: " + sResponse
                                ;
                        _logger->error(__FILEREF__ + errorMessage);

                        throw runtime_error(errorMessage);
                    }                        
                }
                /*
                else
                {
                    string field = "ffmpegEncoderHost";
                    if (Validator::isMetadataPresent(encodeContentResponse, field))
                    {
                        _currentUsedFFMpegEncoderHost = encodeContentResponse.get("ffmpegEncoderHost", "XXX").asString();
                        
                        _logger->info(__FILEREF__ + "Retrieving ffmpegEncoderHost"
                            + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                            + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey) 
                            + "_currentUsedFFMpegEncoderHost: " + _currentUsedFFMpegEncoderHost
                                );                                        
                    }
                    else
                    {
                        string errorMessage = string("Unexpected FFMPEGEncoder response")
                                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                                + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey) 
                                + ", sResponse: " + sResponse
                                ;
                        _logger->error(__FILEREF__ + errorMessage);

                        throw runtime_error(errorMessage);
                    }
                }
                */                        
            }
            
            // loop waiting the end of the encoding
            bool encodingFinished = false;
            int maxEncodingStatusFailures = 1;
            int encodingStatusFailures = 0;
            while(!(encodingFinished || encodingStatusFailures >= maxEncodingStatusFailures))
            {
                this_thread::sleep_for(chrono::seconds(_intervalInSecondsToCheckEncodingFinished));
                
                try
                {
                    encodingFinished = getEncodingStatus(/* _encodingItem->_encodingJobKey */);
                }
                catch(...)
                {
                    _logger->error(__FILEREF__ + "getEncodingStatus failed");
                    
                    encodingStatusFailures++;
                }
            }
            
            chrono::system_clock::time_point endEncoding = chrono::system_clock::now();
            
            _logger->info(__FILEREF__ + "Generated Frames"
                    + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                    + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey) 
                    + ", ffmpegEncoderURL: " + ffmpegEncoderURL
                    + ", body: " + body
                    + ", sResponse: " + sResponse
                    + ", encodingDuration (secs): " + to_string(chrono::duration_cast<chrono::seconds>(endEncoding - startEncoding).count())
                    + ", _intervalInSecondsToCheckEncodingFinished: " + to_string(_intervalInSecondsToCheckEncodingFinished)
            );
        }
        catch(MaxConcurrentJobsReached e)
        {
            string errorMessage = string("MaxConcurrentJobsReached")
                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey) 
                + ", response.str(): " + response.str()
                + ", e.what(): " + e.what()
                ;
            _logger->warn(__FILEREF__ + errorMessage);

            throw e;
        }
        catch (curlpp::LogicError & e) 
        {
            _logger->error(__FILEREF__ + "Encoding URL failed (LogicError)"
                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey) 
                + ", ffmpegEncoderURL: " + ffmpegEncoderURL 
                + ", exception: " + e.what()
                + ", response.str(): " + response.str()
            );
            
            throw e;
        }
        catch (curlpp::RuntimeError & e) 
        {
            _logger->error(__FILEREF__ + "Encoding URL failed (RuntimeError)"
                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey) 
                + ", ffmpegEncoderURL: " + ffmpegEncoderURL 
                + ", exception: " + e.what()
                + ", response.str(): " + response.str()
            );

            throw e;
        }
        catch (runtime_error e)
        {
            _logger->error(__FILEREF__ + "Encoding URL failed (runtime_error)"
                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey) 
                + ", ffmpegEncoderURL: " + ffmpegEncoderURL 
                + ", exception: " + e.what()
                + ", response.str(): " + response.str()
            );

            throw e;
        }
        catch (exception e)
        {
            _logger->error(__FILEREF__ + "Encoding URL failed (exception)"
                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey) 
                + ", ffmpegEncoderURL: " + ffmpegEncoderURL 
                + ", exception: " + e.what()
                + ", response.str(): " + response.str()
            );

            throw e;
        }
    #endif
}

void EncoderVideoAudioProxy::processGeneratedFrames()
{    
    // here we do not have just a profile to be added into MMS but we have
    // one or more MediaItemKeys that have to be ingested
    // One MIK in case of a .mjpeg
    // One or more MIKs in case of .jpg
    
    shared_ptr<MultiLocalAssetIngestionEvent>    multiLocalAssetIngestionEvent = _multiEventsSet->getEventsFactory()
        ->getFreeEvent<MultiLocalAssetIngestionEvent>(MMSENGINE_EVENTTYPEIDENTIFIER_MULTILOCALASSETINGESTIONEVENT);

    multiLocalAssetIngestionEvent->setSource(ENCODERVIDEOAUDIOPROXY);
    multiLocalAssetIngestionEvent->setDestination(MMSENGINEPROCESSORNAME);
    multiLocalAssetIngestionEvent->setExpirationTimePoint(chrono::system_clock::now());

    multiLocalAssetIngestionEvent->setIngestionJobKey(_encodingItem->_ingestionJobKey);
    multiLocalAssetIngestionEvent->setEncodingJobKey(_encodingItem->_encodingJobKey);
    multiLocalAssetIngestionEvent->setWorkspace(_encodingItem->_workspace);
    multiLocalAssetIngestionEvent->setParametersRoot(_encodingItem->_generateFramesData->_generateFramesParametersRoot);

    shared_ptr<Event2>    event = dynamic_pointer_cast<Event2>(multiLocalAssetIngestionEvent);
    _multiEventsSet->addEvent(event);

    _logger->info(__FILEREF__ + "addEvent: EVENT_TYPE (MULTIINGESTASSETEVENT)"
        + ", ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey)
        + ", getEventKey().first: " + to_string(event->getEventKey().first)
        + ", getEventKey().second: " + to_string(event->getEventKey().second));
}

string EncoderVideoAudioProxy::slideShow()
{
    string stagingEncodedAssetPathName;
    
    /*
    _logger->info(__FILEREF__ + "Creating encoderVideoAudioProxy thread"
            + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
        + ", _encodingItem->_encodingProfileTechnology" + to_string(static_cast<int>(_encodingItem->_encodingProfileTechnology))
        + ", _mp4Encoder: " + _mp4Encoder
    );

    if (
        (_encodingItem->_encodingProfileTechnology == MMSEngineDBFacade::EncodingTechnology::MP4 &&
            _mp4Encoder == "FFMPEG") ||
        (_encodingItem->_encodingProfileTechnology == MMSEngineDBFacade::EncodingTechnology::MPEG2_TS &&
            _mpeg2TSEncoder == "FFMPEG") ||
        _encodingItem->_encodingProfileTechnology == MMSEngineDBFacade::EncodingTechnology::WEBM ||
        (_encodingItem->_encodingProfileTechnology == MMSEngineDBFacade::EncodingTechnology::Adobe &&
            _mpeg2TSEncoder == "FFMPEG")
    )
    {
    */
        stagingEncodedAssetPathName = slideShow_through_ffmpeg();
    /*
    }
    else if (_encodingItem->_encodingProfileTechnology == MMSEngineDBFacade::EncodingTechnology::WindowsMedia)
    {
        string errorMessage = __FILEREF__ + "No Encoder available to encode WindowsMedia technology"
            + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                ;
        _logger->error(errorMessage);
        
        throw runtime_error(errorMessage);
    }
    else
    {
        string errorMessage = __FILEREF__ + "Unknown technology and no Encoder available to encode"
            + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                ;
        _logger->error(errorMessage);
        
        throw runtime_error(errorMessage);
    }
    */
    
    return stagingEncodedAssetPathName;
}

string EncoderVideoAudioProxy::slideShow_through_ffmpeg()
{
    
    double durationOfEachSlideInSeconds;
    int outputFrameRate;  
    Json::Value sourcePhysicalPathsRoot(Json::arrayValue);

    {
        string field = "durationOfEachSlideInSeconds";
        durationOfEachSlideInSeconds = _encodingItem->_parametersRoot.get(field, 0).asDouble();

        field = "outputFrameRate";
        outputFrameRate = _encodingItem->_parametersRoot.get(field, 0).asInt();

        field = "sourcePhysicalPaths";
        sourcePhysicalPathsRoot = _encodingItem->_parametersRoot[field];
    }
    
    string slideShowMediaPathName;
    // string encodedFileName;

    
    #ifdef __LOCALENCODER__
        if (*_pRunningEncodingsNumber > _ffmpegMaxCapacity)
        {
            _logger->info("Max ffmpeg encoder capacity is reached"
                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
            );

            throw MaxConcurrentJobsReached();
        }
    #endif

    {        
        string fileFormat = "mp4";

        /*
        encodedFileName =
                to_string(_encodingItem->_ingestionJobKey)
                + "_"
                + to_string(_encodingItem->_encodingJobKey)
                + "." + fileFormat
                ;
        */

        string workspaceIngestionRepository = _mmsStorage->getWorkspaceIngestionRepository(
                _encodingItem->_workspace);
        slideShowMediaPathName = 
                workspaceIngestionRepository + "/" 
                + to_string(_encodingItem->_ingestionJobKey)
                + "." + fileFormat
                ;
    }

    #ifdef __LOCALENCODER__
        (*_pRunningEncodingsNumber)++;
    #else
        string ffmpegEncoderURL;
        ostringstream response;
        try
        {
            _currentUsedFFMpegEncoderHost = _encodersLoadBalancer->getEncoderHost(_encodingItem->_workspace);
            _logger->info(__FILEREF__ + "Configuration item"
                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                + ", _currentUsedFFMpegEncoderHost: " + _currentUsedFFMpegEncoderHost
            );
            ffmpegEncoderURL = 
                    _ffmpegEncoderProtocol
                    + "://"
                    + _currentUsedFFMpegEncoderHost + ":"
                    + to_string(_ffmpegEncoderPort)
                    + _ffmpegSlideShowURI
                    + "/" + to_string(_encodingItem->_encodingJobKey)
            ;
            string body;
            {
                Json::Value slideShowMedatada;
                
                slideShowMedatada["ingestionJobKey"] = (Json::LargestUInt) (_encodingItem->_ingestionJobKey);
                slideShowMedatada["durationOfEachSlideInSeconds"] = durationOfEachSlideInSeconds;
                slideShowMedatada["outputFrameRate"] = outputFrameRate;
                slideShowMedatada["slideShowMediaPathName"] = slideShowMediaPathName;
                slideShowMedatada["sourcePhysicalPaths"] = sourcePhysicalPathsRoot;

                {
                    Json::StreamWriterBuilder wbuilder;
                    
                    body = Json::writeString(wbuilder, slideShowMedatada);
                }
            }
            
            list<string> header;

            header.push_back("Content-Type: application/json");
            {
                string userPasswordEncoded = Convert::base64_encode(_ffmpegEncoderUser + ":" + _ffmpegEncoderPassword);
                string basicAuthorization = string("Authorization: Basic ") + userPasswordEncoded;

                header.push_back(basicAuthorization);
            }
            
            curlpp::Cleanup cleaner;
            curlpp::Easy request;

            // Setting the URL to retrive.
            request.setOpt(new curlpp::options::Url(ffmpegEncoderURL));

            if (_ffmpegEncoderProtocol == "https")
            {
                /*
                    typedef curlpp::OptionTrait<std::string, CURLOPT_SSLCERTPASSWD> SslCertPasswd;                            
                    typedef curlpp::OptionTrait<std::string, CURLOPT_SSLKEY> SslKey;                                          
                    typedef curlpp::OptionTrait<std::string, CURLOPT_SSLKEYTYPE> SslKeyType;                                  
                    typedef curlpp::OptionTrait<std::string, CURLOPT_SSLKEYPASSWD> SslKeyPasswd;                              
                    typedef curlpp::OptionTrait<std::string, CURLOPT_SSLENGINE> SslEngine;                                    
                    typedef curlpp::NoValueOptionTrait<CURLOPT_SSLENGINE_DEFAULT> SslEngineDefault;                           
                    typedef curlpp::OptionTrait<long, CURLOPT_SSLVERSION> SslVersion;                                         
                    typedef curlpp::OptionTrait<std::string, CURLOPT_CAINFO> CaInfo;                                          
                    typedef curlpp::OptionTrait<std::string, CURLOPT_CAPATH> CaPath;                                          
                    typedef curlpp::OptionTrait<std::string, CURLOPT_RANDOM_FILE> RandomFile;                                 
                    typedef curlpp::OptionTrait<std::string, CURLOPT_EGDSOCKET> EgdSocket;                                    
                    typedef curlpp::OptionTrait<std::string, CURLOPT_SSL_CIPHER_LIST> SslCipherList;                          
                    typedef curlpp::OptionTrait<std::string, CURLOPT_KRB4LEVEL> Krb4Level;                                    
                 */
                                                                                                  
                
                /*
                // cert is stored PEM coded in file... 
                // since PEM is default, we needn't set it for PEM 
                // curl_easy_setopt(curl, CURLOPT_SSLCERTTYPE, "PEM");
                curlpp::OptionTrait<string, CURLOPT_SSLCERTTYPE> sslCertType("PEM");
                equest.setOpt(sslCertType);

                // set the cert for client authentication
                // "testcert.pem"
                // curl_easy_setopt(curl, CURLOPT_SSLCERT, pCertFile);
                curlpp::OptionTrait<string, CURLOPT_SSLCERT> sslCert("cert.pem");
                request.setOpt(sslCert);
                 */

                /*
                // sorry, for engine we must set the passphrase
                //   (if the key has one...)
                // const char *pPassphrase = NULL;
                if(pPassphrase)
                  curl_easy_setopt(curl, CURLOPT_KEYPASSWD, pPassphrase);

                // if we use a key stored in a crypto engine,
                //   we must set the key type to "ENG"
                // pKeyType  = "PEM";
                curl_easy_setopt(curl, CURLOPT_SSLKEYTYPE, pKeyType);

                // set the private key (file or ID in engine)
                // pKeyName  = "testkey.pem";
                curl_easy_setopt(curl, CURLOPT_SSLKEY, pKeyName);

                // set the file with the certs vaildating the server
                // *pCACertFile = "cacert.pem";
                curl_easy_setopt(curl, CURLOPT_CAINFO, pCACertFile);
                */
                
                // disconnect if we can't validate server's cert
                bool bSslVerifyPeer = false;
                curlpp::OptionTrait<bool, CURLOPT_SSL_VERIFYPEER> sslVerifyPeer(bSslVerifyPeer);
                request.setOpt(sslVerifyPeer);
                
                curlpp::OptionTrait<bool, CURLOPT_SSL_VERIFYHOST> sslVerifyHost(0L);
                request.setOpt(sslVerifyHost);
                
                // request.setOpt(new curlpp::options::SslEngineDefault());                                              

            }
            request.setOpt(new curlpp::options::HttpHeader(header));
            request.setOpt(new curlpp::options::PostFields(body));
            request.setOpt(new curlpp::options::PostFieldSize(body.length()));

            request.setOpt(new curlpp::options::WriteStream(&response));

            chrono::system_clock::time_point startEncoding = chrono::system_clock::now();

            _logger->info(__FILEREF__ + "SlideShow media file"
                    + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                    + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey) 
                    + ", ffmpegEncoderURL: " + ffmpegEncoderURL
                    + ", body: " + body
            );
            request.perform();

            string sResponse = response.str();
            // LF and CR create problems to the json parser...
            while (sResponse.back() == 10 || sResponse.back() == 13)
                sResponse.pop_back();

            Json::Value slideShowContentResponse;
            try
            {                
                Json::CharReaderBuilder builder;
                Json::CharReader* reader = builder.newCharReader();
                string errors;

                bool parsingSuccessful = reader->parse(sResponse.c_str(),
                        sResponse.c_str() + sResponse.size(), 
                        &slideShowContentResponse, &errors);
                delete reader;

                if (!parsingSuccessful)
                {
                    string errorMessage = __FILEREF__ + "failed to parse the response body"
                            + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                            + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey) 
                            + ", errors: " + errors
                            + ", sResponse: " + sResponse
                            ;
                    _logger->error(errorMessage);

                    throw runtime_error(errorMessage);
                }               
            }
            catch(runtime_error e)
            {
                string errorMessage = string("response Body json is not well format")
                        + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                        + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey) 
                        + ", sResponse: " + sResponse
                        + ", e.what(): " + e.what()
                        ;
                _logger->error(__FILEREF__ + errorMessage);

                throw e;
            }
            catch(...)
            {
                string errorMessage = string("response Body json is not well format")
                        + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                        + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey) 
                        + ", sResponse: " + sResponse
                        ;
                _logger->error(__FILEREF__ + errorMessage);

                throw runtime_error(errorMessage);
            }

            {
                // same string declared in FFMPEGEncoder.cpp
                string noEncodingAvailableMessage("__NO-ENCODING-AVAILABLE__");
            
                string field = "error";
                if (Validator::isMetadataPresent(slideShowContentResponse, field))
                {
                    string error = slideShowContentResponse.get(field, "XXX").asString();
                    
                    if (error.find(noEncodingAvailableMessage) != string::npos)
                    {
                        string errorMessage = string("No Encodings available")
                                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                                + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey) 
                                + ", sResponse: " + sResponse
                                ;
                        _logger->warn(__FILEREF__ + errorMessage);

                        throw MaxConcurrentJobsReached();
                    }
                    else
                    {
                        string errorMessage = string("FFMPEGEncoder error")
                                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                                + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey) 
                                + ", sResponse: " + sResponse
                                ;
                        _logger->error(__FILEREF__ + errorMessage);

                        throw runtime_error(errorMessage);
                    }                        
                }
                /*
                else
                {
                    string field = "ffmpegEncoderHost";
                    if (Validator::isMetadataPresent(encodeContentResponse, field))
                    {
                        _currentUsedFFMpegEncoderHost = encodeContentResponse.get("ffmpegEncoderHost", "XXX").asString();
                        
                        _logger->info(__FILEREF__ + "Retrieving ffmpegEncoderHost"
                            + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                            + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey) 
                            + "_currentUsedFFMpegEncoderHost: " + _currentUsedFFMpegEncoderHost
                                );                                        
                    }
                    else
                    {
                        string errorMessage = string("Unexpected FFMPEGEncoder response")
                                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                                + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey) 
                                + ", sResponse: " + sResponse
                                ;
                        _logger->error(__FILEREF__ + errorMessage);

                        throw runtime_error(errorMessage);
                    }
                }
                */                        
            }
            
            // loop waiting the end of the encoding
            bool encodingFinished = false;
            int maxEncodingStatusFailures = 1;
            int encodingStatusFailures = 0;
            while(!(encodingFinished || encodingStatusFailures >= maxEncodingStatusFailures))
            {
                this_thread::sleep_for(chrono::seconds(_intervalInSecondsToCheckEncodingFinished));
                
                try
                {
                    encodingFinished = getEncodingStatus(/* _encodingItem->_encodingJobKey */);
                }
                catch(...)
                {
                    _logger->error(__FILEREF__ + "getEncodingStatus failed");
                    
                    encodingStatusFailures++;
                }
            }
            
            chrono::system_clock::time_point endEncoding = chrono::system_clock::now();
            
            _logger->info(__FILEREF__ + "SlideShow media file"
                    + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                    + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey) 
                    + ", ffmpegEncoderURL: " + ffmpegEncoderURL
                    + ", body: " + body
                    + ", sResponse: " + sResponse
                    + ", encodingDuration (secs): " + to_string(chrono::duration_cast<chrono::seconds>(endEncoding - startEncoding).count())
                    + ", _intervalInSecondsToCheckEncodingFinished: " + to_string(_intervalInSecondsToCheckEncodingFinished)
            );
        }
        catch(MaxConcurrentJobsReached e)
        {
            string errorMessage = string("MaxConcurrentJobsReached")
                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey) 
                + ", response.str(): " + response.str()
                + ", e.what(): " + e.what()
                ;
            _logger->warn(__FILEREF__ + errorMessage);

            throw e;
        }
        catch (curlpp::LogicError & e) 
        {
            _logger->error(__FILEREF__ + "Encoding URL failed (LogicError)"
                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey) 
                + ", ffmpegEncoderURL: " + ffmpegEncoderURL 
                + ", exception: " + e.what()
                + ", response.str(): " + response.str()
            );
            
            throw e;
        }
        catch (curlpp::RuntimeError & e) 
        {
            _logger->error(__FILEREF__ + "Encoding URL failed (RuntimeError)"
                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey) 
                + ", ffmpegEncoderURL: " + ffmpegEncoderURL 
                + ", exception: " + e.what()
                + ", response.str(): " + response.str()
            );

            throw e;
        }
        catch (runtime_error e)
        {
            _logger->error(__FILEREF__ + "Encoding URL failed (runtime_error)"
                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey) 
                + ", ffmpegEncoderURL: " + ffmpegEncoderURL 
                + ", exception: " + e.what()
                + ", response.str(): " + response.str()
            );

            throw e;
        }
        catch (exception e)
        {
            _logger->error(__FILEREF__ + "Encoding URL failed (exception)"
                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey) 
                + ", ffmpegEncoderURL: " + ffmpegEncoderURL 
                + ", exception: " + e.what()
                + ", response.str(): " + response.str()
            );

            throw e;
        }
    #endif

    return slideShowMediaPathName;
}

void EncoderVideoAudioProxy::processSlideShow(string stagingEncodedAssetPathName)
{
    try
    {
        int outputFrameRate;  
        string field = "outputFrameRate";
        outputFrameRate = _encodingItem->_parametersRoot.get(field, 0).asInt();
    
        size_t extensionIndex = stagingEncodedAssetPathName.find_last_of(".");
        if (extensionIndex == string::npos)
        {
            string errorMessage = __FILEREF__ + "No extention find in the asset file name"
                    + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                    + ", stagingEncodedAssetPathName: " + stagingEncodedAssetPathName;
            _logger->error(errorMessage);

            throw runtime_error(errorMessage);
        }
        string fileFormat = stagingEncodedAssetPathName.substr(extensionIndex + 1);

        size_t fileNameIndex = stagingEncodedAssetPathName.find_last_of("/");
        if (fileNameIndex == string::npos)
        {
            string errorMessage = __FILEREF__ + "No fileName find in the asset path name"
                    + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                    + ", stagingEncodedAssetPathName: " + stagingEncodedAssetPathName;
            _logger->error(errorMessage);

            throw runtime_error(errorMessage);
        }
        string sourceFileName = stagingEncodedAssetPathName.substr(fileNameIndex + 1);


        
        string mediaMetaDataContent = generateMediaMetadataToIngest(_encodingItem->_ingestionJobKey,
            fileFormat, _encodingItem->_slideShowData->_slideShowParametersRoot);
    
        shared_ptr<LocalAssetIngestionEvent>    localAssetIngestionEvent = _multiEventsSet->getEventsFactory()
                ->getFreeEvent<LocalAssetIngestionEvent>(MMSENGINE_EVENTTYPEIDENTIFIER_LOCALASSETINGESTIONEVENT);

        localAssetIngestionEvent->setSource(ENCODERVIDEOAUDIOPROXY);
        localAssetIngestionEvent->setDestination(MMSENGINEPROCESSORNAME);
        localAssetIngestionEvent->setExpirationTimePoint(chrono::system_clock::now());

        localAssetIngestionEvent->setIngestionJobKey(_encodingItem->_ingestionJobKey);
        localAssetIngestionEvent->setIngestionSourceFileName(sourceFileName);
        localAssetIngestionEvent->setMMSSourceFileName(sourceFileName);
        localAssetIngestionEvent->setWorkspace(_encodingItem->_workspace);
        localAssetIngestionEvent->setIngestionType(MMSEngineDBFacade::IngestionType::AddContent);
        localAssetIngestionEvent->setIngestionRowToBeUpdatedAsSuccess(true);
        localAssetIngestionEvent->setForcedAvgFrameRate(to_string(outputFrameRate) + "/1");

        localAssetIngestionEvent->setMetadataContent(mediaMetaDataContent);

        shared_ptr<Event2>    event = dynamic_pointer_cast<Event2>(localAssetIngestionEvent);
        _multiEventsSet->addEvent(event);

        _logger->info(__FILEREF__ + "addEvent: EVENT_TYPE (INGESTASSETEVENT)"
            + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
            + ", ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey)
            + ", sourceFileName: " + sourceFileName
            + ", getEventKey().first: " + to_string(event->getEventKey().first)
            + ", getEventKey().second: " + to_string(event->getEventKey().second));
    }
    catch(runtime_error e)
    {
        _logger->error(__FILEREF__ + "processOverlayedImageOnVideo failed"
            + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
            + ", _encodingItem->_encodingJobKey: " + to_string(_encodingItem->_encodingJobKey)
            + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey)
            + ", _encodingItem->_encodingParameters: " + _encodingItem->_encodingParameters
            + ", stagingEncodedAssetPathName: " + stagingEncodedAssetPathName
            + ", _encodingItem->_workspace->_directoryName: " + _encodingItem->_workspace->_directoryName
            + ", e.what(): " + e.what()
        );
                
        throw e;
    }
    catch(exception e)
    {
        _logger->error(__FILEREF__ + "processOverlayedImageOnVideo failed"
            + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
            + ", _encodingItem->_encodingJobKey: " + to_string(_encodingItem->_encodingJobKey)
            + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey)
            + ", _encodingItem->_encodingParameters: " + _encodingItem->_encodingParameters
            + ", stagingEncodedAssetPathName: " + stagingEncodedAssetPathName
            + ", _encodingItem->_workspace->_directoryName: " + _encodingItem->_workspace->_directoryName
        );
                
        throw e;
    }
}

string EncoderVideoAudioProxy::faceRecognition()
{
    
	string faceRecognitionCascadeName;
	string sourcePhysicalPath;
	string faceRecognitionOutput;
	{
		string field = "faceRecognitionCascadeName";
		faceRecognitionCascadeName = _encodingItem->_parametersRoot.get(field, 0).asString();

		field = "sourcePhysicalPath";
		sourcePhysicalPath = _encodingItem->_parametersRoot.get(field, 0).asString();

		// VideoWithHighlightedFaces or ImagesToBeUsedInDeepLearnedModel
		field = "faceRecognitionOutput";
		faceRecognitionOutput = _encodingItem->_parametersRoot.get(field, 0).asString();
	}
    
	string cascadePathName = _computerVisionCascadePath + "/" + faceRecognitionCascadeName + ".xml";

	cv::CascadeClassifier cascade;
	if (!cascade.load(cascadePathName))
	{
		string errorMessage = __FILEREF__ + "CascadeName could not be loaded"
			+ ", _proxyIdentifier: " + to_string(_proxyIdentifier)
			+ ", cascadePathName: " + cascadePathName;
		_logger->error(errorMessage);

		throw runtime_error(errorMessage);
	}

	cv::VideoCapture capture(sourcePhysicalPath);
	if (!capture.isOpened())
	{
		string errorMessage = __FILEREF__ + "Capture could not be opened"
			+ ", _proxyIdentifier: " + to_string(_proxyIdentifier)
			+ ", sourcePhysicalPath: " + sourcePhysicalPath;
		_logger->error(errorMessage);

		throw runtime_error(errorMessage);
	}

	string faceRecognitionMediaPathName;
	string fileFormat;
	{
		string workspaceIngestionRepository = _mmsStorage->getWorkspaceIngestionRepository(
			_encodingItem->_workspace);
		if (faceRecognitionOutput == "FacesImagesToBeUsedInDeepLearnedModel")
		{
			fileFormat = "jpg";

			faceRecognitionMediaPathName = 
				workspaceIngestionRepository + "/"
				; // sourceFileName is added later
		}
		else // if (faceRecognitionOutput == "VideoWithHighlightedFaces")
		{
			// opencv does not have issues with avi and mov (it seems has issues with mp4)
			fileFormat = "avi";

			faceRecognitionMediaPathName = 
				workspaceIngestionRepository + "/" 
				+ to_string(_encodingItem->_ingestionJobKey)
				+ "." + fileFormat
			;
		}
	}

	_logger->info(__FILEREF__ + "faceRecognition started"
            + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
            + ", _encodingItem->_encodingJobKey: " + to_string(_encodingItem->_encodingJobKey)
            + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey)
            + ", _encodingItem->_encodingParameters: " + _encodingItem->_encodingParameters
			+ ", cascadeName: " + faceRecognitionCascadeName
			+ ", sourcePhysicalPath: " + sourcePhysicalPath
            + ", faceRecognitionMediaPathName: " + faceRecognitionMediaPathName
	);

	cv::VideoWriter writer;
	long totalFramesNumber;
	{
		totalFramesNumber = (long) capture.get(cv::CAP_PROP_FRAME_COUNT);
		double fps = capture.get(cv::CAP_PROP_FPS);
		cv::Size size(
			(int) capture.get(cv::CAP_PROP_FRAME_WIDTH),
			(int) capture.get(cv::CAP_PROP_FRAME_HEIGHT)
		);

		if (faceRecognitionOutput == "VideoWithHighlightedFaces")
		{
			writer.open(faceRecognitionMediaPathName,
				cv::VideoWriter::fourcc('X', '2', '6', '4'), fps, size);
		}
	}

	_logger->info(__FILEREF__ + "generating Face Recognition"
            + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
            + ", _encodingItem->_encodingJobKey: " + to_string(_encodingItem->_encodingJobKey)
            + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey)
            + ", _encodingItem->_encodingParameters: " + _encodingItem->_encodingParameters
			+ ", cascadeName: " + faceRecognitionCascadeName
			+ ", sourcePhysicalPath: " + sourcePhysicalPath
            + ", faceRecognitionMediaPathName: " + faceRecognitionMediaPathName
	);

	cv::Mat bgrFrame;
	cv::Mat grayFrame;
	cv::Mat smallFrame;

	// this is used only in case of faceRecognitionOutput == "FacesImagesToBeUsedInDeepLearnedModel"
	// Essentially the last image source file name will be ingested when we will go out of the
	// loop (while(true)) in order to set the IngestionRowToBeUpdatedAsSuccess flag a true for this last
	// ingestion
	string lastImageSourceFileName;

	long currentFrameIndex = 0;
	while(true)
	{
		capture >> bgrFrame;
		if (bgrFrame.empty())
			break;

		if (currentFrameIndex % 100 == 0)
		{
			_logger->info(__FILEREF__ + "generating Face Recognition"
				+ ", _proxyIdentifier: " + to_string(_proxyIdentifier)
				+ ", _encodingItem->_encodingJobKey: " + to_string(_encodingItem->_encodingJobKey)
				+ ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey)
				+ ", cascadeName: " + faceRecognitionCascadeName
				+ ", sourcePhysicalPath: " + sourcePhysicalPath
				+ ", faceRecognitionMediaPathName: " + faceRecognitionMediaPathName
				+ ", currentFrameIndex: " + to_string(currentFrameIndex)
				+ ", totalFramesNumber: " + to_string(totalFramesNumber)
			);
		}

		{
			/*
			double progress = (currentFrameIndex / totalFramesNumber) * 100;
			// this is to have one decimal in the percentage
			double faceRecognitionPercentage = ((double) ((int) (progress * 10))) / 10;
			*/
			_localEncodingProgress = 100 * currentFrameIndex / totalFramesNumber;
		}
		currentFrameIndex++;

		cv::cvtColor(bgrFrame, grayFrame, cv::COLOR_BGR2GRAY);
		double xAndYScaleFactor = 1 / _computerVisionDefaultScale;
		cv::resize(grayFrame, smallFrame, cv::Size(), xAndYScaleFactor, xAndYScaleFactor,
				cv::INTER_LINEAR_EXACT);
		cv::equalizeHist(smallFrame, smallFrame);

		vector<cv::Rect> faces;
		cascade.detectMultiScale(
			smallFrame,
			faces,
			_computerVisionDefaultScale,
			_computerVisionDefaultMinNeighbors,
			0 | cv::CASCADE_SCALE_IMAGE,
			cv::Size(30,30)
		);

		if (_computerVisionDefaultTryFlip)
		{
			// 1: flip (mirror) horizontally
			cv::flip(smallFrame, smallFrame, 1);
			vector<cv::Rect> faces2;
			cascade.detectMultiScale(
				smallFrame,
				faces2,
				_computerVisionDefaultScale,
				_computerVisionDefaultMinNeighbors,
				0 | cv::CASCADE_SCALE_IMAGE,
				cv::Size(30,30)
			);
			for (vector<cv::Rect>::const_iterator r = faces2.begin(); r != faces2.end(); ++r)
				faces.push_back(cv::Rect(
					smallFrame.cols - r->x - r->width,
					r->y,
					r->width,
					r->height
				));
		}

		for (size_t i = 0; i < faces.size(); i++)
		{
			cv::Rect roiRectScaled = faces[i];
			// cv::Mat smallROI;

			if (faceRecognitionOutput == "VideoWithHighlightedFaces")
			{
				cv::Scalar color = cv::Scalar(255,0,0);
				double aspectRatio = (double) roiRectScaled.width / roiRectScaled.height;
				int thickness = 3;
				int lineType = 8;
				int shift = 0;
				if (0.75 < aspectRatio && aspectRatio < 1.3)
				{
					cv::Point center;
					int radius;

					center.x = cvRound((roiRectScaled.x + roiRectScaled.width*0.5)*_computerVisionDefaultScale);
					center.y = cvRound((roiRectScaled.y + roiRectScaled.height*0.5)*_computerVisionDefaultScale);
					radius = cvRound((roiRectScaled.width + roiRectScaled.height)*0.25*_computerVisionDefaultScale);
					cv::circle(bgrFrame, center, radius, color, thickness, lineType, shift);
				}
				else
				{
					cv::rectangle(bgrFrame,
						cv::Point(cvRound(roiRectScaled.x*_computerVisionDefaultScale),
							cvRound(roiRectScaled.y*_computerVisionDefaultScale)),
						cv::Point(cvRound((roiRectScaled.x + roiRectScaled.width-1)*_computerVisionDefaultScale),
							cvRound((roiRectScaled.y + roiRectScaled.height-1)*_computerVisionDefaultScale)),
						color, thickness, lineType, shift);
				}
			}
			else
			{
				// Crop the full image to that image contained by the rectangle myROI
				// Note that this doesn't copy the data
				cv::Rect roiRect(
						roiRectScaled.x * _computerVisionDefaultScale,
						roiRectScaled.y * _computerVisionDefaultScale,
						roiRectScaled.width * _computerVisionDefaultScale,
						roiRectScaled.height * _computerVisionDefaultScale
				);
				cv::Mat grayFrameCropped(grayFrame, roiRect);

				/*
				cv::Mat cropped;
				// Copy the data into new matrix
				grayFrameCropped.copyTo(cropped);
				*/

				string sourceFileName = to_string(_encodingItem->_ingestionJobKey)
					+ "_"
					+ to_string(currentFrameIndex)
					+ "." + fileFormat
				;

				string faceRecognitionImagePathName = faceRecognitionMediaPathName + sourceFileName;

				cv::imwrite(faceRecognitionImagePathName, grayFrameCropped);
				// cv::imwrite(faceRecognitionImagePathName, cropped);

				if (lastImageSourceFileName == "")
					lastImageSourceFileName = sourceFileName;
				else
				{
					// ingest the face
					string mediaMetaDataContent = generateMediaMetadataToIngest(_encodingItem->_ingestionJobKey,
						fileFormat, _encodingItem->_faceRecognitionData->_faceRecognitionParametersRoot);
    
					shared_ptr<LocalAssetIngestionEvent>    localAssetIngestionEvent = _multiEventsSet->getEventsFactory()
						->getFreeEvent<LocalAssetIngestionEvent>(
								MMSENGINE_EVENTTYPEIDENTIFIER_LOCALASSETINGESTIONEVENT);

					localAssetIngestionEvent->setSource(ENCODERVIDEOAUDIOPROXY);
					localAssetIngestionEvent->setDestination(MMSENGINEPROCESSORNAME);
					localAssetIngestionEvent->setExpirationTimePoint(chrono::system_clock::now());

					localAssetIngestionEvent->setIngestionJobKey(_encodingItem->_ingestionJobKey);
					localAssetIngestionEvent->setIngestionSourceFileName(lastImageSourceFileName);
					localAssetIngestionEvent->setMMSSourceFileName(lastImageSourceFileName);
					localAssetIngestionEvent->setWorkspace(_encodingItem->_workspace);
					localAssetIngestionEvent->setIngestionType(MMSEngineDBFacade::IngestionType::AddContent);
					localAssetIngestionEvent->setIngestionRowToBeUpdatedAsSuccess(false);
					// localAssetIngestionEvent->setForcedAvgFrameRate(to_string(outputFrameRate) + "/1");

					localAssetIngestionEvent->setMetadataContent(mediaMetaDataContent);

					shared_ptr<Event2>    event = dynamic_pointer_cast<Event2>(localAssetIngestionEvent);
					_multiEventsSet->addEvent(event);

					_logger->info(__FILEREF__ + "addEvent: EVENT_TYPE (INGESTASSETEVENT)"
						+ ", _proxyIdentifier: " + to_string(_proxyIdentifier)
						+ ", ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey)
						+ ", sourceFileName: " + lastImageSourceFileName
						+ ", getEventKey().first: " + to_string(event->getEventKey().first)
						+ ", getEventKey().second: " + to_string(event->getEventKey().second));

					lastImageSourceFileName = sourceFileName;
				}
			}
		}

		if (faceRecognitionOutput == "VideoWithHighlightedFaces")
		{
			writer << bgrFrame;
		}
	}

	if (faceRecognitionOutput == "FacesImagesToBeUsedInDeepLearnedModel")
	{
		if (lastImageSourceFileName != "")
		{
			// ingest the face
			string mediaMetaDataContent = generateMediaMetadataToIngest(_encodingItem->_ingestionJobKey,
				fileFormat, _encodingItem->_faceRecognitionData->_faceRecognitionParametersRoot);
  
			shared_ptr<LocalAssetIngestionEvent>    localAssetIngestionEvent = _multiEventsSet->getEventsFactory()
				->getFreeEvent<LocalAssetIngestionEvent>(
						MMSENGINE_EVENTTYPEIDENTIFIER_LOCALASSETINGESTIONEVENT);

			localAssetIngestionEvent->setSource(ENCODERVIDEOAUDIOPROXY);
			localAssetIngestionEvent->setDestination(MMSENGINEPROCESSORNAME);
			localAssetIngestionEvent->setExpirationTimePoint(chrono::system_clock::now());

			localAssetIngestionEvent->setIngestionJobKey(_encodingItem->_ingestionJobKey);
			localAssetIngestionEvent->setIngestionSourceFileName(lastImageSourceFileName);
			localAssetIngestionEvent->setMMSSourceFileName(lastImageSourceFileName);
			localAssetIngestionEvent->setWorkspace(_encodingItem->_workspace);
			localAssetIngestionEvent->setIngestionType(MMSEngineDBFacade::IngestionType::AddContent);
			localAssetIngestionEvent->setIngestionRowToBeUpdatedAsSuccess(true);
			// localAssetIngestionEvent->setForcedAvgFrameRate(to_string(outputFrameRate) + "/1");

			localAssetIngestionEvent->setMetadataContent(mediaMetaDataContent);

			shared_ptr<Event2>    event = dynamic_pointer_cast<Event2>(localAssetIngestionEvent);
			_multiEventsSet->addEvent(event);

			_logger->info(__FILEREF__ + "addEvent: EVENT_TYPE (INGESTASSETEVENT)"
				+ ", _proxyIdentifier: " + to_string(_proxyIdentifier)
				+ ", ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey)
				+ ", sourceFileName: " + lastImageSourceFileName
				+ ", getEventKey().first: " + to_string(event->getEventKey().first)
				+ ", getEventKey().second: " + to_string(event->getEventKey().second));
		}
		else
		{
			// no faces were met, let's update ingestion status
			MMSEngineDBFacade::IngestionStatus newIngestionStatus = MMSEngineDBFacade::IngestionStatus::End_TaskSuccess;                        

			string errorMessage;
			string processorMMS;
			_logger->info(__FILEREF__ + "Update IngestionJob"                                             
				+ ", ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey)                                      
				+ ", IngestionStatus: " + MMSEngineDBFacade::toString(newIngestionStatus)
				+ ", errorMessage: " + errorMessage
				+ ", processorMMS: " + processorMMS
			);                                                                                            
			_mmsEngineDBFacade->updateIngestionJob (_encodingItem->_ingestionJobKey,
					newIngestionStatus, errorMessage, processorMMS);
		}
	}

	capture.release();

	_logger->info(__FILEREF__ + "faceRecognition media done"
            + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
            + ", _encodingItem->_encodingJobKey: " + to_string(_encodingItem->_encodingJobKey)
            + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey)
            + ", _encodingItem->_encodingParameters: " + _encodingItem->_encodingParameters
			+ ", cascadeName: " + faceRecognitionCascadeName
			+ ", sourcePhysicalPath: " + sourcePhysicalPath
            + ", faceRecognitionMediaPathName: " + faceRecognitionMediaPathName
	);


	return faceRecognitionMediaPathName;
}


void EncoderVideoAudioProxy::processFaceRecognition(string stagingEncodedAssetPathName)
{
    try
    {
		string faceRecognitionOutput;
		{
			// VideoWithHighlightedFaces or ImagesToBeUsedInDeepLearnedModel
			string field = "faceRecognitionOutput";
			faceRecognitionOutput = _encodingItem->_parametersRoot.get(field, 0).asString();
		}
    
		if (faceRecognitionOutput == "FacesImagesToBeUsedInDeepLearnedModel")
		{
			// nothing to do, all the faces (images) were already ingested

			return;
		}

		// faceRecognitionOutput is "VideoWithHighlightedFaces"

        size_t extensionIndex = stagingEncodedAssetPathName.find_last_of(".");
        if (extensionIndex == string::npos)
        {
            string errorMessage = __FILEREF__ + "No extention find in the asset file name"
                    + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                    + ", stagingEncodedAssetPathName: " + stagingEncodedAssetPathName;
            _logger->error(errorMessage);

            throw runtime_error(errorMessage);
        }
        string fileFormat = stagingEncodedAssetPathName.substr(extensionIndex + 1);

        size_t fileNameIndex = stagingEncodedAssetPathName.find_last_of("/");
        if (fileNameIndex == string::npos)
        {
            string errorMessage = __FILEREF__ + "No fileName find in the asset path name"
                    + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                    + ", stagingEncodedAssetPathName: " + stagingEncodedAssetPathName;
            _logger->error(errorMessage);

            throw runtime_error(errorMessage);
        }
        string sourceFileName = stagingEncodedAssetPathName.substr(fileNameIndex + 1);


        
        string mediaMetaDataContent = generateMediaMetadataToIngest(_encodingItem->_ingestionJobKey,
            fileFormat, _encodingItem->_faceRecognitionData->_faceRecognitionParametersRoot);
    
        shared_ptr<LocalAssetIngestionEvent>    localAssetIngestionEvent = _multiEventsSet->getEventsFactory()
                ->getFreeEvent<LocalAssetIngestionEvent>(
						MMSENGINE_EVENTTYPEIDENTIFIER_LOCALASSETINGESTIONEVENT);

        localAssetIngestionEvent->setSource(ENCODERVIDEOAUDIOPROXY);
        localAssetIngestionEvent->setDestination(MMSENGINEPROCESSORNAME);
        localAssetIngestionEvent->setExpirationTimePoint(chrono::system_clock::now());

        localAssetIngestionEvent->setIngestionJobKey(_encodingItem->_ingestionJobKey);
        localAssetIngestionEvent->setIngestionSourceFileName(sourceFileName);
        localAssetIngestionEvent->setMMSSourceFileName(sourceFileName);
        localAssetIngestionEvent->setWorkspace(_encodingItem->_workspace);
        localAssetIngestionEvent->setIngestionType(MMSEngineDBFacade::IngestionType::AddContent);
        localAssetIngestionEvent->setIngestionRowToBeUpdatedAsSuccess(true);
        // localAssetIngestionEvent->setForcedAvgFrameRate(to_string(outputFrameRate) + "/1");

        localAssetIngestionEvent->setMetadataContent(mediaMetaDataContent);

        shared_ptr<Event2>    event = dynamic_pointer_cast<Event2>(localAssetIngestionEvent);
        _multiEventsSet->addEvent(event);

        _logger->info(__FILEREF__ + "addEvent: EVENT_TYPE (INGESTASSETEVENT)"
            + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
            + ", ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey)
            + ", sourceFileName: " + sourceFileName
            + ", getEventKey().first: " + to_string(event->getEventKey().first)
            + ", getEventKey().second: " + to_string(event->getEventKey().second));
    }
    catch(runtime_error e)
    {
        _logger->error(__FILEREF__ + "processFaceRecognition failed"
            + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
            + ", _encodingItem->_encodingJobKey: " + to_string(_encodingItem->_encodingJobKey)
            + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey)
            + ", _encodingItem->_encodingParameters: " + _encodingItem->_encodingParameters
            + ", stagingEncodedAssetPathName: " + stagingEncodedAssetPathName
            + ", _encodingItem->_workspace->_directoryName: " + _encodingItem->_workspace->_directoryName
            + ", e.what(): " + e.what()
        );
                
        throw e;
    }
    catch(exception e)
    {
        _logger->error(__FILEREF__ + "processFaceRecognition failed"
            + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
            + ", _encodingItem->_encodingJobKey: " + to_string(_encodingItem->_encodingJobKey)
            + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey)
            + ", _encodingItem->_encodingParameters: " + _encodingItem->_encodingParameters
            + ", stagingEncodedAssetPathName: " + stagingEncodedAssetPathName
            + ", _encodingItem->_workspace->_directoryName: " + _encodingItem->_workspace->_directoryName
        );
                
        throw e;
    }
}


string EncoderVideoAudioProxy::faceIdentification()
{
	// build the deep learned model
	vector<cv::Mat> images;
	vector<int> idImages;
	unordered_map<int, string> idTagMap;
	{
		vector<string> deepLearnedModelTags;

		string field = "deepLearnedModelTags";
		Json::Value deepLearnedModelTagsRoot = _encodingItem->_parametersRoot[field];
		for (int deepLearnedModelTagsIndex = 0;
				deepLearnedModelTagsIndex < deepLearnedModelTagsRoot.size();
				deepLearnedModelTagsIndex++)
		{
			deepLearnedModelTags.push_back(
					deepLearnedModelTagsRoot[deepLearnedModelTagsIndex].asString());
		}

		int64_t mediaItemKey = -1;
		int64_t physicalPathKey = -1;
		bool contentTypePresent = true;
		MMSEngineDBFacade::ContentType contentType = MMSEngineDBFacade::ContentType::Image;
		bool startAndEndIngestionDatePresent = false;
		string startIngestionDate;
		string endIngestionDate;
		string title;
		string ingestionDateOrder;
		bool admin = true;

		int start = 0;
		int rows = 200;
		int totalImagesNumber = -1;
		bool imagesFinished = false;

		int idImageCounter = 0;
		unordered_map<string, int> tagIdMap;

		while(!imagesFinished)
		{
			Json::Value mediaItemsListRoot = _mmsEngineDBFacade->getMediaItemsList(
					_encodingItem->_workspace->_workspaceKey, mediaItemKey, physicalPathKey,
					start, rows, contentTypePresent, contentType,
					startAndEndIngestionDatePresent, startIngestionDate, endIngestionDate,
					title, deepLearnedModelTags, ingestionDateOrder, admin);

			field = "response";
			Json::Value responseRoot = mediaItemsListRoot[field];

			if (totalImagesNumber == -1)
			{
				field = "numFound";
				totalImagesNumber = responseRoot.get(field, 0).asInt();
			}
			
			field = "mediaItems";
			Json::Value mediaItemsArrayRoot = responseRoot[field];
			if (mediaItemsArrayRoot.size() < rows)
				imagesFinished = true;
			else
				start += rows;

			_logger->info(__FILEREF__ + "Called getMediaItemsList"
				+ ", _proxyIdentifier: " + to_string(_proxyIdentifier)
				+ ", _encodingItem->_encodingJobKey: " + to_string(_encodingItem->_encodingJobKey)
				+ ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey)
				+ ", mediaItemsArrayRoot.size(): " + to_string(mediaItemsArrayRoot.size())
			);

			for (int imageIndex = 0; imageIndex < mediaItemsArrayRoot.size(); imageIndex++)
			{
				Json::Value mediaItemRoot = mediaItemsArrayRoot[imageIndex];

				int currentIdImage;
				unordered_map<string, int>::iterator tagIdIterator;

				field = "tags";
				string tags = mediaItemRoot.get(field, 0).asString();
				if (tags.front() == ',')
					tags = tags.substr(1);
				if (tags.back() == ',')
					tags.pop_back();

				tagIdIterator = tagIdMap.find(tags);
				if (tagIdIterator == tagIdMap.end())
				{
					currentIdImage = idImageCounter++;
					tagIdMap.insert(make_pair(tags, currentIdImage));
				}
				else
					currentIdImage = (*tagIdIterator).second;

				{
					unordered_map<int, string>::iterator idTagIterator;

				   	idTagIterator = idTagMap.find(currentIdImage);
					if (idTagIterator == idTagMap.end())
						idTagMap.insert(make_pair(currentIdImage, tags));
				}

				field = "physicalPaths";
				Json::Value physicalPathsArrayRoot = mediaItemRoot[field];
				if (physicalPathsArrayRoot.size() > 0)
				{
					Json::Value physicalPathRoot = physicalPathsArrayRoot[0];

					field = "partitionNumber";
					int partitionNumber = physicalPathRoot.get(field, 0).asInt();

					field = "relativePath";
					string relativePath = physicalPathRoot.get(field, 0).asString();

					field = "fileName";
					string fileName = physicalPathRoot.get(field, 0).asString();

					string mmsImagePathName = _mmsStorage->getMMSAssetPathName(
						partitionNumber,
						_encodingItem->_workspace->_directoryName,
						relativePath,
						fileName);

					images.push_back(cv::imread(mmsImagePathName, 0));
					idImages.push_back(currentIdImage);
				}
			}
		}
	}

	_logger->info(__FILEREF__ + "Deep learned model built"
		+ ", images.size: " + to_string(images.size())
		+ ", idImages.size: " + to_string(idImages.size())
		+ ", idTagMap.size: " + to_string(idTagMap.size())
	);

	if (images.size() == 0)
	{
		string errorMessage = __FILEREF__
			+ "The Deep Learned Model is empty, no deepLearnedModelTags found"
			+ ", _proxyIdentifier: " + to_string(_proxyIdentifier)
		;
		_logger->error(errorMessage);

		throw runtime_error(errorMessage);
	}

	string faceIdentificationCascadeName;
	string sourcePhysicalPath;
	{
		string field = "faceIdentificationCascadeName";
		faceIdentificationCascadeName = _encodingItem->_parametersRoot.get(field, 0).asString();

		field = "sourcePhysicalPath";
		sourcePhysicalPath = _encodingItem->_parametersRoot.get(field, 0).asString();
	}
    
	string cascadePathName = _computerVisionCascadePath + "/"
		+ faceIdentificationCascadeName + ".xml";

	cv::CascadeClassifier cascade;
	if (!cascade.load(cascadePathName))
	{
		string errorMessage = __FILEREF__ + "CascadeName could not be loaded"
			+ ", _proxyIdentifier: " + to_string(_proxyIdentifier)
			+ ", cascadePathName: " + cascadePathName;
		_logger->error(errorMessage);

		throw runtime_error(errorMessage);
	}

	// The following lines create an LBPH model for
	// face recognition and train it with the images and
	// labels.
	//
	// The LBPHFaceRecognizer uses Extended Local Binary Patterns
	// (it's probably configurable with other operators at a later
	// point), and has the following default values
	//
	//      radius = 1
	//      neighbors = 8
	//      grid_x = 8
	//      grid_y = 8
	//
	// So if you want a LBPH FaceRecognizer using a radius of
	// 2 and 16 neighbors, call the factory method with:
	//
	//      cv::face::LBPHFaceRecognizer::create(2, 16);
	//
	// And if you want a threshold (e.g. 123.0) call it with its default values:
	//
	//      cv::face::LBPHFaceRecognizer::create(1,8,8,8,123.0)
	//
	cv::Ptr<cv::face::LBPHFaceRecognizer> recognizerModel = cv::face::LBPHFaceRecognizer::create();
	recognizerModel->train(images, idImages);

	cv::VideoCapture capture(sourcePhysicalPath);
	if (!capture.isOpened())
	{
		string errorMessage = __FILEREF__ + "Capture could not be opened"
			+ ", _proxyIdentifier: " + to_string(_proxyIdentifier)
			+ ", sourcePhysicalPath: " + sourcePhysicalPath;
		_logger->error(errorMessage);

		throw runtime_error(errorMessage);
	}

	string faceIdentificationMediaPathName;
	string fileFormat;
	{
		string workspaceIngestionRepository = _mmsStorage->getWorkspaceIngestionRepository(
			_encodingItem->_workspace);

		{
			// opencv does not have issues with avi and mov (it seems has issues with mp4)
			fileFormat = "avi";

			faceIdentificationMediaPathName = 
				workspaceIngestionRepository + "/" 
				+ to_string(_encodingItem->_ingestionJobKey)
				+ "." + fileFormat
			;
		}
	}

	_logger->info(__FILEREF__ + "faceIdentification started"
            + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
            + ", _encodingItem->_encodingJobKey: " + to_string(_encodingItem->_encodingJobKey)
            + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey)
            + ", _encodingItem->_encodingParameters: " + _encodingItem->_encodingParameters
			+ ", cascadeName: " + faceIdentificationCascadeName
			+ ", sourcePhysicalPath: " + sourcePhysicalPath
            + ", faceIdentificationMediaPathName: " + faceIdentificationMediaPathName
	);

	cv::VideoWriter writer;
	long totalFramesNumber;
	{
		totalFramesNumber = (long) capture.get(cv::CAP_PROP_FRAME_COUNT);
		double fps = capture.get(cv::CAP_PROP_FPS);
		cv::Size size(
			(int) capture.get(cv::CAP_PROP_FRAME_WIDTH),
			(int) capture.get(cv::CAP_PROP_FRAME_HEIGHT)
		);

		{
			writer.open(faceIdentificationMediaPathName,
				cv::VideoWriter::fourcc('X', '2', '6', '4'), fps, size);
		}
	}

	_logger->info(__FILEREF__ + "generating Face Identification"
            + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
            + ", _encodingItem->_encodingJobKey: " + to_string(_encodingItem->_encodingJobKey)
            + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey)
            + ", _encodingItem->_encodingParameters: " + _encodingItem->_encodingParameters
			+ ", cascadeName: " + faceIdentificationCascadeName
			+ ", sourcePhysicalPath: " + sourcePhysicalPath
            + ", faceIdentificationMediaPathName: " + faceIdentificationMediaPathName
	);

	cv::Mat bgrFrame;
	cv::Mat grayFrame;
	cv::Mat smallFrame;

	long currentFrameIndex = 0;
	while(true)
	{
		capture >> bgrFrame;
		if (bgrFrame.empty())
			break;

		if (currentFrameIndex % 100 == 0)
		{
			_logger->info(__FILEREF__ + "generating Face Recognition"
				+ ", _proxyIdentifier: " + to_string(_proxyIdentifier)
				+ ", _encodingJobKey: " + to_string(_encodingItem->_encodingJobKey)
				+ ", _ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey)
				+ ", cascadeName: " + faceIdentificationCascadeName
				+ ", sourcePhysicalPath: " + sourcePhysicalPath
				+ ", faceIdentificationMediaPathName: " + faceIdentificationMediaPathName
				+ ", currentFrameIndex: " + to_string(currentFrameIndex)
				+ ", totalFramesNumber: " + to_string(totalFramesNumber)
			);
		}

		{
			/*
			double progress = (currentFrameIndex / totalFramesNumber) * 100;
			// this is to have one decimal in the percentage
			double faceRecognitionPercentage = ((double) ((int) (progress * 10))) / 10;
			*/
			_localEncodingProgress = 100 * currentFrameIndex / totalFramesNumber;
		}
		currentFrameIndex++;

		cv::cvtColor(bgrFrame, grayFrame, cv::COLOR_BGR2GRAY);
		double xAndYScaleFactor = 1 / _computerVisionDefaultScale;
		cv::resize(grayFrame, smallFrame, cv::Size(), xAndYScaleFactor, xAndYScaleFactor,
				cv::INTER_LINEAR_EXACT);
		cv::equalizeHist(smallFrame, smallFrame);

		vector<cv::Rect> faces;
		cascade.detectMultiScale(
			smallFrame,
			faces,
			_computerVisionDefaultScale,
			_computerVisionDefaultMinNeighbors,
			0 | cv::CASCADE_SCALE_IMAGE,
			cv::Size(30,30)
		);

		if (_computerVisionDefaultTryFlip)
		{
			// 1: flip (mirror) horizontally
			cv::flip(smallFrame, smallFrame, 1);
			vector<cv::Rect> faces2;
			cascade.detectMultiScale(
				smallFrame,
				faces2,
				_computerVisionDefaultScale,
				_computerVisionDefaultMinNeighbors,
				0 | cv::CASCADE_SCALE_IMAGE,
				cv::Size(30,30)
			);
			for (vector<cv::Rect>::const_iterator r = faces2.begin(); r != faces2.end(); ++r)
				faces.push_back(cv::Rect(
					smallFrame.cols - r->x - r->width,
					r->y,
					r->width,
					r->height
				));
		}

		for (size_t i = 0; i < faces.size(); i++)
		{
			cv::Rect roiRectScaled = faces[i];

			// Crop the full image to that image contained by the rectangle myROI
			// Note that this doesn't copy the data
			cv::Rect roiRect(
					roiRectScaled.x * _computerVisionDefaultScale,
					roiRectScaled.y * _computerVisionDefaultScale,
					roiRectScaled.width * _computerVisionDefaultScale,
					roiRectScaled.height * _computerVisionDefaultScale
			);
			cv::Mat grayFrameCropped(grayFrame, roiRect);

			string predictedTags;
			{
				// int predictedLabel = recognizerModel->predict(grayFrameCropped);
				// To get the confidence of a prediction call the model with:
				int predictedIdImage = -1;
				double confidence = 0.0;
				recognizerModel->predict(grayFrameCropped, predictedIdImage, confidence);

				{
					unordered_map<int, string>::iterator idTagIterator;

					idTagIterator = idTagMap.find(predictedIdImage);
					if (idTagIterator != idTagMap.end())
						predictedTags = (*idTagIterator).second;
				}

				_logger->info(__FILEREF__ + "recognizerModel->predict"
					+ ", _proxyIdentifier: " + to_string(_proxyIdentifier)
					+ ", _encodingJobKey: " + to_string(_encodingItem->_encodingJobKey)
					+ ", _ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey)
					+ ", predictedIdImage: " + to_string(predictedIdImage)
					+ ", confidence: " + to_string(confidence)
					+ ", predictedTags: " + predictedTags
				);
			}

			{
				cv::Scalar color = cv::Scalar(255,0,0);
				double aspectRatio = (double) roiRectScaled.width / roiRectScaled.height;
				int thickness = 3;
				int lineType = 8;
				int shift = 0;
				if (0.75 < aspectRatio && aspectRatio < 1.3)
				{
					cv::Point center;
					int radius;

					center.x = cvRound((roiRectScaled.x + roiRectScaled.width*0.5)
							*_computerVisionDefaultScale);
					center.y = cvRound((roiRectScaled.y + roiRectScaled.height*0.5)
							*_computerVisionDefaultScale);
					radius = cvRound((roiRectScaled.width + roiRectScaled.height)*0.25
							*_computerVisionDefaultScale);
					cv::circle(bgrFrame, center, radius, color, thickness, lineType, shift);
				}
				else
				{
					cv::rectangle(bgrFrame,
						cv::Point(cvRound(roiRectScaled.x*_computerVisionDefaultScale),
							cvRound(roiRectScaled.y*_computerVisionDefaultScale)),
						cv::Point(cvRound((roiRectScaled.x + roiRectScaled.width-1)
								*_computerVisionDefaultScale),
							cvRound((roiRectScaled.y + roiRectScaled.height-1)
								*_computerVisionDefaultScale)),
						color, thickness, lineType, shift);
				}

				double fontScale = 2;
				cv::putText(
						bgrFrame,
					   	predictedTags,
						cv::Point(cvRound(roiRectScaled.x*_computerVisionDefaultScale),
							cvRound(roiRectScaled.y*_computerVisionDefaultScale)),
					   	cv::FONT_HERSHEY_PLAIN,
					   	fontScale,
						color,
						thickness);
			}
		}

		writer << bgrFrame;
	}

	capture.release();

	_logger->info(__FILEREF__ + "faceIdentification media done"
            + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
            + ", _encodingItem->_encodingJobKey: " + to_string(_encodingItem->_encodingJobKey)
            + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey)
            + ", _encodingItem->_encodingParameters: " + _encodingItem->_encodingParameters
			+ ", cascadeName: " + faceIdentificationCascadeName
			+ ", sourcePhysicalPath: " + sourcePhysicalPath
            + ", faceIdentificationMediaPathName: " + faceIdentificationMediaPathName
	);


	return faceIdentificationMediaPathName;
}


void EncoderVideoAudioProxy::processFaceIdentification(string stagingEncodedAssetPathName)
{
    try
    {
        size_t extensionIndex = stagingEncodedAssetPathName.find_last_of(".");
        if (extensionIndex == string::npos)
        {
            string errorMessage = __FILEREF__ + "No extention find in the asset file name"
                    + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                    + ", stagingEncodedAssetPathName: " + stagingEncodedAssetPathName;
            _logger->error(errorMessage);

            throw runtime_error(errorMessage);
        }
        string fileFormat = stagingEncodedAssetPathName.substr(extensionIndex + 1);

        size_t fileNameIndex = stagingEncodedAssetPathName.find_last_of("/");
        if (fileNameIndex == string::npos)
        {
            string errorMessage = __FILEREF__ + "No fileName find in the asset path name"
                    + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                    + ", stagingEncodedAssetPathName: " + stagingEncodedAssetPathName;
            _logger->error(errorMessage);

            throw runtime_error(errorMessage);
        }
        string sourceFileName = stagingEncodedAssetPathName.substr(fileNameIndex + 1);


        string mediaMetaDataContent = generateMediaMetadataToIngest(_encodingItem->_ingestionJobKey,
            fileFormat, _encodingItem->_faceIdentificationData->_faceIdentificationParametersRoot);
    
        shared_ptr<LocalAssetIngestionEvent>    localAssetIngestionEvent =
			_multiEventsSet->getEventsFactory() ->getFreeEvent<LocalAssetIngestionEvent>(
						MMSENGINE_EVENTTYPEIDENTIFIER_LOCALASSETINGESTIONEVENT);

        localAssetIngestionEvent->setSource(ENCODERVIDEOAUDIOPROXY);
        localAssetIngestionEvent->setDestination(MMSENGINEPROCESSORNAME);
        localAssetIngestionEvent->setExpirationTimePoint(chrono::system_clock::now());

        localAssetIngestionEvent->setIngestionJobKey(_encodingItem->_ingestionJobKey);
        localAssetIngestionEvent->setIngestionSourceFileName(sourceFileName);
        localAssetIngestionEvent->setMMSSourceFileName(sourceFileName);
        localAssetIngestionEvent->setWorkspace(_encodingItem->_workspace);
        localAssetIngestionEvent->setIngestionType(MMSEngineDBFacade::IngestionType::AddContent);
        localAssetIngestionEvent->setIngestionRowToBeUpdatedAsSuccess(true);
        // localAssetIngestionEvent->setForcedAvgFrameRate(to_string(outputFrameRate) + "/1");

        localAssetIngestionEvent->setMetadataContent(mediaMetaDataContent);

        shared_ptr<Event2>    event = dynamic_pointer_cast<Event2>(localAssetIngestionEvent);
        _multiEventsSet->addEvent(event);

        _logger->info(__FILEREF__ + "addEvent: EVENT_TYPE (INGESTASSETEVENT)"
            + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
            + ", ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey)
            + ", sourceFileName: " + sourceFileName
            + ", getEventKey().first: " + to_string(event->getEventKey().first)
            + ", getEventKey().second: " + to_string(event->getEventKey().second));
    }
    catch(runtime_error e)
    {
        _logger->error(__FILEREF__ + "processFaceIdentification failed"
            + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
            + ", _encodingItem->_encodingJobKey: " + to_string(_encodingItem->_encodingJobKey)
            + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey)
            + ", _encodingItem->_encodingParameters: " + _encodingItem->_encodingParameters
            + ", stagingEncodedAssetPathName: " + stagingEncodedAssetPathName
            + ", _workspace->_directoryName: " + _encodingItem->_workspace->_directoryName
            + ", e.what(): " + e.what()
        );
                
        throw e;
    }
    catch(exception e)
    {
        _logger->error(__FILEREF__ + "processFaceIdentification failed"
            + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
            + ", _encodingItem->_encodingJobKey: " + to_string(_encodingItem->_encodingJobKey)
            + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey)
            + ", _encodingItem->_encodingParameters: " + _encodingItem->_encodingParameters
            + ", stagingEncodedAssetPathName: " + stagingEncodedAssetPathName
            + ", _workspace->_directoryName: " + _encodingItem->_workspace->_directoryName
        );
                
        throw e;
    }
}

string EncoderVideoAudioProxy::liveRecorder()
{

	time_t utcRecordingPeriodStart;
	time_t utcRecordingPeriodEnd;
	{
        string field = "utcRecordingPeriodStart";
        utcRecordingPeriodStart = _encodingItem->_parametersRoot.get(field, 0).asInt64();

        field = "utcRecordingPeriodEnd";
        utcRecordingPeriodEnd = _encodingItem->_parametersRoot.get(field, 0).asInt64();
	}

	time_t utcNow;
	{
		chrono::system_clock::time_point now = chrono::system_clock::now();
		utcNow = chrono::system_clock::to_time_t(now);
	}

	// MMS allocates a thread just 5 minutes before the beginning of the recording
	int timeBeforeToPrepareResourceInMinutes = 5;
	if (utcNow < utcRecordingPeriodStart)
	{
	   	if (utcRecordingPeriodStart - utcNow >= timeBeforeToPrepareResourceInMinutes * 60)
		{
			_logger->info(__FILEREF__ + "Too early to allocate a thread for recording"
                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
			);

			// it is simulated a MaxConcurrentJobsReached to avoid to increase the error counter
			throw MaxConcurrentJobsReached();
		}
	}

	if (utcRecordingPeriodEnd <= utcNow)
	{
		string errorMessage = __FILEREF__ + "Too late to activate the recording"
			+ ", _proxyIdentifier: " + to_string(_proxyIdentifier)
			+ ", utcRecordingPeriodEnd: " + to_string(utcRecordingPeriodEnd)
			+ ", utcNow: " + to_string(utcNow)
			;
		_logger->error(errorMessage);

		throw runtime_error(errorMessage);
	}

	string stagingEncodedAssetPathName;
    
	stagingEncodedAssetPathName = liveRecorder_through_ffmpeg();
    
	return stagingEncodedAssetPathName;
}

string EncoderVideoAudioProxy::liveRecorder_through_ffmpeg()
{

	string liveURL;
	time_t utcRecordingPeriodStart;
	time_t utcRecordingPeriodEnd;
	int segmentDurationInSeconds;
	string outputFileFormat;
	{
        string field = "liveURL";
        liveURL = _encodingItem->_parametersRoot.get(field, "XXX").asString();

        field = "utcRecordingPeriodStart";
        utcRecordingPeriodStart = _encodingItem->_parametersRoot.get(field, 0).asInt64();

        field = "utcRecordingPeriodEnd";
        utcRecordingPeriodEnd = _encodingItem->_parametersRoot.get(field, 0).asInt64();

        field = "segmentDurationInSeconds";
        segmentDurationInSeconds = _encodingItem->_parametersRoot.get(field, 0).asInt();

        field = "outputFileFormat";
        outputFileFormat = _encodingItem->_parametersRoot.get(field, "XXX").asString();
	}

	string segmentListPathName;

    
    #ifdef __LOCALENCODER__
        if (*_pRunningEncodingsNumber > _ffmpegMaxCapacity)
        {
            _logger->info("Max ffmpeg encoder capacity is reached"
                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
            );

            throw MaxConcurrentJobsReached();
        }
    #endif

    {        
		string workspaceIngestionRepository = _mmsStorage->getWorkspaceIngestionRepository(
			_encodingItem->_workspace);
		segmentListPathName =
			workspaceIngestionRepository + "/" 
			+ to_string(_encodingItem->_ingestionJobKey)
			+ ".liveRecorder.list"
		;
    }

    #ifdef __LOCALENCODER__
        (*_pRunningEncodingsNumber)++;
    #else
		string ffmpegEncoderURL;
		ostringstream response;
		try
		{
			_currentUsedFFMpegEncoderHost = _encodersLoadBalancer->getEncoderHost(_encodingItem->_workspace);
            _logger->info(__FILEREF__ + "Configuration item"
                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                + ", _currentUsedFFMpegEncoderHost: " + _currentUsedFFMpegEncoderHost
            );
            ffmpegEncoderURL = 
                    _ffmpegEncoderProtocol
                    + "://"
                    + _currentUsedFFMpegEncoderHost + ":"
                    + to_string(_ffmpegEncoderPort)
                    + _ffmpegLiveRecorderURI
                    + "/" + to_string(_encodingItem->_encodingJobKey)
            ;
            string body;
            {
                Json::Value liveRecorderMedatada;
                
                liveRecorderMedatada["ingestionJobKey"] = (Json::LargestUInt) (_encodingItem->_ingestionJobKey);
                liveRecorderMedatada["segmentListPathName"] = segmentListPathName;
                liveRecorderMedatada["liveURL"] = liveURL;
                liveRecorderMedatada["utcRecordingPeriodStart"] = utcRecordingPeriodStart;
                liveRecorderMedatada["utcRecordingPeriodEnd"] = utcRecordingPeriodEnd;
                liveRecorderMedatada["segmentDurationInSeconds"] = segmentDurationInSeconds;
                liveRecorderMedatada["outputFileFormat"] = outputFileFormat;

                {
                    Json::StreamWriterBuilder wbuilder;
                    
                    body = Json::writeString(wbuilder, liveRecorderMedatada);
                }
            }
            
            list<string> header;

            header.push_back("Content-Type: application/json");
            {
                string userPasswordEncoded = Convert::base64_encode(_ffmpegEncoderUser + ":" + _ffmpegEncoderPassword);
                string basicAuthorization = string("Authorization: Basic ") + userPasswordEncoded;

                header.push_back(basicAuthorization);
            }
            
            curlpp::Cleanup cleaner;
            curlpp::Easy request;

            // Setting the URL to retrive.
            request.setOpt(new curlpp::options::Url(ffmpegEncoderURL));

            if (_ffmpegEncoderProtocol == "https")
            {
                /*
                    typedef curlpp::OptionTrait<std::string, CURLOPT_SSLCERTPASSWD> SslCertPasswd;                            
                    typedef curlpp::OptionTrait<std::string, CURLOPT_SSLKEY> SslKey;                                          
                    typedef curlpp::OptionTrait<std::string, CURLOPT_SSLKEYTYPE> SslKeyType;                                  
                    typedef curlpp::OptionTrait<std::string, CURLOPT_SSLKEYPASSWD> SslKeyPasswd;                              
                    typedef curlpp::OptionTrait<std::string, CURLOPT_SSLENGINE> SslEngine;                                    
                    typedef curlpp::NoValueOptionTrait<CURLOPT_SSLENGINE_DEFAULT> SslEngineDefault;                           
                    typedef curlpp::OptionTrait<long, CURLOPT_SSLVERSION> SslVersion;                                         
                    typedef curlpp::OptionTrait<std::string, CURLOPT_CAINFO> CaInfo;                                          
                    typedef curlpp::OptionTrait<std::string, CURLOPT_CAPATH> CaPath;                                          
                    typedef curlpp::OptionTrait<std::string, CURLOPT_RANDOM_FILE> RandomFile;                                 
                    typedef curlpp::OptionTrait<std::string, CURLOPT_EGDSOCKET> EgdSocket;                                    
                    typedef curlpp::OptionTrait<std::string, CURLOPT_SSL_CIPHER_LIST> SslCipherList;                          
                    typedef curlpp::OptionTrait<std::string, CURLOPT_KRB4LEVEL> Krb4Level;                                    
                 */
                                                                                                  
                
                /*
                // cert is stored PEM coded in file... 
                // since PEM is default, we needn't set it for PEM 
                // curl_easy_setopt(curl, CURLOPT_SSLCERTTYPE, "PEM");
                curlpp::OptionTrait<string, CURLOPT_SSLCERTTYPE> sslCertType("PEM");
                equest.setOpt(sslCertType);

                // set the cert for client authentication
                // "testcert.pem"
                // curl_easy_setopt(curl, CURLOPT_SSLCERT, pCertFile);
                curlpp::OptionTrait<string, CURLOPT_SSLCERT> sslCert("cert.pem");
                request.setOpt(sslCert);
                 */

                /*
                // sorry, for engine we must set the passphrase
                //   (if the key has one...)
                // const char *pPassphrase = NULL;
                if(pPassphrase)
                  curl_easy_setopt(curl, CURLOPT_KEYPASSWD, pPassphrase);

                // if we use a key stored in a crypto engine,
                //   we must set the key type to "ENG"
                // pKeyType  = "PEM";
                curl_easy_setopt(curl, CURLOPT_SSLKEYTYPE, pKeyType);

                // set the private key (file or ID in engine)
                // pKeyName  = "testkey.pem";
                curl_easy_setopt(curl, CURLOPT_SSLKEY, pKeyName);

                // set the file with the certs vaildating the server
                // *pCACertFile = "cacert.pem";
                curl_easy_setopt(curl, CURLOPT_CAINFO, pCACertFile);
                */
                
                // disconnect if we can't validate server's cert
                bool bSslVerifyPeer = false;
                curlpp::OptionTrait<bool, CURLOPT_SSL_VERIFYPEER> sslVerifyPeer(bSslVerifyPeer);
                request.setOpt(sslVerifyPeer);
                
                curlpp::OptionTrait<bool, CURLOPT_SSL_VERIFYHOST> sslVerifyHost(0L);
                request.setOpt(sslVerifyHost);
                
                // request.setOpt(new curlpp::options::SslEngineDefault());                                              

            }
            request.setOpt(new curlpp::options::HttpHeader(header));
            request.setOpt(new curlpp::options::PostFields(body));
            request.setOpt(new curlpp::options::PostFieldSize(body.length()));

            request.setOpt(new curlpp::options::WriteStream(&response));

            chrono::system_clock::time_point startEncoding = chrono::system_clock::now();

            _logger->info(__FILEREF__ + "LiveRecorder media file"
                    + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                    + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey) 
                    + ", ffmpegEncoderURL: " + ffmpegEncoderURL
                    + ", body: " + body
            );
            request.perform();

            string sResponse = response.str();
            // LF and CR create problems to the json parser...
            while (sResponse.back() == 10 || sResponse.back() == 13)
                sResponse.pop_back();

            Json::Value liveRecorderContentResponse;
            try
            {                
                Json::CharReaderBuilder builder;
                Json::CharReader* reader = builder.newCharReader();
                string errors;

                bool parsingSuccessful = reader->parse(sResponse.c_str(),
                        sResponse.c_str() + sResponse.size(), 
                        &liveRecorderContentResponse, &errors);
                delete reader;

                if (!parsingSuccessful)
                {
                    string errorMessage = __FILEREF__ + "failed to parse the response body"
                            + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                            + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey) 
                            + ", errors: " + errors
                            + ", sResponse: " + sResponse
                            ;
                    _logger->error(errorMessage);

                    throw runtime_error(errorMessage);
                }               
            }
            catch(runtime_error e)
            {
                string errorMessage = string("response Body json is not well format")
                        + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                        + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey) 
                        + ", sResponse: " + sResponse
                        + ", e.what(): " + e.what()
                        ;
                _logger->error(__FILEREF__ + errorMessage);

                throw e;
            }
            catch(...)
            {
                string errorMessage = string("response Body json is not well format")
                        + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                        + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey) 
                        + ", sResponse: " + sResponse
                        ;
                _logger->error(__FILEREF__ + errorMessage);

                throw runtime_error(errorMessage);
            }

            {
                // same string declared in FFMPEGEncoder.cpp
                string noEncodingAvailableMessage("__NO-ENCODING-AVAILABLE__");
            
                string field = "error";
                if (Validator::isMetadataPresent(liveRecorderContentResponse, field))
                {
                    string error = liveRecorderContentResponse.get(field, "XXX").asString();
                    
                    if (error.find(noEncodingAvailableMessage) != string::npos)
                    {
                        string errorMessage = string("No Encodings available")
                                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                                + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey) 
                                + ", sResponse: " + sResponse
                                ;
                        _logger->warn(__FILEREF__ + errorMessage);

                        throw MaxConcurrentJobsReached();
                    }
                    else
                    {
                        string errorMessage = string("FFMPEGEncoder error")
                                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                                + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey) 
                                + ", sResponse: " + sResponse
                                ;
                        _logger->error(__FILEREF__ + errorMessage);

                        throw runtime_error(errorMessage);
                    }                        
                }
            }
            
            // loop waiting the end of the encoding
            bool encodingFinished = false;
            int maxEncodingStatusFailures = 1;
            int encodingStatusFailures = 0;
			string lastRecordedAssetFileName;
            while(!(encodingFinished || encodingStatusFailures >= maxEncodingStatusFailures))
            {
                this_thread::sleep_for(chrono::seconds(_intervalInSecondsToCheckEncodingFinished));
                
                try
                {
                    encodingFinished = getEncodingStatus(/* _encodingItem->_encodingJobKey */);

					lastRecordedAssetFileName = processLastGeneratedLiveRecorderFiles(
						segmentListPathName, lastRecordedAssetFileName);
                }
                catch(...)
                {
                    _logger->error(__FILEREF__ + "getEncodingStatus failed");
                    
                    encodingStatusFailures++;
                }
            }
            
            chrono::system_clock::time_point endEncoding = chrono::system_clock::now();
            
            _logger->info(__FILEREF__ + "LiveRecorder media file"
                    + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                    + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey) 
                    + ", ffmpegEncoderURL: " + ffmpegEncoderURL
                    + ", body: " + body
                    + ", sResponse: " + sResponse
                    + ", encodingDuration (secs): " + to_string(chrono::duration_cast<chrono::seconds>(endEncoding - startEncoding).count())
                    + ", _intervalInSecondsToCheckEncodingFinished: " + to_string(_intervalInSecondsToCheckEncodingFinished)
            );
		}
		catch(MaxConcurrentJobsReached e)
		{
            string errorMessage = string("MaxConcurrentJobsReached")
                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey) 
                + ", response.str(): " + response.str()
                + ", e.what(): " + e.what()
                ;
            _logger->warn(__FILEREF__ + errorMessage);

            throw e;
        }
        catch (curlpp::LogicError & e) 
        {
            _logger->error(__FILEREF__ + "Encoding URL failed (LogicError)"
                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey) 
                + ", ffmpegEncoderURL: " + ffmpegEncoderURL 
                + ", exception: " + e.what()
                + ", response.str(): " + response.str()
            );
            
            throw e;
        }
        catch (curlpp::RuntimeError & e) 
        {
            _logger->error(__FILEREF__ + "Encoding URL failed (RuntimeError)"
                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey) 
                + ", ffmpegEncoderURL: " + ffmpegEncoderURL 
                + ", exception: " + e.what()
                + ", response.str(): " + response.str()
            );

            throw e;
        }
        catch (runtime_error e)
        {
            _logger->error(__FILEREF__ + "Encoding URL failed (runtime_error)"
                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey) 
                + ", ffmpegEncoderURL: " + ffmpegEncoderURL 
                + ", exception: " + e.what()
                + ", response.str(): " + response.str()
            );

            throw e;
        }
        catch (exception e)
        {
            _logger->error(__FILEREF__ + "Encoding URL failed (exception)"
                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey) 
                + ", ffmpegEncoderURL: " + ffmpegEncoderURL 
                + ", exception: " + e.what()
                + ", response.str(): " + response.str()
            );

            throw e;
        }
    #endif

    return segmentListPathName;
}

string EncoderVideoAudioProxy::processLastGeneratedLiveRecorderFiles(
	string segmentListPathName, string lastRecordedAssetFileName)
{

	string newLastRecordedAssetFileName;
    try
    {
		this_thread::sleep_for(chrono::seconds(_secondsToWaitNFSBuffers));

		_logger->info(__FILEREF__ + "processing LiveRecorder segment files"
			+ ", segmentListPathName: " + segmentListPathName);

		ifstream segmentList(segmentListPathName);
		if (!segmentList)
        {
            string errorMessage = __FILEREF__ + "No segment list file found"
                    + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                    + ", segmentListPathName: " + segmentListPathName;
            _logger->error(errorMessage);

            throw runtime_error(errorMessage);
        }

		string outputFileFormat;
		{
        	string field = "outputFileFormat";
        	outputFileFormat = _encodingItem->_parametersRoot.get(field, "XXX").asString();
		}

		bool reachedNextFileToProcess = false;
		string currentRecordedAssetFileName;
		while(getline(segmentList, currentRecordedAssetFileName))
		{
			_logger->info(__FILEREF__ + "0000 processing LiveRecorder file"
				+ ", currentRecordedAssetFileName: " + currentRecordedAssetFileName);
			if (!reachedNextFileToProcess)
			{
				if (lastRecordedAssetFileName == "")
				{
					_logger->info(__FILEREF__ + "0000 lastRecordedAssetFileName == ");

					reachedNextFileToProcess = true;
				}
				else if (currentRecordedAssetFileName == lastRecordedAssetFileName)
				{
					_logger->info(__FILEREF__ + "0000 currentRecordedAssetFileName == lastRecordedAssetFileName");
					reachedNextFileToProcess = true;

					continue;
				}
				else
				{
					_logger->info(__FILEREF__ + "0000 continue");
					continue;
				}
			}

			_logger->info(__FILEREF__ + "processing LiveRecorder file"
				+ ", currentRecordedAssetFileName: " + currentRecordedAssetFileName);

			newLastRecordedAssetFileName = currentRecordedAssetFileName;

			bool ingestionRowToBeUpdatedAsSuccess = false; // isLastLiveRecorderFile();	// check inside the directory if there are newer files

			string mediaMetaDataContent = generateMediaMetadataToIngest(_encodingItem->_ingestionJobKey,
				outputFileFormat, _encodingItem->_liveRecorderData->_liveRecorderParametersRoot);
    
			shared_ptr<LocalAssetIngestionEvent>    localAssetIngestionEvent = _multiEventsSet->getEventsFactory()
                ->getFreeEvent<LocalAssetIngestionEvent>(MMSENGINE_EVENTTYPEIDENTIFIER_LOCALASSETINGESTIONEVENT);

			localAssetIngestionEvent->setSource(ENCODERVIDEOAUDIOPROXY);
			localAssetIngestionEvent->setDestination(MMSENGINEPROCESSORNAME);
			localAssetIngestionEvent->setExpirationTimePoint(chrono::system_clock::now());

			localAssetIngestionEvent->setIngestionJobKey(_encodingItem->_ingestionJobKey);
			localAssetIngestionEvent->setIngestionSourceFileName(currentRecordedAssetFileName);
			localAssetIngestionEvent->setMMSSourceFileName(currentRecordedAssetFileName);
			localAssetIngestionEvent->setWorkspace(_encodingItem->_workspace);
			localAssetIngestionEvent->setIngestionType(MMSEngineDBFacade::IngestionType::AddContent);
			localAssetIngestionEvent->setIngestionRowToBeUpdatedAsSuccess(ingestionRowToBeUpdatedAsSuccess);
			// localAssetIngestionEvent->setForcedAvgFrameRate(to_string(outputFrameRate) + "/1");

			localAssetIngestionEvent->setMetadataContent(mediaMetaDataContent);

			shared_ptr<Event2>    event = dynamic_pointer_cast<Event2>(localAssetIngestionEvent);
			_multiEventsSet->addEvent(event);

			_logger->info(__FILEREF__ + "addEvent: EVENT_TYPE (INGESTASSETEVENT)"
				+ ", _proxyIdentifier: " + to_string(_proxyIdentifier)
				+ ", ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey)
				+ ", sourceFileName: " + currentRecordedAssetFileName
				+ ", getEventKey().first: " + to_string(event->getEventKey().first)
				+ ", getEventKey().second: " + to_string(event->getEventKey().second));
		}
    }
    catch(runtime_error e)
    {
        _logger->error(__FILEREF__ + "processLastGeneratedLiveRecorderFiles failed"
            + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
            + ", _encodingItem->_encodingJobKey: " + to_string(_encodingItem->_encodingJobKey)
            + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey)
            + ", _encodingItem->_encodingParameters: " + _encodingItem->_encodingParameters
            + ", segmentListPathName: " + segmentListPathName
            + ", _encodingItem->_workspace->_directoryName: " + _encodingItem->_workspace->_directoryName
            + ", e.what(): " + e.what()
        );
                
        throw e;
    }
    catch(exception e)
    {
        _logger->error(__FILEREF__ + "processLastGeneratedLiveRecorderFiles failed"
            + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
            + ", _encodingItem->_encodingJobKey: " + to_string(_encodingItem->_encodingJobKey)
            + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey)
            + ", _encodingItem->_encodingParameters: " + _encodingItem->_encodingParameters
            + ", segmentListPathName: " + segmentListPathName
            + ", _encodingItem->_workspace->_directoryName: " + _encodingItem->_workspace->_directoryName
        );
                
        throw e;
    }

	return newLastRecordedAssetFileName;
}

void EncoderVideoAudioProxy::processLiveRecorder(string stagingEncodedAssetPathName)
{
    try
    {
		_logger->info(__FILEREF__ + "remove"
			+ ", stagingEncodedAssetPathName: " + stagingEncodedAssetPathName
		);
		FileIO::remove(stagingEncodedAssetPathName);
    }
    catch(runtime_error e)
    {
        _logger->error(__FILEREF__ + "processLiveRecorder failed"
            + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
            + ", _encodingItem->_encodingJobKey: " + to_string(_encodingItem->_encodingJobKey)
            + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey)
            + ", _encodingItem->_encodingParameters: " + _encodingItem->_encodingParameters
            + ", stagingEncodedAssetPathName: " + stagingEncodedAssetPathName
            + ", _encodingItem->_workspace->_directoryName: " + _encodingItem->_workspace->_directoryName
            + ", e.what(): " + e.what()
        );
                
        throw e;
    }
    catch(exception e)
    {
        _logger->error(__FILEREF__ + "processLiveRecorder failed"
            + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
            + ", _encodingItem->_encodingJobKey: " + to_string(_encodingItem->_encodingJobKey)
            + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey)
            + ", _encodingItem->_encodingParameters: " + _encodingItem->_encodingParameters
            + ", stagingEncodedAssetPathName: " + stagingEncodedAssetPathName
            + ", _encodingItem->_workspace->_directoryName: " + _encodingItem->_workspace->_directoryName
        );
                
        throw e;
    }
}

int EncoderVideoAudioProxy::getEncodingProgress()
{
    int encodingProgress = 0;
    
    #ifdef __LOCALENCODER__
        try
        {
            encodingProgress = _ffmpeg->getEncodingProgress();
        }
        catch(FFMpegEncodingStatusNotAvailable e)
        {
            _logger->error(__FILEREF__ + "_ffmpeg->getEncodingProgress failed"
                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                + ", e.what(): " + e.what()
            );

            throw EncodingStatusNotAvailable();
        }
        catch(exception e)
        {
            _logger->error(__FILEREF__ + "_ffmpeg->getEncodingProgress failed"
                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                + ", e.what(): " + e.what()
            );

            throw e;
        }
    #else
		if (_encodingItem->_encodingType == MMSEngineDBFacade::EncodingType::FaceRecognition
				|| _encodingItem->_encodingType == MMSEngineDBFacade::EncodingType::FaceIdentification)
		{
			_logger->info(__FILEREF__ + "encodingProgress"
				+ ", _proxyIdentifier: " + to_string(_proxyIdentifier)
				+ ", _encodingItem->_encodingJobKey: " + to_string(_encodingItem->_encodingJobKey)
				+ ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey)
				+ ", encodingProgress: " + to_string(_localEncodingProgress)
			);

			encodingProgress = _localEncodingProgress;
		}
		else if (_encodingItem->_encodingType == MMSEngineDBFacade::EncodingType::LiveRecorder)
		{
			time_t utcRecordingPeriodStart;
			time_t utcRecordingPeriodEnd;
			{
				string field = "utcRecordingPeriodStart";
				utcRecordingPeriodStart = _encodingItem->_parametersRoot.get(field, 0).asInt64();

				field = "utcRecordingPeriodEnd";
				utcRecordingPeriodEnd = _encodingItem->_parametersRoot.get(field, 0).asInt64();
			}

			time_t utcNow;
			{
				chrono::system_clock::time_point now = chrono::system_clock::now();
				utcNow = chrono::system_clock::to_time_t(now);
			}

			if (utcNow < utcRecordingPeriodStart)
				encodingProgress = 0;
			else if (utcRecordingPeriodStart < utcNow && utcNow < utcRecordingPeriodEnd)
				encodingProgress = ((utcNow - utcRecordingPeriodStart) * 100) /
					(utcRecordingPeriodEnd - utcRecordingPeriodStart);
			else
				encodingProgress = 100;

			_logger->info(__FILEREF__ + "encodingProgress"
				+ ", _proxyIdentifier: " + to_string(_proxyIdentifier)
				+ ", _encodingItem->_encodingJobKey: " + to_string(_encodingItem->_encodingJobKey)
				+ ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey)
				+ ", encodingProgress: " + to_string(encodingProgress)
			);
		}
		else
		{
			string ffmpegEncoderURL;
			ostringstream response;
			try
			{
				if (_currentUsedFFMpegEncoderHost == "")
				{
					string errorMessage = __FILEREF__ + "no _currentUsedFFMpegEncoderHost initialized"
                        + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                        + ", _currentUsedFFMpegEncoderHost: " + _currentUsedFFMpegEncoderHost
                        ;
					_logger->error(errorMessage);

					throw runtime_error(errorMessage);
				}
            
				ffmpegEncoderURL = 
                    _ffmpegEncoderProtocol
                    + "://"
                    + _currentUsedFFMpegEncoderHost + ":"
                    + to_string(_ffmpegEncoderPort)
                    + _ffmpegEncoderProgressURI
                    + "/" + to_string(_encodingItem->_encodingJobKey)
				;
            
				list<string> header;

				{
					string userPasswordEncoded = Convert::base64_encode(_ffmpegEncoderUser + ":" + _ffmpegEncoderPassword);
					string basicAuthorization = string("Authorization: Basic ") + userPasswordEncoded;

					header.push_back(basicAuthorization);
				}
            
				curlpp::Cleanup cleaner;
				curlpp::Easy request;

				// Setting the URL to retrive.
				request.setOpt(new curlpp::options::Url(ffmpegEncoderURL));
				if (_ffmpegEncoderProtocol == "https")
				{
					/*
                    typedef curlpp::OptionTrait<std::string, CURLOPT_SSLCERTPASSWD> SslCertPasswd;                            
                    typedef curlpp::OptionTrait<std::string, CURLOPT_SSLKEY> SslKey;                                          
                    typedef curlpp::OptionTrait<std::string, CURLOPT_SSLKEYTYPE> SslKeyType;                                  
                    typedef curlpp::OptionTrait<std::string, CURLOPT_SSLKEYPASSWD> SslKeyPasswd;                              
                    typedef curlpp::OptionTrait<std::string, CURLOPT_SSLENGINE> SslEngine;                                    
                    typedef curlpp::NoValueOptionTrait<CURLOPT_SSLENGINE_DEFAULT> SslEngineDefault;                           
                    typedef curlpp::OptionTrait<long, CURLOPT_SSLVERSION> SslVersion;                                         
                    typedef curlpp::OptionTrait<std::string, CURLOPT_CAINFO> CaInfo;                                          
                    typedef curlpp::OptionTrait<std::string, CURLOPT_CAPATH> CaPath;                                          
                    typedef curlpp::OptionTrait<std::string, CURLOPT_RANDOM_FILE> RandomFile;                                 
                    typedef curlpp::OptionTrait<std::string, CURLOPT_EGDSOCKET> EgdSocket;                                    
                    typedef curlpp::OptionTrait<std::string, CURLOPT_SSL_CIPHER_LIST> SslCipherList;                          
                    typedef curlpp::OptionTrait<std::string, CURLOPT_KRB4LEVEL> Krb4Level;                                    
					*/
                                                                                                  
                
					/*
					// cert is stored PEM coded in file... 
					// since PEM is default, we needn't set it for PEM 
					// curl_easy_setopt(curl, CURLOPT_SSLCERTTYPE, "PEM");
					curlpp::OptionTrait<string, CURLOPT_SSLCERTTYPE> sslCertType("PEM");
					equest.setOpt(sslCertType);

					// set the cert for client authentication
					// "testcert.pem"
					// curl_easy_setopt(curl, CURLOPT_SSLCERT, pCertFile);
					curlpp::OptionTrait<string, CURLOPT_SSLCERT> sslCert("cert.pem");
					request.setOpt(sslCert);
					*/

					/*
					// sorry, for engine we must set the passphrase
					//   (if the key has one...)
					// const char *pPassphrase = NULL;
					if(pPassphrase)
					curl_easy_setopt(curl, CURLOPT_KEYPASSWD, pPassphrase);

					// if we use a key stored in a crypto engine,
					//   we must set the key type to "ENG"
					// pKeyType  = "PEM";
					curl_easy_setopt(curl, CURLOPT_SSLKEYTYPE, pKeyType);

					// set the private key (file or ID in engine)
					// pKeyName  = "testkey.pem";
					curl_easy_setopt(curl, CURLOPT_SSLKEY, pKeyName);

					// set the file with the certs vaildating the server
					// *pCACertFile = "cacert.pem";
					curl_easy_setopt(curl, CURLOPT_CAINFO, pCACertFile);
					*/
                
					// disconnect if we can't validate server's cert
					bool bSslVerifyPeer = false;
					curlpp::OptionTrait<bool, CURLOPT_SSL_VERIFYPEER> sslVerifyPeer(bSslVerifyPeer);
					request.setOpt(sslVerifyPeer);
                
					curlpp::OptionTrait<bool, CURLOPT_SSL_VERIFYHOST> sslVerifyHost(0L);
					request.setOpt(sslVerifyHost);
                
					// request.setOpt(new curlpp::options::SslEngineDefault());                                              

				}

				request.setOpt(new curlpp::options::HttpHeader(header));

				request.setOpt(new curlpp::options::WriteStream(&response));

				chrono::system_clock::time_point startEncoding = chrono::system_clock::now();

				_logger->info(__FILEREF__ + "getEncodingProgress"
                        + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                    + ", ffmpegEncoderURL: " + ffmpegEncoderURL
				);
				request.perform();
				chrono::system_clock::time_point endEncoding = chrono::system_clock::now();
				_logger->info(__FILEREF__ + "getEncodingProgress"
                        + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                    + ", ffmpegEncoderURL: " + ffmpegEncoderURL
                    + ", encodingDuration (secs): " + to_string(chrono::duration_cast<chrono::seconds>(endEncoding - startEncoding).count())
                    + ", response.str: " + response.str()
				);
            
				string sResponse = response.str();
				// LF and CR create problems to the json parser...
				while (sResponse.back() == 10 || sResponse.back() == 13)
					sResponse.pop_back();
            
				try
				{
					Json::Value encodeProgressResponse;
                
					Json::CharReaderBuilder builder;
					Json::CharReader* reader = builder.newCharReader();
					string errors;

					bool parsingSuccessful = reader->parse(sResponse.c_str(),
                        sResponse.c_str() + sResponse.size(), 
                        &encodeProgressResponse, &errors);
					delete reader;

					if (!parsingSuccessful)
					{
						string errorMessage = __FILEREF__ + "failed to parse the response body"
							+ ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                            + ", errors: " + errors
                            + ", sResponse: " + sResponse
                            ;
						_logger->error(errorMessage);

						throw runtime_error(errorMessage);
					}
                
					string field = "error";
					if (Validator::isMetadataPresent(encodeProgressResponse, field))
					{
						string error = encodeProgressResponse.get(field, "XXX").asString();
                    
						// same string declared in FFMPEGEncoder.cpp
						string noEncodingJobKeyFound("__NO-ENCODINGJOBKEY-FOUND__");
            
						if (error.find(noEncodingJobKeyFound) != string::npos)
						{
							string errorMessage = string("No EncodingJobKey found")
								+ ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                                + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey) 
                                + ", sResponse: " + sResponse
                                ;
							_logger->warn(__FILEREF__ + errorMessage);

							throw NoEncodingJobKeyFound();
						}
						else
						{
							string errorMessage = string("FFMPEGEncoder error")
								+ ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                                + ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey) 
                                + ", sResponse: " + sResponse
                                ;
							_logger->error(__FILEREF__ + errorMessage);

							throw runtime_error(errorMessage);
						}                        
					}
					else
					{
						string field = "encodingProgress";
						if (Validator::isMetadataPresent(encodeProgressResponse, field))
						{
							encodingProgress = encodeProgressResponse.get("encodingProgress", "XXX").asInt();
                        
							_logger->info(__FILEREF__ + "Retrieving encodingProgress"
								+ ", _proxyIdentifier: " + to_string(_proxyIdentifier)
								+ ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey) 
								+ ", encodingProgress: " + to_string(encodingProgress)
                                );                                        
						}
						else
						{
							string errorMessage = string("Unexpected FFMPEGEncoder response")
								+ ", _proxyIdentifier: " + to_string(_proxyIdentifier)
								+ ", _encodingItem->_ingestionJobKey: " + to_string(_encodingItem->_ingestionJobKey) 
                                + ", sResponse: " + sResponse
                                ;
							_logger->error(__FILEREF__ + errorMessage);

							throw runtime_error(errorMessage);
						}
					}                        
				}
				catch(NoEncodingJobKeyFound e)
				{
					string errorMessage = string("NoEncodingJobKeyFound")
                        + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                        + ", sResponse: " + sResponse
                        + ", e.what(): " + e.what()
                        ;
					_logger->warn(__FILEREF__ + errorMessage);

					throw NoEncodingJobKeyFound();
				}
				catch(runtime_error e)
				{
					string errorMessage = string("runtime_error")
                        + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                        + ", sResponse: " + sResponse
                        + ", e.what(): " + e.what()
                        ;
					_logger->error(__FILEREF__ + errorMessage);

					throw runtime_error(errorMessage);
				}
				catch(...)
				{
					string errorMessage = string("response Body json is not well format")
                        + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                        + ", sResponse: " + sResponse
                        ;
					_logger->error(__FILEREF__ + errorMessage);

					throw runtime_error(errorMessage);
				}
			}
			catch (curlpp::LogicError & e) 
			{
				_logger->error(__FILEREF__ + "Progress URL failed (LogicError)"
                        + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
					+ ", encodingJobKey: " + to_string(_encodingItem->_encodingJobKey) 
					+ ", ffmpegEncoderURL: " + ffmpegEncoderURL 
					+ ", exception: " + e.what()
					+ ", response.str(): " + response.str()
				);
            
				throw e;
			}
			catch (curlpp::RuntimeError & e) 
			{ 
				string errorMessage = string("Progress URL failed (RuntimeError)")
					+ ", _proxyIdentifier: " + to_string(_proxyIdentifier)
					+ ", encodingJobKey: " + to_string(_encodingItem->_encodingJobKey) 
					+ ", ffmpegEncoderURL: " + ffmpegEncoderURL 
					+ ", exception: " + e.what()
					+ ", response.str(): " + response.str()
				;
            
				_logger->error(__FILEREF__ + errorMessage);

				throw runtime_error(errorMessage);
			}
			catch (NoEncodingJobKeyFound e)
			{
				_logger->warn(__FILEREF__ + "Progress URL failed (NoEncodingJobKeyFound)"
					+ ", _proxyIdentifier: " + to_string(_proxyIdentifier)
					+ ", encodingJobKey: " + to_string(_encodingItem->_encodingJobKey) 
					+ ", ffmpegEncoderURL: " + ffmpegEncoderURL 
					+ ", exception: " + e.what()
					+ ", response.str(): " + response.str()
				);

				throw e;
			}
			catch (runtime_error e)
			{
				_logger->error(__FILEREF__ + "Progress URL failed (runtime_error)"
					+ ", _proxyIdentifier: " + to_string(_proxyIdentifier)
					+ ", encodingJobKey: " + to_string(_encodingItem->_encodingJobKey) 
					+ ", ffmpegEncoderURL: " + ffmpegEncoderURL 
					+ ", exception: " + e.what()
					+ ", response.str(): " + response.str()
				);

				throw e;
			}
			catch (exception e)
			{
				_logger->error(__FILEREF__ + "Progress URL failed (exception)"
					+ ", _proxyIdentifier: " + to_string(_proxyIdentifier)
					+ ", encodingJobKey: " + to_string(_encodingItem->_encodingJobKey) 
					+ ", ffmpegEncoderURL: " + ffmpegEncoderURL 
					+ ", exception: " + e.what()
					+ ", response.str(): " + response.str()
				);

				throw e;
			}
		}
    #endif

    return encodingProgress;
}

bool EncoderVideoAudioProxy::getEncodingStatus()
{
    bool encodingFinished;
    
    string ffmpegEncoderURL;
    ostringstream response;
    try
    {
        ffmpegEncoderURL = 
                _ffmpegEncoderProtocol
                + "://"                
                + _currentUsedFFMpegEncoderHost + ":"
                + to_string(_ffmpegEncoderPort)
                + _ffmpegEncoderStatusURI
                + "/" + to_string(_encodingItem->_encodingJobKey)
        ;

        list<string> header;

        {
            string userPasswordEncoded = Convert::base64_encode(_ffmpegEncoderUser + ":" + _ffmpegEncoderPassword);
            string basicAuthorization = string("Authorization: Basic ") + userPasswordEncoded;

            header.push_back(basicAuthorization);
        }

        curlpp::Cleanup cleaner;
        curlpp::Easy request;

        // Setting the URL to retrive.
        request.setOpt(new curlpp::options::Url(ffmpegEncoderURL));

        if (_ffmpegEncoderProtocol == "https")
        {
            /*
                typedef curlpp::OptionTrait<std::string, CURLOPT_SSLCERTPASSWD> SslCertPasswd;                            
                typedef curlpp::OptionTrait<std::string, CURLOPT_SSLKEY> SslKey;                                          
                typedef curlpp::OptionTrait<std::string, CURLOPT_SSLKEYTYPE> SslKeyType;                                  
                typedef curlpp::OptionTrait<std::string, CURLOPT_SSLKEYPASSWD> SslKeyPasswd;                              
                typedef curlpp::OptionTrait<std::string, CURLOPT_SSLENGINE> SslEngine;                                    
                typedef curlpp::NoValueOptionTrait<CURLOPT_SSLENGINE_DEFAULT> SslEngineDefault;                           
                typedef curlpp::OptionTrait<long, CURLOPT_SSLVERSION> SslVersion;                                         
                typedef curlpp::OptionTrait<std::string, CURLOPT_CAINFO> CaInfo;                                          
                typedef curlpp::OptionTrait<std::string, CURLOPT_CAPATH> CaPath;                                          
                typedef curlpp::OptionTrait<std::string, CURLOPT_RANDOM_FILE> RandomFile;                                 
                typedef curlpp::OptionTrait<std::string, CURLOPT_EGDSOCKET> EgdSocket;                                    
                typedef curlpp::OptionTrait<std::string, CURLOPT_SSL_CIPHER_LIST> SslCipherList;                          
                typedef curlpp::OptionTrait<std::string, CURLOPT_KRB4LEVEL> Krb4Level;                                    
             */


            /*
            // cert is stored PEM coded in file... 
            // since PEM is default, we needn't set it for PEM 
            // curl_easy_setopt(curl, CURLOPT_SSLCERTTYPE, "PEM");
            curlpp::OptionTrait<string, CURLOPT_SSLCERTTYPE> sslCertType("PEM");
            equest.setOpt(sslCertType);

            // set the cert for client authentication
            // "testcert.pem"
            // curl_easy_setopt(curl, CURLOPT_SSLCERT, pCertFile);
            curlpp::OptionTrait<string, CURLOPT_SSLCERT> sslCert("cert.pem");
            request.setOpt(sslCert);
             */

            /*
            // sorry, for engine we must set the passphrase
            //   (if the key has one...)
            // const char *pPassphrase = NULL;
            if(pPassphrase)
              curl_easy_setopt(curl, CURLOPT_KEYPASSWD, pPassphrase);

            // if we use a key stored in a crypto engine,
            //   we must set the key type to "ENG"
            // pKeyType  = "PEM";
            curl_easy_setopt(curl, CURLOPT_SSLKEYTYPE, pKeyType);

            // set the private key (file or ID in engine)
            // pKeyName  = "testkey.pem";
            curl_easy_setopt(curl, CURLOPT_SSLKEY, pKeyName);

            // set the file with the certs vaildating the server
            // *pCACertFile = "cacert.pem";
            curl_easy_setopt(curl, CURLOPT_CAINFO, pCACertFile);
            */

            // disconnect if we can't validate server's cert
            bool bSslVerifyPeer = false;
            curlpp::OptionTrait<bool, CURLOPT_SSL_VERIFYPEER> sslVerifyPeer(bSslVerifyPeer);
            request.setOpt(sslVerifyPeer);

            curlpp::OptionTrait<bool, CURLOPT_SSL_VERIFYHOST> sslVerifyHost(0L);
            request.setOpt(sslVerifyHost);

            // request.setOpt(new curlpp::options::SslEngineDefault());                                              

        }

        request.setOpt(new curlpp::options::HttpHeader(header));

        request.setOpt(new curlpp::options::WriteStream(&response));

        chrono::system_clock::time_point startEncoding = chrono::system_clock::now();

        _logger->info(__FILEREF__ + "getEncodingStatus"
                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                + ", ffmpegEncoderURL: " + ffmpegEncoderURL
        );
        request.perform();
        chrono::system_clock::time_point endEncoding = chrono::system_clock::now();
        _logger->info(__FILEREF__ + "getEncodingStatus"
                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                + ", ffmpegEncoderURL: " + ffmpegEncoderURL
                + ", encodingDuration (secs): " + to_string(chrono::duration_cast<chrono::seconds>(endEncoding - startEncoding).count())
        );

        string sResponse = response.str();
        // LF and CR create problems to the json parser...
        while (sResponse.back() == 10 || sResponse.back() == 13)
            sResponse.pop_back();
            
        try
        {
            Json::Value encodeStatusResponse;

            Json::CharReaderBuilder builder;
            Json::CharReader* reader = builder.newCharReader();
            string errors;

            bool parsingSuccessful = reader->parse(sResponse.c_str(),
                    sResponse.c_str() + sResponse.size(), 
                    &encodeStatusResponse, &errors);
            delete reader;

            if (!parsingSuccessful)
            {
                string errorMessage = __FILEREF__ + "failed to parse the response body"
                        + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                        + ", errors: " + errors
                        + ", sResponse: " + sResponse
                        ;
                _logger->error(errorMessage);

                throw runtime_error(errorMessage);
            }

            encodingFinished = encodeStatusResponse.get("encodingFinished", "XXX").asBool();
        }
        catch(...)
        {
            string errorMessage = string("response Body json is not well format")
                    + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                    + ", sResponse: " + sResponse
                    ;
            _logger->error(__FILEREF__ + errorMessage);

            throw runtime_error(errorMessage);
        }
    }
    catch (curlpp::LogicError & e) 
    {
        _logger->error(__FILEREF__ + "Progress URL failed (LogicError)"
            + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
            + ", encodingJobKey: " + to_string(_encodingItem->_encodingJobKey) 
            + ", ffmpegEncoderURL: " + ffmpegEncoderURL 
            + ", exception: " + e.what()
            + ", response.str(): " + response.str()
        );

        throw e;
    }
    catch (curlpp::RuntimeError & e) 
    { 
        string errorMessage = string("Progress URL failed (RuntimeError)")
            + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
            + ", encodingJobKey: " + to_string(_encodingItem->_encodingJobKey) 
            + ", ffmpegEncoderURL: " + ffmpegEncoderURL 
            + ", exception: " + e.what()
            + ", response.str(): " + response.str()
        ;

        _logger->error(__FILEREF__ + errorMessage);

        throw runtime_error(errorMessage);
    }
    catch (runtime_error e)
    {
        _logger->error(__FILEREF__ + "Progress URL failed (exception)"
            + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
            + ", encodingJobKey: " + to_string(_encodingItem->_encodingJobKey) 
            + ", ffmpegEncoderURL: " + ffmpegEncoderURL 
            + ", exception: " + e.what()
            + ", response.str(): " + response.str()
        );

        throw e;
    }
    catch (exception e)
    {
        _logger->error(__FILEREF__ + "Progress URL failed (exception)"
            + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
            + ", encodingJobKey: " + to_string(_encodingItem->_encodingJobKey) 
            + ", ffmpegEncoderURL: " + ffmpegEncoderURL 
            + ", exception: " + e.what()
            + ", response.str(): " + response.str()
        );

        throw e;
    }

    return encodingFinished;
}

string EncoderVideoAudioProxy::generateMediaMetadataToIngest(
        int64_t ingestionJobKey,
        string fileFormat,
        Json::Value parametersRoot
)
{
    string field = "FileFormat";
    if (_mmsEngineDBFacade->isMetadataPresent(parametersRoot, field))
    {
        string fileFormatSpecifiedByUser = parametersRoot.get(field, "XXX").asString();
        if (fileFormatSpecifiedByUser != fileFormat)
        {
            string errorMessage = string("Wrong fileFormat")
                + ", _proxyIdentifier: " + to_string(_proxyIdentifier)
                + ", ingestionJobKey: " + to_string(ingestionJobKey)
                + ", fileFormatSpecifiedByUser: " + fileFormatSpecifiedByUser
                + ", fileFormat: " + fileFormat
            ;
            _logger->error(__FILEREF__ + errorMessage);

            throw runtime_error(errorMessage);
        }
    }
    else
    {
        parametersRoot[field] = fileFormat;
    }
    
    string mediaMetadata;
    {
        Json::StreamWriterBuilder wbuilder;
        mediaMetadata = Json::writeString(wbuilder, parametersRoot);
    }
                        
    _logger->info(__FILEREF__ + "Media metadata generated"
        + ", ingestionJobKey: " + to_string(ingestionJobKey)
        + ", mediaMetadata: " + mediaMetadata
            );

    return mediaMetadata;
}

