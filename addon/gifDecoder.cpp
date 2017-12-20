//
//  gifDecoder.cpp
//  gifMy
//
//  Created by yangruting on 16/11/4.
//  Copyright © 2016年 yangruting. All rights reserved.
//

#include "gifDecoder.h"
#include <stdio.h>
#include <stdlib.h>
#include <typeinfo>
#include <memory.h>
#include <log/Log.h>
#include <node_buffer.h>
#include <arpa/inet.h>
#include <env.h>
#include <env-inl.h>
#include <node.h>
#define LOG_TAG "GifDecoder"
using namespace yunos;

void decoder_gif(uv_work_t* req) {
    NewShareData * my_data = static_cast<NewShareData *>(req->data);
    HandleScope scope(isolate);
    GifDecoder obj = my_data->obj;
    FILE *fp = fopen(obj->filename);
    if (fp != NULL) {
        gfs = Gif_ReadFile(fp);
        if (gfs == 0) {
            
            // isolate->ThrowException(Exception::TypeError(
            //     String::NewFromUtf8(isolate, "It is not the correct png format")));
            // return;
        } else {
            if (gfs->background != 256) {
                background = &gfs->global->col[gfs->background];
            }
            obj->GetCurrentBmp();
        }
        fclose(fp);
    }  else {
        // isolate->ThrowException(Exception::TypeError(
        //         String::NewFromUtf8(isolate, "File dose not exist")));
        // return;
    }
}

void new_finish(uv_work_t* req, int status) {
    NewShareData * my_data = static_cast<NewShareData *>(req->data);
    Isolate * isolate = my_data->isolate;
    HandleScope scope(isolate);
    Local<Function> js_callback = Local<Function>::New(isolate, my_data->js_callback);
    js_callback->Call(isolate->GetCurrentContext()->Global(), 0, null);
    delete my_data;
}

Persistent<Function> GifDecoder::constructor;
void GifDecoder::New(const FunctionCallbackInfo<Value> &args) {
    Isolate *isolate = args.GetIsolate();
    HandleScope scope(isolate);
    Local<Context> context = isolate->GetCurrentContext();
    if (args.IsConstructCall()) {
        GifDecoder *obj;
        String::Utf8Value filename(args[0]);
        char* fn = *filename;
        NewShareData* req = new NewShareData();
        // env = args.GetCurrent();
        Environment* env = Environment::GetCurrent(args);
        uv_loop_t* loop = env->event_loop();
        obj = new GifDecoder();
        req->request.data = req;
        req->isolate = isolate;
        req->obj = obj;
        req->js_callback.Reset(isolate, Local<Function>::Cast(args[0]));
        uv_queue_work(loop, &(req->request), decoder_gif, new_finish); 
        // if (obj->errorType == -1) {
        //     isolate->ThrowException(Exception::TypeError(
        //         String::NewFromUtf8(isolate, "File dose not exist")));
        //     return;
        // } else if (obj->errorType == 0) {
        //     isolate->ThrowException(Exception::TypeError(
        //         String::NewFromUtf8(isolate, "It is not the correct png format")));
        //     return;
        // } else {
        //     obj->Wrap(args.This());
        //     args.GetReturnValue().Set(args.This());
        // }
    }
}



GifDecoder::GifDecoder() {
    currentIndex = 0;
}

GifDecoder::~GifDecoder() {
    GifDeleteBmpImage();
    Gif_DeleteStream(gfs);
    Gif_Delete(background);
}

void GifDecoder::
GifDeleteBmpImage()
{
    if (currentFrame != NULL) {
        delete[] currentFrame->bmpPixel;
        currentFrame = NULL;
    }
    if (previousFrame != NULL) {
        delete[] previousFrame->bmpPixel;
        previousFrame = NULL;
    }
    if (previousLastFrame != NULL) {
        delete[] previousLastFrame->bmpPixel;
        previousLastFrame = NULL;
    }
}
// void GifDecoder::DecodeFile() {
//     OpenFile(filename);
//     if (fp != NULL) {
//         gfs = Gif_ReadFile(fp);
//         if (gfs == 0) {
//             errorType = 0;
//         } else {
//             errorType = 1;
//             if (gfs->background != 256) {
//                 background = &gfs->global->col[gfs->background];
//             }
//             currentIndex = 0;
//             GetCurrentBmp();
//         }
//         CloseFile();
//     }  else {
//         errorType = -1;
//     }
// }
void GifDecoder::FrameCount(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = args.GetIsolate();
    HandleScope scope(isolate);
    GifDecoder *gifDecoder = ObjectWrap::Unwrap<GifDecoder>(args.This());

    Local<Number> frameCount = Number::New(isolate, Gif_ImageCount(gifDecoder->gfs));
    args.GetReturnValue().Set(frameCount);
}

void GifDecoder::GetScreenWidth(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = args.GetIsolate();
    HandleScope scope(isolate);
    GifDecoder *gifDecoder = ObjectWrap::Unwrap<GifDecoder>(args.This());

    Local<Number> width = Number::New(isolate, gifDecoder->gfs->screen_width);
    args.GetReturnValue().Set(width);
}

