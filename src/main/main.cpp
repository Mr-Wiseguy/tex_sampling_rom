#include <cstring>
#include <memory>

extern "C" { 
#include <debug.h>
}
#include <main.h>
#include <title.h>
#include <gfx.h>
#include <input.h>
#include <profiling.h>
#include <platform.h>
#include <scene.h>
#include <files.h>
#include <mem.h>
#include <text.h>
#include <vassert.h>

#include <platform_gfx.h>

uint32_t g_gameTimer = 0;
uint32_t g_graphicsTimer = 0;

std::unique_ptr<Scene> cur_scene;
std::unique_ptr<Scene> loading_scene;

void start_scene_load(std::unique_ptr<Scene>&& new_scene)
{
    if (!loading_scene)
    {
        loading_scene = std::move(new_scene);
    }
}

bool is_scene_loading()
{
    return bool{loading_scene};
}

int cur_level_idx = 0;

void update()
{
    // If there's a scene that is currently loading,
    if (loading_scene)
    {
        cur_scene->unloading_update();
        bool loaded = loading_scene->load();
        if (loaded)
        {
            cur_scene = std::move(loading_scene);
        }
    }
    else
    {
        cur_scene->update();
    }
}

// #ifdef DEBUG_MODE
#define PROFILING
// #endif

void audioInit();

uint8_t odd;

int main(UNUSED int argc, UNUSED char **arg)
{
    int frame = 0;
    
    debug_printf("Main\n");

    platformInit();

    initInput();
    initGfx();
    audioInit();
    text_init();

    cur_scene = std::make_unique<TitleScene>();//std::make_unique<GameplayScene>();
    cur_scene->load();

    while (1)
    {
        odd = 0;
        profiler_frame_setup();
        // Read player input (TODO move this to another thread)
        // debug_printf("before input polling\n");
        beginInputPolling();
        // debug_printf("before start frame\n");
        startFrame();
        profiler_update(ProfilerTime::PROFILER_TIME_SETUP);
        // debug_printf("before read input\n");
        readInput();
        profiler_update(ProfilerTime::PROFILER_TIME_CONTROLLERS);

        // if (frame < 10)
        // {
        //     memset(g_InputData, 0, sizeof(g_InputData));
        // }
        update();
        g_gameTimer++;
        profiler_update(ProfilerTime::PROFILER_TIME_UPDATE);

#ifdef FPS30
        odd = 1;
        // If the game is running at 30 FPS graphics, update the scene again to have a 60Hz update rate
        beginInputPolling();
        readInput();
        profiler_update(ProfilerTime::PROFILER_TIME_CONTROLLERS2);

        // if (frame < 10)
        // {
        //     memset(g_InputData, 0, sizeof(g_InputData));
        // }
        // Update the scene
        update();
        g_gameTimer++;
        profiler_update(ProfilerTime::PROFILER_TIME_UPDATE2);
#endif
        // Draw the current scene
        cur_scene->draw(is_scene_loading());
        g_graphicsTimer++;

        profiler_update(ProfilerTime::PROFILER_TIME_GFX);
        profiler_print_times();
        endFrame();

        cur_scene->after_gfx();


        frame++;
    }
}
