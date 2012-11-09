/*
**
** Copyright 2008, Google Inc.
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

//#define LOG_NDEBUG 0
#define LOG_TAG "CameraHardware"
#include <utils/Log.h>
#define VALIDATE(error)\
    if (error != CAM_ERROR_NONE)\
	{\
		LOGE("error code #%d at %s(%d)\n", error, __FUNCTION__, __LINE__);\
	}

#include "CameraHardware.h"
#include <utils/threads.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <utils/Timers.h>
#include <codecDef.h>
#include <codecJP.h>
#include <misc.h>
#include <cutils/properties.h>

/* Share streaming buffers to CamereEngine */
#define CAMHW_SHARE_BUFFER

/* dump the n-th frames */
//#define DUMP_PREVIEW_FRAME 200

#ifdef CAMHW_DUMP
FILE *fpCAMHW;
#endif

#define min(a,b) ((a<b)?(a):(b))

namespace android {

CameraHardware::CameraHardware()
                  : mParameters(),
                    mPreviewHeap(0),
                    mRawHeap(0),
                    mBuffersEmpty(false),
                    mOutstandingBuffers(0),
                    mPreviewFrameSize(0),
                    mRawPictureCallback(0),
                    mJpegPictureCallback(0),
                    mPictureCallbackCookie(0),
                    mPreviewCallback(0),
                    mPreviewCallbackCookie(0),
                    mAutoFocusCallback(0),
                    mAutoFocusCallbackCookie(0),
                    mCurrentPreviewFrame(0),
                    mPreviewFrameCount(0),
                    mRecordingCallback(0),
                    mRecordingCallbackCookie(0),
                    mRecordingEnabled(false),
                    mOverlayEnabled(false),
                    pDisplayCfg(NULL),
                    mUseCase(CAM_USECASE_STILL)
{
    LOGD("%s\n", __FUNCTION__);

    /*
     * setprop camera.hardware.crop x
     * x = 0 or 1
     * If sensor and display width-height-ratio mistach, it's impossible to
     * get both full screen and full FOV preview. Thus, you can choose
     * either full screen or full FOV, by setting camera.hardware.autocrop:
     * 1: full screen, NOT full FOV (CameraEngine do cropping automatically)
     * 0: full FOV, NOT full screen (do resize/rotate in this file by yourself)
     */
    char value[PROPERTY_VALUE_MAX];
    property_get("camera.hardware.autocrop", value, "0");
    mPreviewAutoCrop = atoi(value);
    LOGD("camera.hardware.autocrop: %d", mPreviewAutoCrop);

    /*
     * setprop camera.hardware.rotate x
     * x = 0 (disable/default) or 1 (enable)
     */
    property_get("camera.hardware.rotate", value, "0");
    mPreviewRotate = atoi(value);
    LOGD("camera.hardware.rotate: %d", mPreviewRotate);

	mPreviewBufReq.pBufArray = NULL;
	mCaptureBufReq.pBufArray = NULL;
	ceInit();

    initDefaultParameters();
}

void CameraHardware::initDefaultParameters()
{
    LOGD("%s\n", __FUNCTION__);
    CameraParameters p;

    p.setPreviewSize(640, 480);
    p.setPreviewFrameRate(30);
    p.setPreviewFormat("yuv420sp");

    p.setPictureSize(640, 480/*2048, 1536*/);
    p.setPictureFormat("yuv420sp"/*"jpeg"*/);

    if (setParameters(p) != NO_ERROR) {
        LOGE("Failed to set default parameters?!");
    }
}

void CameraHardware::initHeapLocked()
{
    LOGD("%s\n", __FUNCTION__);
    int width, height;
    mParameters.getPreviewSize(&width, &height);

    LOGD("initHeapLocked: preview size=%dx%d", width, height);

    // Note that we enforce yuv preview format  in setParameters().
    int how_big = 0;

    if (strcmp(mParameters.getPreviewFormat(), "yuv422sp") == 0) {
        how_big = width * height * 2;
    }
    else if (strcmp(mParameters.getPreviewFormat(), "yuv420sp") == 0) {
		// FIXME: 2*W*H is chosen because Camera Engine's yuv420 output
		//		  has to be converted to rgb565 unless SurfaceFlinger
		//		  supports yuv
		how_big = width * height * 2;
		//how_big = width * height * 3 / 2;
    }
    else {
        LOGE("Only yuv422sp/yuv420sp preview is supported");
        return;
    }

#if 0
    // If we are being reinitialized to the same size as before, no
    // work needs to be done.
    if (how_big == mPreviewFrameSize)
        return;
#endif

    mPreviewFrameSize = how_big;

    /*
     * Make a new mmap'ed heap that can be shared across processes.
     * Two more buffers are reserved for color space conversion and
     * resize/rotate.
     */
#ifdef NO_OVERLAY2
    mPreviewHeap = new MemoryHeapBase(mPreviewFrameSize * (kBufferCount+3));
#else
    mPreviewHeap = new MemoryHeapBase(mPreviewFrameSize * (kBufferCount+2));
#endif

	/*
	 * Make an IMemory for each streaming buffer so that we can reuse
	 * them in callbacks.
	 */
    for (int i = 0; i < kBufferCount; i++) {
        mBuffers[i] = new MemoryBase(mPreviewHeap, i * mPreviewFrameSize, mPreviewFrameSize);
    }

	/*
	 * Configure color space conversion work buffer
	 */
	mCSCWorkBuf.pBuffer[0] = (Ipp8u *)(mPreviewHeap->base()) + kBufferCount * mPreviewFrameSize;
	mCSCWorkBuf.pBuffer[1] = mCSCWorkBuf.pBuffer[0] + (width * height);
	mCSCWorkBuf.pBuffer[2] = mCSCWorkBuf.pBuffer[1] + (width * height >> 2);
	mCSCWorkBuf.iStep[0] = width;
	mCSCWorkBuf.iStep[1] = width >> 1;
	mCSCWorkBuf.iStep[2] = width >> 1;
	mCSCWorkBuf.iWidth = width;
	mCSCWorkBuf.iHeight = height;

	/*
	 * Configure resize/rotate work buffer
	 */
	mRszRotWorkBuf.pBuffer[0] = (Ipp8u *)(mPreviewHeap->base())
								+ (kBufferCount + 1) * mPreviewFrameSize;
	mRszRotWorkBuf.iWidth = width;
	mRszRotWorkBuf.iHeight = height;

#ifdef NO_OVERLAY2
    mRGBWorkBuf = new MemoryBase(mPreviewHeap, (kBufferCount + 2) * mPreviewFrameSize, mPreviewFrameSize);
#endif
}

CameraHardware::~CameraHardware()
{
    LOGD("%s\n", __FUNCTION__);

	ceDeinit();

    singleton.clear();
}

sp<IMemoryHeap> CameraHardware::getPreviewHeap() const
{
    LOGD("%s\n", __FUNCTION__);
    return mPreviewHeap;
}

sp<IMemoryHeap> CameraHardware::getRawHeap() const
{
    return mRawHeap;
}

// ---------------------------------------------------------------------------
static inline void* byteOffset(void* p, size_t offset)
{
    return (void*)((uint8_t*)p + offset);
}

int CameraHardware::previewThread()
{
    // wait for all buffers being returned by encoder after 
    // recording has been stopped
    mBuffersLock.lock();
    while (!mRecordingEnabled && mOutstandingBuffers) {
        LOGV("previewThread: waiting for %d outstanding buffers...",
             mOutstandingBuffers);
        mBuffersCond.wait(mBuffersLock);
    }
    mBuffersLock.unlock();            

	mLock.lock();
        // the attributes below can change under our feet...

        int previewFrameRate = mParameters.getPreviewFrameRate();

        // Find the offset within the heap of the current buffer.
        ssize_t offset = mCurrentPreviewFrame * mPreviewFrameSize;

        sp<MemoryHeapBase> heap = mPreviewHeap;

        sp<MemoryBase> buffer = mBuffers[mCurrentPreviewFrame];

    mLock.unlock();

    // TODO: here check all the conditions that could go wrong
    if (buffer != 0) {
        mBuffersLock.lock();

        // Calculate how long to wait between frames.
        //int delay = (int)(1000000.0f / float(previewFrameRate));

        // This is always valid, even if the client died -- the memory
        // is still mapped in our process.
        void *base = heap->base();

        // Fill the current frame with the preview camera.
        uint8_t *frame = ((uint8_t *)base) + offset;
        int index = -1;
        CAM_Error error = CAM_ERROR_NONE;

        if (!mRecordingEnabled) {
            // If preview only, buffer has been used by SurfaceFlinger 
            // after mPreviewCallback returns
            mBuffersLock.unlock();            
     
            error = ceGetPreviewFrame(frame, false, &index);

            if (CAM_ERROR_NONE == error) {
                if (mCurrentPreviewFrame != index) {
                    LOGV("previewThread: streaming buffer disorder Current %d vs DQ %d",
                          mCurrentPreviewFrame, index);
                }                
                mPreviewCallback(buffer, mPreviewCallbackCookie);
                CAM_SendCommand(mCEHandle,
                                CAM_AUTOCAM_ENQUEUE_BUF,
                                0,
                                &mPreviewBufReq.pBufArray[mCurrentPreviewFrame]);
                // Advance the buffer pointer
                mCurrentPreviewFrame = (mCurrentPreviewFrame + 1) % kBufferCount;
           }
       }
       else {
            mOverlayLock.lock();
            error = ceGetPreviewFrame(frame, true, &index);
      
            if (CAM_ERROR_NONE == error) {
                //LOGV("previewThread: generated frame to buffer %d", mCurrentPreviewFrame);
                if (mCurrentPreviewFrame != index) {
                    LOGV("previewThread: streaming buffer disorder Current %d vs DQ %d",
                          mCurrentPreviewFrame, index);
                }                
                // Advance the buffer pointer.
                mCurrentPreviewFrame = (mCurrentPreviewFrame + 1) % kBufferCount;
                mOutstandingBuffers++;
                LOGV("previewThread: mOutstandingBuffers %d", mOutstandingBuffers);
                mBuffersLock.unlock();
                // unlock mBuffersLock before callback to avoid dead lock
		nsecs_t timestamp = systemTime();
		mRecordingCallback(timestamp, buffer, mRecordingCallbackCookie);
                
                mOverlayLock.unlock();
            }
            else if (CAM_ERROR_BUFFERNOTAVAILABLE == error) {                        
                mOverlayLock.unlock();
                
                // Wait for buffers to continue processing
                mBuffersEmpty = true;
                while (mBuffersEmpty) {
                    LOGV("previewThread waiting...");
                    mBuffersCond.wait(mBuffersLock);
                }
                mBuffersLock.unlock();            
            }
        }

#if 0
        CAM_Error error =  ceGetPreviewFrame(frame);

        if (CAM_ERROR_NONE == error) {
            mBuffersLock.unlock();            
            //LOGV("previewThread: generated frame to buffer %d", mCurrentPreviewFrame);

            // Notify the client of a new frame.
            if (!mRecordingEnabled) {
                // If preview only, buffer has been used by SurfaceFlinger 
                // after mPreviewCallback returns
                mPreviewCallback(buffer, mPreviewCallbackCookie);
                CAM_SendCommand(mCEHandle,
                                CAM_AUTOCAM_ENQUEUE_BUF,
                                0,
                                &mPreviewBufReq.pBufArray[mCurrentPreviewFrame]);
            }
            else {
                mRecordingCallback(buffer, mRecordingCallbackCookie);
            }

            // Advance the buffer pointer.
            mCurrentPreviewFrame = (mCurrentPreviewFrame + 1) % kBufferCount;

            mOverlayLock.unlock();
            // Wait for it...
            //usleep(delay); //no need to sleep here, because v4l2 DQ is a blocking call
            //LOGD("%f\n", (double)systemTime(SYSTEM_TIME_MONOTONIC));
        }
        else if (CAM_ERROR_BUFFERNOTAVAILABLE == error && mRecordingEnabled) {
            mOverlayLock.unlock();
            // Wait for buffers to continue processing
            mBuffersEmpty = true;
            while (mBuffersEmpty) {
                LOGV("previewThread waiting...");
                mBuffersCond.wait(mBuffersLock);
            }
            mBuffersLock.unlock();            
        }
#endif        
    }

    return NO_ERROR;
}

status_t CameraHardware::startPreview(preview_callback cb, void* user)
{
    LOGD("%s\n", __FUNCTION__);
    Mutex::Autolock lock(mLock);
    if (mPreviewThread != 0) {
        // already running
        return INVALID_OPERATION;
    }
    mPreviewCallback = cb;
    mPreviewCallbackCookie = user;

    mPreviewFrameCount = 0;
    mCurrentPreviewFrame = 0;

    ceStartPreview();

    mPreviewThread = new PreviewThread(this);
    return NO_ERROR;
}

void CameraHardware::stopPreview()
{
    LOGD("%s: %d\n", __FUNCTION__, mPreviewFrameCount);
    sp<PreviewThread> previewThread;

    { // scope for the lock
        Mutex::Autolock lock(mLock);
        previewThread = mPreviewThread;
    }

    // don't hold the lock while waiting for the thread to quit
    if (previewThread != 0) {
        previewThread->requestExitAndWait();
    }

    Mutex::Autolock lock(mLock);
    mPreviewThread.clear();
}

bool CameraHardware::previewEnabled() {
    return mPreviewThread != 0;
}

status_t CameraHardware::startRecording(recording_callback cb, void* user)
{
    mOverlayLock.lock();        
    
    mRecordingCallback = cb;
    mRecordingCallbackCookie = user;
    mRecordingEnabled = true;

#ifndef NO_OVERLAY2
    /* try to open overlay for display */
    if (mOverlayEnabled) {
        /* already has overlay opened */
        LOGD("overlay has already been opened");
        return NO_ERROR;    
    }
    
    int ret = 0;
	int preview_width, preview_height;
	mParameters.getPreviewSize(&preview_width, &preview_height);
     
    if (0 !=display_open(&pDisplayCfg)){
        LOGD("Failed to open overlay");
        return INVALID_OPERATION;
    }
    pDisplayCfg->bpp = 24;
    pDisplayCfg->format = FORMAT_PLANAR_420;
    pDisplayCfg->screen_width = get_base_fb_width();
    if (mPreviewRotate) {
        pDisplayCfg->screen_height = get_base_fb_height();
    }
    else {
        pDisplayCfg->screen_height = min(get_base_fb_height(), preview_height*get_base_fb_width()/preview_width);
    }
    pDisplayCfg->screen_pos_x = 0;
    pDisplayCfg->screen_pos_y = (get_base_fb_height() - pDisplayCfg->screen_height)/2;
    pDisplayCfg->step = pDisplayCfg->screen_width;
    pDisplayCfg->height = pDisplayCfg->screen_height;
    if (0 != (ret = display_config(pDisplayCfg))){
        display_close(&pDisplayCfg);
        LOGD("Failed to config overlay: error code %d", ret);
        return INVALID_OPERATION;
    }

    mOverlayEnabled = true;
    LOGD("Overlay has been enabled");    
#endif

    mOverlayLock.unlock();        

#ifdef CAMHW_PROF
    mProcessTime = 0;
    mThreadTime = 0;
    mRecordFrameCnt = 0;
 
    char profID[PROPERTY_VALUE_MAX];
    property_get("camera.hardware.profID", profID, "0");
    mProfilingID = atoi(profID);
#endif

#ifdef CAMHW_DUMP
    char value[PROPERTY_VALUE_MAX];
    property_get("camera.hardware.dump", value, "0");
    mDumpCnt = atoi(value);

    fpCAMHW = NULL;
    if (mDumpCnt > 0) {
        fpCAMHW = fopen("/data/dump.yuv", "wb");
    }
#endif

    return NO_ERROR;
}

void CameraHardware::stopRecording()
{
    mOverlayLock.lock();        
    mRecordingCallback = NULL;
    mRecordingCallbackCookie = NULL;
    mRecordingEnabled = false;

#ifdef CAMHW_PROF
    LOGD("ID=%d, mProcessTime=%dms, mThreadTime=%dms, mRecordFrameCnt=%d", mProfilingID, mProcessTime/1000, mThreadTime/1000, mRecordFrameCnt);
#endif

#ifndef NO_OVERLAY2
    /* close overlay if it's opened */ 
    if (mOverlayEnabled && pDisplayCfg) {
        mOverlayEnabled = false;
        display_close(&pDisplayCfg);
        pDisplayCfg = NULL;
        LOGD("Overlay has been closed");
    }
#endif

    mOverlayLock.unlock();        

#ifdef CAMHW_DUMP
    if (fpCAMHW) {
        fclose(fpCAMHW);
        fpCAMHW = NULL;
    }
#endif
}

void CameraHardware::releaseRecordingFrame(const sp<IMemory>& mem)
{
    ssize_t offset = 0;
    size_t size = 0;
    int index;
    CAM_Error error = CAM_ERROR_NONE;    

    mem->getMemory(&offset, &size);
    index = offset / mPreviewFrameSize;   

    if (index >= 0 && index < kBufferCount) {
        LOGV("releaseRecordingFrame: buffer #%d returned", index);  
        /* signal preview thread to resume processing buffers */
        mBuffersLock.lock();
        error = CAM_SendCommand(mCEHandle,
                                CAM_AUTOCAM_ENQUEUE_BUF,
                                0,
                                &mPreviewBufReq.pBufArray[index]);
        VALIDATE(error);
        mOutstandingBuffers--;
        LOGV("releaseRecordingFrame: mOutstandingBuffers %d", mOutstandingBuffers);
        if (mBuffersEmpty) {
            mBuffersEmpty = false;
            mBuffersCond.signal();
            LOGV("releaseRecordingFrame: signal empty");  
        }
        if (0 == mOutstandingBuffers) {
            mBuffersCond.signal();
            LOGV("releaseRecordingFrame: signal release");  
        }
        mBuffersLock.unlock();
    }
    else {
        LOGE("releaseRecordingFrame: invalid index %d", index);
    } 
}

// ---------------------------------------------------------------------------

int CameraHardware::beginAutoFocusThread(void *cookie)
{
    LOGD("%s\n", __FUNCTION__);
    CameraHardware *c = (CameraHardware *)cookie;
    return c->autoFocusThread();
}

int CameraHardware::autoFocusThread()
{
    if (mAutoFocusCallback != NULL) {
        mAutoFocusCallback(true, mAutoFocusCallbackCookie);
        mAutoFocusCallback = NULL;
        return NO_ERROR;
    }
    return UNKNOWN_ERROR;
}

status_t CameraHardware::autoFocus(autofocus_callback af_cb,
                                       void *user)
{
    LOGD("%s\n", __FUNCTION__);
    Mutex::Autolock lock(mLock);

    if (mAutoFocusCallback != NULL) {
        return mAutoFocusCallback == af_cb ? NO_ERROR : INVALID_OPERATION;
    }

    mAutoFocusCallback = af_cb;
    mAutoFocusCallbackCookie = user;
    if (createThread(beginAutoFocusThread, this) == false)
        return UNKNOWN_ERROR;
    return NO_ERROR;
}

/*static*/ int CameraHardware::beginPictureThread(void *cookie)
{
    LOGD("%s\n", __FUNCTION__);
    CameraHardware *c = (CameraHardware *)cookie;
    return c->pictureThread();
}

int CameraHardware::pictureThread()
{
    LOGD("enter %s\n", __FUNCTION__);

	uint8_t *snapshot, *picture;
	sp<MemoryHeapBase> snapshot_heap = new MemoryHeapBase(mPreviewFrameSize);

	int w, h;
	mParameters.getPictureSize(&w, &h);
	sp<MemoryHeapBase> pic_heap = new MemoryHeapBase(w*h*2);

	if (mShutterCallback)
        mShutterCallback(mPictureCallbackCookie);

    if (mRawPictureCallback) {
        snapshot = (uint8_t *)snapshot_heap->base();
    }
	else
		snapshot = NULL;

    if (mJpegPictureCallback) {
        picture = (uint8_t *)pic_heap->base();
    }
	else
		picture = NULL;

	int picture_size = (int)pic_heap->getSize();
	ceGetCapturePicture(snapshot, picture, &picture_size);

	if (mRawPictureCallback) {
	    sp<MemoryBase> snapshot_mem = new MemoryBase(snapshot_heap, 0, mPreviewFrameSize);
    	mRawPictureCallback(snapshot_mem, mPictureCallbackCookie);
    }

	if (mJpegPictureCallback) {
	    sp<MemoryBase> pic_mem = new MemoryBase(pic_heap, 0, picture_size);
    	mJpegPictureCallback(pic_mem, mPictureCallbackCookie);
    }

	LOGD("%s exits @ %f\n", __FUNCTION__, (double)systemTime(SYSTEM_TIME_MONOTONIC));
    return NO_ERROR;
}

status_t CameraHardware::takePicture(shutter_callback shutter_cb,
                                         raw_callback raw_cb,
                                         jpeg_callback jpeg_cb,
                                         void* user)
{
    LOGD("%s @ %f\n", __FUNCTION__, (double)systemTime(SYSTEM_TIME_MONOTONIC));
    stopPreview();
    mShutterCallback = shutter_cb;
    mRawPictureCallback = raw_cb;
    mJpegPictureCallback = jpeg_cb;
    mPictureCallbackCookie = user;

	ceStartCapture();

    if (createThread(beginPictureThread, this) == false)
        return -1;

	LOGD("%s Done!\n", __FUNCTION__);
    return NO_ERROR;
}

status_t CameraHardware::cancelPicture(bool cancel_shutter,
                                           bool cancel_raw,
                                           bool cancel_jpeg)
{
    LOGD("%s\n", __FUNCTION__);
    if (cancel_shutter) mShutterCallback = NULL;
    if (cancel_raw) mRawPictureCallback = NULL;
    if (cancel_jpeg) mJpegPictureCallback = NULL;
    return NO_ERROR;
}

status_t CameraHardware::dump(int fd, const Vector<String16>& args) const
{
    LOGD("%s\n", __FUNCTION__);
    const size_t SIZE = 256;
    char buffer[SIZE];
    String8 result;
    AutoMutex lock(&mLock);
	/* FIXME: add dump information later
	if (mFakeCamera != 0) {
        mFakeCamera->dump(fd, args);
        mParameters.dump(fd, args);
        snprintf(buffer, 255, " preview frame(%d), size (%d), running(%s)\n", mCurrentPreviewFrame, mPreviewFrameSize, mPreviewRunning?"true": "false");
        result.append(buffer);
    } else {
        result.append("No camera client yet.\n");
    }*/
    write(fd, result.string(), result.size());
    return NO_ERROR;
}

status_t CameraHardware::setParameters(const CameraParameters& params)
{
    LOGD("%s\n", __FUNCTION__);
    Mutex::Autolock lock(mLock);
    // XXX verify params

    if (previewEnabled()) {
        LOGE("setParameters: can NOT change parameters while previewing");
        return -1;
    }

    if ((strcmp(params.getPreviewFormat(), "yuv422sp") != 0) &&
        (strcmp(params.getPreviewFormat(), "yuv420sp") != 0)) {
        LOGE("Only yuv420sp/yuv422sp preview is supported");
        return -1;
    }

    if (strcmp(params.getPictureFormat(), "jpeg") != 0 &&
		(strcmp(params.getPictureFormat(), "yuv422sp") != 0) &&
        (strcmp(params.getPictureFormat(), "yuv420sp") != 0)) {
        LOGE("Only yuv420sp/yuv422sp/jpeg captures are supported");
        return -1;
    }

    mParameters = params;

    ceSetParameters();

    initHeapLocked();

    return NO_ERROR;
}

CameraParameters CameraHardware::getParameters() const
{
    LOGD("%s\n", __FUNCTION__);
    Mutex::Autolock lock(mLock);
    return mParameters;
}

void CameraHardware::release()
{
    LOGD("%s\n", __FUNCTION__);
}

CAM_Error CameraHardware::ceInit()
{
	LOGD("%s\n", __FUNCTION__);

	CAM_Error error = CAM_ERROR_NONE;

	error = CAM_GetHandle(&entry_autocam, &mCEHandle);
	VALIDATE(error);

	int iSensorID = 0;
	error = CAM_SendCommand(mCEHandle, CAM_AUTOCAM_SET_SENSORID, -1, &iSensorID);

#ifdef CAMHW_SHARE_BUFFER
    mPreviewBufReq.pBufArray = (CAM_ImageBuffer *)malloc(kBufferCount * sizeof(CAM_ImageBuffer));
#endif

    return error;

}

CAM_Error CameraHardware::ceDeinit()
{
	LOGD("%s\n", __FUNCTION__);

	CAM_Error error = CAM_ERROR_NONE;

	error = CAM_SendCommand(mCEHandle, CAM_AUTOCAM_IDLE, -1, NULL);
	VALIDATE(error);

	if (mPreviewBufReq.pBufArray){
#ifndef CAMHW_SHARE_BUFFER
		error = CAM_SendCommand(mCEHandle,
								CAM_AUTOCAM_FREE_BUF,
								0,
								&mPreviewBufReq);
		VALIDATE(error);
#else	
        if (mPreviewBufReq.pBufArray) {
            free(mPreviewBufReq.pBufArray);
            mPreviewBufReq.pBufArray = NULL;
        }

        error = CAM_SendCommand(mCEHandle,
                                CAM_AUTOCAM_UNUSE_BUF,
                                0,
                                &mPreviewBufReq);
        VALIDATE(error);
#endif
	}

	if (mCaptureBufReq.pBufArray){
		error = CAM_SendCommand(mCEHandle,
								CAM_AUTOCAM_FREE_BUF,
								1,
								&mCaptureBufReq);
		VALIDATE(error);
	}

	error = CAM_FreeHandle(&mCEHandle);
    VALIDATE(error);

    return error;
}

CAM_Error CameraHardware::ceSetParameters()
{
	LOGD("%s\n", __FUNCTION__);

	CAM_Error error = CAM_ERROR_NONE;
    CAM_ImageFormat ePreviewFormat, eCaptureFormat;

    if (strcmp(mParameters.getPreviewFormat(), "yuv422sp") == 0) {
        ePreviewFormat = CAM_IMGFMT_YCC422P;
    }
    else if (strcmp(mParameters.getPreviewFormat(), "yuv420sp") == 0) {
        ePreviewFormat = CAM_IMGFMT_YCC420P;
    }
    else {
        LOGE("Only yuv422sp/yuv420sp preview is supported");
        error = CAM_ERROR_BADARGUMENT;
        return error;
    }
    error = CAM_SendCommand(mCEHandle,
                            CAM_AUTOCAM_SET_PREVIEW_FORMAT,
                            -1,
                            &ePreviewFormat);
    VALIDATE(error);

    int burstcount = kBufferCount;
    error = CAM_SendCommand(mCEHandle,
							CAM_AUTOCAM_SET_BURSTCOUNT,
							0,
							&burstcount);
	VALIDATE(error);

	if (strcmp(mParameters.getPictureFormat(), "jpeg") == 0) {
		eCaptureFormat = CAM_IMGFMT_JPEG411;
	}
	else if(strcmp(mParameters.getPreviewFormat(), "yuv422sp") == 0) {
        eCaptureFormat = CAM_IMGFMT_YCC422P;
    }
    else if (strcmp(mParameters.getPreviewFormat(), "yuv420sp") == 0) {
        eCaptureFormat = CAM_IMGFMT_YCC420P;
    }
	else {
		LOGE("Only yuv420sp/yuv422sp/jpeg captures are supported");
        error = CAM_ERROR_BADARGUMENT;
        return error;
	}

	error = CAM_SendCommand(mCEHandle,
                            CAM_AUTOCAM_SET_CAPTURE_FORMAT,
                            -1,
                            &eCaptureFormat);
    VALIDATE(error);

    return error;
}

CAM_Error CameraHardware::ceStartPreview()
{
	LOGD("%s\n", __FUNCTION__);

	CAM_Error error = CAM_ERROR_NONE;
    int i = 0;

    /* Set preview parameters */
	CAM_Size szPreviewSize;
	mParameters.getPreviewSize(&szPreviewSize.iWidth,
                              &szPreviewSize.iHeight);

    error = CAM_SendCommand(mCEHandle,
                            CAM_AUTOCAM_SET_PREVIEW_SIZE,
                            -1,
                            &szPreviewSize);
    VALIDATE(error);
   
    int iPreviewResIndex;
 	if (szPreviewSize.iWidth <= 320) {
		iPreviewResIndex = 0; /* QVGA */
	}
	else {
		iPreviewResIndex = 1; /* VGA */
	}
#ifdef LITTLETON
    iPreviewResIndex = 0;
#endif
    error = CAM_SendCommand(mCEHandle,
                            CAM_ATUOCAM_SET_PREVIEW_RESINDEX,
                            -1,
                            &iPreviewResIndex);
    VALIDATE(error);

    CAM_FlipRotate eFlipRotate;
    CAM_Bool bAutoCrop;
    if (mPreviewAutoCrop) {
	    eFlipRotate = CAM_FLIPROTATE_NORMAL;
	    bAutoCrop = CAM_TRUE;
    }
    else {
	    eFlipRotate = CAM_FLIPROTATE_NORMAL;
	    bAutoCrop = CAM_FALSE;
    }
	error = CAM_SendCommand(mCEHandle,
							CAM_AUTOCAM_SET_PREVIEW_FLIPROTATE,
							-1,
							&eFlipRotate);
	VALIDATE(error);

	error = CAM_SendCommand(mCEHandle,
							CAM_AUTOCAM_SET_PREVIEW_AUTOCROP,
							-1,
							&bAutoCrop);
	VALIDATE(error);

	/* Buffer negotiation on preview port */
#ifndef CAMHW_SHARE_BUFFER
    if (mPreviewBufReq.pBufArray){
        error = CAM_SendCommand(mCEHandle,
                                CAM_AUTOCAM_FREE_BUF,
                                0,
                                &mPreviewBufReq);
        VALIDATE(error);
    }

    error = CAM_SendCommand(mCEHandle,
                            CAM_AUTOCAM_GET_BUFREQ,
                            0,
                            &mPreviewBufReq);
    VALIDATE(error);

    if (mPreviewBufReq.iCount != kBufferCount) {    
        LOGE("kBufferCount doesn't match with buffer requirement");
    }

    error = CAM_SendCommand(mCEHandle,
                            CAM_AUTOCAM_ALLOC_BUF,
                            0,
                            &mPreviewBufReq);
    VALIDATE(error);
#else
    if (mPreviewBufReq.pBufArray) {
        error = CAM_SendCommand(mCEHandle,
                                CAM_AUTOCAM_UNUSE_BUF,
                                0,
                                &mPreviewBufReq);
        VALIDATE(error);
    }

    error = CAM_SendCommand(mCEHandle,
                            CAM_AUTOCAM_GET_BUFREQ,
                            0,
                            &mPreviewBufReq);
    VALIDATE(error);

    if (mPreviewBufReq.iCount != kBufferCount) {    
        LOGE("kBufferCount doesn't match with buffer requirement");
    }

    ssize_t offset = 0;
    size_t size = 0;

    for (int i = 0; i < mPreviewBufReq.iCount; i++) {
        mBuffers[i]->getMemory(&offset, &size);
        mPreviewBufReq.pBufArray[i].pBuffer[0] = (uint8_t *)mPreviewHeap->base() + offset;
        mPreviewBufReq.pBufArray[i].pBuffer[1] = mPreviewBufReq.pBufArray[i].pBuffer[0] + mPreviewBufReq.iBufLen[0];
        mPreviewBufReq.pBufArray[i].pBuffer[2] = mPreviewBufReq.pBufArray[i].pBuffer[1] + mPreviewBufReq.iBufLen[1];
        mPreviewBufReq.pBufArray[i].eFormat = mPreviewBufReq.eFormat;
        mPreviewBufReq.pBufArray[i].iWidth = mPreviewBufReq.iWidth;
        mPreviewBufReq.pBufArray[i].iHeight = mPreviewBufReq.iHeight;
        mPreviewBufReq.pBufArray[i].iAllocLen[0] = mPreviewBufReq.iBufLen[0];
        mPreviewBufReq.pBufArray[i].iAllocLen[1] = mPreviewBufReq.iBufLen[1];
        mPreviewBufReq.pBufArray[i].iAllocLen[2] = mPreviewBufReq.iBufLen[2];
        mPreviewBufReq.pBufArray[i].iStep[0] = mPreviewBufReq.iStep[0];
        mPreviewBufReq.pBufArray[i].iStep[1] = mPreviewBufReq.iStep[1];
        mPreviewBufReq.pBufArray[i].iStep[2] = mPreviewBufReq.iStep[2];
        LOGV("preview buffer #%d: format %d, %p:%p:%p, %dx%d, %d:%d:%d", 
              i, 
              mPreviewBufReq.pBufArray[i].eFormat,
              mPreviewBufReq.pBufArray[i].pBuffer[0],
              mPreviewBufReq.pBufArray[i].pBuffer[1],
              mPreviewBufReq.pBufArray[i].pBuffer[2],
              mPreviewBufReq.pBufArray[i].iWidth,
              mPreviewBufReq.pBufArray[i].iHeight,
              mPreviewBufReq.pBufArray[i].iStep[0],
              mPreviewBufReq.pBufArray[i].iStep[1],
              mPreviewBufReq.pBufArray[i].iStep[2]);
    }

    error = CAM_SendCommand(mCEHandle,
                            CAM_AUTOCAM_USE_BUF,
                            0,
                            &mPreviewBufReq);
    VALIDATE(error);
#endif

    /* Start preview */
    error = CAM_SendCommand(mCEHandle,
                            CAM_AUTOCAM_PREVIEW,
                            -1,
                            NULL);
    VALIDATE(error);

    for (i = 0; i < mPreviewBufReq.iCount; i++){
        error = CAM_SendCommand(mCEHandle,
                                CAM_AUTOCAM_ENQUEUE_BUF,
                                0,
                                &mPreviewBufReq.pBufArray[i]);
        VALIDATE(error);
    }

    return error;
}

CAM_Error CameraHardware::ceStopPreview()
{
	LOGD("%s\n", __FUNCTION__);

	CAM_Error error = CAM_ERROR_NONE;

	return error;
}

CAM_Error CameraHardware::ceStartCapture()
{
	LOGD("%s\n", __FUNCTION__);

	CAM_Error error = CAM_ERROR_NONE;

	/* possible to change preview size here?
	CAM_Size szSnapshotSize;
	mParameters.getPreviewSize(&szSnapshotSize.iWidth,
                              &szSnapshotSize.iHeight);
    error = CAM_SendCommand(mCEHandle,
                            CAM_AUTOCAM_SET_PREVIEW_SIZE,
                            -1,
                            &szSnapshotSize);
    VALIDATE(error);
    */

	CAM_FlipRotate eFlipRotate = CAM_FLIPROTATE_HFLIP;
	error = CAM_SendCommand(mCEHandle,
							CAM_AUTOCAM_SET_PREVIEW_FLIPROTATE,
							-1,
							&eFlipRotate);
	VALIDATE(error);

	/* Set capture parameters */
	int iCaptureWidth, iCaptureHeight;
	int iCaptureResIndex;
	mParameters.getPictureSize(&iCaptureWidth, &iCaptureHeight);
	/* we don't support JPEG resize, so fit to sensor size*/
	if (iCaptureWidth <= 640) {
		iCaptureResIndex = 1; /* VGA */
	}
	else {
		iCaptureResIndex = 3; /* QXGA */
	}
	error = CAM_SendCommand(mCEHandle,
							CAM_AUTOCAM_SET_CAPTURE_RESINDEX,
							-1,
							&iCaptureResIndex);


	/* Buffer negotiation on CAPTURE port */
	if (mCaptureBufReq.pBufArray){
		error = CAM_SendCommand(mCEHandle,
								CAM_AUTOCAM_FREE_BUF,
								1,
								&mCaptureBufReq);
		VALIDATE(error);
	}


	error = CAM_SendCommand(mCEHandle,
							CAM_AUTOCAM_GET_BUFREQ,
							1,
							&mCaptureBufReq);
	VALIDATE(error);


    error = CAM_SendCommand(mCEHandle,
							CAM_AUTOCAM_ALLOC_BUF,
							1,
							&mCaptureBufReq);
	VALIDATE(error);


	/* Start capture */
	error = CAM_SendCommand(mCEHandle, CAM_AUTOCAM_CAPTURE, -1, NULL);
	VALIDATE(error);

	LOGD("Start still capturing...\n");

	return error;
}

CAM_Error CameraHardware::ceGetPreviewFrame(uint8_t *frame, bool bRecordOn, int *index)
{
	//LOGD("%s\n", __FUNCTION__);

	CAM_Error error = CAM_ERROR_NONE;
    CAM_ImageBuffer *pImgBuf, *pYUV420Buf;
    int port = 0;

#ifdef CAMHW_PROF
    struct timespec processtime_start, threadtime_start;
    struct timespec processtime_end, threadtime_end;

    if (2 == mProfilingID) {
        clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &processtime_start);
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &threadtime_start);
    }