void GifDecoder::GetScreenHeight(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = args.GetIsolate();
    HandleScope scope(isolate);
    GifDecoder *gifDecoder = ObjectWrap::Unwrap<GifDecoder>(args.This());

    Local<Number> height = Number::New(isolate, gifDecoder->gfs->screen_height);
    args.GetReturnValue().Set(height);
}

MaybeLocal<Object> CreateShadowBuffer(Isolate* isolate, char* data, size_t length) {
    EscapableHandleScope scope(isolate);
    node::Environment* env = node::Environment::GetCurrent(isolate);
    if (length > 0 && data != NULL) {
        Local<ArrayBuffer> ab = ArrayBuffer::New(isolate, data, length);
        Local<Uint8Array> ui = Uint8Array::New(ab, 0, length);
        Maybe<bool> mb = ui->SetPrototype(env->context(), env->buffer_prototype_object());
        if (mb.FromMaybe(false))
            return scope.Escape(ui);
    }
    return Local<Object>();
}

void GifDecoder::NextFrame(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = args.GetIsolate();
    HandleScope scope(isolate);
    GifDecoder *gifDecoder = ObjectWrap::Unwrap<GifDecoder>(args.This());
    gifDecoder->GetCurrentBmp();
    MaybeLocal<Object> buffer = CreateShadowBuffer(isolate, (char*)gifDecoder->current, (size_t)gifDecoder->gfs->screen_width * gifDecoder->gfs->screen_height * 4);
    Local<Object> bmp = buffer.ToLocalChecked();

    Local<Object> obj = Object::New(isolate);
    obj->Set(String::NewFromUtf8(isolate, "delay"), Number::New(isolate, Gif_ImageDelay(gifDecoder->current->delay)));
    obj->Set(String::NewFromUtf8(isolate, "image"), bmp);
    args.GetReturnValue().Set(obj);
    GetCurrentBmp();
}

void GifDecoder::GetDelay(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = args.GetIsolate();
    HandleScope scope(isolate);
    GifDecoder *gifDecoder = ObjectWrap::Unwrap<GifDecoder>(args.This());
    if (!args[0]->IsNumber()) {
        isolate->ThrowException(Exception::TypeError(
                                                     String::NewFromUtf8(isolate, "Wrong arguments")));
        return;
    }
    int index = args[0]->NumberValue();

    Local<Number> delay = Number::New(isolate, Gif_ImageDelay(gifDecoder->gfs->images[index]));
    args.GetReturnValue().Set(delay);
}

void GifDecoder::LoopCount(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = args.GetIsolate();
    HandleScope scope(isolate);
    GifDecoder *gifDecoder = ObjectWrap::Unwrap<GifDecoder>(args.This());

    Local<Number> loopCount = Number::New(isolate, gifDecoder->gfs->loopcount);
    args.GetReturnValue().Set(loopCount);
}

void GifDecoder::GetCurrentBmp() {
    if (previousFrame != NULL) {
        previousLastFrame = previousFrame;
    }
    if (currentFrame != NULL) {
        previousFrame = currentFrame;
    }
    currentFrame->delay = Gif_ImageDelay(gfs->images[currentIndex])
    SetPixels(currentIndex);
    if (previousLastFrame != NULL) {
        delete[] previousLastFrame->bmpPixel;
        previousLastFrame = NULL;
    }
    currentIndex++;
    if (currentIndex >= Gif_ImageCount(gfs)) {
        GifDeleteBmpImage();
        currentIndex = 0;
    }
}

