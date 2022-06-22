#include <n64_model.h>
#include <model.h>
#include <files.h>

extern "C" {
#include <debug.h>
}

void TriangleGroup::adjust_offsets(void *base_addr)
{
    triangles = ::add_offset(triangles, base_addr);
}

void MaterialDraw::adjust_offsets(void *base_addr)
{
    groups = ::add_offset(groups, base_addr);
    for (size_t i = 0; i < num_groups; i++)
    {
        groups[i].adjust_offsets(base_addr);
    }
}

size_t MaterialDraw::gfx_length() const
{
    // Skip empty material draws (could exist for the purpose of just switching material)
    if (num_groups == 0)
    {
        return 0;
    }
    size_t num_commands = 1; // end DL
    for (size_t group_idx = 0; group_idx < num_groups; group_idx++)
    {
        num_commands += 
            1 + // vertex load
            (groups[group_idx].num_tris + 1) / 2; // tri commands, tris divided by 2 rounded up for the last 1tri
    }
    return num_commands;
}

void JointMeshLayer::adjust_offsets(void *base_addr)
{
    draws = ::add_offset(draws, base_addr);
    for (size_t draw_idx = 0; draw_idx < num_draws; draw_idx++)
    {
        draws[draw_idx].adjust_offsets(base_addr);
    }
}

size_t JointMeshLayer::gfx_length() const
{
    size_t num_commands = 0;
    for (size_t draw_idx = 0; draw_idx < num_draws; draw_idx++)
    {
        num_commands += draws[draw_idx].gfx_length();
    }
    return num_commands;
}

void Joint::adjust_offsets(void *base_addr)
{
    for (auto& layer : layers)
    {
        layer.adjust_offsets(base_addr);
    }
}

size_t Joint::gfx_length() const
{
    size_t num_commands = 0;
    for (const auto& layer : layers)
    {
        num_commands += layer.gfx_length();
    }
    return num_commands;
}

void Model::adjust_offsets()
{
    void *base_addr = this;
    // debug_printf("Adjusting offsets of model at 0x%08X\n", base_addr);
    // debug_printf("  Joints: %d Materials: %d\n", num_joints, num_materials);
    joints    = ::add_offset(joints, base_addr);
    materials = ::add_offset(materials, base_addr);
    images    = ::add_offset(images, base_addr);
    verts     = ::add_offset(verts, base_addr);
    for (size_t joint_idx = 0; joint_idx < num_joints; joint_idx++)
    {
        joints[joint_idx].adjust_offsets(base_addr);
    }
    for (size_t mat_idx = 0; mat_idx < num_materials; mat_idx++)
    {
        materials[mat_idx] = ::add_offset(materials[mat_idx], base_addr);
    }
    for (size_t img_idx = 0; img_idx < num_images; img_idx++)
    {
        images[img_idx] = ::add_offset(images[img_idx], base_addr);
    }
}

size_t Model::gfx_length() const
{
    size_t num_commands = 0;
    for (size_t joint_idx = 0; joint_idx < num_joints; joint_idx++)
    {
        num_commands += joints[joint_idx].gfx_length();
    }
    for (size_t mat_idx = 0; mat_idx < num_materials; mat_idx++)
    {
        num_commands += materials[mat_idx]->gfx_length;
    }
    return num_commands;
}

