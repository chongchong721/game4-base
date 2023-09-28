#include "PlayMode.hpp"

#include "LitColorTextureProgram.hpp"
#include "TextureProgram.hpp"

#include "read_write_chunk.hpp"

#include "DrawLines.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <random>
#include <fstream>
#include <algorithm>

GLuint hexapod_meshes_for_lit_color_texture_program = 0;
Load< MeshBuffer > hexapod_meshes(LoadTagDefault, []() -> MeshBuffer const * {
	MeshBuffer const *ret = new MeshBuffer(data_path("hexapod.pnct"));
	hexapod_meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret;
});

Load< Scene > hexapod_scene(LoadTagDefault, []() -> Scene const * {
	return new Scene(data_path("hexapod.scene"), [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name){
		Mesh const &mesh = hexapod_meshes->lookup(mesh_name);

		scene.drawables.emplace_back(transform);
		Scene::Drawable &drawable = scene.drawables.back();

		drawable.pipeline = lit_color_texture_program_pipeline;

		drawable.pipeline.vao = hexapod_meshes_for_lit_color_texture_program;
		drawable.pipeline.type = mesh.type;
		drawable.pipeline.start = mesh.start;
		drawable.pipeline.count = mesh.count;

	});
});

Load< Sound::Sample > dusty_floor_sample(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("dusty-floor.opus"));
});

bool compareByNo(StoryState &a, StoryState &b){
	return a.state_No < b.state_No;
}

PlayMode::PlayMode() : scene(*hexapod_scene) {
	// Initialize story state
	std::ifstream in(data_path("metadata"));
	read_chunk<StateMetadata>(in,"stmd",&state_metadata);

	// bad hardcode
	in = std::ifstream(data_path("statename"));
	//uint32_t state_name_length = 341;
	std::vector<char> state_name;
	read_chunk<char>(in,"stat",&state_name);

	//uint32_t story_length = 2307;
	in = std::ifstream(data_path("story"));
	std::vector<char> stories;
	read_chunk<char>(in,"stor",&stories);

	//uint32_t offset_length = 19;
	in = std::ifstream(data_path("tostate"));
	std::vector<uint32_t> to_states;
	read_chunk<uint32_t>(in,"to00",&to_states);


	auto story_data = stories.data();
	auto state_name_data = state_name.data();
	auto tostate_data = to_states.data();

	for(uint i = 0; i < state_metadata.size();i++){
		story_state.emplace_back();
		auto &current_state = story_state.back();
		current_state.state_No = state_metadata[i].state_NO;

		auto offset = state_metadata[i].name_offset;
		auto length = state_metadata[i].name_length;

		current_state.state_name = std::string(state_name_data + offset,length);

		offset = state_metadata[i].story_offset;
		length = state_metadata[i].story_length;

		current_state.story_string = std::string(story_data + offset,length);

		//std::cout << current_state.story_string << std::endl;

		offset = state_metadata[i].to_state_offset;
		length = state_metadata[i].to_state_num;

		for (uint32_t i = 0; i < length; i ++){
			uint32_t idx = *(tostate_data + offset + i);
			current_state.to_state.emplace_back(idx);
		}
	}

	std::sort(story_state.begin(),story_state.end(),compareByNo);


	current_state = story_state[start_state_no];



	//get pointers to leg for convenience:
	for (auto &transform : scene.transforms) {
		if (transform.name == "Hip.FL") hip = &transform;
		else if (transform.name == "UpperLeg.FL") upper_leg = &transform;
		else if (transform.name == "LowerLeg.FL") lower_leg = &transform;
	}
	if (hip == nullptr) throw std::runtime_error("Hip not found.");
	if (upper_leg == nullptr) throw std::runtime_error("Upper leg not found.");
	if (lower_leg == nullptr) throw std::runtime_error("Lower leg not found.");

	hip_base_rotation = hip->rotation;
	upper_leg_base_rotation = upper_leg->rotation;
	lower_leg_base_rotation = lower_leg->rotation;

	//get pointer to camera for convenience:
	if (scene.cameras.size() != 1) throw std::runtime_error("Expecting scene to have exactly one camera, but it has " + std::to_string(scene.cameras.size()));
	camera = &scene.cameras.front();

	//start music loop playing:
	// (note: position will be over-ridden in update())
	//leg_tip_loop = Sound::loop_3D(*dusty_floor_sample, 1.0f, get_leg_tip_position(), 10.0f);

	//Initialize hb/ft shaper
	current_shaper = new HBShaper(data_path("Mooli-Regular.ttf"));
}

PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	if (evt.type == SDL_KEYDOWN) {
		if (evt.key.keysym.sym == SDLK_ESCAPE) {
			SDL_SetRelativeMouseMode(SDL_FALSE);
			return true;
		} else if (evt.key.keysym.sym == SDLK_1) {
			n1.downs += 1;
			n1.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_2) {
			n2.downs += 1;
			n2.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_4) {
			n4.downs += 1;
			n4.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_3) {
			n3.downs += 1;
			n3.pressed = true;
			return true;
		}
	} else if (evt.type == SDL_KEYUP) {
		if (evt.key.keysym.sym == SDLK_1) {
			n1.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_2) {
			n2.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_4) {
			n4.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_3) {
			n3.pressed = false;
			return true;
		}
	} 

	return false;
}

