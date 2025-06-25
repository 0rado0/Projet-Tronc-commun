#include "cgp/cgp.hpp" // Give access to the complete CGP library
#include "environment.hpp" // The general scene environment + project variable
#include <iostream> 
#include <cmath>
#include <random>
#include <algorithm>

#include <chrono>
#include <thread>
#include <stb_image.h>

// Sound miniaudio Library
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

// Custom scene and assets of this code
#include "scene.hpp"
#include "terrain.hpp"
#include "map.hpp"


#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif


/*______________ Setting up string for Game Config text file ______________*/

char string_music[64];

/*______________ Custom Scene define in "scene.hpp" ______________*/

scene_structure scene;

/*______________ Functions and Variables Initialization ______________*/

// Functions
window_structure standard_window_initialization();
void initialize_default_shaders();
void animation_loop();
void window_size_callback(GLFWwindow* window, int width, int height);
void keyboard_callback(GLFWwindow* window, int key, int, int action, int mods);
void process_system_shortcuts();
void camera_update();
void music_manager();
void display_gui_default();
void loadTexture(const char* path, GLuint& textureID, bool reverse);
void display_speedometer(const scene_structure& scene);
void AddImageRotated(ImDrawList* draw_list, void* tex_id, ImVec2 center, ImVec2 size, float angle_rad, ImVec2 pivot);
void display_race_info(const scene_structure& scene, const std::chrono::time_point<std::chrono::high_resolution_clock>& t_start);
void display_race_finished(const scene_structure&);
void display_checkpoint_time(const scene_structure& scene, const std::chrono::time_point<std::chrono::high_resolution_clock>& t_start);
void read_game_config(const char* filename, char* string_music, scene_structure& scene);

// OpenGL and Input
timer_fps fps_record;
bool FullScreenDefault = false;

// Multi-threading
std::thread physics_thread;
std::thread input_thread;

// Music
ma_engine engine;
ma_sound sound1;

// 2D UI
map mini_map;
GLuint speedometer_texture = 0;
GLuint speedometer_number_texture = 0;
GLuint needle_speed_texture = 0;
GLuint needle_we_texture = 0;

// Timer
using Clock = std::chrono::high_resolution_clock;
using TimePoint = std::chrono::time_point<Clock>;
TimePoint t_start;


/*______________ Main Function ______________*/

