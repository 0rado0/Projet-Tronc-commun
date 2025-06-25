#pragma once


#include "cgp/cgp.hpp"

// This definitions allow to use the structures: mesh, mesh_drawable, etc. without mentionning explicitly cgp::
using cgp::mesh;
using cgp::mesh_drawable;
using cgp::vec3;
using cgp::numarray;
using cgp::timer_basic;

//Variables associated to the GUI (buttons, etc)
struct gui_parameters_mini_map {
	bool display_frame = true;
	bool display_wireframe = false;
};

// A 2D map for the display of the mini map and the map information
struct map : cgp::scene_inputs_generic {
	
public:
	// ****************************** //
	// Elements and shapes of the scene
	// ****************************** //
	cgp::window_structure window;
	GLuint textureMap = 0;
	GLuint textureCursor = 0;

	// Minimap elements
	GLuint minimapVAO = 0;
	GLuint minimapVBO = 0;
	GLuint minimapEBO = 0;
	GLuint shader_minimap = 0;
	GLuint pin_texture = 0;

	int mapWidth, mapHeight, channels;
	int imgHeight = 0;
	float scale;
	float correct_scale = 65.0f / 12.0f * 100; // Circle test: 12 when (scale is 1/100) according to minimap, while currently it is 65 on real world
	float size = 1.0f;
	int width, height;

	mesh_drawable global_frame;          // The standard global frame
	cgp::environment_generic_structure environment;   // Standard environment controler
	gui_parameters_mini_map gui;                  // Standard GUI element storage
	std::vector<std::vector<cgp::vec3>> minimap_color_matrix;
	
	GLuint LoadShader(const char* vertexPath, const char* fragmentPath);
	void initMinimapFBO(const char* vertexPath, const char* fragmentPath); // Initialize the FBO Circular Mini map
	void loadTexture(const char* path, GLuint& textureID, bool reverse = false);
	void fill_minimap_color();
	ImVec2 worldToMinimap(const cgp::vec2& pos, float refX, float refY, float refAngle, ImVec2 center, float size, float zoom) const;
	ImVec2 Minimaptoworld(const cgp::vec2& pos, float size) const;
	cgp::vec3 get_minimap_color_at(const cgp::vec2& world_pos) const;
	void drawRotatedMap(int xcenter, int ycenter, int radius, float xfocus, float yfocus, float zangle, float scale, float zoom);
	void renderMinimap(float carX, float carY, float carAngle, int screenwidth, int screenheight);

};





