#include "Mode.hpp"

#include "Scene.hpp"
#include "Sound.hpp"
#include "hb_shaper.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>


const uint32_t change_line = 50;

typedef enum{
	NONE,
	MAGE,
	THIEF
}Role;




struct StateMetadata{
    uint32_t state_NO;
    uint32_t name_length;
    uint32_t name_offset;
    uint32_t story_length;
    uint32_t story_offset;
    uint32_t to_state_num;
    uint32_t to_state_offset;
};



class StoryState{
public:
    uint32_t state_No;

    std::string state_name;
    std::string story_string;

    std::vector<uint32_t> to_state;

	std::vector<std::string> return_main_story(){
		auto length = story_string.size();
		std::vector<std::string> ret;
		for( uint32_t length_handled = 0; length_handled < length;){
			if (length_handled + change_line < length){
				auto nearest_space_idx = story_string.rfind(" ", length_handled + change_line);
				if (nearest_space_idx != std::string::npos){
					if (nearest_space_idx <= length){
						std::string line = std::string(story_string.begin()+length_handled,story_string.begin()+nearest_space_idx);
						length_handled  = nearest_space_idx;
						ret.push_back(line);
					}else{
						std::string line = std::string(story_string.begin()+length_handled,story_string.end());
						length_handled = length;
						// Over
						ret.push_back(line);
					}
					
				}else{
					if (length_handled + change_line <= length){
						std::string line = std::string(story_string.begin()+length_handled,story_string.begin()+length_handled+change_line);
						length_handled += change_line;
						ret.push_back(line);
					}else{
						std::string line = std::string(story_string.begin()+length_handled,story_string.end());
						length_handled = length;
						// Over
						ret.push_back(line);
					}
				}
			}else{//last iteration, dont care space
				std::string line = std::string(story_string.begin() + length_handled,story_string.end());
				length_handled = length;
				ret.push_back(line);
			}
			

		}
		return ret;
	}


	std::vector<std::string> return_options(std::vector<StoryState> story_state, Role role){
		// Need to hardcode the role...
		if (state_No == 1 ){
			std::vector<std::string> ret;

			auto option0 = std::to_string(1) + ". " + story_state[to_state[0]].state_name;
			ret.push_back(option0);

			if(role == MAGE){
				auto option1 = std::to_string(2) + ". " + story_state[to_state[2]].state_name;
				option1 = std::string(option1.begin(),option1.end()-1);
				ret.push_back(option1);
			}else if(role == THIEF){
				auto option1 = std::to_string(2) + ". " + story_state[to_state[1]].state_name;
				option1 = std::string(option1.begin(),option1.end()-1);
				ret.push_back(option1);
			}

			return ret;
		}else{
			std::vector<std::string> ret;
			for(uint32_t i = 0; i < to_state.size();i++){
				auto _to_state = story_state[to_state[i]];
				auto option = std::to_string(i+1) +". " + _to_state.state_name;
				ret.push_back(option);
			}
			return ret;
		}

	}
};

struct PlayMode : Mode {
	PlayMode();
	virtual ~PlayMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

	void draw_text(std::string text, float x, float y, float scale);
	void draw_fontmesh(std::vector<std::shared_ptr<FontMesh>>);

	//----- game state -----

	void enter_next_state(uint32_t option_idx);

	//input tracking:
	struct Button {
		uint8_t downs = 0;
		uint8_t pressed = 0;
	} n1, n2, n3, n4;

	//local copy of the game scene (so code can change it during gameplay):
	Scene scene;

	//hexapod leg to wobble:
	Scene::Transform *hip = nullptr;
	Scene::Transform *upper_leg = nullptr;
	Scene::Transform *lower_leg = nullptr;
	glm::quat hip_base_rotation;
	glm::quat upper_leg_base_rotation;
	glm::quat lower_leg_base_rotation;
	float wobble = 0.0f;


	
	//camera:
	Scene::Camera *camera = nullptr;

	//shaper
	HBShaper *current_shaper;

	// Font Meshes
	std::vector<std::shared_ptr<FontMesh>> fontmeshes;


	// Storys
	std::vector<StoryState> story_state;
	std::vector<StateMetadata> state_metadata;

	StoryState current_state;
	Role role = NONE;		

	uint32_t start_state_no = 6;
	std::vector<uint32_t> end_state_no{7,8};

	// Hardcode this if-branch. No time to make a elegant implementation..
	uint32_t check_state = 1;
	uint32_t t_do_state = 2;
	uint32_t m_do_state = 3;

};