int main(int, char* argv[])
{
	std::cout << "Run " << argv[0] << std::endl;


	/*______________ Game Config Text File Initalisation ______________*/

	read_game_config("game_config.txt", string_music, scene);

	std::cout << "Game Configuration Texte File: Reading..." << std::endl;
	std::cout << "_________________________________________" << std::endl;
	printf("Map: %s\n", scene.string_map);
	printf("Cycle: %s\n", scene.string_cycle);
	printf("Music: %s\n", string_music);
	printf("Lap: %f\n", scene.car.current_lap.y);
	printf("Render distance: %f\n", scene.render_distance);
	std::cout << "_________________________________________" << std::endl;
	

	/*______________ Input Initalisation ______________*/

	for (std::pair <int,char> key_pair : supported_keys) {
		KeyPressAction[key_pair.second] = 0;
		KeyJustPressed[key_pair.second] = false;
	}
	
	
	/*______________ Window Initialization ______________*/
	
	// Standard Initialization of an OpenGL ready window
	scene.window = standard_window_initialization();

	std::string iconPath = project::path + "assets/ToTally_Quick_Jolt_Racing.png";
	int width, height, channels;
	unsigned char* pixels = stbi_load(iconPath.c_str(), &width, &height, &channels, 4); // Loading Image in RGBA
	if (!pixels) {
		std::cerr << "Erreur: Impossible de charger l'image de l'icone !" << std::endl;
	}
	GLFWimage image;
	image.width = width;
	image.height = height;
	image.pixels = pixels;
	glfwSetWindowIcon(scene.window.glfw_window, 1, &image); // Apply the icone on the window
	stbi_image_free(pixels); // Free Memory after use

	/*______________ Path Project and Shaders Initialization ______________*/

	// Initialize default path for assets
	project::path = cgp::project_path_find(argv[0], "shaders/");
	// Initialize default shaders
	initialize_default_shaders();

	/*______________ Mini Map Initialization ______________*/

	std::string map_string = std::string(scene.string_map);
	std::string map_pic;
	if (map_string == "Great-Plains") {
		map_pic = project::path + "assets/mini_map/great_plains.png";
	}
	else if (map_string == "Dry-Desert") {
		map_pic = project::path + "assets/mini_map/dry_desert.png";
	}
	else if (map_string == "Snowy-Lands") {
		map_pic = project::path + "assets/mini_map/snowy_lands.png";
	}
	else if (map_string == "Sunny-Coast") {
		map_pic = project::path + "assets/mini_map/sunny_coast.png";
	}
	else if (map_string == "Mud-Forest") {
		map_pic = project::path + "assets/mini_map/mud_forest.png";
	}
	else if (map_string == "Rocky-Mountains") {
		map_pic = project::path + "assets/mini_map/rocky_mountains.png";
	}
	else if (map_string == "Candy-Road") {
		map_pic = project::path + "assets/mini_map/candy_road.png";
	}
	else {
		map_pic = project::path + "assets/mini_map/great_plains.png";
	}


	//std::string map_pic = project::path + "assets/mini_map/snowy_lands.png"; // Track image
	std::string cursor_pic = project::path + "assets/mini_map/curseur.png"; // Cursor Player Image
	std::string pin_texture = project::path + "assets/mini_map/red_pin.png"; // Pin for tracked entities

	mini_map.loadTexture(map_pic.c_str(), mini_map.textureMap, true);
	mini_map.loadTexture(cursor_pic.c_str(), mini_map.textureCursor);
	mini_map.loadTexture(pin_texture.c_str(), mini_map.pin_texture);

	// Loading Vertex and Fragment, Verifying if files existe
	std::string vertexPath = project::path + "shaders/minimap/minimap.vert.glsl"; // Vertex
	std::string fragmentPath = project::path + "shaders/minimap/minimap.frag.glsl"; // Fragment
	std::ifstream v_test(vertexPath);
	std::ifstream f_test(fragmentPath);
	if (!v_test.good() || !f_test.good()) {
		std::cerr << "Erreur: Fichiers shader introuvables." << std::endl;
		std::cerr << "Chemins recherches: " << std::endl;
		std::cerr << "- " << vertexPath << std::endl;
		std::cerr << "- " << fragmentPath << std::endl;
	}
	mini_map.initMinimapFBO(vertexPath.c_str(), fragmentPath.c_str());


	// Mini Map settings and color setup
	mini_map.scale = 1.0f / 400;
	mini_map.fill_minimap_color();
	// Terrain Minimap linking
	scene.ground_mesh_2.mini_map = &mini_map;

	/*______________ Speedometer Setup ______________*/

	std::string speedometer_pic = project::path + "assets/speedometer/Speedometer_empty.png"; // Speedometer
	std::string needle_speed_pic = project::path + "assets/speedometer/Needle_red.png"; // Needle Speed	
	std::string needle_we_pic = project::path + "assets/speedometer/Needle_white.png"; // Needle We	
	std::string speedometer_number_pic = project::path + "assets/speedometer/Speedometer_numbers.png"; // Needle We	
	loadTexture(speedometer_pic.c_str(), speedometer_texture,false);
	loadTexture(needle_speed_pic.c_str(), needle_speed_texture,false);
	loadTexture(needle_we_pic.c_str(), needle_we_texture, false);
	loadTexture(speedometer_number_pic.c_str(), speedometer_number_texture, false);


	/*______________ Scene Initialization ______________*/

	// Custom scene initialization
	std::cout << "Initialize data of the scene ..." << std::endl;
	scene.initialize();
	std::cout << "Initialization finished\n" << std::endl;


	/*______________ Animation Loop Setup ______________*/

	std::cout << "Start animation loop starting ..." << std::endl;
	fps_record.start();
	// Call the main display loop in the function animation_loop
	//  The following part is simply a loop that call the function "animation_loop"
	//  (This call is different when we compile in standard mode with GLFW, than when we compile with emscripten to output the result in a webpage.)
#ifndef __EMSCRIPTEN__
    double lasttime = glfwGetTime();
	// Default mode to run the animation/display loop with GLFW in C++ (With Full Screen)
	if (FullScreenDefault)
		scene.window.is_full_screen = !scene.window.is_full_screen;
	if (scene.window.is_full_screen)
		scene.window.set_full_screen();

	/*______________ Weather Print and Music Manager ______________*/

	std::cout<<"\nWeather Cycle chosen: " << scene.weather_str << std::endl;
	music_manager();

	/*______________ Physics Simulation Thread Setup ______________*/

	scene.stop_simulation = false;
	physics_thread = std::thread(
		&scene_structure::physics_loop,
		&scene
		/*std::ref(scene.car),
		std::ref(scene.param))*/);

	/*______________ Input Thread Setup ______________*/
	input_thread = std::thread(
		&start_input_reader
	);
	//
	t_start = Clock::now();

	/*______________ Animation Loop ______________*/

	while (!glfwWindowShouldClose(scene.window.glfw_window)) {

		// Callback Key Set
		glfwPollEvents();

		// Drawing and Animation loop
		animation_loop();

		// FPS limitation
		if(project::fps_limiting){
			while (glfwGetTime() < lasttime + 1.0 / project::fps_max) {	}
        	lasttime = glfwGetTime();
		}

		/*auto now = std::chrono::high_resolution_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - t_start).count();
		std::cout << "[TIME] dt=" << duration << std::endl;*/
	}

#else
	// Specific loop if compiled for EMScripten
	emscripten_set_main_loop(animation_loop, 0, 1);
#endif

	/*______________ Cleanup and End of Main Function ______________*/

	std::cout << "\nAnimation loop stopped" << std::endl;


	// Proper stop of the physics simulation thread
	scene.stop_simulation = true;
	{
    std::lock_guard<std::mutex> lock(scene.sync_mutex);
    scene.physics_ready_for_car_draw = true;
    scene.physics_done_cv.notify_all();
	}
	physics_thread.join();

	stop_input_reader();
	input_thread.join();
	
	//cgp::imgui_cleanup();
	ma_sound_uninit(&sound1);
	ma_engine_uninit(&engine);
	glfwDestroyWindow(scene.window.glfw_window);
	glfwTerminate();

	return 0;
}

