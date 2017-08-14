//
//  LiveVideo.h
//  Game
//
//  Created by zhufu on 2017/3/1.
//
//

#ifndef LiveVideo_h
#define LiveVideo_h

#include <stdio.h>
#include "ui/cocosGUI.h"

USING_NS_CC;

#endif /* LiveVideo_h */
class LiveVideo : public Node
{
private:
    struct FrameData {
        int64_t pts;
        char* buf;
    };
public:
    static Scene* createScene();
    static LiveVideo* create();
    virtual bool init() override;
    void initEvents();
    int ffmpegDecode(const char* filePath);
    void flipVertical(int width, int height, char* arr);
    
    virtual void draw(Renderer* renderer, const Mat4 &transform, uint32_t flags) override;
    
    void loadShader();
    void loadTexture();
    void loadRectangle();
    void onDraw();
    void initCommand();
    long getCurrentTime();
    long getDifferTime(std::vector<FrameData>::iterator it);
    char* getBuff();
    
    void addPlayButton();
    void addPlayHKSButton();
    void addStopButton();
    void addRefreshButton();
    
    void play();
    void playHKS();
    void stop();
    void refresh();
    
    void setPlay(bool playFlag);
    bool isPlay();
    
    void clearBuf();
private:
    CustomCommand _command;
    GLProgram* _glProgram;
    bool _playFlag = false;
    GLuint _textureY;
    GLuint _textureU;
    GLuint _textureV;
    GLuint _gVAO = 0;
    GLuint _gVBO = 0;
    int _pixel_w = 320, _pixel_h = 180;
    
    std::vector<FrameData> _data;
    int _frameRate;
    
    int64_t _start_time;
    long _system_start_time;
    char* currentLivePath = new char[256];
    
    long now;
};
