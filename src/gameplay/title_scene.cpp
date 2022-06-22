#include <cmath>

#include <title.h>
#include <camera.h>
#include <mathutils.h>
#include <physics.h>
#include <ecs.h>
#include <main.h>
#include <files.h>
#include <input.h>
#include <collision.h>
#include <text.h>
#include <audio.h>
#include <platform_gfx.h>

extern "C" {
#include <debug.h>
}

TitleScene::TitleScene() : title_timer_(0), tex0_(nullptr), tex1_(nullptr)
{
}

extern u32 fillColor;

bool TitleScene::load()
{
    debug_printf("Title load\n");
    fillColor = GPACK_RGBA5551(0, 0, 0, 1) << 16 | GPACK_RGBA5551(0, 0, 0, 1);

    tex0_ = get_or_load_image("textures/first");
    tex1_ = get_or_load_image("textures/second");

    title_state_ = TitleState::Startup;
    return true;
}

void TitleScene::update()
{
    // title_timer_++;
}

void TitleScene::after_gfx()
{

}

void TitleScene::unloading_update()
{

}
