#include "hb_shaper.hpp"


HBShaper::HBShaper(std::string path){
    ft_face = new FT_Face;
    fontfile = path.c_str();
    FT_Init_FreeType( &library );
	ft_error = FT_New_Face( library, fontfile, 0, ft_face );/* create face object */
  	/* error handling omitted */

	if(ft_error){
		printf("error\n");
	}
    ft_error = FT_Set_Char_Size( *ft_face, 16 * 64, 0, 100, 0 );
	hb_font = hb_ft_font_create(*ft_face,NULL);

    buffer = hb_buffer_create();
}

FT_GlyphSlot HBShaper::getSlot(){
    return this->slot;
}

// Bind a texture ID(no data is copied)
uint HBShaper::getTextureId(int width,int height){
    GLuint textureId;
    glGenTextures(1, &textureId);
    GL_ERRORS();    
    glBindTexture(GL_TEXTURE_2D, textureId);
    GL_ERRORS();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    GL_ERRORS();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    GL_ERRORS();
    GL_ERRORS();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    GL_ERRORS();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    GL_ERRORS();
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_ALPHA, GL_UNSIGNED_BYTE, 0);
    GL_ERRORS();

    return textureId;
}


void HBShaper::uploadTextureData(unsigned int textureId, int width, int height, unsigned char* data){
    glBindTexture(GL_TEXTURE_2D, textureId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_ALPHA, GL_UNSIGNED_BYTE, data);
}



std::vector<std::shared_ptr<FontMesh>> HBShaper::updateGlyph(std::string text, float x, float y, float scale){
    ft_error = FT_Set_Char_Size( *ft_face, scale * 16 * 64, 0, 100, 0 );


    std::vector<std::shared_ptr<FontMesh>> meshes;

    hb_buffer_reset(buffer);

    hb_buffer_add_utf8(buffer,text.c_str(),-1,0,-1);
	hb_buffer_guess_segment_properties(buffer);
    hb_shape(this->hb_font,buffer,NULL,0);

    info = hb_buffer_get_glyph_infos(buffer,&glyph_count);
    pos = hb_buffer_get_glyph_positions(buffer,&glyph_count);

    for(uint i = 0; i < glyph_count; i++){
        auto glyph = rasterize(ft_face,info[i].codepoint);

        // This pow log... seems like some strange constraint in older version of OpenGL, which we don't have to do right now.
        int twidth = std::pow(2, std::ceil(std::log(glyph->width)/std::log(2)));
        int theight = std::pow(2, std::ceil(std::log(glyph->height)/std::log(2)));

        auto tdata = new unsigned char[twidth * theight] ();

        for(uint iy = 0; iy < glyph->height; ++iy) {
            memcpy(tdata + iy * twidth, glyph->buffer + iy * glyph->width, glyph->width);
        }

        // // There are some bugs tdata is not the same as buffer

        // for(int iy = 0; iy < theight; ++iy) {
        //     for(int ix = 0; ix < twidth; ++ix) {
        //         int c = (int) tdata[iy * twidth + ix];
        //         std::cout << (c == 255 ? '#' : '`');
        //     }
        //     std::cout << std::endl;
        // }
        // std::cout << std::endl;

        // for(uint iy = 0; iy < glyph->height; ++iy) {
        //     for(uint ix = 0; ix < glyph->width; ++ix) {
        //         int c = (int) glyph->buffer[iy * glyph->width + ix];
        //         std::cout << (c == 255 ? '#' : '`');
        //     }
        //     std::cout << std::endl;
        // }
        // std::cout << std::endl;



        // Set glyph vertices and the corresponding texure coordinate

        float s0 = 0.0;
        float t0 = 0.0;
        float s1 = (float) glyph->width / twidth;
        float t1 = (float) glyph->height / theight;
        float xa = (float) pos[i].x_advance / 64;
        float ya = (float) pos[i].y_advance / 64;
        float xo = (float) pos[i].x_offset / 64;
        float yo = (float) pos[i].y_offset / 64;
        float x0 = x + xo + glyph->bearing_x;
        float y0 = floor(y + yo + glyph->bearing_y);
        float x1 = x0 + glyph->width;
        float y1 = floor(y0 - glyph->height);

        Vertex* vertices = new Vertex[4];     
        vertices[0] = Vertex(x0,y0, s0,t0);
        vertices[1] = Vertex(x0,y1, s0,t1);
        vertices[2] = Vertex(x1,y1, s1,t1);
        vertices[3] = Vertex(x1,y0, s1,t0);

        unsigned short* indices = new unsigned short[6]; 
        indices[0] = 0; indices[1] = 1;
        indices[2] = 2; indices[3] = 0;
        indices[4] = 2; indices[5] = 3;

        auto m = std::make_shared<FontMesh>();
        m->indices = indices;
        m->textureData = tdata;
        m->textureId = getTextureId(twidth, theight);
        GL_ERRORS();    

        m->vertices = vertices;
        m->nbIndices = 6;
        m->nbVertices = 4;

        uploadTextureData(m->textureId, twidth, theight, tdata);
        GL_ERRORS();

        meshes.push_back(m);

        //Do not delete here, delete at meshes.clear()
        //delete tdata;

        x += xa;
        y += ya;
    }

    return meshes;
}

GlyphInfo::GlyphInfo(unsigned int textureID,uint width, uint height, float b_x, float b_y , unsigned char*data){
    this->textureID = textureID;
    this->width = width;
    this->height = height;
    this->bearing_x = b_x;
    this->bearing_y = b_y;
    this->buffer = data;
}


std::unique_ptr<GlyphInfo> HBShaper::rasterize(FT_Face * face, uint glyphIndex){
    auto g = std::make_unique<GlyphInfo>();

    FT_Int32 flags = FT_LOAD_DEFAULT;

    FT_Load_Glyph(*face,glyphIndex,flags);

    FT_GlyphSlot slot = (*face)->glyph;
    FT_Render_Glyph(slot,FT_RENDER_MODE_NORMAL);

    FT_Bitmap ftBitmap = slot->bitmap;


    g->buffer = ftBitmap.buffer;
    g->width = ftBitmap.width;
    g->height = ftBitmap.rows;
    g->bearing_x = slot->bitmap_left;
    g->bearing_y = slot->bitmap_top;

    return g;
}

void HBShaper::uploadMeshes(const std::vector<std::shared_ptr<FontMesh>> fontmeshes){
    for(auto fontmesh:fontmeshes){
        std::vector<float> data;

            for(uint i = 0; i < fontmesh->nbVertices; i++) {
                data.push_back(fontmesh->vertices[i].x);
                data.push_back(fontmesh->vertices[i].y);
                data.push_back(fontmesh->vertices[i].s);
                data.push_back(fontmesh->vertices[i].t);
            }

            glBindTexture(GL_TEXTURE_2D, fontmesh->textureId);

            glGenBuffers(1, &fontmesh->vertexBuffer);
            glBindBuffer(GL_ARRAY_BUFFER, fontmesh->vertexBuffer);
            glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), &data[0], GL_STATIC_DRAW); 

            glGenBuffers(1, &fontmesh->indexBuffer);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, fontmesh->indexBuffer);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, fontmesh->nbIndices * sizeof(unsigned short), fontmesh->indices, GL_STATIC_DRAW);
    }
}

void HBShaper::render(const std::vector<std::shared_ptr<FontMesh>> fontmeshes){

}