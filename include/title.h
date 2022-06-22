#ifndef __TITLE_H__
#define __TITLE_H__

#include <scene.h>

enum class TitleState {
    Startup,
    DisplayTitle
};

class TitleScene : public Scene {
public:
    TitleScene();
    // Called every frame after the scene is constructed, stops being called once it returns true
    bool load() override final;
    // Called every frame while the scene is active at a fixed 60Hz rate for logic handling
    void update() override final;
    // Called every frame while the scene is active every frame for drawing the scene contents
    void draw(bool unloading) override final;
    // Called every frame while the scene is active after graphics processing is complete
    void after_gfx() override final;
    // Called every frame while the scene is being unloaded
    void unloading_update() override final;
private:
    int title_timer_;
    TitleState title_state_;
    void* tex0_;
    void* tex1_;
};

#endif