#endif

    error = CAM_SendCommand(mCEHandle,
                            CAM_AUTOCAM_DEQUEUE_BUF,
                            (int)&port,
                            &pImgBuf);
#ifdef CAMHW_PROF
    if (2 == mProfilingID) {
        clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &processtime_end);
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &threadtime_end);
        mProcessTime += ((processtime_end.tv_sec-processtime_start.tv_sec)*1000000+(processtime_end.tv_nsec-processtime_start.tv_nsec)/1000);
        mThreadTime += ((threadtime_end.tv_sec-threadtime_start.tv_sec)*1000000+(threadtime_end.tv_nsec-threadtime_start.tv_nsec)/1000);
        mRecordFrameCnt++;
    }
#endif
    if (CAM_ERROR_NONE != error) {
        return error;
    }
    *index = pImgBuf->iIndex;
    mPreviewFrameCount++;

#ifdef DUMP_PREVIEW_FRAME
	static int dump_422_cnt = 0;
	if (DUMP_PREVIEW_FRAME == dump_422_cnt++) {
		FILE *fp = fopen("/data/422.yuv", "wb");
		if (fp) {
			fwrite(pImgBuf->pBuffer[0], 1, pImgBuf->iWidth * pImgBuf->iHeight, fp);
			fwrite(pImgBuf->pBuffer[1], 1, pImgBuf->iWidth * pImgBuf->iHeight >> 1, fp);
			fwrite(pImgBuf->pBuffer[2], 1, pImgBuf->iWidth * pImgBuf->iHeight >> 1, fp);
		}
		fclose(fp);
	}
