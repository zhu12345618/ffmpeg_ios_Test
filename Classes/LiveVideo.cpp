//
//  LiveVideo.cpp
//  Game
//
//  Created by zhufu on 2017/3/1.
//
//

#include "LiveVideo.h"
#include "HelloWorldScene.h"

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
}

Scene* LiveVideo::createScene()
{
    auto scene = Scene::create();
    auto sprite = Sprite::create("HelloWorld.png");
    
    
    // position the sprite on the center of the screen
    sprite->setPosition(Vec2(Director::getInstance()->getVisibleSize().width/2-140, Director::getInstance()->getVisibleSize().height/2+100));
    
    // add the sprite as a child to this layer
    scene->addChild(sprite, 0);
    LiveVideo* liveVideo = LiveVideo::create();
    scene->addChild(liveVideo);
    liveVideo->init();
    auto layer = HelloWorld::create();
    scene->addChild(layer);
    return scene;
}

LiveVideo* LiveVideo::create()
{
    LiveVideo* liveVideo = new(std::nothrow) LiveVideo();
    if(liveVideo)
    {
        liveVideo->autorelease();
        return liveVideo;
    }
    CC_SAFE_DELETE(liveVideo);
    return nullptr;
}

bool LiveVideo::init()
{
    initEvents();
    initCommand();
    //ffmpeg解码线程
    play();
    
    addPlayButton();
    addPlayHKSButton();
    addStopButton();
    addRefreshButton();
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    loadShader();
    loadTexture();
    loadRectangle();
    return true;
}

void LiveVideo::addPlayButton()
{
    auto button = ui::Button::create("CloseNormal.png", "CloseSelected.png");
    button->setTitleText("play");
    button->setPosition(Vec2(170, 100));
    button->addClickEventListener(CC_CALLBACK_0(LiveVideo::play, this));
    button->setContentSize(Size(100, 100));
    addChild(button);
}

void LiveVideo::addPlayHKSButton()
{
    auto button = ui::Button::create("CloseNormal.png", "CloseSelected.png");
    button->setTitleText("playHKS");
    button->setPosition(Vec2(240, 100));
    button->addClickEventListener(CC_CALLBACK_0(LiveVideo::playHKS, this));
    button->setContentSize(Size(100, 100));
    addChild(button);
}

void LiveVideo::addStopButton()
{
    auto button = ui::Button::create("CloseNormal.png", "CloseSelected.png");
    button->setTitleText("stop");
    button->setPosition(Vec2(170, 50));
    button->addClickEventListener(CC_CALLBACK_0(LiveVideo::stop, this));
    button->setContentSize(Size(100, 100));
    addChild(button);
}

void LiveVideo::addRefreshButton()
{
    auto button = ui::Button::create("CloseNormal.png", "CloseSelected.png");
    button->setTitleText("refresh");
    button->setPosition(Vec2(240, 50));
    button->addClickEventListener(CC_CALLBACK_0(LiveVideo::refresh, this));
    button->setContentSize(Size(100, 100));
    addChild(button);
}

void LiveVideo::initEvents()
{
    EventListenerCustom* stopDecodeListener = EventListenerCustom::create("stopFFmpegDecode", CC_CALLBACK_0(LiveVideo::stop, this));
    Director::getInstance()->getEventDispatcher()->addEventListenerWithFixedPriority(stopDecodeListener, 1);
    
    EventListenerCustom* startDecodeListener = EventListenerCustom::create("startFFmpegDecode", CC_CALLBACK_0(LiveVideo::ffmpegDecode, this, currentLivePath));
    Director::getInstance()->getEventDispatcher()->addEventListenerWithFixedPriority(startDecodeListener, 1);
}

void LiveVideo::play()
{
    if(!isPlay())
    {
        const char* filePath = "rtmp://113.10.194.251/live/mtable1";
        setPlay(true);
        std::thread t(&LiveVideo::ffmpegDecode, this, filePath);
        t.detach();
        
    }
    
}

void LiveVideo::playHKS()
{
    if(!isPlay())
    {
        const char* filePath ="rtmp://live.hkstv.hk.lxdns.com/live/hks";
        setPlay(true);
        std::thread t(&LiveVideo::ffmpegDecode, this, filePath);
        t.detach();
        
    }
    
}

void LiveVideo::stop()
{
    setPlay(false);
}

