#pragma once
#include "Renderer.hpp"
#include "SceneSprites.hpp"

namespace Dialogue {
inline void Draw(int page,float time,float w,float h) {
    using namespace Renderer;
    LocoData::Color4 white{255,255,255,255};
    DrawScreenRect(0,0,w,h*0.07f,{0,0,0,255});
    DrawScreenRect(0,h*0.91f,w,h*0.09f,{0,0,0,255});
    float cx=w*.77f,cy=h*.49f,rx=w*.22f,ry=h*.31f;
    DrawScreenEllipse(cx,cy,rx,ry,white);
    DrawScreenEllipse(w*.77f,h*.79f,w*.025f,h*.045f,white);
    DrawScreenEllipse(w*.80f,h*.855f,w*.014f,h*.016f,white);
    DrawScreenEllipse(w*.83f,h*.878f,w*.009f,h*.011f,white);
    const Sprite* texts[]={&SceneSprites::TEXT0,&SceneSprites::TEXT1,&SceneSprites::TEXT2,&SceneSprites::TEXT3};
    DrawScreenSprite(*texts[page],cx,cy,rx*1.78f,rx*1.78f*.4f);
    for(int i=0;i<4;++i) {
        float x=(i<3?.09f+i*.115f:.89f)*w;
        const auto& mui=i==3?SceneSprites::MUI_SMILE:SceneSprites::MUI;
        float height=h*.22f,width=height*mui.width/mui.height;
        DrawScreenSprite(mui,x,h*.925f,width,height,white,std::sin(time*2+i)*.025f);
    }
    DrawScreenText(w*.54f,h*.956f,"A / 2",{242,181,37,255},1.1f);
}
inline void DrawHint(int id,float time,float w,float h){
    using namespace Renderer;Draw(0,time,w,h);
    float cx=w*.77f,cy=h*.49f,rx=w*.22f;DrawScreenEllipse(cx,cy,rx,h*.31f,{255,255,255,255});
    const Sprite* hints[]={&SceneSprites::HINT1,&SceneSprites::HINT2,&SceneSprites::HINT3,&SceneSprites::HINT4,&SceneSprites::HINT5,&SceneSprites::HINT6};
    if(id>=1 && id<=6)DrawScreenSprite(*hints[id-1],cx,cy,rx*1.78f,rx*1.78f*.4f);
}
inline void DrawReward(int page,float time,float w,float h) {
    using namespace Renderer;
    LocoData::Color4 white{255,255,255,255};
    Draw(0,time,w,h);
    float cx=w*.77f,cy=h*.49f,rx=w*.22f;
    DrawScreenEllipse(cx,cy,rx,h*.31f,white);
    DrawScreenSprite(SceneSprites::MUI_REWARD_TEXT,cx,cy,rx*1.75f,rx*1.75f*.4f);
    if(page==0){
        float x=w*.5f,y=h*.77f;
        DrawScreenEllipse(x,y,w*.105f,h*.095f,white);
        for(int i=0;i<9;++i){float a=i*2*M_PI/9;DrawScreenCircle(x+std::cos(a)*w*.085f,y+std::sin(a)*h*.07f,w*.023f,white);}
        DrawScreenSprite(SceneSprites::MUI_EGG,x,y-h*.12f,h*.22f,h*.22f);
    } else {
        DrawScreenRect(w*.25f,h*.78f,w*.62f,h*.13f,{255,218,173,255});
        DrawScreenSprite(SceneSprites::HOUSE_PART9,w*.25f,h*.785f,w*.15f,w*.15f*144/128);
        DrawScreenSprite(SceneSprites::HOUSE_PART_TITLE,w*.60f,h*.845f,w*.46f,w*.46f/8);
    }
    for(int i=0;i<4;++i){float x=(i<3?.09f+i*.115f:.89f)*w;
        const auto& mui=i==3?SceneSprites::MUI_SMILE:SceneSprites::MUI;
        float height=h*.22f;DrawScreenSprite(mui,x,h*.925f,height*mui.width/mui.height,height);}
}
}
