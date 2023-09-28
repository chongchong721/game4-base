#include "TextureProgram.hpp"

#include "gl_compile_program.hpp"
#include "gl_errors.hpp"

Scene::Drawable::Pipeline texture_program_pipeline;

Load< TextureProgram > texture_program(LoadTagEarly, []() -> TextureProgram const * {
	TextureProgram *ret = new TextureProgram();

	//----- build the pipeline template -----
	texture_program_pipeline.program = ret->program;

	//texture_program_pipeline.OBJECT_TO_CLIP_mat4 = ret->OBJECT_TO_CLIP_mat4;

	//make a 1-pixel white texture to bind by default:
	GLuint tex;
	glGenTextures(1, &tex);

	glBindTexture(GL_TEXTURE_2D, tex);
	std::vector< glm::u8vec4 > tex_data(1, glm::u8vec4(0xff));
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, tex_data.data());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glBindTexture(GL_TEXTURE_2D, 0);


	texture_program_pipeline.textures[0].texture = tex;
	texture_program_pipeline.textures[0].target = GL_TEXTURE_2D;

	return ret;
});

//try to modify the texture program
//https://learnopengl.com/In-Practice/Text-Rendering
//https://github.com/tangrams/harfbuzz-example/blob/master/src/glutils.h


TextureProgram::TextureProgram() {
	//Compile vertex and fragment shaders using the convenient 'gl_compile_program' helper function:
	program = gl_compile_program(
		//vertex shader:
		"#version 330\n"
		"uniform mat4 projection;\n"
		"in vec4 a_position;\n"
        "in vec2 a_uv;\n"
        "out vec2 f_uv;\n"
        "void main() {\n"
        "	gl_Position = projection * a_position;\n"
        "	f_uv = a_uv;\n"
        "}\n",

		// Fragment
		"#version 330\n"
		"out vec4 fragColor;\n"
		"uniform sampler2D u_tex;\n"
        "in vec2 f_uv;\n"
        "void main() {\n"
        "    fragColor = vec4(1.0,1.0,1.0, texture(u_tex, f_uv).w);\n"
        "}\n"
		
	);
	//As you can see above, adjacent strings in C/C++ are concatenated.
	// this is very useful for writing long shader programs inline.

	//look up the locations of vertex attributes:
	//Vertex_vec4 = glGetAttribLocation(program, "vertex");
	positionLoc = glGetAttribLocation(program,"a_position");
	uvLoc = glGetAttribLocation(program,"a_uv");

	//look up the locations of uniforms:
	PROJECTION_mat = glGetUniformLocation(program,"projection");
	GLuint TEX_sampler2D = glGetUniformLocation(program, "u_tex");


	//set TEX to always refer to texture binding zero:
	glUseProgram(program); //bind program -- glUniform* calls refer to this program now


	glUniform1i(TEX_sampler2D, 0); //set TEX to sample from GL_TEXTURE0

	glUseProgram(0); //unbind program -- glUniform* calls refer to ??? now
}

TextureProgram::~TextureProgram() {
	glDeleteProgram(program);
	program = 0;
}