void LiveVideo::refresh()
{
    if(_playFlag)
    {
        clearBuf();
    } else {
        play();
    }
    
}

void LiveVideo::setPlay(bool playFlag)
{
    _playFlag = playFlag;
}

bool LiveVideo::isPlay()
{
    return _playFlag;
}

long LiveVideo::getCurrentTime()
{
    struct timeval now;
    gettimeofday(&now, NULL);
    return now.tv_sec*1000 +  (int)(now.tv_usec/1000+0.5);
}

void LiveVideo::flipVertical(int width, int height, char* arr)
{
    int index = 0, f_index, cycle=height>>1;
    char buf;
    for (int i = 0; i < cycle; i++)
    {
        for (int j = 0; j < width; j++)
        {
            //当前像素
            index = i*width + j;
            //需要交换的像素
            f_index = (height - 1 - i)*width + j;
            //缓存当前像素
            buf = arr[index];
            //交换像素
            arr[index] = arr[f_index];
            //交换回像素
            arr[f_index] = buf;
        }
    }
}

int LiveVideo::ffmpegDecode(const char* filePath)
{
    //开始解码
    CCLOG("filePath %s", filePath);
//    strcpy(currentLivePath, filePath);
    av_register_all();
    avformat_network_init();
    AVFormatContext* pFormat = NULL;
    AVCodecContext* video_dec_ctx = NULL;
    AVCodec* video_dec = NULL;
    AVPacket *pkt = NULL;
    AVFrame *pFrame = NULL;
    
    do {
        
        if (avformat_open_input(&pFormat, filePath, NULL, NULL) < 0)
        {
            CCLOG("Couldn't open input stream.（无法打开输入流）\n");
            break;
        }
        
        CCLOG("%lld", pFormat->probesize);
        CCLOG("%lld", pFormat->max_analyze_duration);
        pFormat->probesize = 100;
        pFormat->max_analyze_duration = 0;
        if (avformat_find_stream_info(pFormat, NULL) < 0)
        {
            CCLOG("Couldn't find stream information.（无法获取流信息）\n");
            break;
        }
        
        int i, videoIndex=-1;
        for(i = 0; i<pFormat->nb_streams; i++)
        {
            if(pFormat->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO)
            {
                videoIndex=i;
                break;
            }
        }
        if(videoIndex==-1)
        {
            printf("Didn't find a video stream.（没有找到视频流）\n");
            break;
        }
        video_dec_ctx=pFormat->streams[videoIndex]->codec;
        _frameRate = pFormat->streams[videoIndex]->r_frame_rate.num;
        if(_frameRate == 0)
        {
            _frameRate = 1;
        }
        
        _system_start_time = getCurrentTime();
        _start_time = pFormat->streams[videoIndex]->start_time;
        
        _pixel_w = video_dec_ctx->width;
        _pixel_h = video_dec_ctx->height;
        int pixel_w2 = _pixel_w >> 1;
        int pixel_h2 = _pixel_h >> 1;
        video_dec=avcodec_find_decoder(video_dec_ctx->codec_id);
        if(video_dec==NULL)
        {
            printf("Codec not found.（没有找到解码器）\n");
            break;
        }
        if(avcodec_open2(video_dec_ctx, video_dec,NULL)<0)
        {
            printf("Could not open codec.（无法打开解码器）\n");
            break;
        }
        
        
        pkt = av_packet_alloc();
        av_init_packet(pkt);
        pFrame = av_frame_alloc();
    
    
        while (_playFlag)
        {
            if(av_read_frame(pFormat, pkt) < 0)
            {
                CCLOG("读取帧失败!!!!");
                break;
            }
            if (pkt->stream_index == videoIndex)
            {
                int got_picture = 0,ret = 0;
                ret = avcodec_decode_video2(video_dec_ctx, pFrame, &got_picture, pkt);
                if (ret < 0)
                {
                    printf("Decode Error.（解码错误）\n");
                    break;
                }
                if (got_picture)
                {
                    char* buf = new char[video_dec_ctx->height * video_dec_ctx->width * 3 / 2];
                    int a = 0, i;
                    for (i = 0; i<_pixel_h; i++)
                    {
                        memcpy(buf + a, pFrame->data[0] + i * pFrame->linesize[0], _pixel_w);
                        a += _pixel_w;
                    }
                    flipVertical(_pixel_w, _pixel_h, buf);
                    for (i = 0; i<pixel_h2; i++)
                    {
                        memcpy(buf + a, pFrame->data[1] + i * pFrame->linesize[1], pixel_w2);
                        a += pixel_w2;
                    }
                    flipVertical(pixel_w2, pixel_h2, buf+_pixel_w*_pixel_h);
                    for (i = 0; i<pixel_h2; i++)
                    {
                        memcpy(buf + a, pFrame->data[2] + i * pFrame->linesize[2], pixel_w2);
                        a += pixel_w2;
                    }
                    flipVertical(pixel_w2, pixel_h2, buf+_pixel_w*_pixel_h+_pixel_w*_pixel_h/4);
                    FrameData data;
                    data.pts = pFrame->pkt_pts;
                    data.buf = buf;
                    
                    _data.push_back(data);
                    buf = NULL;
                    
                    CCLOG("pts %lld", pkt->pts);
                    CCLOG("pts %lld", pFrame->pts);
                    AVRational test;
                    test = av_get_time_base_q();
                    CCLOG("test %d, %d", test.num, test.den);
                }
            }
            
            av_packet_unref(pkt);
        }
    } while (0);
    
    _playFlag = false;
    clearBuf();
    if(pFrame)
    {
        av_frame_free(&pFrame);
        av_free(pFrame);
        delete pFrame;
        pFrame = NULL;
    }
    
    if(pkt)
    {
        av_packet_free_side_data(pkt);
        av_packet_unref(pkt);
        av_packet_free(&pkt);
    }
    
    if(video_dec_ctx)
    {
        avcodec_close(video_dec_ctx);
        video_dec_ctx = NULL;
    }
    
    if(pFormat)
    {
        avformat_close_input(&pFormat);
        avformat_free_context(pFormat);
        delete pFormat;
        pFormat = NULL;
    }
    
    return 0;
}