Gfx *process_texture_params(TextureParams* params, Gfx *cur_gfx, char const* const* images, int tex_index)
{
    const char* image = images[params->image_index];
    uint32_t width = params->image_width;
    uint32_t height = params->image_height;
    uint32_t format = params->image_format;
    uint32_t format_type = format >> 4;
    uint32_t format_size = format & 0b1111;
    uint32_t tmem_word_addr = params->tmem_word_address;
    uint32_t cwm = params->clamp_wrap_mirror;
    uint32_t cwm_s = cwm & 0b1111;
    uint32_t cwm_t = cwm >> 4;
    uint32_t mask_shift_s = params->mask_shift_s;
    uint32_t mask_shift_t = params->mask_shift_t;
    uint32_t mask_s = (mask_shift_s >> 4) & 0xF;
    uint32_t mask_t = (mask_shift_t >> 4) & 0xF;
    uint32_t shift_s = (mask_shift_s >> 0) & 0xF;
    uint32_t shift_t = (mask_shift_t >> 0) & 0xF;
    // uint32_t settile_bits =
    //     (cwm_t        << 18) |
    //     (mask_shift_t << 10) |
    //     (cwm_s        <<  8) |
    //     (mask_shift_s <<  0);
    // uint32_t tmem_size = width * height;

    void* image_data = get_or_load_image(image);

    // TODO clean this up
    switch (format_size)
    {
        case G_IM_SIZ_32b:
            gDPLoadMultiBlock(cur_gfx++, image_data, tmem_word_addr, tex_index, format_type, G_IM_SIZ_32b, width, height, 0, cwm_s, cwm_t, mask_s, mask_t, shift_s, shift_t);
            break;
        case G_IM_SIZ_16b:
            gDPLoadMultiBlock(cur_gfx++, image_data, tmem_word_addr, tex_index, format_type, G_IM_SIZ_16b, width, height, 0, cwm_s, cwm_t, mask_s, mask_t, shift_s, shift_t);
                break;
        case G_IM_SIZ_8b:
            gDPLoadMultiBlock(cur_gfx++, image_data, tmem_word_addr, tex_index, format_type, G_IM_SIZ_8b, width, height, 0, cwm_s, cwm_t, mask_s, mask_t, shift_s, shift_t);
            break;    
        case G_IM_SIZ_4b:
            gDPLoadMultiBlock_4b(cur_gfx++, image_data, tmem_word_addr, tex_index, format_type, width, height, 0, cwm_s, cwm_t, mask_s, mask_t, shift_s, shift_t);
            break;
    }
    // Override the pipesync with enabling textures
    gSPTexture(cur_gfx - 3, 0xFFFF, 0xFFFF, 0, tex_index, G_ON);

    // uint32_t dxt = 0;
    // uint32_t row_bytes = 0;
    // switch (format_size)
    // {
    //     case G_IM_SIZ_32b:
    //         tmem_size *= 4;
    //         row_bytes = width * 4;
    //         dxt = CALC_DXT(width, G_IM_SIZ_32b_BYTES);
    //         while (1);
    //         break;
    //     case G_IM_SIZ_16b:
    //         tmem_size *= 2;
    //         row_bytes = width * 2;
    //         dxt = CALC_DXT(width, G_IM_SIZ_16b_BYTES);
    //         break;
    //     case G_IM_SIZ_8b:
    //         tmem_size *= 1;
    //         row_bytes = width;
    //         dxt = CALC_DXT(width, G_IM_SIZ_8b_BYTES);
    //         break;
    //     case G_IM_SIZ_4b:
    //         tmem_size = round_up_divide<2>(tmem_size);
    //         row_bytes = round_up_divide<2>(width);
    //         dxt = CALC_DXT_4b(width);
    //         break;
    // }
    
    // uint32_t lines = round_up_divide<sizeof(uint64_t)>(row_bytes);

    // gDPSetTextureImage(cur_gfx++, format_type, G_IM_SIZ_16b, 1, get_or_load_image(image));
    // gDPSetTile(cur_gfx, format_type, G_IM_SIZ_16b, 0, tmem_word_addr, G_TX_LOADTILE - tex_index, 0,    0, 5, 0, 0, 5, 0);
    // cur_gfx->words.w1 |= settile_bits;
    // cur_gfx++;
    // gDPLoadSync(cur_gfx++);
    // gDPLoadBlock(cur_gfx++, G_TX_LOADTILE - tex_index, 0, 0, round_up_divide<2>(tmem_size) - 1, dxt);
    // // gDPPipeSync(cur_gfx++);
    // gSPTexture(cur_gfx++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE + tex_index, G_ON);
    // gDPSetTile(cur_gfx, format_type, format_size, lines, tmem_word_addr, G_TX_RENDERTILE + tex_index, 0,    0, 5, 0, 0, 5, 0);
    // cur_gfx->words.w1 |= settile_bits;
    // cur_gfx++;
    // gDPSetTileSize(cur_gfx++, G_TX_RENDERTILE + tex_index, 0, 0, (width - 1) << G_TEXTURE_IMAGE_FRAC, (height - 1) << G_TEXTURE_IMAGE_FRAC);

    return cur_gfx;
}

