//
//  gifDecoder.hpp
//  gifMy
//
//  Created by yangruting on 16/11/4.
//  Copyright © 2016年 yangruting. All rights reserved.
//

#ifndef gifDecoder_h
#define gifDecoder_h

#include <stdio.h>
#include <v8.h>
#include <node.h>
#include <node_object_wrap.h>
#include <lcdfgif/gif.h>
using namespace v8;
namespace yunos {
struct Frame {
    int delay;
    uint8_t* bmpPixel;
};

struct NewShareData
{
    uv_work_t request;
    GifDecoder obj;
    Isolate * isolate;
    Persistent<Function> js_callback;
};

struct ShareData
{
    uv_work_t request;
    Isolate * isolate;
    Persistent<Function> js_callback;
};

class GifDecoder : public node::ObjectWrap {
public:
    GifDecoder(char * filename);
    ~GifDecoder();
    static void FrameCount(const FunctionCallbackInfo<Value>& args);
    static void NextFrame(const FunctionCallbackInfo<Value>& args);
    static void GetDelay(const FunctionCallbackInfo<Value>& args);
    static void LoopCount(const FunctionCallbackInfo<Value>& args);
    static void GetScreenWidth(const FunctionCallbackInfo<Value>& args);
    static void GetScreenHeight(const FunctionCallbackInfo<Value>& args);
public:
    static void New(const FunctionCallbackInfo<Value> &args);
    static void DecoderInit(Local<Object> exports);
private:
    void OpenFile(char * filename);
    void CloseFile();
    void GetCurrentBmp();
    void SetPixels(int index);
    void GifDeleteBmpImage();
private:
    int errorType;
    int currentIndex;
public:
    static Persistent<Function> constructor;
public:
    Frame* currentFrame;
    Frame* previousFrame;
    Frame* previousLastFrame;
    Gif_Stream* gfs;
    Gif_Color *background;
};
}

#endif /* gifDecoder_hpp */
