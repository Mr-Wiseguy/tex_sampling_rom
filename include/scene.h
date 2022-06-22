#ifndef __SCENE_H__
#define __SCENE_H__

#include <memory>

class Scene {
public:
    virtual ~Scene() {}
    // Called every frame after the scene is constructed, stops being called once it returns true
    virtual bool load() = 0;
    // Called every frame while the scene is active at a fixed 60Hz rate for logic handling
    virtual void update() = 0;
    // Called every frame while the scene is active every frame for drawing the scene contents
    virtual void draw(bool unloading) = 0;
    // Called every frame while the scene is active after graphics processing is complete
    virtual void after_gfx() = 0;
    // Called every frame while the scene is being unloaded
    virtual void unloading_update() = 0;
    // Gets the current level's index, returns -1 unless this scene is that of an actual level
    virtual int get_level_index() { return -1; }
};

void start_scene_load(std::unique_ptr<Scene>&& new_scene);
bool is_scene_loading();

#endif