Gfx *MaterialHeader::setup_gfx(Gfx *gfx_pos, char const* const* images)
{
    Gfx* gfx_start = gfx_pos;
    const char* material_data = reinterpret_cast<const char*>(this) + sizeof(*this);
    gDPPipeSync(gfx_pos++);
    if ((flags & MaterialFlags::set_rendermode) != MaterialFlags::none)
    {
        gfx_pos->words.w0 = ((uint32_t*)material_data)[0];
        gfx_pos->words.w1 = ((uint32_t*)material_data)[1];
        material_data += sizeof(Gfx);
        gfx_pos++;
    }
    if ((flags & MaterialFlags::set_combiner) != MaterialFlags::none)
    {
        gfx_pos->words.w0 = ((uint32_t*)material_data)[0];
        gfx_pos->words.w1 = ((uint32_t*)material_data)[1];
        material_data += sizeof(Gfx);
        gfx_pos++;
    }
    if ((flags & MaterialFlags::set_env) != MaterialFlags::none)
    {
        uint32_t env = *(uint32_t*)material_data;
        gDPSetColor(gfx_pos++, G_SETENVCOLOR, env);
        material_data += sizeof(uint32_t);
    }
    if ((flags & MaterialFlags::set_prim) != MaterialFlags::none)
    {
        uint32_t prim = *(uint32_t*)material_data;
        gDPSetColor(gfx_pos++, G_SETPRIMCOLOR, prim);
        material_data += sizeof(uint32_t);
    }
    int tex_0_width = 0, tex_0_height = 0;
    int tex_1_width = 0, tex_1_height = 0;
    if ((flags & MaterialFlags::tex0) != MaterialFlags::none)
    {
        gfx_pos = process_texture_params((TextureParams*)material_data, gfx_pos, images, 0);
        tex_0_width = ((TextureParams*)material_data)->image_width;
        tex_0_height = ((TextureParams*)material_data)->image_width;
        material_data += sizeof(TextureParams);
    }
    if ((flags & MaterialFlags::tex1) != MaterialFlags::none)
    {
        gfx_pos = process_texture_params((TextureParams*)material_data, gfx_pos, images, 1);
        tex_1_width = ((TextureParams*)material_data)->image_width;
        tex_1_height = ((TextureParams*)material_data)->image_width;
        material_data += sizeof(TextureParams);
    }
    if ((flags & MaterialFlags::set_geometry_mode) != MaterialFlags::none)
    {
        uint32_t geometry_mode = *(uint32_t*)material_data;
        gSPLoadGeometryMode(gfx_pos++, geometry_mode);
        if (geometry_mode & (G_TEXTURE_GEN | G_TEXTURE_GEN_LINEAR))
        {
            if ((flags & MaterialFlags::tex0) != MaterialFlags::none)
            {
                gSPTexture(gfx_pos++, tex_0_width << 6, tex_0_height << 6, 0, G_TX_RENDERTILE + 0, G_ON);
            }
            if ((flags & MaterialFlags::tex1) != MaterialFlags::none)
            {
                gSPTexture(gfx_pos++, tex_1_width << 6, tex_1_height << 6, 0, G_TX_RENDERTILE + 1, G_ON);
            }
        }
        material_data += sizeof(uint32_t);
    }
    if ((flags & MaterialFlags::two_cycle) != MaterialFlags::none)
    {
        gDPSetCycleType(gfx_pos++, G_CYC_2CYCLE);
    }
    if ((flags & MaterialFlags::point_filter) != MaterialFlags::none)
    {
        gDPSetTextureFilter(gfx_pos++, G_TF_POINT);
    }
    gSPEndDisplayList(gfx_pos++);

    if (gfx_pos - gfx_start != gfx_length)
    {
        while (1);
    }

    return gfx_pos;
}

