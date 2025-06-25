#pragma once

#include "cgp/cgp.hpp"

using namespace cgp;

extern std::map<char, int> KeyPressAction;
extern std::map<char, int> KeyJustPressed;

// ********************************************************************* //
// This file contains variables defining your global scene environment.
// ********************************************************************* //


// An environment structure contains variables that are needed to the drawing of an element in the scene, but that are not related to a particular shape or mesh.
// The environment contains typically the camera and the light.
struct environment_structure : environment_generic_structure
{
	// Color of the background of the scene
	vec3 background_color = {1,1,1}; // Used in the main program

	// The position/orientation of a camera that can rotates freely around a specific position
	mat4 camera_view;

	// A projection structure (perspective or orthogonal projection)
	mat4 camera_projection;

	// The position of a light
	vec3 light = {1,1,1};

	// Additional uniforms that can be attached to the environment if needed (empty by default)
	uniform_generic_structure uniform_generic;


	// This function will be called in the draw() call of a drawable element.
	//  The function is expected to send the uniform variables to the shader (e.g. camera, light)
	void send_opengl_uniform(opengl_shader_structure const& shader, bool expected = default_expected_uniform) const override;


};

struct entity {
	std::string type;
	int Id;
	cgp::vec3 position;
	GLuint textureID;
	bounding_box box;
	bool collision;
	std::vector<vec3> extra_data;
	float zangle;
	bool tracked = false;
};
																									
struct List_entity {
	std::vector<entity> List;
	void add_entity(entity element);
	
	std::vector<entity>::iterator begin();
	std::vector<entity>::iterator end();
	std::vector<entity>::const_iterator begin() const;
	std::vector<entity>::const_iterator end()   const;
};



// Global variables storing general information on your project
// Note: the default values are in the file environment.cpp
struct project {

	// Global variable storing the relative path to the root of the project (access to shaders/, assets/, etc)
	//  Accessible via project::path
	static std::string path;

	// ImGui Window Scale: change this value (default=1) for larger/smaller gui window
	static float gui_scale;

	// Window refresh rate
	static bool fps_limiting; // Is FPS limited automatically
	static float fps_max; // Maximal default FPS (used only of fps_max is true)
	static bool vsync; // Automatic synchronization of GLFW with the vertical-monitor refresh

	// Initial window size: expressed as ratio of screen in [0,1], or absolute pixel value if > 1
	static float initial_window_size_width;
	static float initial_window_size_height;

	static List_entity All_entities;
	static List_entity Tracked_entities;
	static numarray<vec3> GrassPositions; 
	static numarray<vec3> TreePositions;
	static numarray<vec3> CactusPositions;
	static numarray<vec4> CheckpointPositions;
	static void filled_Grass_entity(numarray<vec3> GrassPositions);
	static void filled_Tree_entity(numarray<vec3> TreePositions, mesh& tree_base, float scaling);
	static void filled_Cactus_entity(numarray<vec3> CactusPositions, mesh& cactus_base, float scaling);
	static void filled_Checkpoint_entity(numarray<vec4> CheckpointPositions, mesh& checkpoint_base, float scaling);
	static void filled_L_Entity(mesh& tree_base, float scaling_tree, mesh& cactus_base, float scaling_cactus,mesh& checkpoint_base, float scaling_checkpoint);

	static numarray<vec3> getGrassPositionsinit();
	static numarray<vec3> getTreePositionsinit();
	static numarray<vec3> getCactusPositionsinit();
	static numarray<vec4> getCheckpointPositionsinit();

};
numarray<vec3> getTreePositions();
numarray<vec3> getCactusPositions();
numarray<vec3> getGrassPositions();
numarray<vec3> getCheckpointPositions();
numarray<vec3> getCheckpointAngles();


const std::vector < std::pair<int, char>> supported_keys = {
	{GLFW_KEY_W,'Z'}, {GLFW_KEY_A,'Q'}, {GLFW_KEY_S,'S'}, {GLFW_KEY_D,'D'},
	{GLFW_KEY_J,'J'}, {GLFW_KEY_K,'K'},
	{GLFW_KEY_P,'P'}, {GLFW_KEY_V,'V'}, {GLFW_KEY_F,'F'}, {GLFW_KEY_Q,'A'}, {GLFW_KEY_E,'E'}
};




