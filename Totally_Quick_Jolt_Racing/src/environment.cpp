#include "environment.hpp"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif


std::map<char, int> KeyPressAction;
std::map<char, int> KeyJustPressed;

// Change these global values to modify the default behavior
// ************************************************************* //
// The initial zoom factor on the GUI
float project::gui_scale = 1.0f;
// Is FPS limited automatically
bool project::fps_limiting = true;
// Maximal default FPS (used only of fps_max is true)
float project::fps_max=60.0f;
// Automatic synchronization of GLFW with the vertical-monitor refresh
bool project::vsync=true;     
// Initial dimension of the OpenGL window (ratio if in [0,1], and absolute pixel size if > 1)
float project::initial_window_size_width  = 0.5f; 
float project::initial_window_size_height = 0.5f;
// ************************************************************* //


// This path will be automatically filled when the program starts
std::string project::path = ""; 
List_entity project::All_entities;
List_entity project::Tracked_entities;

std::vector<entity>::iterator List_entity::begin() { return List.begin(); }
std::vector<entity>::iterator List_entity::end() { return List.end(); }
std::vector<entity>::const_iterator List_entity::begin() const { return List.begin(); }
std::vector<entity>::const_iterator List_entity::end()   const { return List.end(); }

//Data Items
//numarray<vec3> project::GrassPositions = {{2.0f, 3.5f, 0.0f},{5.0f, 2.0f, 0.0f},{8.0f, 6.0f, 0.0f},{-20.0f, 20.0f, 0.0f}};
numarray<vec3> project::GrassPositions = {};
/*
numarray<vec3> project::TreePositions = {
	{ 1.0f, 3.5f, 0.0f }, { 4.0f, 2.0f, 0.0f }, { 7.0f, 6.0f, 0.0f }, { 10.0f, 12.0f, 0.0f },
    { -1.0f, -6.0f, 0.0f }, { -7.0f, -6.0f, 0.0f }, { 23.0f, 6.0f, 0.0f }, { -5.0f, -3.0f, 0.0f },
    { -120.0f, 45.0f, 0.0f }, { 88.0f, -76.0f, 0.0f }, { 134.0f, 123.0f, 0.0f }, { -175.0f, -190.0f, 0.0f },
    { -65.0f, 70.0f, 0.0f }, { 90.0f, -30.0f, 0.0f }, { -30.0f, 160.0f, 0.0f }, { 45.0f, -150.0f, 0.0f },
    { 110.0f, 55.0f, 0.0f }, { -140.0f, 110.0f, 0.0f }, { 200.0f, 0.0f, 0.0f }, { -200.0f, -50.0f, 0.0f },
    { 135.0f, -110.0f, 0.0f }, { 75.0f, 175.0f, 0.0f }, { -90.0f, 88.0f, 0.0f }, { 66.0f, -177.0f, 0.0f },
    { -170.0f, 33.0f, 0.0f }, { 160.0f, -25.0f, 0.0f }, { -45.0f, 145.0f, 0.0f }, { 23.0f, -190.0f, 0.0f },
    { -115.0f, -130.0f, 0.0f }, { 130.0f, -165.0f, 0.0f }, { -150.0f, 180.0f, 0.0f }, { 70.0f, 140.0f, 0.0f },
    { -25.0f, -180.0f, 0.0f }, { 195.0f, 195.0f, 0.0f }, { -195.0f, -195.0f, 0.0f }, { 60.0f, 20.0f, 0.0f },
    { -160.0f, 0.0f, 0.0f }, { 0.0f, -160.0f, 0.0f }, { 145.0f, 100.0f, 0.0f }, { -110.0f, 150.0f, 0.0f },
    { 88.0f, -120.0f, 0.0f }, { -75.0f, -95.0f, 0.0f }, { 170.0f, 40.0f, 0.0f }, { -190.0f, 80.0f, 0.0f },
    { 15.0f, 180.0f, 0.0f }, { -45.0f, -170.0f, 0.0f }, { 100.0f, -100.0f, 0.0f }, { -130.0f, 30.0f, 0.0f },
    { 155.0f, -55.0f, 0.0f }, { -60.0f, 195.0f, 0.0f }
};*/
numarray<vec3> project::TreePositions = { { 195.0f, 195.0f, 0.0f } };
numarray<vec3> project::CactusPositions = { { -195.0f, 195.0f, 0.0f } };
//numarray<vec4> project::CheckpointPositions = {{20.0f, 20.0f, 0.0f,0.0f}, {20.0f, 30.0f, 0.0f,0.0f},{25.0f, 20.0f, 0.0f,M_PI/2} };
numarray<vec4> project::CheckpointPositions = {};