void animation_loop()
{
	/*______________ Window Characteristics Update ______________*/

	emscripten_update_window_size(scene.window.width, scene.window.height); // emscripten

	scene.camera_projection.aspect_ratio = scene.window.aspect_ratio();
	scene.environment.camera_projection = scene.camera_projection.matrix();
	glViewport(0, 0, scene.window.width, scene.window.height);

	vec3 const& background_color = scene.environment.background_color;
	glClearColor(background_color.x, background_color.y, background_color.z, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);

	float const time_interval = fps_record.update();
	if (fps_record.event) {
		std::string const title = "ToTally Quick Jolt RAcing - " + str(fps_record.fps) + " fps";
		glfwSetWindowTitle(scene.window.glfw_window, title.c_str());
	}

	/*______________ Frame ImGUI ______________*/

	imgui_create_frame();

	/*______________ Scene Drawing ______________*/

	scene.display_frame();

	/*______________ Mini Map Update ______________*/

	float carX = scene.car.x_getter();
	float carY = scene.car.y_getter();
	float z_angle = scene.car.z_angle;
	int screenwidth = scene.window.width;
	int screenheight = scene.window.height;

	mini_map.renderMinimap(carX, carY, z_angle, screenwidth, screenheight);

	/*______________ Speedometer and Texte Update ______________*/
	
	display_speedometer(scene);
	display_race_info(scene, t_start);
	if (scene.car.finished_race) { display_race_finished(scene); }


	/*______________ Car Timers Update ______________*/

	if (scene.car.bool_new_checkpoint) {
		scene.car.bool_new_checkpoint = false; // on réinitialise pour éviter de répéter

		auto now = std::chrono::high_resolution_clock::now();
		scene.car.checkpoint_time_display_start = now;
		scene.car.checkpoint_text_visible = true;

		scene.car.last_checkpoint_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - t_start).count();

		// Si on est revenu à checkpoint 0 avec un nouveau tour
		if (scene.car.current_checkpoint.x == 0 && scene.car.current_lap.x > 1) {
			int elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - t_start).count();
			scene.car.lap_times_ms.push_back(elapsed);
		}
	}
	display_checkpoint_time(scene, t_start); // display nothing after t from new checkpoint > 3s


	/*______________ ImGui Render ______________*/

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

	/*______________ Input Interaction and Camera Update ______________*/

	process_system_shortcuts();
	camera_update();
	scene.idle_frame();


	/*______________ End of the current Animation Loop ______________*/

	imgui_render_frame(scene.window.glfw_window);
	glfwSwapBuffers(scene.window.glfw_window);
}


void initialize_default_shaders()
{
	// Generate the default directory from which the shaders are found
	//  By default, it should be "shaders/"
	std::string default_path_shaders = project::path +"shaders/";

	// Set standard mesh shader for mesh_drawable
	mesh_drawable::default_shader.load(default_path_shaders +"mesh/mesh.vert.glsl", default_path_shaders +"mesh/mesh.frag.glsl");
	triangles_drawable::default_shader.load(default_path_shaders +"mesh/mesh.vert.glsl", default_path_shaders +"mesh/mesh.frag.glsl");

	// Set default white texture
	image_structure const white_image = image_structure{ 1,1,image_color_type::rgba,{255,255,255,255} };
	mesh_drawable::default_texture.initialize_texture_2d_on_gpu(white_image);
	triangles_drawable::default_texture.initialize_texture_2d_on_gpu(white_image);

	// Set standard uniform color for curve/segment_drawable
	curve_drawable::default_shader.load(default_path_shaders +"single_color/single_color.vert.glsl", default_path_shaders+"single_color/single_color.frag.glsl");
}

/*______________ Window Resize Callback ______________*/

void window_size_callback(GLFWwindow*, int width, int height)
{
	scene.window.width = width;
	scene.window.height = height;
}

/*______________ Standard Window Procedure ______________*/

window_structure standard_window_initialization()
{
	/*______________ Initialize GLFW and create window ______________*/

	// First initialize GLFW
	scene.window.initialize_glfw();

	// Compute initial window width and height
	int window_width = int(project::initial_window_size_width);
	int window_height = int(project::initial_window_size_height);
	if(project::initial_window_size_width<1)
		window_width = project::initial_window_size_width * scene.window.monitor_width();
	if(project::initial_window_size_height<1)
		window_height = project::initial_window_size_height * scene.window.monitor_height();

	// Create the window using GLFW
	window_structure window;
	window.create_window(window_width, window_height, "ToTally Quick Jolt RAcing", CGP_OPENGL_VERSION_MAJOR, CGP_OPENGL_VERSION_MINOR);

	
	/*______________ Display Information ______________*/

	// Display window size
	std::cout << "\nWindow (" << window.width << "px x " << window.height << "px) created" << std::endl;
	std::cout << "Monitor: " << glfwGetMonitorName(window.monitor) << " - Resolution (" << window.screen_resolution_width << "x" << window.screen_resolution_height << ")\n" << std::endl;

	// Display debug information on command line
	std::cout << "OpenGL Information:" << std::endl;
	std::cout << cgp::opengl_info_display() << std::endl;


	/*______________ Initialize ImGUI ______________*/

	cgp::imgui_init(window.glfw_window);

	// Set the callback functions for the inputs
	glfwSetWindowSizeCallback(window.glfw_window, window_size_callback);  // Event called when the window is rescaled        
	glfwSetKeyCallback(window.glfw_window, keyboard_callback);            // Event called when a keyboard touch is pressed/released

	return window;
}



