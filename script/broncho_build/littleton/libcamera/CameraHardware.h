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

#ifndef ANDROID_HARDWARE_CAMERA_HARDWARE_H
#define ANDROID_HARDWARE_CAMERA_HARDWARE_H

#include <utils/threads.h>
#include <ui/CameraHardwareInterface.h>
#include <utils/MemoryBase.h>
#include <utils/MemoryHeapBase.h>
#include <utils/threads.h>

#include "CamAutoCam.h"
#include "ippIP.h"
#include "display.h"

#define CAMHW_DUMP
#define CAMHW_PROF
#ifdef CAMHW_PROF
#include <time.h>
#endif

namespace android {

class CameraHardware : public CameraHardwareInterface {
public:
    virtual sp<IMemoryHeap> getPreviewHeap() const;
    virtual sp<IMemoryHeap> getRawHeap() const;

    virtual status_t    startPreview(preview_callback cb, void* user);
    virtual void        stopPreview();
    virtual bool        previewEnabled();
    virtual status_t    startRecording(recording_callback cb, void* user);
    virtual void        stopRecording();
    virtual bool        recordingEnabled(){return mRecordingEnabled;}
    virtual bool        useOverlay(){return mOverlayEnabled;}
    virtual void        releaseRecordingFrame(const sp<IMemory>& mem);

    virtual status_t    autoFocus(autofocus_callback, void *user);
    virtual status_t    takePicture(shutter_callback,
                                    raw_callback,
                                    jpeg_callback,
                                    void* user);
    virtual status_t    cancelPicture(bool cancel_shutter,
                                      bool cancel_raw,
                                      bool cancel_jpeg);
    virtual status_t    dump(int fd, const Vector<String16>& args) const;
    virtual status_t    setParameters(const CameraParameters& params);
    virtual CameraParameters  getParameters() const;
    virtual void release();

    static sp<CameraHardwareInterface> createInstance();

private:
                        CameraHardware();
    virtual             ~CameraHardware();

    static wp<CameraHardwareInterface> singleton;

    static const int kBufferCount = 4;

    class PreviewThread : public Thread {
        CameraHardware* mHardware;
    public:
        PreviewThread(CameraHardware* hw)
            : Thread(false), mHardware(hw) { }
        virtual void onFirstRef() {
            run("CameraPreviewThread", PRIORITY_URGENT_DISPLAY);
        }
        virtual bool threadLoop() {
            mHardware->previewThread();
            // loop until we need to quit
            return true;
        }
    };

    void initDefaultParameters();
    void initHeapLocked();

    int previewThread();

    static int beginAutoFocusThread(void *cookie);
    int autoFocusThread();

    static int beginPictureThread(void *cookie);
    int pictureThread();

    mutable Mutex       mLock;

    CameraParameters    mParameters;

    sp<MemoryHeapBase>  mPreviewHeap;
    sp<MemoryHeapBase>  mRawHeap;
    sp<MemoryBase>      mBuffers[kBufferCount];

    Mutex               mBuffersLock;
    Condition           mBuffersCond;
    /*
     * When video encoding can't catch up with camera input, streaming
     * buffer queue will finally drain. Then, preview thread begins to
     * wait for empty buffer, and sets mBuffersEmpty to true.
     * After an empty buffer is returned through releaseRecordingFrame(),
     * preview thread will be signaled and mBuffersEmpty will be reset
     * to false.
     */
    bool                mBuffersEmpty;
    /*
     * After recording stops, we can't start preview immediately in case
     * some buffers are still being encoded, so wait until all buffers
     * have been returned througn releaseRecordingFrame(), that is 
     * mOurstandingBuffers == 0.
     */
    int                 mOutstandingBuffers;

    bool                mPreviewRunning;
    int                 mPreviewFrameSize;

    shutter_callback    mShutterCallback;
    raw_callback        mRawPictureCallback;
    jpeg_callback       mJpegPictureCallback;
    void                *mPictureCallbackCookie;

    // protected by mLock
    sp<PreviewThread>   mPreviewThread;
    preview_callback    mPreviewCallback;
    void                *mPreviewCallbackCookie;

    autofocus_callback  mAutoFocusCallback;
    void                *mAutoFocusCallbackCookie;

    // only used from PreviewThread
    int                 mCurrentPreviewFrame;
    int                 mPreviewFrameCount;
  
    // used by recording 
    recording_callback  mRecordingCallback;
    void                *mRecordingCallbackCookie;
    bool                mRecordingEnabled;
    bool                mOverlayEnabled;
    Mutex               mOverlayLock; 
    display_cfg         *pDisplayCfg;    

    // camera engine stuff
    CAM_DeviceHandle    mCEHandle;
    CAM_UseCase         mUseCase;
    CAM_ImageBufferReq  mPreviewBufReq;
    CAM_ImageBufferReq  mCaptureBufReq;
    CAM_ImageBuffer     mCSCWorkBuf;
    CAM_ImageBuffer     mRszRotWorkBuf;
#ifdef NO_OVERLAY2
    sp<MemoryBase>      mRGBWorkBuf;
#endif
    
    bool                mPreviewAutoCrop;
    bool                mPreviewRotate;

    CAM_Error ceInit();
    CAM_Error ceDeinit();
    CAM_Error ceSetParameters();
    CAM_Error ceStartPreview();
    CAM_Error ceStopPreview();
    CAM_Error ceStartCapture();
    CAM_Error ceGetPreviewFrame(uint8_t *frame, bool bRecordOn, int *index);
    CAM_Error ceGetCapturePicture(uint8_t *snapshot, uint8_t *picture, int *pPicSize);
    CAM_Error ceYUV420ToRGB565(Ipp16u *pDst, CAM_ImageBuffer *pImgBuf);
    CAM_Error ceYUV420ToRGB565RszRot(Ipp16u *pDst, CAM_ImageBuffer *pImgBuf);
    CAM_Error ceYUVToJPEG(uint8_t *picture, CAM_ImageBuffer *pImgBuf, int *pPicSize);
};

#ifdef CAMHW_PROF
    // profiling
    long                mProcessTime;
    long                mThreadTime;
    int                 mRecordFrameCnt;
    int                 mProfilingID;
#endif

#ifdef CAMHW_DUMP
    int                 mDumpCnt;
#endif
}; // namespace android

#endif
