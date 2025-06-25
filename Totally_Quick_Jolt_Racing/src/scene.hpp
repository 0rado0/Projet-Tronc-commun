#pragma once


#include <mutex>
#include <thread>
#include <atomic>
#include <condition_variable>

#include "cgp/cgp.hpp"
#include "environment.hpp"
#include "driving_car.hpp"
#include "terrain.hpp"
#include "shadow_mapping.hpp"
#include "simulation.hpp"
#include "spatial_partitionning.hpp"


// This definitions allow to use the structures: mesh, mesh_drawable, etc. without mentionning explicitly cgp::
using cgp::mesh;
using cgp::mesh_drawable;
using cgp::vec3;
using cgp::numarray;
using cgp::timer_basic;

// Variables associated to the GUI (buttons, etc)
struct gui_parameters {
	bool display_frame = true;
	bool display_wireframe = false;

	int number_of_instances = 500;
	int max_number_of_instances = 200000;
	float scaling_grass = 1.0f;
};

// The structure of the custom scene
struct scene_structure : cgp::scene_inputs_generic {
	
	// ****************************** //
	// Elements and shapes of the scene
	// ****************************** //
	camera_controller_orbit_euler camera_control;
	camera_projection_perspective camera_projection;
	window_structure window;

	mesh_drawable global_frame;          // The standard global frame
	environment_structure environment;   // Standard environment controler
	input_devices inputs;                // Storage for inputs status (mouse, keyboard, window dimension)
	gui_parameters gui;                  // Standard GUI element storage
	simulation_parameter param;

	std::string weather_str;
	
	// ****************************** //
	// Elements and shapes of the scene
	// ****************************** //

	timer_basic timer;


	driving_car car;

	mesh ground_mesh;
	terrain ground_mesh_2;
	mesh_drawable ground;
	mesh_drawable ground_2;
	mesh_drawable grass;
	mesh_drawable tree;
	mesh_drawable cactus;
	mesh_drawable checkpoint;
	skybox_drawable skybox;


	shadow_mapping_structure shadow_mapping; // Helper structure to handle the shadow mapping
	mat4 view_light; // view matrix of the light.
	opengl_shader_structure shader_mesh_with_shadow; // Shader used to display the shadow on standard meshes
	opengl_shader_structure shader_depth_map; // Shader used to create the depth map
	opengl_shader_structure shader_depth_map_deformable;

	// Spatial Partitionning
	std::unique_ptr<SpatialPartitioning> spatialPartitioning;
	InstanceBuffer treeInstanceBuffer, cactusInstanceBuffer, grassInstanceBuffer, checkpointInstanceBuffer, treeAngleBuffer, cactusAngleBuffer, grassAngleBuffer, checkpointAngleBuffer;
	numarray<vec3> treeAngles, cactusAngles, grassAngles;

	float render_distance = 50.0f;
	float chunk_size = 10.0f;
	char string_cycle[32];
	char string_map[64];

	// Deformable for physics
	std::vector<bounding_box> static_boxes;


	// Global varianles for the threading
	std::mutex car_mutex;
	std::atomic<bool> stop_simulation;
	std::thread physics_thread;

	// Threading synchronisation with Global Variables Setup
	std::mutex sync_mutex;
	std::condition_variable physics_done_cv;
	bool render_ready_for_physics = true; // initialisé à true pour la première frame
	bool physics_ready_for_car_draw = false;
	// ****************************** //
	// Functions
	// ****************************** //
	scene_structure() = default;
	void initialize();    // Standard initialization to be called before the animation loop
	void getVisibleInstances(const vec3& position, float radius, std::vector<int>& visibleTreeIndices, std::vector<int>& visibleCactusIndices, std::vector<int>& visibleGrassIndices, std::vector<int>& visibleCheckpointIndices);
	void display_frame(); // The frame display to be called within the animation loop
	//void display_gui();   // The display of the GUI, also called within the animation loop
	
	void keyboard_event();
	void idle_frame();
	void process_keyboard_physics_input();
	void reset_one_frame_inputs();
	void physics_loop();

	void display_info();
	void Reading_Minimap_model();
	void Set_height_and_rotation();

	protected:
	
};