/*______________ Input Buffer Interaction ______________*/

// This function is called everytime a keyboard touch is pressed/released
void keyboard_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	//ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);
	bool imgui_capture_keyboard = ImGui::GetIO().WantCaptureKeyboard;
	if(!imgui_capture_keyboard){
		scene.inputs.keyboard.update_from_glfw_key(key, action);
		scene.keyboard_event();

		// Look for this key in supported_keys
		for (const auto& key_pair : supported_keys) {
			char key_char = key_pair.second;
			KeyJustPressed[key_char] = 0;
			if (key == key_pair.first) {

				if (action == GLFW_PRESS) {
					KeyPressAction[key_char] = 1;
					KeyJustPressed[key_char] = 1; // Will be reset at end of physics step
				}
				else if (action == GLFW_RELEASE) {
					KeyPressAction[key_char] = 0;
				}
				break;
			}
		}
	}
}

/*______________ Debug and Window Input Interaction ______________*/

void process_system_shortcuts()
{
	// For fullscreen toggle (only needs GLFW_PRESS detection)
	static bool toggle_fullscreen_handled = false;
	if (KeyJustPressed['F'] && scene.inputs.keyboard.shift && !toggle_fullscreen_handled) {
		scene.window.is_full_screen = !scene.window.is_full_screen;
		toggle_fullscreen_handled = true;

		if (scene.window.is_full_screen)
			scene.window.set_full_screen();
		else
			scene.window.set_windowed_screen();
	}
	if (!KeyJustPressed['F']) {
		toggle_fullscreen_handled = false;
	}


	// Debug camera info
	static bool show_debug_handled = false;
	if (KeyJustPressed['V'] && scene.inputs.keyboard.shift && !show_debug_handled) {
		auto const camera_model = scene.camera_control.camera_model;
		std::cout << "\nDebug camera (position = " << str(camera_model.position()) << "):\n" << std::endl;
		std::cout << "  Frame matrix:\n" << str_pretty(camera_model.matrix_frame()) << std::endl;
		std::cout << "  View matrix:\n" << str_pretty(camera_model.matrix_view()) << std::endl;
		show_debug_handled = true;
	}
	if (!KeyJustPressed['V']) {
		show_debug_handled = false;
	}
}

/*______________ Camera Update ______________*/

void camera_update() {
	
	// Offset avec Rotation en fonction de l'angle de la voiture
	cgp::vec3 offset = { 0.0f, -6.0f, 1.5f };
	float angle_offset = scene.car.z_angle;//-60.0f * M_PI / 180.0f * (-0.5 - 0.5 * cos(flip_angle));
	rotation_transform ZAxis = rotation_transform::from_axis_angle({ 0,0,1 }, angle_offset);
	cgp::vec3 offset_adjusted = ZAxis * offset*(1+0.02*norm(average(scene.car.velocity)));
	// Position cible de la caméra (sans lissage)
	cgp::vec3 target_position = scene.car.com+offset_adjusted; //+offset offset_adjusted;
	
	// Lissage de la position avec une interpolation linéaire
	float smooth_factor = 0.7f; // Ajustable pour changer la réactivité
	cgp::vec3 camera_position = (1 - smooth_factor) * scene.camera_control.previous_eye_position + smooth_factor * target_position;


	// Orientation de la caméra
	scene.camera_control.look_at(
		camera_position /* position of the camera in the 3D scene */,
		scene.car.com /* targeted point in 3D scene */,
		{ 0,0,1 } /* direction of the "up" vector */);

	// Signal that rendering is complete and physics can continue
}

/*______________ Music Manager ______________*/

enum MusicID {
	TWIST_STONE,
	ROAD_MAIN,
	FLASH_CONTENT,
	ADD_ON_DRIFT,
	JUST_IN_TIME,
	THIBOCALPYSPE,
	MUSIC_COUNT
};

struct MusicInfo {
	std::string filename;
	bool hasCustomLoop;
	double customLoopStartSeconds;
};