#endif

	if (strcmp(mParameters.getPreviewFormat(), "yuv422sp") == 0) {
		IppiSize roiSize = {mCSCWorkBuf.iWidth, mCSCWorkBuf.iHeight};

		pYUV420Buf = &mCSCWorkBuf;

		ippiYCbCr422ToYCbCr420Rotate_8u_P3R(
                                            (const Ipp8u **)pImgBuf->pBuffer,
                                            pImgBuf->iStep,
                                            pYUV420Buf->pBuffer,
                                            pYUV420Buf->iStep,
                                            roiSize,
                                            ippCameraRotateDisable);
	}
	else {
		pYUV420Buf = pImgBuf;
	}

#ifdef DUMP_PREVIEW_FRAME
	static int dump_420_cnt = 0;
	if (DUMP_PREVIEW_FRAME == dump_420_cnt++) {
		FILE *fp = fopen("/data/420.yuv", "wb");
		if (fp) {
			fwrite(pYUV420Buf->pBuffer[0], 1, pYUV420Buf->iWidth * pYUV420Buf->iHeight, fp);
			fwrite(pYUV420Buf->pBuffer[1], 1, pYUV420Buf->iWidth * pYUV420Buf->iHeight >> 2, fp);
			fwrite(pYUV420Buf->pBuffer[2], 1, pYUV420Buf->iWidth * pYUV420Buf->iHeight >> 2, fp);
		}
		fclose(fp);
	}
