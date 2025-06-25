#include "scene.hpp"
#include "environment.hpp"
#include <chrono>
#include <random>
#include <condition_variable>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

using namespace cgp;



mat4 create_view_matrix(vec3 eye, vec3 center, vec3 up)
{
	// Calculate forward vector (normalized)
	vec3 f = normalize(center - eye);

	// Calculate right vector (normalized)
	vec3 r = normalize(cross(f, up));

	// Calculate new up vector (normalized)
	vec3 u = normalize(cross(r, f));

	// Create view matrix
	mat4 view(r.x, r.y, r.z, -dot(r,eye),
			  u.x, u.y, u.z, -dot(u,eye),
			  -f.x,-f.y,-f.z, dot(f,eye),
			  0.0f,0.0f,0.0f, 1.0f);
	return view;

}

// This function is called only once at the beginning of the program
// This function can contain any complex operation that can be pre-computed once
void scene_structure::initialize()
{
	std::cout << "Start function scene_structure::initialize()" << std::endl;

	/*______________ Camera Setup ______________*/

	// Set the behavior of the camera and its initial position
	camera_control.initialize(inputs, window);
	camera_control.set_rotation_axis_z(); // camera rotates around z-axis

	//   look_at(camera_position, targeted_point, up_direction)
	camera_control.look_at(
		{ 0.0f, -6.0f, 1.5f } /* position of the camera in the 3D scene */,
		{ 0,0,0 } /* targeted point in 3D scene */,
		{ 0,0,1 } /* direction of the "up" vector */);
	
	display_info(); // PRINTING EVERYTHING WE WANT

	environment.background_color = { 0.85f, 0.94f, 1.0f };
	std::string name_map = std::string(string_map);

	/*______________ Loading the Terrain ______________*/
	int Nu = 200;
    int Nv = 200;
    float x_min = -200.0f;
    float x_max = 200.0f;
    float y_min = -200.f;
    float y_max = 200.0f;

	ground_mesh_2 = terrain(Nu,Nv, x_min, x_max, y_min, y_max, ground_mesh_2.mini_map);
	if(name_map == "Rocky-Mountains"){
		ground_mesh_2.scale_perlin = 5.0f;
	}
	ground_mesh_2.add_nose();
	
	// Create the flat ground
	float const L = 200.0f; // size of the ground
	ground_mesh = mesh_primitive_quadrangle({-L,-L,0.0f}, { L,-L,0.0f }, { L,L,0.0f }, { -L,L,0.0f });
	ground.initialize_data_on_gpu(ground_mesh);
	ground_2.initialize_data_on_gpu(ground_mesh_2);

	ground.material.color = { 0.4f, 0.6f, 0.3f };
	//ground_2.material.color = { 0.4f, 0.6f, 0.3f };

	/*______________ Instancing Position, Bounding Box Setup and z-axis Adjustment ______________*/

	
	mesh tree_base;
	std::cout<<"----aaa1----"<<std::endl;
	mesh cactus_base;
	if(name_map == "Sunny-Coast"){
		tree_base = mesh_load_file_obj(project::path + "assets/palmier/palmier.obj");
		cactus_base = mesh_load_file_obj(project::path + "assets/cactus/cactus.obj");
	}else if(name_map == "Candy-Road"){
		tree_base = mesh_load_file_obj(project::path + "assets/loly/lolipops_1.obj");
		std::cout<<"----aaa2----"<<std::endl;
		cactus_base = mesh_load_file_obj(project::path + "assets/candycane/candycane.obj");
		std::cout<<"----aaa3----"<<std::endl;
	}else{
		tree_base = mesh_load_file_obj(project::path + "assets/tree/tree.obj");
		cactus_base = mesh_load_file_obj(project::path + "assets/cactus/cactus.obj");
	}
	float scaling_tree = 0.5f;
	float scaling_cactus = 0.5f;
	mesh checkpoint_base = mesh_load_file_obj(project::path + "assets/checkpoint/Checkpoint_Arch.obj");
	float scaling_checkpoint = 1.0f;
	
	Reading_Minimap_model();
	project::filled_L_Entity(tree_base,scaling_tree,cactus_base,scaling_cactus,checkpoint_base,scaling_checkpoint);
	Set_height_and_rotation();

	/*______________ Spatial Partitioning Setup ______________*/

	spatialPartitioning = std::make_unique<SpatialGrid>(chunk_size);
	/*
	std::map<vec3, int, bool(*)(const vec3&, const vec3&)> colorToZoneMap(CustomZone::vec3_less);
	// Add your zone mappings (example):
	colorToZoneMap[{0.0f, 0.2f, 0.1f}] = 1;
	colorToZoneMap[{0.3f, 0.5f, 0.4f}] = 2;
	// Create the CustomZone
	spatialPartitioning = std::make_unique<CustomZone>(colorToZoneMap);
	*/


	numarray<vec3> treePositions = getTreePositions();
	numarray<vec3> cactusPositions = getCactusPositions();
	numarray<vec3> grassPositions = getGrassPositions();
	numarray<vec3> checkpointsPositions;
	for (const auto& cp : getCheckpointPositions())
		checkpointsPositions.push_back({ cp.x, cp.y, cp.z });

	// Prepare entities for the spatial partitioning
	std::vector<entity*> allEntities;
	for (entity& e : project::All_entities.List) {
		allEntities.push_back(&e);
	}

	// Build the spatial partitioning structure
	spatialPartitioning->build(allEntities);

	// Initialisation des buffers d'instances visibles
	treeInstanceBuffer.initialize(100);
	treeAngleBuffer.initialize(100);
	cactusInstanceBuffer.initialize(100);
	cactusAngleBuffer.initialize(100);
	grassInstanceBuffer.initialize(100);
	grassAngleBuffer.initialize(100);
	checkpointInstanceBuffer.initialize(50);
	checkpointAngleBuffer.initialize(50);

	/*______________ Loading the Car ______________*/

	mesh car_mesh = mesh_load_file_obj(project::path + car.name_obj_getter());
	car_mesh.centered();
	car.initialize(car_mesh); // Initialization for the deformable type
	vec3 center, velocity, angular_velocity, color;
	vec3 offset_local = { 0.0f, -4.0f, 0.0f };

	auto rotate_z = [](vec3 p, float angle) -> vec3 {
		float c = std::cos(angle);
		float s = std::sin(angle);
		return {
			c * p.x - s * p.y,
			s * p.x + c * p.y,
			p.z
		};
	};
	int ID_Max = 0;
	for (entity& e : project::All_entities.List) {
		if (e.type == "checkpoint"){
			if (e.extra_data[3].x == 0) {
				vec3 pos_start = e.position;
				float zangle = e.zangle - M_PI / 2;
				if (std::abs(zangle) < 0.01) { zangle = 0.01; }

				vec3 offset_global = rotate_z(offset_local, -zangle);
				center = pos_start + offset_global;
				center.z = 1.5f;

				// Rotate and translate all car points manually
				float c = std::cos(-zangle);
				float s = std::sin(-zangle);

				car.set_position_and_velocity(center, velocity, angular_velocity);
				vec3 com = car.com;

				for (vec3& p : car.position) {
					// Translate to local frame (relative to center of mass)
					vec3 local = p - com;

					// Rotate around Z axis
					vec3 rotated = {
						c * local.x - s * local.y,
						s * local.x + c * local.y,
						local.z
					};

					// Translate to global center
					p = center + rotated;
				}
			}
			if (e.extra_data[3].x > ID_Max) {
				ID_Max = e.extra_data[3].x;
			}
		}
	}
	car.drawable.texture.load_and_initialize_texture_2d_on_gpu(project::path + car.name_texture_getter(), GL_REPEAT, GL_REPEAT);
	car.t = &ground_mesh_2;
	car.max_checkpoint = ID_Max+1;

	/*______________ Trees Setup ______________*/

	// Mesh and Instancing Setup
	// Mesh done before filled_L_Entity

	tree.initialize_data_on_gpu(tree_base);
	
	if(name_map == "Sunny-Coast"){
		tree.texture.load_and_initialize_texture_2d_on_gpu(project::path + "assets/palmier/palmier.png");
	}else if(name_map == "Candy-Road"){
		tree.texture.load_and_initialize_texture_2d_on_gpu(project::path + "assets/loly/Loly.png");
	}else{
		tree.texture.load_and_initialize_texture_2d_on_gpu(project::path + "assets/tree/arbre-texture-fix.png");
	}
	
	

	
	// Shader Setup
	tree.shader.load(project::path + "shaders/mesh_with_shadow/tree_instancing_with_shadow.vert.glsl", project::path + "shaders/instancing/instancing.frag.glsl");

	// Scaling Setup
	tree.model.set_scaling(scaling_tree);

	// Instancing Setup
	numarray<vec3> instance_colors_tree(getTreePositions().size());
	for (int i = 0; i < instance_colors_tree.size(); ++i)
		instance_colors_tree[i] = { 1.0f, 1.0f, 1.0f };
	tree.initialize_supplementary_data_on_gpu(instance_colors_tree, /*location*/ 4, /*divisor: 1=per instance, 0=per vertex*/ 1);

	tree.initialize_supplementary_data_on_gpu(treePositions, /*location*/ 5, /*divisor: 1=per instance, 0=per vertex*/ 1);

	treeAngles.resize(treePositions.size());
	constexpr uint32_t FIXED_SEED = 12345;
	std::mt19937     rng_tree(FIXED_SEED);
	std::uniform_real_distribution<float> dist_tree(0.0f, 2.0f * M_PI);
	for (size_t i = 0; i < treeAngles.size(); ++i) {
		// On ne fait que la rotation autour de Z ici
		treeAngles[i] = vec3(0.0f, 0.0f, dist_tree(rng_tree));
	}
	tree.initialize_supplementary_data_on_gpu(treeAngles, /*location=*/6, /*divisor=*/1);

	/*______________ cactuss Setup ______________*/

	// Mesh and Instancing Setup
	// Mesh done before filled_L_Entity
	cactus.initialize_data_on_gpu(cactus_base);
	std::cout<<"----aaa1----"<<std::endl;
	if(name_map == "Sunny-Coast"){
		cactus.texture.load_and_initialize_texture_2d_on_gpu(project::path + "assets/cactus/cactus-texture-fix.jpg");
	}else if(name_map == "Candy-Road"){
		cactus.texture.load_and_initialize_texture_2d_on_gpu(project::path + "assets/candycane/candycane.png");
		std::cout<<"----aaa2----"<<std::endl;
	}else{
		cactus.texture.load_and_initialize_texture_2d_on_gpu(project::path + "assets/cactus/cactus-texture-fix.jpg");
	}
	
	

	
	// Shader Setup
	cactus.shader.load(project::path + "shaders/mesh_with_shadow/cactus_instancing_with_shadow.vert.glsl", project::path + "shaders/instancing/instancing.frag.glsl");

	// Scaling Setup
	cactus.model.set_scaling(scaling_cactus);

	// Instancing Setup
	numarray<vec3> instance_colors_cactus(getCactusPositions().size());
	for (int i = 0; i < instance_colors_cactus.size(); ++i)
		instance_colors_cactus[i] = { 1.0f, 1.0f, 1.0f };
	cactus.initialize_supplementary_data_on_gpu(instance_colors_cactus, /*location*/ 4, /*divisor: 1=per instance, 0=per vertex*/ 1);

	cactus.initialize_supplementary_data_on_gpu(cactusPositions, /*location*/ 5, /*divisor: 1=per instance, 0=per vertex*/ 1);

	cactusAngles.resize(cactusPositions.size());
	//constexpr uint32_t FIXED_SEED = 12345;
	std::mt19937     rng_cactus(FIXED_SEED);
	std::uniform_real_distribution<float> dist_cactus(0.0f, 2.0f * M_PI);
	for (size_t i = 0; i < cactusAngles.size(); ++i) {
		// On ne fait que la rotation autour de Z ici
		cactusAngles[i] = vec3(0.0f, 0.0f, dist_cactus(rng_cactus));
	}
	cactus.initialize_supplementary_data_on_gpu(cactusAngles, /*location=*/6, /*divisor=*/1);
	
	/*______________ Grass Setup ______________*/

	// Mesh Setup
	grass.initialize_data_on_gpu(mesh_primitive_quadrangle({ -0.5f,0.0f,0.0f }, { 0.5f,0.0f,0.0f }, { 0.5f,0.0f,1.0f }, { -0.5f,0.0f,1.0f }));
	grass.material.phong = { 1,0,0,1 };
	grass.texture.load_and_initialize_texture_2d_on_gpu(project::path + "assets/grass.png");

	// Shader Setup
	grass.shader.load(project::path + "shaders/mesh_with_shadow/grass_instancing_with_shadow.vert.glsl", project::path + "shaders/instancing/instancing.frag.glsl");
	

	// Instancing Setup
	numarray<vec3> instance_colors_grass(getGrassPositions().size());
	for(int i=0; i<instance_colors_grass.size(); ++i)
		instance_colors_grass[i] = { 1.0f, 1.0f, 1.0f };
	grass.initialize_supplementary_data_on_gpu(instance_colors_grass, /*location*/ 4, /*divisor: 1=per instance, 0=per vertex*/ 1);

	grass.initialize_supplementary_data_on_gpu(grassPositions, /*location*/ 5, /*divisor: 1=per instance, 0=per vertex*/ 1);

	grassAngles.resize(grassPositions.size());
	std::mt19937     rng_grass(FIXED_SEED);
	std::uniform_real_distribution<float> dist_grass(0.0f, 2.0f * M_PI);
	for (size_t i = 0; i < grassAngles.size(); ++i) {
		// On ne fait que la rotation autour de Z ici
		grassAngles[i] = vec3(0.0f, 0.0f, dist_grass(rng_grass));
	}

	grass.initialize_supplementary_data_on_gpu(grassAngles, /*location=*/6, /*divisor=*/1);

	/*______________ Checkpoint Setup ______________*/

	// Mesh and Instancing Setup
	// Mesh done before filled_L_Entity

	
	checkpoint.initialize_data_on_gpu(checkpoint_base);
	checkpoint.texture.load_and_initialize_texture_2d_on_gpu(project::path + "assets/checkpoint/Checkpoint_Arch_Texture.png");

	/*rotation_transform R1 = rotation_transform::from_axis_angle({ 0,0,1 }, Pi / 2);
	rotation_transform R2 = rotation_transform::from_axis_angle({ 1,0,0 }, Pi / 4);
	rotation_transform R3 = R1 * R2;*/
	//checkpoint.model.set_rotation(R3);
	
	// Shader Setup
	checkpoint.shader.load(project::path + "shaders/mesh_with_shadow/checkpoint_instancing_with_shadow.vert.glsl", project::path + "shaders/instancing/instancing.frag.glsl");

	// Scaling Setup
	checkpoint.model.set_scaling(scaling_checkpoint);
	
	// Instancing Setup
	numarray<vec3> instance_colors_checkpoint(getCheckpointPositions().size());
	for (int i = 0; i < instance_colors_checkpoint.size(); ++i)
		instance_colors_checkpoint[i] = { 1.0f, 1.0f, 1.0f };
	checkpoint.initialize_supplementary_data_on_gpu(instance_colors_checkpoint, 4,  1);
	
	numarray<vec3> checkpointPositions = getCheckpointPositions();
	checkpoint.initialize_supplementary_data_on_gpu(checkpointPositions,  5,  1);

	numarray<vec3> checkpointAngles = getCheckpointAngles();
	checkpoint.initialize_supplementary_data_on_gpu(checkpointAngles, 6, 1);

	
	
	/*______________ Skybox Setup ______________*/
	
	// Choosing the cycle used
	enum Weather {
		DAY,
		NIGHT,
		CYCLE_COUNT
	};
	std::random_device rd;
	std::mt19937 gen(rd());

	std::string cycle_string = std::string(string_cycle);

	Weather weather_selected;
	// Now you can do comparisons like:
	if (cycle_string == "Day") {
		weather_selected = DAY;
	}
	else if (cycle_string == "Night") {
		weather_selected = NIGHT;
	}
	else {
		std::uniform_int_distribution<> distrib(0, CYCLE_COUNT - 1);
		weather_selected = static_cast<Weather>(distrib(gen));
	}

	// Switching cases depending on the weather, preparing the Light Setup
	float light_intensity;
	cgp::vec3 background_color;
	std::string skybox_filename;

	switch (weather_selected)
	{
	case DAY:
		light_intensity = 1.0f;
		background_color = light_intensity * cgp::vec3(0.85f, 0.94f, 1.0f); // blue sky
		weather_str = "Day";
		skybox_filename = "skybox_day.png";
		break;

	case NIGHT:
		light_intensity = 0.4f;
		background_color = light_intensity * cgp::vec3(0.05f, 0.05f, 0.1f); // very dark blue
		weather_str = "Night";
		skybox_filename = "skybox_night.png";
		break;

	default:
		light_intensity = 1.0f;
		background_color = light_intensity * cgp::vec3(0.85f, 0.94f, 1.0f); // blue sky
		weather_str = "None";
		skybox_filename = "skybox_day.png"; // fallback
		break;
	}

	// Image grid Setup
	image_structure image_skybox_template = image_load_file(project::path + "assets/skybox/" + skybox_filename);
	std::vector<image_structure> image_grid = image_split_grid(image_skybox_template, 4, 3); // Split the image into a grid of 4 x 3 sub-images

	// Special Mesh Setup
	skybox.initialize_data_on_gpu();
	skybox.texture.initialize_cubemap_on_gpu(image_grid[1], image_grid[7], image_grid[5], image_grid[3], image_grid[10], image_grid[4]);
	// Look at skybox_debug.png to see the correspondance of the image index
	skybox.model.set_scaling(100.0);
	skybox.model.set_rotation(rotation_transform::from_axis_angle({ 1, 0, 0 }, M_PI/ 2));


	/*______________ Lighting and Matrix Light View Setup ______________*/

	// Initial light position (above the car)
	vec3 initial_car_position = center;
	vec3 initial_light_position = initial_car_position + vec3(0, 0, 50.0f);

	// Store the light position in environment
	environment.uniform_generic.uniform_vec3["light_position"] = initial_light_position;

	// Create initial view matrix from light
	view_light = create_view_matrix(initial_light_position, initial_car_position, { 0, 1, 0 });
	environment.uniform_generic.uniform_mat4["view_light"] = view_light;

	// Set projection matrix - ensure it's wide enough for your scene
	environment.uniform_generic.uniform_mat4["projection_light"] =
		projection_orthographic(-30.0f, 30.0f, -30.0f, 30.0f, 0.1f, 150.0f);

	environment.uniform_generic.uniform_float["light_intensity"] = light_intensity;
	environment.background_color = background_color;

	/*______________ Shaders Setup ______________*/

	// Initialize the shadow mapping structure
	shader_depth_map.load(project::path + "shaders/depth_map/depth_map.vert.glsl", project::path + "shaders/depth_map/depth_map.frag.glsl");
	shadow_mapping.initialize(shader_depth_map);
	shader_depth_map_deformable.load(project::path + "shaders/depth_map/car_depth_map.vert.glsl", project::path + "shaders/depth_map/depth_map.frag.glsl");

	// Drawables that can have Visual Shaders
	car.drawable.supplementary_texture["depth_map_texture"] = shadow_mapping.get_depth_map_texture();
	ground.supplementary_texture["depth_map_texture"] = shadow_mapping.get_depth_map_texture();
	ground_2.supplementary_texture["depth_map_texture"] = shadow_mapping.get_depth_map_texture();

	tree.supplementary_texture["depth_map_texture"] = shadow_mapping.get_depth_map_texture();
	cactus.supplementary_texture["depth_map_texture"] = shadow_mapping.get_depth_map_texture();
	grass.supplementary_texture["depth_map_texture"] = shadow_mapping.get_depth_map_texture();
	checkpoint.supplementary_texture["depth_map_texture"] = shadow_mapping.get_depth_map_texture();


	// Shader File to apply Visual Shader on drawables
	shader_mesh_with_shadow.load(project::path + "shaders/mesh_with_shadow/mesh_with_shadow.vert.glsl", project::path + "shaders/mesh_with_shadow/mesh_with_shadow.frag.glsl");
	
	// Receiving Shaders to chosen Drawables
	car.drawable.shader = shader_mesh_with_shadow;
	ground.shader = shader_mesh_with_shadow;
	ground_2.shader = shader_mesh_with_shadow;
}