void PlayMode::update(float elapsed) {

	

	//move sound to follow leg tip position:
	//leg_tip_loop->set_position(get_leg_tip_position(), 1.0f / 60.0f);

	{
		// This means the key is up but it has been pressed and we did not handle it. Obviously There could be bug if you press multiple keys. But...
		if (!n1.pressed && n1.downs >0){
			n1.downs = 0;
			enter_next_state(0);
		} 
		if (!n2.pressed && n2.downs >0){
			n2.downs = 0;
			enter_next_state(1);
		} 
		if (!n3.pressed && n3.downs >0){
			n3.downs = 0;
			enter_next_state(2);
		} 
		if (!n4.pressed && n4.downs >0){
			n4.downs = 0;
			enter_next_state(3);
		} 
		

	}
	// Need to delete
	for( auto fontmesh:fontmeshes ){
		delete fontmesh->indices;
		delete fontmesh->textureData;
		delete fontmesh->vertices;
	}
	fontmeshes.clear();
	
	static uint32_t x_left_alignment = 160u;
	std::vector<std::pair<uint32_t,uint32_t>> text_location;

	auto story_to_draw = current_state.return_main_story();
	auto nstoryline = story_to_draw.size();
	uint32_t last_line_location = 0;
	for (uint i = 0; i < nstoryline; i++){
		text_location.emplace_back(std::make_pair(x_left_alignment,600 - i*50));
		last_line_location = 600 - i * 50;
	}

	auto option_to_draw = current_state.return_options(story_state,role);
	auto noptionline = option_to_draw.size();
	for (uint i = 0; i < noptionline; i++){
		text_location.emplace_back(std::make_pair(x_left_alignment,last_line_location - 100 - i*50));
	}

	//https://stackoverflow.com/questions/201718/concatenating-two-stdvectors
	story_to_draw.insert(story_to_draw.end(),std::make_move_iterator(option_to_draw.begin()),std::make_move_iterator(option_to_draw.end()));

	// Update fontmeshes
	uint i = 0;
	for (auto text : story_to_draw){
		auto part_meshes = current_shaper->updateGlyph(text,text_location[i].first,text_location[i].second,1.5);
		fontmeshes.insert(fontmeshes.end(),part_meshes.begin(),part_meshes.end());
		++i;
	}


	//reset button press counters:
	// n1.downs = 0;
	// n2.downs = 0;
	// n4.downs = 0;
	// n3.downs = 0;
}

void PlayMode::enter_next_state(uint32_t idx){
	auto to_state = current_state.to_state;
	auto length = to_state.size();
	if (current_state.state_No != check_state){
		if(length < idx + 1){
			return;
		}

		auto next_state_id = to_state[idx];
		auto next_state = story_state[next_state_id];

		// This is another hardcode..
		if (next_state.state_name == "You're a thief"){
			role = THIEF;
		}else if(next_state.state_name == "You're a magician"){
			role = MAGE;
		}

		current_state = next_state;

	}else{//hardcode special case..
		if(length < idx+2){
			return;
		}else{
			if(idx == 0){
				auto next_state_id = to_state[idx];
				auto next_state = story_state[next_state_id];
				current_state = next_state;
			}else{
				if(role == MAGE){
					auto next_state = story_state[m_do_state];
					current_state = next_state;
				}else if(role == THIEF){
					auto next_state = story_state[t_do_state];
					current_state = next_state;
				}
			}
		}
		
		
	}


}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	// //update camera aspect ratio for drawable:
	// camera->aspect = float(drawable_size.x) / float(drawable_size.y);

	// //set up light type and position for lit_color_texture_program:
	// // TODO: consider using the Light(s) in the scene to do this
	// glUseProgram(lit_color_texture_program->program);
	// glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 1);
	// glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 0.0f,-1.0f)));
	// glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 0.95f)));
	// glUseProgram(0);

	// glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	// glClearDepth(1.0f); //1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
	// glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// glEnable(GL_DEPTH_TEST);
	// glDepthFunc(GL_LESS); //this is the default depth comparison function, but FYI you can change it.

	// scene.draw(*camera);

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f );
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	draw_fontmesh(fontmeshes);
	//draw_text(text,500,500,1.0);

	// { //use DrawLines to overlay some text:
	// 	glDisable(GL_DEPTH_TEST);
	// 	float aspect = float(drawable_size.x) / float(drawable_size.y);
	// 	DrawLines lines(glm::mat4(
	// 		1.0f / aspect, 0.0f, 0.0f, 0.0f,
	// 		0.0f, 1.0f, 0.0f, 0.0f,
	// 		0.0f, 0.0f, 1.0f, 0.0f,
	// 		0.0f, 0.0f, 0.0f, 1.0f
	// 	));

	// 	constexpr float H = 0.09f;
	// 	lines.draw_text("Mouse motion rotates camera; WASD moves; escape ungrabs mouse",
	// 		glm::vec3(-aspect + 0.1f * H, -1.0 + 0.1f * H, 0.0),
	// 		glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
	// 		glm::u8vec4(0x00, 0x00, 0x00, 0x00));
	// 	float ofs = 2.0f / drawable_size.y;
	// 	lines.draw_text("Mouse motion rotates camera; WASD moves; escape ungrabs mouse",
	// 		glm::vec3(-aspect + 0.1f * H + ofs, -1.0 + + 0.1f * H + ofs, 0.0),
	// 		glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
	// 		glm::u8vec4(0xff, 0xff, 0xff, 0x00));
	// }


	GL_ERRORS();

	

}