Gfx *MaterialDraw::setup_gfx(Gfx* gfx_pos, Vtx* verts) const
{
    // Skip empty material draws (could exist for the purpose of just switching material)
    if (num_groups == 0)
    {
        return gfx_pos;
    }
    // Iterate over every triangle group in this draw
    for (size_t group_idx = 0; group_idx < num_groups; group_idx++)
    {
        const auto& cur_group = groups[group_idx];
        // Add the triangle group's vertex load
        gSPVertex(gfx_pos++, &verts[cur_group.load.start], cur_group.load.count, cur_group.load.buffer_offset);
        // Add the triangle group's triangle commands
        for (size_t tri2_index = 0; tri2_index < cur_group.num_tris / 2; tri2_index++)
        {
            const auto& tri1 = cur_group.triangles[tri2_index * 2 + 0];
            const auto& tri2 = cur_group.triangles[tri2_index * 2 + 1];
            gSP2Triangles(gfx_pos++,
                tri1[0], tri1[1], tri1[2], 0x00,
                tri2[0], tri2[1], tri2[2], 0x00);
        }
        if (cur_group.num_tris & 1) // If odd number of tris, add the last 1tri command
        {
            const auto& tri = cur_group.triangles[cur_group.num_tris - 1];
            gSP1Triangle(gfx_pos++, tri[0], tri[1], tri[2], 0x00);
        }
    }
    // Terminate the draw's DL
    gSPEndDisplayList(gfx_pos++);
    return gfx_pos;
}

void Model::setup_gfx()
{
    // Allocate buffer to hold all joints gfx commands
    size_t num_gfx = gfx_length();
    // Use new instead of make_unique to avoid unnecessary initialization
    gfx = std::unique_ptr<Gfx[]>(new Gfx[num_gfx]);
    Gfx *cur_gfx = &gfx[0];
    // Set up the DL for every material
    for (size_t material_idx = 0; material_idx < num_materials; material_idx++)
    {
        materials[material_idx]->gfx = cur_gfx;
        cur_gfx = materials[material_idx]->setup_gfx(cur_gfx, images);
    }
    // Set up the DLs for every joint
    for (size_t joint_idx = 0; joint_idx < num_joints; joint_idx++)
    {
        for (size_t layer = 0; layer < gfx::draw_layers; layer++)
        {
            auto& cur_layer = joints[joint_idx].layers[layer];
            for (size_t draw_idx = 0; draw_idx < cur_layer.num_draws; draw_idx++)
            {
                cur_layer.draws[draw_idx].gfx = cur_gfx;
                cur_gfx = cur_layer.draws[draw_idx].setup_gfx(cur_gfx, verts);
            }
        }
    }
    // infinite loop to check if my math was wrong
    if (cur_gfx - &gfx[0] != static_cast<ptrdiff_t>(num_gfx))
    {
        while (1);
    }
}


// Test data

template<class T, size_t N>
constexpr size_t array_size(T (&)[N]) { return N; }

constexpr Vtx make_vert_normal(int16_t x, int16_t y, int16_t z, int16_t s, int16_t t, int8_t nx, int8_t ny, int8_t nz)
{
    Vtx ret {};
    ret.n.ob[0] = x;
    ret.n.ob[1] = y;
    ret.n.ob[2] = z;
    ret.n.tc[0] = s;
    ret.n.tc[1] = t;
    ret.n.n[0] = nx;
    ret.n.n[1] = ny;
    ret.n.n[2] = nz;
    return ret;
}