//__________________________________________________________________//
/*______________ Getting Visible Instances ______________*/

void scene_structure::getVisibleInstances(const vec3& position, float radius,
	std::vector<int>& visibleTreeIndices,
	std::vector<int>& visibleCactusIndices,
	std::vector<int>& visibleGrassIndices,
	std::vector<int>& visibleCheckpointIndices) {

	if (spatialPartitioning) {
		spatialPartitioning->getVisibleInstances(position, radius, visibleTreeIndices, visibleCactusIndices, visibleGrassIndices, visibleCheckpointIndices, getTreePositions(), getCactusPositions(),getGrassPositions(), getCheckpointPositions(), environment.camera_projection* environment.camera_view);
	}
}


/*______________ Drawing Function ______________*/

// This function is called permanently at every new frame
// Note that you should avoid having costly computation and large allocation defined there. This function is mostly used to call the draw() functions on pre-existing data.
void scene_structure::display_frame()
{	
	/*______________ Light Position Update ______________*/
	
	// Create the light matrix
	vec3 car_position = car.com;
	vec3 light_position = car_position + vec3(0, 0, 50.0f); // Position the light 50 units above the car
	vec3 light_target = car_position;  // Look at the car
	vec3 light_up = { 0, 1, 0 };  // Up direction

	// Update view matrix from light's perspective
	view_light = create_view_matrix(light_position, light_target, light_up);

	// Important: Pass the actual light position to shaders
	environment.uniform_generic.uniform_mat4["view_light"] = view_light;
	environment.uniform_generic.uniform_vec3["light_position"] = light_position;

	// Make sure to update view_light before drawing the depth map
	glUseProgram(shader_depth_map.id);
	glUniform3fv(glGetUniformLocation(shader_depth_map.id, "light_position"), 1, ptr(light_position));
	glUniform1f(glGetUniformLocation(shader_depth_map.id, "time"), timer.t);

	/*______________ Drawing from Light POV ______________*/

	glUseProgram(shader_depth_map.id);
	glUniform1f(glGetUniformLocation(shader_depth_map.id, "time"), timer.t);


	//Start
	shadow_mapping.start_pass_depth_map_creation();

	// Use shadow_mapping.draw_depth_map(drawable, environment) on all shape that emits shadows
	shadow_mapping.draw_depth_map(car.drawable, environment,1, &shader_depth_map_deformable);
	shadow_mapping.draw_depth_map(ground_2, environment);

	// Apply Instancing Visibles Drawing
	std::vector<int> visibleTreeIndices, visibleCactusIndices, visibleGrassIndices, visibleCheckpointIndices;
	getVisibleInstances(car.com, render_distance, visibleTreeIndices, visibleCactusIndices, visibleGrassIndices, visibleCheckpointIndices);

	
	shadow_mapping.draw_depth_map_selected(tree, getTreePositions(), treeAngles, visibleTreeIndices, treeInstanceBuffer, treeAngleBuffer, environment);
	shadow_mapping.draw_depth_map_selected(cactus, getCactusPositions(), cactusAngles, visibleCactusIndices, cactusInstanceBuffer, cactusAngleBuffer, environment);
	shadow_mapping.draw_depth_map_selected(grass, getGrassPositions(), grassAngles, visibleGrassIndices, grassInstanceBuffer, grassAngleBuffer, environment);
	shadow_mapping.draw_depth_map_selected(checkpoint, getCheckpointPositions(),getCheckpointAngles(), visibleCheckpointIndices, checkpointInstanceBuffer, checkpointAngleBuffer, environment);


	shadow_mapping.end_pass_depth_map_creation();
	// End
	

	/*______________ Drawing from Camera POV ______________*/

	glViewport(0, 0, window.width, window.height);
	environment.uniform_generic.uniform_float["time"] = timer.t;

	// Drawing Simple Elements
	//draw(ground, environment);
	draw(ground_2, environment);
	draw(skybox, environment);


	// Drawing Visible Instancing Elements

	
	drawSelectedInstances(tree, getTreePositions(), treeAngles, visibleTreeIndices, treeInstanceBuffer, treeAngleBuffer, environment);
	drawSelectedInstances(cactus, getCactusPositions(), cactusAngles, visibleCactusIndices, cactusInstanceBuffer, cactusAngleBuffer, environment);
	drawSelectedInstances(grass, getGrassPositions(), grassAngles, visibleGrassIndices, grassInstanceBuffer, grassAngleBuffer, environment);
	drawSelectedInstances(checkpoint, getCheckpointPositions(), getCheckpointAngles(), visibleCheckpointIndices, checkpointInstanceBuffer, checkpointAngleBuffer, environment);
	

	/*______________ Threading Drawing Setting ______________*/

	// Signal that rendering is about to begin and wait for physics to complete its turn
	{
		std::unique_lock<std::mutex> lock(sync_mutex);
		physics_done_cv.wait(lock, [this]{ return physics_ready_for_car_draw || stop_simulation; });
		physics_ready_for_car_draw = false;
		render_ready_for_physics = true;
		physics_done_cv.notify_one(); // réveille la physique pour la prochaine frame
	}
	{
		std::lock_guard<std::mutex> lock(car_mutex);
		car.update_drawable();
		draw(car.drawable, environment);
	}
}