void GifDecoder::SetPixels(int index) {
    Gif_Image* gfi = Gif_GetImage(gfs, index);
    uint8_t* dest = new uint8_t[gfs->screen_height * gfs->screen_width * 4];
    memset(dest, 0, gfs->screen_height * gfs->screen_width * 4);
    uint8_t lastDispose = 0;
    uint8_t* lastImage = NULL;
    uint16_t lrx;
    uint16_t lry;
    uint16_t lrw;
    uint16_t lrh;
    Gif_Colormap *act;
    int transparency = 0;
    short transparent;
    if (index > 0) {
        Gif_Image* lastGfi = Gif_GetImage(gfs, index - 1);
        lastImage = previousFrame->bmpPixel;
        lrx = lastGfi->left;
        lry = lastGfi->top;
        lrw = lastGfi->width;
        lrh = lastGfi->height;
        lastDispose = lastGfi->disposal;
        if (lastDispose == 0) {
            lastDispose = 1;
        }
    }

    transparent = gfi->transparent;
    transparency = gfi->transparency;

    if (gfi->local) {
        act = gfi->local;
    } else {
        act = gfs->global;
        if (gfs->background == transparent) {
            background = new Gif_Color();
            background->gfc_red = 0;
            background->gfc_green = 0;
            background->gfc_blue = 0;
            background->haspixel = 0;
            background->opacity = 0;
        }
    }

    Gif_Color *save = new Gif_Color();
    save->gfc_red = 0;
    save->gfc_green = 0;
    save->gfc_blue = 0;
    save->haspixel = 0;
    save->opacity = 0;
    if (transparency) {
        save->gfc_red = (&act->col[transparent])->gfc_red;
        save->gfc_blue = (&act->col[transparent])->gfc_blue;
        save->gfc_green = (&act->col[transparent])->gfc_green;
        save->haspixel = (&act->col[transparent])->haspixel;
        save->opacity = (&act->col[transparent])->opacity;
        Gif_Color *now = new Gif_Color();
        now->gfc_red = 0;
        now->gfc_green = 0;
        now->gfc_blue = 0;
        now->haspixel = 0;
        now->opacity = 0;
        act->col[transparent] = *now; // set transparent color if specified
    }

    if (lastDispose > 0) {
        if (lastDispose == 3) {
            // use image before last
            int n = index - 1;
            if (n > 0) {
                lastImage = previousLastFrame->bmpPixel;
            } else {
                lastImage = NULL;
            }
        }

        if (lastImage != NULL) {
            for (int i = 0; i < gfs->screen_width * gfs->screen_height * 4; i++) {
                dest[i] = lastImage[i];
            }
            // copy pixels
            if (lastDispose == 2) {
                // fill last image rect area with background color
                Gif_Color *c = new Gif_Color;
                c->gfc_red = 0;
                c->gfc_green = 0;
                c->gfc_blue = 0;
                c->haspixel = 0;
                c->opacity = 0;
                if (!transparency) {
                    c = background;
                }
                for (int i = 0; i < lrh; i++) {
                    int n1 = (lry + i) * gfi->width + lrx;
                    int n2 = n1 + lrw;
                    for (int k = n1; k < n2; k++) {
                        dest[k * 4] = c->gfc_red;
                        dest[k * 4 + 1] = c->gfc_green;
                        dest[k * 4 + 2] = c->gfc_blue;
                        dest[k * 4 + 3] = c->opacity;
                    }
                }
            }
        }
    }

    // copy each source line to the appropriate place in the destination
    int pass = 1;
    int inc = 8;
    int iline = 0;
    for (int i = 0; i < gfi->height; i++) {
        int line = i;
        if (gfi->interlace) {
            if (iline >= gfi->height) {
                pass++;
                switch (pass) {
                    case 2:
                        iline = 4;
                        break;
                    case 3:
                        iline = 2;
                        inc = 4;
                        break;
                    case 4:
                        iline = 1;
                        inc = 2;
                }
            }
            line = iline;
            iline += inc;
        }
        line += gfi->top;
        if (line < gfs->screen_height) {
            int k = line * gfs->screen_width;
            int dx = k + gfi->left; // start of line in dest
            int dlim = dx + gfi->width; // end of dest line
            if ((k + gfs->screen_width) < dlim) {
                dlim = k + gfs->screen_width; // past dest edge
            }
            int sx = i * gfi->width; // start of line in source
            while (dx < dlim) {
                // map color and insert in destination
                int index1 = ((int) gfi->img[sx / gfi->width][sx % gfi->width]) & 0xff;
                sx++;
                Gif_Color *c = &act->col[index1];
                if (c->opacity != 0) {
                    dest[dx * 4] = c->gfc_red;
                    dest[dx * 4 + 1] = c->gfc_green;
                    dest[dx * 4 + 2] = c->gfc_blue;
                    dest[dx * 4 + 3] = c->opacity;
                }
                dx++;
            }
        }
    }

    if (transparency) {
        act->col[transparent] = *save;
    }

    delete save;
}

static void DecoderInit(Handle<Object> exports) {
    Isolate *isolate = exports->GetIsolate();
    Local<FunctionTemplate> tpl = FunctionTemplate::New(isolate, GifDecoder::New);
    tpl->SetClassName(String::NewFromUtf8(isolate, "GifDecoder"));
    tpl->InstanceTemplate()->SetInternalFieldCount(1);
    NODE_SET_PROTOTYPE_METHOD(tpl, "LoopCount", GifDecoder::LoopCount);
    NODE_SET_PROTOTYPE_METHOD(tpl, "GetScreenWidth", GifDecoder::GetScreenWidth);
    NODE_SET_PROTOTYPE_METHOD(tpl, "GetScreenHeight", GifDecoder::GetScreenHeight);
    NODE_SET_PROTOTYPE_METHOD(tpl, "FrameCount", GifDecoder::FrameCount);
    NODE_SET_PROTOTYPE_METHOD(tpl, "GetDelay", GifDecoder::GetDelay);
    NODE_SET_PROTOTYPE_METHOD(tpl, "NextFrame", GifDecoder::NextFrame);
    GifDecoder::constructor.Reset(isolate, tpl->GetFunction());
    exports->Set(String::NewFromUtf8(isolate, "GifDecoder"), tpl->GetFunction());
}

NODE_MODULE(node_gifdecoder, DecoderInit);