#endif

#ifdef CAMHW_DUMP
    if (fpCAMHW) {
        if (mDumpCnt-- > 0) {
            fwrite(pYUV420Buf->pBuffer[0], 1, pYUV420Buf->iWidth * pYUV420Buf->iHeight, fpCAMHW);
            fwrite(pYUV420Buf->pBuffer[1], 1, pYUV420Buf->iWidth * pYUV420Buf->iHeight >> 2, fpCAMHW);
            fwrite(pYUV420Buf->pBuffer[2], 1, pYUV420Buf->iWidth * pYUV420Buf->iHeight >> 2, fpCAMHW);
        }
    }
#endif

    if (bRecordOn) {       
#ifdef CAMHW_PROF
        if (0 == mProfilingID) {
            clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &processtime_start);
            clock_gettime(CLOCK_THREAD_CPUTIME_ID, &threadtime_start);
        }
#endif

#ifdef NO_OVERLAY2
        /* convert to RGB and post to SurfaceFlinger */     
        Ipp16u *rgb_frame = (Ipp16u *)mRGBWorkBuf->pointer();
#ifndef CAMHW_SHARE_BUFFER
        if (mPreviewAutoCrop) {
            ceYUV420ToRGB565(rgb_frame, pYUV420Buf);
        }
        else {
	        ceYUV420ToRGB565RszRot(rgb_frame, pYUV420Buf);
        }