void music_manager() {
	// Liste des musiques
	std::vector<MusicInfo> musicList = {
		{"Twist_Stone.mp3", true, 184.2},           // Specific loop at 3'04"2
		{"Road_Main(Turbo_Crush).mp3", false, 0.0}, // No custom loop
		{"Flash_Content.mp3", false, 0.0}  ,        // No custom loop
		{"Add_on_Drift.mp3", false, 0.0}   ,       // No custom loop
		{"Just_in_Time.mp3", false, 0.0},          // No custom loop
		{"Thibocalypse.mp3", false, 0.0}           // No custom loop
	};
	std::string music_string = std::string(string_music);

	MusicID selected;
	// Now you can do comparisons like:
	if (music_string == "Twist Stone") {
		selected = TWIST_STONE;
	}
	else if(music_string == "Road Main") {
		selected = ROAD_MAIN;
	}
	else if (music_string == "Flash Content") {
		selected = FLASH_CONTENT;
	}
	else if (music_string == "Add on Drift") {
		selected = ADD_ON_DRIFT;
	}
	else if (music_string == "Just in Time") {
		selected = JUST_IN_TIME;
	}
	else if (music_string == "Thibocalypse") {
		selected = THIBOCALPYSPE;
	}
	else{
		std::srand((unsigned int)std::time(nullptr));
		selected = static_cast<MusicID>(std::rand() % MUSIC_COUNT);
	}
	MusicInfo& music = musicList[selected];
	std::string path = project::path + "assets/music/" + music.filename;

	FILE* file = fopen(path.c_str(), "rb");
	if (!file) {
		std::cout << "Fichier introuvable!\n Music chosen : " << music.filename << std::endl;
	}
	else {
		std::cout << "Music chosen : " << music.filename << std::endl;
		fclose(file);
	}

	// Initialisation du moteur audio
	ma_engine_init(NULL, &engine);

	// Chargement du son
	ma_result result = ma_sound_init_from_file(&engine, path.c_str(), MA_SOUND_FLAG_STREAM, NULL, NULL, &sound1);
	if (result != MA_SUCCESS) {
		printf("Erreur chargement audio.\n");
	}

	// Obtenir la source de données du son
	ma_data_source* pDataSource = ma_sound_get_data_source(&sound1);
	if (pDataSource == NULL && music.hasCustomLoop) {
		printf("Impossible d'obtenir la source de donnees.\n");
	}

	// Getting Sample rate
	ma_uint64 sampleRate = ma_engine_get_sample_rate(&engine);

	// Setting up the loop
	ma_uint64 loopStart;
	if (music.hasCustomLoop) {
		loopStart = static_cast<ma_uint64>(music.customLoopStartSeconds * sampleRate);
	}
	ma_uint64 loopEnd; // Fin du morceau
	ma_uint64 totalFrames;
	if (ma_data_source_get_length_in_pcm_frames(pDataSource, &totalFrames) != MA_SUCCESS) {
		printf("Impossible d'obtenir la duree du fichier.\n");
	}
	loopEnd = totalFrames;

	result = ma_data_source_set_loop_point_in_pcm_frames(pDataSource, loopStart, loopEnd);
	if (result != MA_SUCCESS && music.hasCustomLoop) {
		printf("Impossible de definir les points de boucle.\n");
	}

	// Activer le mode loop
	ma_sound_set_looping(&sound1, MA_TRUE);

	// Demarrer le son
	ma_sound_start(&sound1);

}


/*______________ GUI Default Display ______________*/

void display_gui_default()
{
	std::string fps_txt = str(fps_record.fps)+" fps";

	if(scene.inputs.keyboard.ctrl)
		fps_txt += " [ctrl]";
	if(scene.inputs.keyboard.shift)
		fps_txt += " [shift]";

	ImGui::Text( fps_txt.c_str(), "%s" );
	if(ImGui::CollapsingHeader("Window")) {
		ImGui::Indent();
#ifndef __EMSCRIPTEN__
		bool changed_screen_mode = ImGui::Checkbox("Full Screen", &scene.window.is_full_screen);
		if(changed_screen_mode){	
			if (scene.window.is_full_screen)
				scene.window.set_full_screen();
			else
				scene.window.set_windowed_screen();
		}
#endif
		ImGui::SliderFloat("Gui Scale", &project::gui_scale, 0.5f, 2.5f);

#ifndef __EMSCRIPTEN__
		// Arbitrary limits the refresh rate to a maximal frame per seconds.
		//  This limits the risk of having different behaviors when you use different machine. 
		ImGui::Checkbox("FPS limiting",&project::fps_limiting);
		if(project::fps_limiting){
			ImGui::SliderFloat("FPS limit",&project::fps_max, 10, 250);
		}
#endif
		// vsync is the default synchronization of frame refresh with the screen frequency
		//   vsync may or may not be enforced by your GPU driver and OS (on top of the GLFW request).
		//   de-activating vsync may generate arbitrary large FPS depending on your GPU and scene.
		if(ImGui::Checkbox("vsync (screen sync)",&project::vsync)){
			project::vsync==true? glfwSwapInterval(1) : glfwSwapInterval(0); 
		}

		std::string window_size = "Window "+str(scene.window.width)+"px x "+str(scene.window.height)+"px";
		ImGui::Text( window_size.c_str(), "%s" );

		ImGui::Unindent();


		ImGui::Spacing();ImGui::Separator();ImGui::Spacing();
	}
}


