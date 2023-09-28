#define GLEW_STATIC
#include "GL.hpp"
#include <ft2build.h>
#include FT_FREETYPE_H
#include <math.h>
#include <hb.h>
#include <hb-ft.h>
#include <iostream>
#include <glm/glm.hpp>
#include <memory>
#include <vector>
#include "GLFW/glfw3.h"
#include "gl_errors.hpp"


#define FONT_SIZE 36
#define MARGIN (FONT_SIZE * .5)




/***********************************************************************
*************/

#define WIDTH   1280
#define HEIGHT  720

// Most of the frame is adapted from https://github.com/tangrams/harfbuzz-example

struct Vertex {
        float x, y;
        float s, t;

        Vertex() {}
        Vertex(float _x, float _y, float _s, float _t) : x(_x), y(_y), s(_s), t(_t) {}
};


typedef struct{
    unsigned short * indices;
    unsigned char * textureData;
    unsigned int textureId;
    Vertex* vertices;
    unsigned int nbIndices;
    unsigned int nbVertices;
    GLuint vertexBuffer;
    GLuint indexBuffer;
} FontMesh;


class GlyphInfo{

public:
    unsigned char * buffer;
    unsigned int width;
    unsigned int height;
    float bearing_x;
    float bearing_y;
    unsigned int textureID;

    GlyphInfo() = default;
    GlyphInfo(unsigned int textureID,uint width, uint height, float b_x, float b_y , unsigned char*data);
};


class HBShaper{
public:

    const char* fontfile;
    FT_Library library;
	FT_Face *ft_face;
	FT_Error ft_error;
	FT_GlyphSlot  slot;
    hb_font_t *hb_font;

    hb_buffer_t *buffer;

    unsigned int glyph_count;

    hb_glyph_info_t *info;
    hb_glyph_position_t *pos;

    HBShaper(std::string path);

    std::unique_ptr<GlyphInfo> rasterize(FT_Face * face, uint glyphIndex);

    uint getTextureId(int width,int height);
    void uploadTextureData(unsigned int textureId, int width, int height, unsigned char* data);


    FT_GlyphSlot getSlot();

    // return the glyph count
    std::vector<std::shared_ptr<FontMesh>> updateGlyph(std::string text,float,float,float = 1.0);
    void uploadMeshes(const std::vector<std::shared_ptr<FontMesh>> fontmeshes);


    // Draw using opengl
    void render(const std::vector<std::shared_ptr<FontMesh>> fontmeshes);
};