void LiveVideo::clearBuf()
{
    std::vector<FrameData>::iterator it;
    for(it = _data.begin(); it != _data.end(); )
    {
        if(it->buf)
        {
            delete[] it->buf;
            it->buf = NULL;
        }
        it = _data.erase(it);
    }
}

void LiveVideo::loadShader()
{
    _glProgram = new GLProgram();
    _glProgram->initWithFilenames("shader/vertexShader.vsh", "shader/fragmentShader.fsh");
    _glProgram->link();
}

void LiveVideo::loadTexture()
{
    glGenTextures(1, &_textureY);
    glBindTexture(GL_TEXTURE_2D, _textureY);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    glGenTextures(1, &_textureU);
    glBindTexture(GL_TEXTURE_2D, _textureU);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    glGenTextures(1, &_textureV);
    glBindTexture(GL_TEXTURE_2D, _textureV);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

void LiveVideo::loadRectangle()
{
    //创建vao
    glGenVertexArrays(1, &_gVAO);
    glBindVertexArray(_gVAO);
    
    //创建vbo
    glGenBuffers(1, &_gVBO);
    glBindBuffer(GL_ARRAY_BUFFER, _gVBO);
    
    //创建顶点数组
    GLfloat vertex[] = {
        //  X     Y     Z       U     V
        -1.0f, 1.0f, 0.0f,   0.0f, 1.0f,
        1.0f, 1.0f, 0.0f,   1.0f, 1.0f,
        -1.0f, -1.0f, 0.0f,   0.0f, 0.0f,
        1.0f, -1.0f, 0.0f,   1.0f, 0.0f,
    };
    
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertex), vertex, GL_STATIC_DRAW);
    
    //绑定数据到vertexIn
    glEnableVertexAttribArray(_glProgram->getAttribLocation("vertexIn"));
    glVertexAttribPointer(_glProgram->getAttribLocation("vertexIn"), 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), NULL);
    
    //绑定数据到textureIn
    glEnableVertexAttribArray(_glProgram->getAttribLocation("textureIn"));
    glVertexAttribPointer(_glProgram->getAttribLocation("textureIn"), 2, GL_FLOAT, GL_TRUE, 5 * sizeof(GLfloat), (const GLvoid*)(3 * sizeof(GLfloat)));
    
    //unbind the VAO
    glBindVertexArray(0);
}

void LiveVideo::initCommand()
{
    _command.init(_globalZOrder);
    _command.func = CC_CALLBACK_0(LiveVideo::onDraw, this);
}