/*______________ Loading Texture ______________*/

void loadTexture(const char* path, GLuint& textureID, bool reverse) {
	if (textureID != 0) {
		glDeleteTextures(1, &textureID);
	}
	textureID = 0;

	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);

	if (reverse) {
		stbi_set_flip_vertically_on_load(true);
	}
	else {
		stbi_set_flip_vertically_on_load(false);
	}
	int width, heigth, channels;
	unsigned char* data = stbi_load(path, &width, &heigth, &channels, 4);
	if (data) {
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, heigth, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		stbi_image_free(data);

		//Commentaire a laisser usuable later pour de vrai
		//std::cout << "Texture chargée: " << path << " (" << mapWidth << "x" << mapHeight << ")" << std::endl;
	}
	else {
		std::cerr << "Failed to load texture: " << path << std::endl;
	}
}

/*______________ Speedometer Display ______________*/

void display_speedometer(const scene_structure& scene) {
	float speed = cgp::norm(average(scene.car.velocity));
	float max_speed = scene.car.velocity_max;
	float We = scene.car.We;
	float We_max = scene.car.We_max;

	ImGuiIO& io = ImGui::GetIO();

	// References dimensions for reduced window
	float ref_width = 960.0f;
	float ref_height = 540.0f;

	float scale = std::min<float>(io.DisplaySize.x / ref_width, io.DisplaySize.y / ref_height);

	// Size and position in pixels, referenced to the base
	ImVec2 ref_size = ImVec2(240.0f, 240.0f);
	ImVec2 ref_offset_from_corner = ImVec2(20.0f, -60.0f); // marge depuis le coin

	// Apply scaling
	ImVec2 size = ImVec2(ref_size.x * scale, ref_size.y * scale);
	ImVec2 offset = ImVec2(ref_offset_from_corner.x * scale, ref_offset_from_corner.y * scale);

	ImVec2 win_pos = ImVec2(io.DisplaySize.x - size.x - offset.x, io.DisplaySize.y - size.y - offset.y);
	ImVec2 center = ImVec2(win_pos.x + size.x * 0.5f, win_pos.y + size.y * 0.5f);

	ImGui::SetNextWindowPos(win_pos);
	ImGui::SetNextWindowSize(size);
	ImGui::Begin("Speedometer", nullptr,
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoBackground |
		ImGuiWindowFlags_NoScrollbar);

	ImDrawList* draw_list = ImGui::GetWindowDrawList();

	// Frame Image
	
	draw_list->AddImage((void*)(intptr_t)speedometer_texture,
		win_pos,
		ImVec2(win_pos.x + size.x, win_pos.y + size.y));
		

	// Needle Rotation (Speed)
	float speed_ratio = std::min<float>(speed / max_speed, 2.0f);
	float min_angle = 115.0f;
	float angle_speed = (-min_angle + speed_ratio * 2*min_angle) * 3.14159f / 180.0f;

	// Needle Rotation (We)
	float We_ratio = std::min<float>(We / We_max, 2.0f);
	float angle_We = (-min_angle + We_ratio * 2 * min_angle) * 3.14159f / 180.0f;

	// Size of needle
	ImVec2 needle_speed_size = ImVec2(7, 90);
	ImVec2 needle_We_size = ImVec2(4, 50);
	needle_speed_size.x *= scale;
	needle_speed_size.y *= scale;
	needle_We_size.x *= scale;
	needle_We_size.y *= scale;

	ImVec2 correction = ImVec2(5.0f * scale, -5.0f * scale);
	ImVec2 center_corrected = ImVec2(center.x + correction.x, center.y + correction.y);

	// Pivot = center of the image of the needle (horizontal)
	ImVec2 pivot = ImVec2(0.5f, 1.0f); // close to bottom right corner
	// Speed Needle
	AddImageRotated(draw_list, (void*)(intptr_t)needle_speed_texture, center_corrected, needle_speed_size, angle_speed, pivot);

	// We Needle
	AddImageRotated(draw_list, (void*)(intptr_t)needle_we_texture, center_corrected, needle_We_size, angle_We, pivot);

	// Getting car's gear
	int gear = scene.car.speed_counter;

	// Define gear's text
	char gear_text[4];
	if (gear == 0)
		snprintf(gear_text, sizeof(gear_text), "N"); // Neutral
	else
		snprintf(gear_text, sizeof(gear_text), "%d", gear);

	// Size and position of the text
	float font_scale = 2.5f * scale;
	ImVec2 text_size = ImGui::CalcTextSize(gear_text);
	ImVec2 text_pos = ImVec2(center.x - text_size.x * 0.5f * font_scale, center.y - text_size.y * 0.5f * font_scale);

	// Apply scaling manually
	draw_list->AddText(nullptr, ImGui::GetFontSize() * font_scale, text_pos, IM_COL32(255, 255, 255, 255), gear_text);
	// Draw numbers above the needles
	draw_list->AddImage((void*)(intptr_t)speedometer_number_texture,
		win_pos,
		ImVec2(win_pos.x + size.x, win_pos.y + size.y));


	ImGui::End();
}