void environment_structure::send_opengl_uniform(opengl_shader_structure const& shader, bool expected) const
{
	opengl_uniform(shader, "projection", camera_projection, expected);
	opengl_uniform(shader, "view", camera_view, expected);
	opengl_uniform(shader, "light", light, false);

	uniform_generic.send_opengl_uniform(shader, expected);

}

void List_entity::add_entity(entity element){List.push_back(element);}

void project::filled_Grass_entity(numarray<vec3> GrassPositions){
	for(int i = 0; i < GrassPositions.size(); i++){
		entity grass;
		grass.type = "grass";
		grass.position = GrassPositions(i);
		grass.textureID = 0;
		grass.collision = false;
		project::All_entities.add_entity(grass);
	}
}

void project::filled_Tree_entity(numarray<vec3> TreePositions, mesh& tree_base, float scaling){
	for(int i = 0; i < TreePositions.size(); i++){
		entity tree;
		tree.type = "tree";
		tree.position = TreePositions(i);
		tree.textureID = 0;
		tree.collision = true;

		bool Simplified = true; // Bounding box are narrower
		if (Simplified) {
			tree.box.p_min = tree.position;
			tree.box.p_max = tree.position;
			tree.box.p_max.z += 5.0f * scaling;         // trunk height
			tree.box.p_min.x -= scaling;         // very small for x/y
			tree.box.p_max.x += scaling;
			tree.box.p_min.y -= scaling;
			tree.box.p_max.y += scaling;

			tree.box.radius = 0.3f; // radius around the trunk
		}
		else {
			numarray<vec3> translated_points;
			for (const vec3& p : tree_base.position) {
				translated_points.push_back(scaling * p + tree.position);
			}
			// Initialize the box with these translated points
			tree.box.initialize(translated_points);
			tree.box.radius = 0.3f;
		}
		project::All_entities.add_entity(tree);
	}
}

void project::filled_Cactus_entity(numarray<vec3> CactusPositions, mesh& cactus_base, float scaling){
	for(int i = 0; i < CactusPositions.size(); i++){
		entity cactus;
		cactus.type = "cactus";
		cactus.position = CactusPositions(i);
		cactus.textureID = 0;
		cactus.collision = true;

		bool Simplified = true; // Bounding box are narrower
		if (Simplified) {
			cactus.box.p_min = cactus.position;
			cactus.box.p_max = cactus.position;
			cactus.box.p_max.z += 5.0f * scaling;         // trunk height
			cactus.box.p_min.x -= scaling;         // very small for x/y
			cactus.box.p_max.x += scaling;
			cactus.box.p_min.y -= scaling;
			cactus.box.p_max.y += scaling;

			cactus.box.radius = 0.3f; // radius around the trunk
		}
		else {
			numarray<vec3> translated_points;
			for (const vec3& p : cactus_base.position) {
				translated_points.push_back(scaling * p + cactus.position);
			}
			// Initialize the box with these translated points
			cactus.box.initialize(translated_points);
			cactus.box.radius = 0.3f;
		}
		project::All_entities.add_entity(cactus);
	}
}