#else
        if (strcmp(mParameters.getPreviewFormat(), "yuv422sp") == 0) {
            if (mPreviewAutoCrop) {
                ceYUV420ToRGB565(rgb_frame, pYUV420Buf);
            }
            else {
                memset(rgb_frame, 0, mPreviewFrameSize);
                ceYUV420ToRGB565RszRot(rgb_frame, pYUV420Buf);
            }
        }
        else {
            if (mPreviewAutoCrop) {
                ceYUV420ToRGB565((Ipp16u *)mCSCWorkBuf.pBuffer[0], pYUV420Buf);
            }
            else {
	            ceYUV420ToRGB565RszRot((Ipp16u *)mCSCWorkBuf.pBuffer[0], pYUV420Buf);
            }
            memcpy(rgb_frame, mCSCWorkBuf.pBuffer[0], mPreviewFrameSize);
        }
#endif // CAMHW_SHARE_BUFFER
        mPreviewCallback(mRGBWorkBuf, mPreviewCallbackCookie);
#else
        /* post to overlay directly */
        IppiSize srcSize, dstSize;
        int rcpRatiox, rcpRatioy;
        Ipp8u *pOverlayBuf[3];
        int dstStep[3];
 
        srcSize.width = pYUV420Buf->iWidth;
        srcSize.height = pYUV420Buf->iHeight;
        
        /* dstSize is the size, in pixels, after resize but before rotate */
        IppCameraRotation eRotation;
        if (mPreviewRotate) {
            eRotation = ippCameraRotate90R;
            dstSize.width = pDisplayCfg->screen_height;
            dstSize.height = pDisplayCfg->screen_width;
        }
        else {
            eRotation = ippCameraRotateDisable;
            dstSize.width = pDisplayCfg->screen_width;
            dstSize.height = pDisplayCfg->screen_height;
        }

        rcpRatiox = (((srcSize.width&~1)-1) << 16)/((dstSize.width&~1)-1); 
        rcpRatioy = (((srcSize.height&~1)-1) <<16)/((dstSize.height&~1)-1); 

        get_overlay_yuv(pDisplayCfg, &pOverlayBuf[0], &pOverlayBuf[1], &pOverlayBuf[2]);
        dstStep[0] = dstSize.width;
        dstStep[1] = dstStep[2] = dstSize.width/2;

       if ( ippiYCbCr420RszRot_8u_P3R((const Ipp8u**)pYUV420Buf->pBuffer, 
                                  pYUV420Buf->iStep,
                                  srcSize,
                                  pOverlayBuf,
                                  dstStep,
                                  dstSize,
                                  ippCameraInterpBilinear,
                                  eRotation,
                                  rcpRatiox,
                                  rcpRatioy)) {
            LOGE("ippiYCbCr420RszRot_8u_P3R failed %s(%d)", __FUNCTION__, __LINE__);
        }