void AddImageRotated(ImDrawList* draw_list, void* tex_id, ImVec2 center, ImVec2 size, float angle_rad, ImVec2 pivot)
{
	ImVec2 half_size = ImVec2(size.x * 0.5f, size.y * 0.5f);
	ImVec2 offset = ImVec2(size.x * pivot.x, size.y * pivot.y);

	ImVec2 corners[4];
	float sinA = std::sin(angle_rad);
	float cosA = std::cos(angle_rad);

	for (int i = 0; i < 4; ++i) {
		float x = (i == 0 || i == 3) ? 0.0f : size.x;
		float y = (i == 0 || i == 1) ? 0.0f : size.y;

		// décalage autour du pivot
		x -= offset.x;
		y -= offset.y;

		// rotation
		float x_rot = x * cosA - y * sinA;
		float y_rot = x * sinA + y * cosA;

		corners[i] = ImVec2(center.x + x_rot, center.y + y_rot);
	}
	draw_list->AddImageQuad(
		tex_id,
		corners[0], corners[1], corners[2], corners[3],
		ImVec2(0, 0), ImVec2(1, 0), ImVec2(1, 1), ImVec2(0, 1),
		IM_COL32_WHITE
	);

}

void display_race_info(const scene_structure& scene, const std::chrono::time_point<std::chrono::high_resolution_clock>& t_start)
{
	using namespace std::chrono;

	// 1. Chrono
	auto now = high_resolution_clock::now();
	auto duration = duration_cast<milliseconds>(now - t_start).count();

	int minutes = (duration / 1000) / 60;
	int seconds = (duration / 1000) % 60;
	int millis = duration % 1000;

	// 2. Data
	int checkpoint = static_cast<int>(scene.car.current_checkpoint.x)+1;
	int max_checkpoint = scene.car.max_checkpoint;
	int lap = static_cast<int>(scene.car.current_lap.x);
	int max_lap = static_cast<int>(scene.car.current_lap.y);

	// 3. Display Text

	std::ostringstream oss;
	oss << "Timer: " << minutes << "'" << std::setw(2) << std::setfill('0') << seconds
		<< "\"" << std::setw(3) << std::setfill('0') << millis
		<< "    Checkpoint: " << checkpoint << "/" << max_checkpoint
		<< "    Lap: " << lap << "/" << max_lap;
	std::string content = "  " + oss.str();


	// 4. Display ImGui
	ImGuiIO& io = ImGui::GetIO();
	ImVec2 text_size = ImGui::CalcTextSize(content.c_str());
	float font_scale = 1.4f;
	text_size.x *= font_scale;
	text_size.y *= font_scale;
	ImVec2 padding = ImVec2(20.0f, 10.0f); // marge interne
	ImVec2 window_size = ImVec2(text_size.x + 2 * padding.x, text_size.y + 2 * padding.y);
	ImVec2 window_pos = ImVec2(io.DisplaySize.x * 0.5f - window_size.x * 0.5f, 10.0f);

	ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always);
	ImGui::SetNextWindowSize(window_size);
	ImGui::SetNextWindowBgAlpha(0.4f); // Gray Transparent background

	ImGui::Begin("Race Info", nullptr,
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoScrollbar |
		ImGuiWindowFlags_NoInputs);

	// Couleur dynamique
	ImU32 color;
	if (lap == max_lap) {
		// Animation rouge qui pulse
		float t = duration_cast<milliseconds>(now.time_since_epoch()).count() / 1000.0f;
		float f = 0.5f+(std::sin(4.0f *M_PI* t)/2);
		int red = static_cast<int>(80 + 120 * f);
		int green = static_cast<int>(143);;
		int blue = static_cast<int>(248);;
		color = IM_COL32(red, green, blue, 255); //80 143 248
	}
	else {
		color = IM_COL32(255, 255, 0, 255); // jaune
	}

	// Effet de relief (shadow)
	ImVec2 shadow_offset = ImVec2(2, 2);
	ImVec2 base_pos = ImGui::GetCursorScreenPos();

	ImDrawList* draw_list = ImGui::GetWindowDrawList();
	ImGui::SetWindowFontScale(font_scale);

	draw_list->AddText(ImVec2(base_pos.x + shadow_offset.x, base_pos.y + shadow_offset.y), IM_COL32(0, 0, 0, 200), content.c_str()); // ombre
	draw_list->AddText(base_pos, color, content.c_str()); // texte

	//ImGui::SetCursorScreenPos(ImVec2(base_pos.x, base_pos.y + text_size.y)); // avance pour éviter recouvrement

	ImGui::End();
}