void LiveVideo::draw(Renderer *render, const Mat4 &transform, uint32_t flags)
{
    
    onDraw();
}

void LiveVideo::onDraw()
{
    now = getCurrentTime();
    
    char* buf = getBuff();
    
    
    if(buf ==NULL)
    {
        return;
    }
    // clear everything
    glClearColor(0, 0, 0, 1); // black
    glClear(GL_COLOR_BUFFER_BIT);
    
    // bind the program (the shaders)
    _glProgram->use();
    
    /**
     * 为了直播临时添加的bindLiveVideoTexture2DN
     */
    GL::bindLiveVideoTexture2DN(0, _textureY);
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 0x1903,
                 _pixel_w,
                 _pixel_h,
                 0,
                 0x1903,
                 GL_UNSIGNED_BYTE,
                 buf);
    GLuint p =  glGetUniformLocation(_glProgram->getProgram(), "tex_y");
    glUniform1i(p, 0);
    
    
    GL::bindLiveVideoTexture2DN(1, _textureU);
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 0x1903,
                 _pixel_w/2,
                 _pixel_h/2,
                 0,
                 0x1903,
                 GL_UNSIGNED_BYTE,
                 buf + _pixel_w*_pixel_h);
    GLuint p1 =  glGetUniformLocation(_glProgram->getProgram(), "tex_u");
    glUniform1i(p1, 1);
    
    
    GL::bindLiveVideoTexture2DN(2, _textureV);
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 0x1903,
                 _pixel_w/2,
                 _pixel_h/2,
                 0,
                 0x1903,
                 GL_UNSIGNED_BYTE,
                 buf + _pixel_w*_pixel_h + _pixel_w*_pixel_h/4);
    GLuint p2 =  glGetUniformLocation(_glProgram->getProgram(), "tex_v");
    glUniform1i(p2, 2);
    
    
    buf = NULL;
    
    //    glProgram->setUniform("tex", 0); //set to 0 because the texture is bound to GL_TEXTURE0
    
    // bind the VAO (the triangle)
    glBindVertexArray(_gVAO);
    
    // draw the VAO
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    
    // unbind the VAO, the program and the texture
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(0);
}

char* LiveVideo::getBuff()
{
    char* buf = NULL;
    std::vector<FrameData>::iterator it;
    std::vector<FrameData>::iterator temp;
    for(it = _data.begin(); it != _data.end(); )
    {
        for(temp = _data.begin(); temp != _data.end();)
        {
            if(temp != it && temp->buf == it->buf)
            {
                CCLOG("~!!!有两个指针指向同一个地址");
                temp = _data.erase(temp);
            } else {
                ++temp;
            }
        }
        
        
        if(getDifferTime(it) < 0 && _data.size() > 1)
        {
            if(it->buf)
            {
                delete[] it->buf;
                it->buf = NULL;
            }
            it = _data.erase(it);
        }
        else
        {
            buf = it->buf;
            ++it;
            break;
        }
    }
    return buf;
}

long LiveVideo::getDifferTime(std::vector<FrameData>::iterator it)
{
    
    int64_t differTime = (it->pts - _start_time) - (now - _system_start_time);
    
    
    if(_data.size() > 40)
    {
        
        if(_data.size() <= 60)
        {
            if(_system_start_time > now - (_data.at(_data.size() - 25).pts - _start_time))
            {
                _system_start_time -= 20;
            }
            differTime = (it->pts - _start_time) - (now - _system_start_time + (_data.size()- 30)*20);
        }
        if(_data.size() > 60)
        {
            if(_system_start_time > now - (_data.at(_data.size() - 25).pts - _start_time))
            {
                _system_start_time -= 40;
            }
            differTime = (it->pts - _start_time) - (now - _system_start_time + (_data.size()- 30)*40);
        }
        
    }
    if(_data.size() > 30 && _data.size() <= 40)
    {
        differTime = (it->pts - _start_time) - (now - _system_start_time + (_data.size()- 30)*2);
    }
    
    if(_data.size() <= 15)
    {
        if(_system_start_time < now - (_data.begin()->pts - _start_time))
        {
            _system_start_time += 10;
        }
        differTime = (it->pts - _start_time) - (now - _system_start_time + (_data.size() - 15)*40);
    }
    return differTime;
}