void project::filled_Checkpoint_entity(numarray<vec4> CheckpointPositions, mesh& checkpoint_base, float scaling) {
	int ID_max = 0;

	auto rotate_z = [](const vec3& p, const vec3& origin, float angle) {
		float c = std::cos(angle);
		float s = std::sin(angle);
		vec3 translated = p - origin;
		vec3 rotated;
		rotated.x = c * translated.x - s * translated.y;
		rotated.y = s * translated.x + c * translated.y;
		rotated.z = translated.z;
		return rotated + origin;
		};

	for (int i = 0; i < CheckpointPositions.size(); i++) {
		entity checkpoint;
		checkpoint.type = "checkpoint";
		int ID_checkpoint = int(CheckpointPositions(i).z);
		checkpoint.position = CheckpointPositions(i).xyz();
		checkpoint.position.z = 0;
		checkpoint.zangle = CheckpointPositions(i).w;
		checkpoint.textureID = 0;
		checkpoint.collision = true;
		numarray<vec3> translated_points;

		// Angle Bounding Box Setup
		float cos_a = std::cos(-checkpoint.zangle); // CAREFUL, this is counter-clock wise
		float sin_a = std::sin(-checkpoint.zangle);
		for (const vec3& p : checkpoint_base.position) {
			// Rotate around Z-axis
			vec3 rotated;
			rotated.x = cos_a * p.x - sin_a * p.y;
			rotated.y = sin_a * p.x + cos_a * p.y;
			rotated.z = p.z;
			translated_points.push_back(scaling * rotated + checkpoint.position);
		}
		// Initialize the box with these translated points
		checkpoint.box.initialize(translated_points);
		checkpoint.box.radius = 1.6f * scaling;

		// Unrotated bounding_box to know local center before rotation
		numarray<vec3> unrotated_points;
		for (const vec3& p : checkpoint_base.position) {
			vec3 transformed = scaling * p + checkpoint.position;
			unrotated_points.push_back(transformed);
		}
		bounding_box box_local;
		box_local.initialize(unrotated_points);
		vec3 local_center1 = { box_local.p_min.x + 1.0f * scaling, box_local.p_min.y + 1.0f * scaling, 0.0f };
		vec3 local_center2 = { box_local.p_max.x - 1.0f * scaling, box_local.p_max.y - 1.0f * scaling, 0.0f };
		vec3 box_center = 0.5f * (box_local.p_min + box_local.p_max);

		// Apply rotation around the box center
		vec3 rotated_center1 = rotate_z(local_center1, box_center, -checkpoint.zangle);
		vec3 rotated_center2 = rotate_z(local_center2, box_center, -checkpoint.zangle);

		checkpoint.extra_data.push_back(rotated_center1);
		checkpoint.extra_data.push_back(rotated_center2);


		float xangle = 0;
		float yangle = 0;
		checkpoint.extra_data.push_back({xangle,yangle,checkpoint.zangle});

		int first = 0;
		if (ID_checkpoint == 0) { first = 1; }
		int last = 0;
		if (ID_checkpoint > ID_max) { ID_max = ID_checkpoint; }
		checkpoint.extra_data.push_back({ ID_checkpoint,first,last });

		project::All_entities.add_entity(checkpoint);
	}
	for (entity &e : project::All_entities.List) {
		if (e.type == "checkpoint" && e.extra_data[3].x==ID_max) {
			e.extra_data[3].z = 1;
		}
	}
}

void project::filled_L_Entity(mesh& tree_base, float scaling, mesh& cactus_base, float scaling_cactus, mesh& checkpoint_base, float scaling_checkpoint){
	filled_Checkpoint_entity(getCheckpointPositionsinit(), checkpoint_base, scaling_checkpoint);
	filled_Tree_entity(getTreePositionsinit(), tree_base, scaling);
	filled_Cactus_entity(getCactusPositionsinit(), cactus_base, scaling);
	filled_Grass_entity(getGrassPositionsinit());
}

numarray<vec3> project::getGrassPositionsinit() {
	return GrassPositions;
}

numarray<vec3> project::getTreePositionsinit() {
	return TreePositions;
}
numarray<vec3> project::getCactusPositionsinit() {
	return CactusPositions;
}

numarray<vec4> project::getCheckpointPositionsinit() {
	return CheckpointPositions;
}

numarray<vec3> getTreePositions() {
	numarray<vec3> TreePositions;
	for(entity const&e : project::All_entities.List){
		if(e.type == "tree"){
			TreePositions.push_back(e.position);
		}
	}
	return TreePositions;
}

numarray<vec3> getCactusPositions() {
	numarray<vec3> CactusPositions;
	for(entity const&e : project::All_entities.List){
		if(e.type == "cactus"){
			CactusPositions.push_back(e.position);
		}
	}
	return CactusPositions;
}

numarray<vec3> getGrassPositions() {
	numarray<vec3> GrassPositions;
	for(entity const&e : project::All_entities.List){
		if(e.type == "grass"){
			GrassPositions.push_back(e.position);
		}
	}
	return GrassPositions;
}

numarray<vec3> getCheckpointPositions() {
	numarray<vec3> CheckpointPositions;
	for (entity const&e : project::All_entities.List) {
		if (e.type == "checkpoint") {
			CheckpointPositions.push_back({ e.position });
		}
	}
	return CheckpointPositions;
}

numarray<vec3> getCheckpointAngles() {
	numarray<vec3> CheckpointAngles;
	for (entity const& e: project::All_entities.List) {
		if (e.type == "checkpoint") {
			CheckpointAngles.push_back({ e.extra_data[2]});
		}
	}
	return CheckpointAngles;
}