#endif //NO_OVERLAY2

#ifdef CAMHW_PROF
        if (0 == mProfilingID) {
            clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &processtime_end);
            clock_gettime(CLOCK_THREAD_CPUTIME_ID, &threadtime_end);
            mProcessTime += ((processtime_end.tv_sec-processtime_start.tv_sec)*1000000+(processtime_end.tv_nsec-processtime_start.tv_nsec)/1000);
            mThreadTime += ((threadtime_end.tv_sec-threadtime_start.tv_sec)*1000000+(threadtime_end.tv_nsec-threadtime_start.tv_nsec)/1000);
            mRecordFrameCnt++;
        }
#endif
      
#ifdef CAMHW_PROF
        if (1 == mProfilingID) {
            clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &processtime_start);
            clock_gettime(CLOCK_THREAD_CPUTIME_ID, &threadtime_start);
        }
#endif

#ifndef CAMHW_SHARE_BUFFER
        /* copy out YUV420 frame for encoding */
        uint8_t *pDst = frame;
        int length = pYUV420Buf->iWidth * pYUV420Buf->iHeight;
        memcpy(pDst, pYUV420Buf->pBuffer[0], length);
        pDst += length;
        memcpy(pDst, pYUV420Buf->pBuffer[1], length>>2);
        pDst += (length>>2);
        memcpy(pDst, pYUV420Buf->pBuffer[2], length>>2);
#else 
        /*
         * Else, the streaming buffer is shared with Camera Engine.
         * TODO: if your encoder doesn't agree with Camera Engine's
         * output format, add color space coversion here.  
         */
#endif

#ifdef CAMHW_PROF
        if (1 == mProfilingID) {
            clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &processtime_end);
            clock_gettime(CLOCK_THREAD_CPUTIME_ID, &threadtime_end);
            mProcessTime += ((processtime_end.tv_sec-processtime_start.tv_sec)*1000000+(processtime_end.tv_nsec-processtime_start.tv_nsec)/1000);
            mThreadTime += ((threadtime_end.tv_sec-threadtime_start.tv_sec)*1000000+(threadtime_end.tv_nsec-threadtime_start.tv_nsec)/1000);
            mRecordFrameCnt++;
        }