/*______________ Reading the Mini Map into the Scene ______________*/

void scene_structure::Reading_Minimap_model() {
	vec3 tree_color = { 16.0f / 256, 108.0f / 256, 13.0f / 256 };
	vec3 cactus_color = { 116.0f / 256, 108.0f / 256, 13.0f / 256 };
	vec3 grass_color = { 83.0f / 256, 149.0f / 256, 81.0f / 256 };
	// vec3 checkpoint_color{ 1, y, z }; where y is 10*ID and z = rotation/2

	int i = 0;
	for (int y = 0;y < ground_mesh_2.mini_map->height;y++) {
		for (int x = 0; x < ground_mesh_2.mini_map->width;x++) {
			vec3 color_cell = ground_mesh_2.mini_map->minimap_color_matrix[y][x];
			if (norm(color_cell - grass_color) < 0.01f) {
				ImVec2 rel_pos = ground_mesh_2.mini_map->Minimaptoworld({ x,y }, ground_mesh_2.x_max);
				vec3 newGrass = { rel_pos.x,rel_pos.y,0.0f };
				project::GrassPositions.push_back(newGrass);
			}
			else if (norm(color_cell - tree_color) < 0.01f) {
				ImVec2 rel_pos = ground_mesh_2.mini_map->Minimaptoworld({ x,y }, ground_mesh_2.x_max);
				vec3 newTree = { rel_pos.x,rel_pos.y,0.0f };
				project::TreePositions.push_back(newTree);
			}
			else if (norm(color_cell - cactus_color) < 0.01f) {
				ImVec2 rel_pos = ground_mesh_2.mini_map->Minimaptoworld({ x,y }, ground_mesh_2.x_max);
				vec3 newCactus = { rel_pos.x,rel_pos.y,0.0f };
				project::CactusPositions.push_back(newCactus);
			}
			else if (std::abs(color_cell.x - 1.0f) < 0.01f) {
				ImVec2 rel_pos = ground_mesh_2.mini_map->Minimaptoworld({ x,y }, ground_mesh_2.x_max);
				float rad_angle = 256 * 2 * color_cell.z / 180 * M_PI;
				float ID = (256 * float(color_cell.y) / 10);
				vec4 newCheckpoint = { rel_pos.x,rel_pos.y,ID, rad_angle };
				project::CheckpointPositions.push_back(newCheckpoint);
			}
		}
	}
}