void display_race_finished(const scene_structure& scene)
{
	std::string content = "Race Finished!";

	ImGuiIO& io = ImGui::GetIO();

	// Taille du texte
	ImVec2 text_size = ImGui::CalcTextSize(content.c_str());
	float font_scale = 3.0f; // très grand

	text_size.x *= font_scale;
	text_size.y *= font_scale;

	// Position centrée
	ImVec2 center = ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f);
	ImVec2 pos = ImVec2(center.x - text_size.x * 0.5f, center.y - text_size.y * 0.5f);

	// Position + taille
	ImGui::SetNextWindowPos(pos, ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(text_size.x + 20.0f, text_size.y + 20.0f)); // marge autour
	ImGui::SetNextWindowBgAlpha(0.4f); // fond semi-transparent

	ImGui::Begin("Race Finished Text", nullptr,
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoScrollbar |
		ImGuiWindowFlags_NoInputs);

	ImGui::SetWindowFontScale(font_scale);

	ImVec2 base_pos = ImGui::GetCursorScreenPos();
	ImVec2 shadow_offset = ImVec2(3, 3);

	ImDrawList* draw_list = ImGui::GetWindowDrawList();

	// Ombre
	draw_list->AddText(ImVec2(base_pos.x + shadow_offset.x, base_pos.y + shadow_offset.y), IM_COL32(0, 0, 0, 200), content.c_str());
	// Texte rouge
	draw_list->AddText(base_pos, IM_COL32(255, 0, 0, 255), content.c_str());

	ImGui::End();
}

void display_checkpoint_time(const scene_structure& scene, const std::chrono::time_point<std::chrono::high_resolution_clock>& t_start)
{
	using namespace std::chrono;

	if (!scene.car.checkpoint_text_visible)
		return;

	auto now = high_resolution_clock::now();
	float elapsed_sec = duration_cast<duration<float>>(now - scene.car.checkpoint_time_display_start).count();
	if (elapsed_sec > 3.0f)
		return;

	// Affichage du temps FIXÉ au moment du checkpoint
	int duration = scene.car.last_checkpoint_time_ms;
	int minutes = (duration / 1000) / 60;
	int seconds = (duration / 1000) % 60;
	int millis = duration % 1000;

	std::ostringstream oss;
	oss << "Checkpoint: " << minutes << "'" << std::setw(2) << std::setfill('0') << seconds
		<< "\"" << std::setw(3) << std::setfill('0') << millis;
	std::string content = oss.str();

	ImGuiIO& io = ImGui::GetIO();
	ImVec2 text_size = ImGui::CalcTextSize(content.c_str());
	float font_scale = 1.4f;

	text_size.x *= font_scale;
	text_size.y *= font_scale;

	ImVec2 pos = ImVec2(io.DisplaySize.x * 0.5f - text_size.x * 0.5f, 60.0f); // sous le timer

	ImGui::SetNextWindowPos(pos, ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(text_size.x + 20.0f, text_size.y + 20.0f));
	ImGui::SetNextWindowBgAlpha(0.1f);

	ImGui::Begin("Checkpoint Text", nullptr,
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoScrollbar |
		ImGuiWindowFlags_NoInputs);

	ImGui::SetWindowFontScale(font_scale);

	ImVec2 base_pos = ImGui::GetCursorScreenPos();
	ImVec2 shadow_offset = ImVec2(2, 2);
	ImDrawList* draw_list = ImGui::GetWindowDrawList();

	draw_list->AddText(ImVec2(base_pos.x + shadow_offset.x, base_pos.y + shadow_offset.y), IM_COL32(0, 0, 0, 180), content.c_str());
	draw_list->AddText(base_pos, IM_COL32(255, 255, 255, 255), content.c_str());

	ImGui::End();
}


void read_game_config(const char* filename, char* string_music, scene_structure &scene) {
	FILE* file = fopen(filename, "r");
	if (!file) {
		perror("Erreur ouverture du fichier config");
		return;
	}

	char line[128]; // Max line
	int values_read = 0;

	while (fgets(line, sizeof(line), file)) {
		line[strcspn(line, "\r\n")] = 0;

		// Ignore commentaires ou lignes vides
		if (line[0] == '#' || line[0] == '\n') continue;

		// Nettoyage des fins de ligne
		line[strcspn(line, "\r\n")] = '\0';

		switch (values_read) {
		case 0:
			strncpy(scene.string_map, line, 63);
			scene.string_map[63] = '\0';
			break;
		case 1:
			strncpy(scene.string_cycle, line, 31);
			scene.string_cycle[31] = '\0';
			break;
		case 2:
			// ignore ligne "Music: ..." explicative
			break;
		case 3:
			strncpy(string_music, line, 63);
			string_music[63] = '\0';
			break;
		case 4:
		{
			char* endptr1;
			errno = 0;
			float lap_value = strtof(line, &endptr1);
			if (errno != 0 || *endptr1 != '\0') {
				perror("strtof error for lap");
				printf("line: '%s', endptr1: '%s'\n", line, endptr1);
			}
			else {
				scene.car.current_lap.y = (int)lap_value; // Convert to int if that's what's expected
			}
		}
		break;
		case 5:
		{
			char* endptr;
			errno = 0;
			float render_value = strtof(line, &endptr);
			if (errno != 0 || *endptr != '\0') {
				fprintf(stderr, "⚠️ Ligne invalide pour render_distance: '%s'\n", line);
			}
			else {
				scene.render_distance = render_value; // Keep as float
			}
		}
		break;
		default:
			break;
		}
		values_read++;
	}

	fclose(file);
}