#endif
    }
    else {
        /* convert to RGB for SurfaceFlinger */
#ifndef CAMHW_SHARE_BUFFER
        if (mPreviewAutoCrop) {
            ceYUV420ToRGB565((Ipp16u *)frame, pYUV420Buf);
        }
        else {
	        ceYUV420ToRGB565RszRot((Ipp16u *)frame, pYUV420Buf);
        }
#else
        if (strcmp(mParameters.getPreviewFormat(), "yuv422sp") == 0) {
            if (mPreviewAutoCrop) {
                ceYUV420ToRGB565((Ipp16u *)frame, pYUV420Buf);
            }
            else {
                memset(frame, 0, mPreviewFrameSize);
                ceYUV420ToRGB565RszRot((Ipp16u *)frame, pYUV420Buf);
            }
        }
        else {
            if (mPreviewAutoCrop) {
                ceYUV420ToRGB565((Ipp16u *)mCSCWorkBuf.pBuffer[0], pYUV420Buf);
            }
            else {
	            ceYUV420ToRGB565RszRot((Ipp16u *)mCSCWorkBuf.pBuffer[0], pYUV420Buf);
            }
            memcpy(frame, mCSCWorkBuf.pBuffer[0], mPreviewFrameSize);
        }
#endif // CAMHW_SHARE_BUFFER
    }

    return error;
}

CAM_Error CameraHardware::ceGetCapturePicture(uint8_t *snapshot,
										      uint8_t *picture,
										      int *pPicSize)
{
	LOGD("%s\n", __FUNCTION__);

	CAM_Error error = CAM_ERROR_NONE;
	int port, filledLen;
	CAM_ImageBuffer *pImgBuf, *pYUV420Buf;

	error = CAM_SendCommand(mCEHandle,
							CAM_AUTOCAM_ENQUEUE_BUF,
							0,
							&mPreviewBufReq.pBufArray[0]);
	VALIDATE(error);


	error = CAM_SendCommand(mCEHandle,
							CAM_AUTOCAM_ENQUEUE_BUF,
							1,
							&mCaptureBufReq.pBufArray[0]);
	VALIDATE(error);


	// get snapshot frame
	port = -1;
    error = CAM_SendCommand(mCEHandle, CAM_AUTOCAM_DEQUEUE_BUF, (int)&port, &pImgBuf);
	VALIDATE(error);

	if (strcmp(mParameters.getPreviewFormat(), "yuv422sp") == 0) {
		IppiSize roiSize = {mCSCWorkBuf.iWidth, mCSCWorkBuf.iHeight};

		pYUV420Buf = &mCSCWorkBuf;

		ippiYCbCr422ToYCbCr420Rotate_8u_P3R(
                                            (const Ipp8u **)pImgBuf->pBuffer,
                                            pImgBuf->iStep,
                                            pYUV420Buf->pBuffer,
                                            pYUV420Buf->iStep,
                                            roiSize,
                                            ippCameraRotateDisable);
	}
	else {
		pYUV420Buf = pImgBuf;
	}

	ceYUV420ToRGB565((Ipp16u *)snapshot, pYUV420Buf);

	// get capture picture
	port = -1;
	error = CAM_SendCommand(mCEHandle, CAM_AUTOCAM_DEQUEUE_BUF, (int)&port, &pImgBuf);
	VALIDATE(error);

	/*
	 * only support jpeg still capture,
	 * so we need SW JPEG encoder in YUV case
	 */
	if ((strcmp(mParameters.getPictureFormat(), "jpeg") == 0)) {
	    if (picture != NULL){
			memcpy(picture, pImgBuf->pBuffer[0], pImgBuf->iFilledLen[0]);
			*pPicSize = pImgBuf->iFilledLen[0];
			LOGD("captured one picture\n");
		}
	}
	else if ((strcmp(mParameters.getPictureFormat(), "yuv422sp") == 0) ||
            (strcmp(mParameters.getPictureFormat(), "yuv420sp") == 0)) {
#if 0
		{
			FILE *fp_video = fopen("/data/video.yuv", "wb");
			if (fp_video) {
				fwrite(pImgBuf->pBuffer[0], 1, pImgBuf->iFilledLen[0], fp_video);
				fwrite(pImgBuf->pBuffer[1], 1, pImgBuf->iFilledLen[1], fp_video);
				fwrite(pImgBuf->pBuffer[2], 1, pImgBuf->iFilledLen[2], fp_video);
			}
			fclose(fp_video);
		}
#endif
		error = ceYUVToJPEG(picture, pImgBuf, pPicSize);
		VALIDATE(error);
#if 0
        {
            FILE *fp_jpg = fopen("/data/still.jpg", "wb");
            if (fp_jpg) {
                fwrite(picture, 1, *pPicSize, fp_jpg);
            }
            fclose(fp_jpg);
        }
#endif
	}
	else {
		// sth. must be wrong
		error = CAM_ERROR_BADARGUMENT;
		VALIDATE(error);
	}

	return error;
}

CAM_Error CameraHardware::ceYUV420ToRGB565(Ipp16u *pDst,
										   CAM_ImageBuffer *pImgBuf)
{
	//LOGD("%s\n", __FUNCTION__);

	int width, height;
	mParameters.getPreviewSize(&width, &height);

	Ipp8u *pSrc[3];
	pSrc[0] = pImgBuf->pBuffer[0];
	pSrc[1] = pImgBuf->pBuffer[1];
	pSrc[2] = pImgBuf->pBuffer[2];
	int srcStep[3] = {width, width>>1, width>>1};
	int dstStep = width<<1;
	IppiSize roiSize = {width, height};

	ippiYUV420ToBGR565_8u16u_P3C3R(pSrc, srcStep, pDst, dstStep, roiSize);

	return CAM_ERROR_NONE;
}

CAM_Error CameraHardware::ceYUV420ToRGB565RszRot(Ipp16u *pDst,
											   	CAM_ImageBuffer *pImgBuf)
{
	Ipp8u *pHeap = (Ipp8u *)mRszRotWorkBuf.pBuffer[0];

    int screen_width, screen_height; /* may not be full screen display */
	mParameters.getPreviewSize(&screen_width, &screen_height);
    
	IppCameraInterpolation 	interpolation	= ippCameraInterpBilinear;
	IppCameraRotation 		rotation		= ippCameraRotateDisable;
	Ipp8u *pSrc[3];
	int srcStep[3];
	Ipp8u *pThumb[3];
	int thumbStep[3];
	int rcpRatiox, rcpRatioy;
    IppiSize 				srcSize 		= {pImgBuf->iWidth, pImgBuf->iHeight};
	IppiSize 				thumbSize		= {240, 180};
	IppiSize				unrotSize		= {240, 180};

    if (screen_width < thumbSize.width || screen_height < thumbSize.height) {
        unrotSize.width = thumbSize.width = screen_width;
        unrotSize.height = thumbSize.height = screen_height;
    }

	pSrc[0] = pImgBuf->pBuffer[0];
    pSrc[1] = pImgBuf->pBuffer[1];
    pSrc[2] = pImgBuf->pBuffer[2];

	srcStep[0] = pImgBuf->iWidth;
	srcStep[1] = pImgBuf->iWidth>>1;
	srcStep[2] = pImgBuf->iWidth>>1;

	pThumb[0] = pHeap;
	pThumb[1] = pThumb[0] + (thumbSize.height * thumbSize.width);
	pThumb[2] = pThumb[1] + (thumbSize.height * thumbSize.width >> 2);

	thumbStep[0] = thumbSize.width;
	thumbStep[1] = thumbSize.width >> 1;
	thumbStep[2] = thumbSize.width >> 1;

	rcpRatiox = (((srcSize.width&~1)-1) << 16)/((unrotSize.width&~1)-1);
    rcpRatioy = (((srcSize.height&~1)-1) <<16)/((unrotSize.height&~1)-1);
    
#if 0
    LOGV("src: %dx%d,%d:%d:%d;", 
         srcSize.width, srcSize.height, srcStep[0], srcStep[1], srcStep[2]);
#endif
    
    ippiYCbCr420RszRot_8u_P3R((const Ipp8u**)pSrc,
        srcStep, srcSize,
		pThumb, thumbStep, unrotSize,
		interpolation, rotation, rcpRatiox, rcpRatioy);

#if DUMP_PREVIEW_FRAME
    {
    static int thumb_cnt = 0;
     
    if (DUMP_PREVIEW_FRAME == thumb_cnt++) {    
        FILE *fp = fopen("/data/thumb.yuv", "wb");
        if (fp) {
            fwrite(pThumb[0], 1, thumbStep[0]*thumbSize.height, fp);
            fwrite(pThumb[1], 1, thumbStep[1]*thumbSize.height>>1, fp);
            fwrite(pThumb[2], 1, thumbStep[2]*thumbSize.height>>1, fp);
        }
        fclose(fp);
    }
    }
#endif

	// yuv2rgb
	int dstStep = screen_width<<1;
    int dstStart = ((screen_height-thumbSize.height)>>1)*screen_width;
	IppiSize roiSize = {thumbSize.width, thumbSize.height};

#if 0
    LOGV("src: %dx%d,%d:%d:%d; rgb: %dx%d,%d,%d", 
         srcSize.width, srcSize.height, srcStep[0], srcStep[1], srcStep[2],
         thumbSize.width, thumbSize.height, dstStart, dstStep);
#endif
    ippiYUV420ToBGR565_8u16u_P3C3R(pThumb, thumbStep, pDst+dstStart, dstStep, roiSize);

	return CAM_ERROR_NONE;
}