/*______________ Configuration of Instancing Objects (Heigth and Rotation) ______________*/

void scene_structure::Set_height_and_rotation() {
	for (entity &e : project::All_entities.List) {
		std::string path_texture;
		e.position[2] = ground_mesh_2.get_height_interpo({ e.position[0],e.position[1] });

		if (e.type == "tree") { path_texture = project::path + "assets/mini_map/tree_minimap.png"; }
		else if (e.type == "grass") { path_texture = project::path + "assets/mini_map/grass_minimap.png"; }
		else if (e.type == "cactus") { path_texture = project::path + "assets/mini_map/cactus_minimap.png"; }
		else if (e.type == "checkpoint") {
			path_texture = project::path + "assets/mini_map/checkpoint_mini_map.png";

			vec3 center_1 = e.extra_data[0];
			vec3 center_2 = e.extra_data[1];
			float center_1_height = ground_mesh_2.get_height_interpo({ center_1.x,center_1.y });
			float center_2_height = ground_mesh_2.get_height_interpo({ center_2.x,center_2.y });
			float diff = (center_1_height - center_2_height) / norm(center_1 - center_2);
			float rising_angle = sin(diff);
			e.extra_data[2].x = rising_angle;

			if (e.extra_data[3].x==1) {
				e.tracked = true;
			}
		}

		else { path_texture = project::path + "assets/mini_map/red_pin.png"; }
		ground_mesh_2.mini_map->loadTexture(path_texture.c_str(), e.textureID);

	}
}