glm::vec3 PlayMode::get_leg_tip_position() {
	//the vertex position here was read from the model in blender:
	return lower_leg->make_local_to_world() * glm::vec4(-1.26137f, -11.861f, 0.0f, 1.0f);
}


void PlayMode::draw_fontmesh(std::vector<std::shared_ptr<FontMesh>> fontmeshes){
	//----------------------------------------------
	//based -- in part -- on code from LitColorTextureProgram

	auto &program = texture_program;

	// For each glyph, we treat it as two triangles. use gl_DrawElements

	glUseProgram(program->program);

	static glm::mat4 projection = glm::ortho(0.0f,1280.0f,0.0f,720.0f);
	glUniformMatrix4fv(program->PROJECTION_mat,1,GL_FALSE,glm::value_ptr(projection));
	current_shaper->uploadMeshes(fontmeshes);

	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	GL_ERRORS();

	for(auto fontmesh:fontmeshes){

		// Need to create this vertex array or VertexAttribPointer will return error
		// https://stackoverflow.com/questions/21652546/what-is-the-role-of-glbindvertexarrays-vs-glbindbuffer-and-what-is-their-relatio
		GLuint array;
		glGenVertexArrays(1,&array);
		glBindVertexArray(array);

		glBindBuffer(GL_ARRAY_BUFFER, fontmesh->vertexBuffer);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, fontmesh->indexBuffer);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, fontmesh->textureId);
		GL_ERRORS();

		glVertexAttribPointer(program->positionLoc, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
		GL_ERRORS();

		glEnableVertexAttribArray(program->positionLoc);
		GL_ERRORS();

		glVertexAttribPointer(program->uvLoc, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (const GLvoid*)(2 * sizeof(float)));
		GL_ERRORS();

		glEnableVertexAttribArray(program->uvLoc);
		GL_ERRORS();

		glDrawElements(GL_TRIANGLES, fontmesh->nbIndices, GL_UNSIGNED_SHORT, 0);
		GL_ERRORS();
	}



	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
	GL_ERRORS();

	glBindTexture(GL_TEXTURE_2D, 0);

	glUseProgram(0);

	GL_ERRORS();
}


void PlayMode::draw_text(std::string text,float x, float y, float scale){
//----------------------------------------------
	//based -- in part -- on code from LitColorTextureProgram

	auto &program = texture_program;

	// For each glyph, we treat it as two triangles. use gl_DrawElements

	glUseProgram(program->program);

	static glm::mat4 projection = glm::ortho(0.0f,1280.0f,0.0f,720.0f);
	glUniformMatrix4fv(program->PROJECTION_mat,1,GL_FALSE,glm::value_ptr(projection));


	// Serious memory leak here. Maybe should use valgrind to check..

	auto fontmeshes = current_shaper->updateGlyph(text,x,y,3.0);

	current_shaper->uploadMeshes(fontmeshes);

	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	GL_ERRORS();

	for(auto fontmesh:fontmeshes){

		// Need to create this vertex array or VertexAttribPointer will return error
		// https://stackoverflow.com/questions/21652546/what-is-the-role-of-glbindvertexarrays-vs-glbindbuffer-and-what-is-their-relatio
		GLuint array;
		glGenVertexArrays(1,&array);
		glBindVertexArray(array);

		glBindBuffer(GL_ARRAY_BUFFER, fontmesh->vertexBuffer);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, fontmesh->indexBuffer);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, fontmesh->textureId);
		GL_ERRORS();

		glVertexAttribPointer(program->positionLoc, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
		GL_ERRORS();

		glEnableVertexAttribArray(program->positionLoc);
		GL_ERRORS();

		glVertexAttribPointer(program->uvLoc, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (const GLvoid*)(2 * sizeof(float)));
		GL_ERRORS();

		glEnableVertexAttribArray(program->uvLoc);
		GL_ERRORS();

		glDrawElements(GL_TRIANGLES, fontmesh->nbIndices, GL_UNSIGNED_SHORT, 0);
		GL_ERRORS();
	}



	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
	GL_ERRORS();

	glBindTexture(GL_TEXTURE_2D, 0);

	glUseProgram(0);

	GL_ERRORS();
}