CAM_Error CameraHardware::ceYUVToJPEG(uint8_t *picture,
									  CAM_ImageBuffer *pImgBuf,
									  int *pPicSize)
{
	IppPicture 					YUVPic;
	IppPicture					*pSrcPic = &YUVPic;
	IppBitstream				BitStream;
	IppBitstream 				*pBitstream = &BitStream;
	void               			*pEncoderState;
	MiscGeneralCallbackTable 	*pCallBackTable;
    IppJPEGEncoderParam 		encoderPar;
	int 						ret;

	pSrcPic->picWidth 			= pImgBuf->iWidth;
	pSrcPic->picHeight 			= pImgBuf->iHeight;
	pSrcPic->picFormat 			= (pImgBuf->eFormat == CAM_IMGFMT_YCC420P ? JPEG_YUV411 : JPEG_YUV422);
	pSrcPic->picChannelNum 		= 3;
	pSrcPic->picPlaneNum 		= 3;
	pSrcPic->picPlaneStep[0] 	= pImgBuf->iStep[0];
	pSrcPic->picPlaneStep[1] 	= pImgBuf->iStep[1];
	pSrcPic->picPlaneStep[2] 	= pImgBuf->iStep[2];
	pSrcPic->ppPicPlane[0] 		= pImgBuf->pBuffer[0];
	pSrcPic->ppPicPlane[1] 		= pImgBuf->pBuffer[1];
	pSrcPic->ppPicPlane[2] 		= pImgBuf->pBuffer[2];
	pSrcPic->picROI.x 			= 0;
	pSrcPic->picROI.y 			= 0;
	pSrcPic->picROI.width 		= pImgBuf->iWidth;;
	pSrcPic->picROI.height 		= pImgBuf->iHeight;

	pBitstream->pBsBuffer 		= (Ipp8u *)picture;
	pBitstream->bsByteLen 		= *pPicSize;
	pBitstream->pBsCurByte 		= (Ipp8u *)picture;
	pBitstream->bsCurBitOffset 	= 0;

	encoderPar.nQuality         = 85;
	encoderPar.nRestartInterval = 0;
	encoderPar.nJPEGMode        = JPEG_BASELINE;
	encoderPar.nSubsampling     = pSrcPic->picFormat == JPEG_YUV411 ? JPEG_411 : JPEG_422;
	encoderPar.nBufType			= JPEG_INTEGRATEBUF;

	encoderPar.pSrcPicHandler   = (void*)-1;
	encoderPar.pStreamHandler   = (void*)-1;

#if 0
LOGD("encoderPar.nBufType = %d\n", encoderPar.nBufType);
LOGD("encoderPar.nJPEGMode = %d\n", encoderPar.nJPEGMode);
LOGD("encoderPar.nQuality = %d\n", encoderPar.nQuality);
LOGD("encoderPar.nRestartInterval = %d\n", encoderPar.nRestartInterval);
LOGD("encoderPar.nSubsampling = %d\n", encoderPar.nSubsampling);
LOGD("encoderPar.pSrcPicHandler = %p\n", encoderPar.pSrcPicHandler);
LOGD("encoderPar.pStreamHandler = %p\n", encoderPar.pStreamHandler);

LOGD("pSrcPic->picWidth = %d\n", pSrcPic->picWidth);
LOGD("pSrcPic->picHeight = %d\n", pSrcPic->picHeight);
LOGD("pSrcPic->picFormat = %d\n", pSrcPic->picFormat);
LOGD("pSrcPic->picChannelNum = %d\n", pSrcPic->picChannelNum);
LOGD("pSrcPic->picPlaneNum = %d\n", pSrcPic->picPlaneNum);
LOGD("pSrcPic->picPlaneStep[0] = %d\n", pSrcPic->picPlaneStep[0]);
LOGD("pSrcPic->picPlaneStep[1] = %d\n", pSrcPic->picPlaneStep[1]);
LOGD("pSrcPic->picPlaneStep[2] = %d\n", pSrcPic->picPlaneStep[2]);
LOGD("pSrcPic->ppPicPlane[0] = %p\n", pSrcPic->ppPicPlane[0]);
LOGD("pSrcPic->ppPicPlane[1] = %p\n", pSrcPic->ppPicPlane[1]);
LOGD("pSrcPic->ppPicPlane[2] = %p\n", pSrcPic->ppPicPlane[2]);
LOGD("pSrcPic->picROI.x = %d\n", pSrcPic->picROI.x);
LOGD("pSrcPic->picROI.y = %d\n", pSrcPic->picROI.y);
LOGD("pSrcPic->picROI.width = %d\n", pSrcPic->picROI.width);
LOGD("pSrcPic->picROI.height = %d\n", pSrcPic->picROI.height);
LOGD("pSrcPic->picStatus = %d\n", pSrcPic->picStatus);
LOGD("pSrcPic->picTimeStamp = %d\n", (int)pSrcPic->picTimeStamp);
LOGD("pSrcPic->picOrderCnt = %d\n", pSrcPic->picOrderCnt);
#endif

    if ( miscInitGeneralCallbackTable(&pCallBackTable) != 0 ) {
		return CAM_ERROR_OUTOFMEMORY;
	}

	ret = EncoderInitAlloc_JPEG(&encoderPar,
								pSrcPic,
								pBitstream,
								pCallBackTable,
								&pEncoderState);

    if (IPP_STATUS_NOERR != ret) {
        return CAM_ERROR_BADARGUMENT;
    }

	ret = Encode_JPEG (pSrcPic, pBitstream, pEncoderState);
    if (IPP_STATUS_NOERR != ret) {
        EncoderFree_JPEG(&pEncoderState);
		miscFreeGeneralCallbackTable(&pCallBackTable);
        return CAM_ERROR_OPFAILED;
    }

    /*if (IPP_STATUS_NOERR != Encode_JPEG (0,
                                         pBitstream,
                                         pEncoderState)) {
        EncoderFree_JPEG(&pEncoderState);
        return CAM_ERROR_OPFAILED;
    }*/

	*pPicSize = (int)pBitstream->pBsCurByte - (int)pBitstream->pBsBuffer;

    EncoderFree_JPEG(&pEncoderState);
	miscFreeGeneralCallbackTable(&pCallBackTable);

    return CAM_ERROR_NONE;
}

wp<CameraHardwareInterface> CameraHardware::singleton;

sp<CameraHardwareInterface> CameraHardware::createInstance()
{
    LOGD("%s\n", __FUNCTION__);
    if (singleton != 0) {
        sp<CameraHardwareInterface> hardware = singleton.promote();
        if (hardware != 0) {
            return hardware;
        }
    }
    sp<CameraHardwareInterface> hardware(new CameraHardware());
    singleton = hardware;
    return hardware;
}

extern "C" sp<CameraHardwareInterface> openCameraHardware()
{
    LOGD("%s\n", __FUNCTION__);
    return CameraHardware::createInstance();
}

}; // namespace android