/*______________ Threading Physic Loop Setting ______________*/

void scene_structure::process_keyboard_physics_input() {
	cgp::vec3 diff_forward = car.com - camera_control.camera_model.position();
	diff_forward.z = 0;
	diff_forward /= 5 * std::sqrt(diff_forward.x * diff_forward.x + diff_forward.y * diff_forward.y + diff_forward.z * diff_forward.z);

	cgp::vec3 lateral_vector = { diff_forward.y, diff_forward.x, diff_forward.z };
	diff_forward = { 0.0f, 1.0f, 0.0f };

	if (KeyPressAction['Z']) {

		car.avance = 1;
	}
	if (KeyPressAction['S']) {
		car.avance = -1;
	}
	if (KeyPressAction['D']) {
		car.sens_rotate = 1;
	}
	if (KeyPressAction['Q']) {
		car.sens_rotate = -1;
	}
	if (KeyPressAction['K']) {
		car.moove(lateral_vector);
	}
	if (KeyPressAction['J']) {
		car.moove(-lateral_vector);
	}

	if (KeyJustPressed['E']) {
		car.speed_counter++;
	}

	if (KeyJustPressed['A']) {
		car.speed_counter--;
	}
}

void scene_structure::reset_one_frame_inputs() {
	for (const auto& key_pair : supported_keys) {
		char key_char = key_pair.second;
		KeyJustPressed[key_char] = 0;
	}
}

void scene_structure::physics_loop()
{
	while (!stop_simulation) {
		{
        std::unique_lock<std::mutex> lock(sync_mutex);
        physics_done_cv.wait(lock, [this]{ return render_ready_for_physics || stop_simulation; });
        if (stop_simulation) break;
        render_ready_for_physics = false;
    	}
		// Updating the physics
		float dt = timer.update();
		if (dt > 0.2f) dt = 0.2f;
		param.time_step = dt;

		if (param.time_step > 1e-6f) {
			std::lock_guard<std::mutex> lock(car_mutex);
			process_keyboard_physics_input();

			//auto t0 = std::chrono::high_resolution_clock::now();
			simulation_step(car, param);
			//auto t1 = std::chrono::high_resolution_clock::now();
			//std::cout << "Physics step time: " << std::chrono::duration<float, std::milli>(t1 - t0).count() << " ms" << std::endl;

			reset_one_frame_inputs();
		}
		// Signal that physics is complete and rendering can continue
		{
        std::lock_guard<std::mutex> lock(sync_mutex);
        physics_ready_for_car_draw = true;
        physics_done_cv.notify_one();
    	}
		// ~2kHz max simulation rate
		//std::this_thread::sleep_for(std::chrono::microseconds(500)); //500
	}
}

/*______________ Event and display redirection ______________*/

void scene_structure::keyboard_event()
{
	camera_control.action_keyboard(environment.camera_view);
}
void scene_structure::idle_frame()
{
	camera_control.idle_frame(environment.camera_view);
}

/*______________ Display Info at launch ______________*/

void scene_structure::display_info()
{
	std::cout << "\nSCENE INFO:" << std::endl;
	std::cout << "-----------------------------------------------" << std::endl;
	std::cout << "This is a project made for CPE Lyon PTC Project, powered by OpenGL using CGP library and miniaudio.h file." << std::endl;
	std::cout << "The deformation and position are fully set in the corresponding shader (shaders/instancing)." << std::endl;
	std::cout << "A Spatial Partitionning is used, for Spatial Grid, please edit the files to edit the Render Distance and Chunk Size" << std::endl;
	std::cout << "Controls: ZQSD to move the car" << std::endl;
	std::cout << "-----------------------------------------------\n" << std::endl;
}