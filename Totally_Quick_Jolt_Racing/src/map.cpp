#include "map.hpp"
#include "environment.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

using namespace cgp;

/*______________ Fuction to verify OpenGL error ______________*/

void checkGLError(const char* operation) {
    GLenum error;
    while ((error = glGetError()) != GL_NO_ERROR) {
        std::string errorStr;
        switch (error) {
        case GL_INVALID_ENUM: errorStr = "GL_INVALID_ENUM"; break;
        case GL_INVALID_VALUE: errorStr = "GL_INVALID_VALUE"; break;
        case GL_INVALID_OPERATION: errorStr = "GL_INVALID_OPERATION"; break;
        case GL_STACK_OVERFLOW: errorStr = "GL_STACK_OVERFLOW"; break;
        case GL_STACK_UNDERFLOW: errorStr = "GL_STACK_UNDERFLOW"; break;
        case GL_OUT_OF_MEMORY: errorStr = "GL_OUT_OF_MEMORY"; break;
        default: errorStr = "Unknown error"; break;
        }
        std::cerr << "OpenGL error after " << operation << ": " << errorStr << " (" << error << ")" << std::endl;
    }
}

/*______________ Loading Shader ______________*/

GLuint map::LoadShader(const char* vertexPath, const char* fragmentPath) {
    std::ifstream vShaderFile(vertexPath);
    std::ifstream fShaderFile(fragmentPath);
    if (!vShaderFile.is_open() || !fShaderFile.is_open()) {
        std::cerr << "Erreur ouverture fichiers shaders: " << vertexPath << " ou " << fragmentPath << std::endl;
        return 0;
    }

    std::stringstream vShaderStream, fShaderStream;
    vShaderStream << vShaderFile.rdbuf();
    fShaderStream << fShaderFile.rdbuf();

    std::string vShaderCode = vShaderStream.str();
    std::string fShaderCode = fShaderStream.str();

    const char* vShaderSource = vShaderCode.c_str();
    const char* fShaderSource = fShaderCode.c_str();

    // Compile vertex shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vShaderSource, nullptr);
    glCompileShader(vertexShader);

    GLint success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
        std::cerr << "Erreur compilation vertex shader: " << infoLog << std::endl;
    }

    // Compile fragment shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fShaderSource, nullptr);
    glCompileShader(fragmentShader);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
        std::cerr << "Erreur compilation fragment shader: " << infoLog << std::endl;
    }

    // Link of the shaders
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
        std::cerr << "Erreur linkage shader program: " << infoLog << std::endl;
    }

    // Clean-up
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

/*______________ Mini Map FBO initialization ______________*/

void map::initMinimapFBO(const char* vertexPath, const char* fragmentPath) {


    // Free existing ressources if necessary
    if (minimapVAO != 0) {
        glDeleteVertexArrays(1, &minimapVAO);
        glDeleteBuffers(1, &minimapVBO);
        glDeleteBuffers(1, &minimapEBO);
    }

    // Initialize at 0
    minimapVAO = 0;
    minimapVBO = 0;
    minimapEBO = 0;

    // Creation of VAO, VBO and EBO objets
    glGenVertexArrays(1, &minimapVAO);
    glGenBuffers(1, &minimapVBO);
    glGenBuffers(1, &minimapEBO);

    // Verifying they are created
    if (minimapVAO == 0 || minimapVBO == 0 || minimapEBO == 0) {
        std::cerr << "Erreur: Impossible de créer les objets OpenGL pour la minimap" << std::endl;
        return;
    }

    // VAO et VBO Creation for the Mini Map Quad
    float minimap_quad[] = {
        // pos        // tex
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f,
        -1.0f,  1.0f,  0.0f, 1.0f
    };
    GLuint quad_indices[] = {
        0, 1, 2,
        2, 3, 0
    };

    glGenVertexArrays(1, &minimapVAO);
    checkGLError("glGenVertexArrays");
    glGenBuffers(1, &minimapVBO);
    glGenBuffers(1, &minimapEBO);

    glBindVertexArray(minimapVAO);

    glBindBuffer(GL_ARRAY_BUFFER, minimapVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(minimap_quad), minimap_quad, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, minimapEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quad_indices), quad_indices, GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Texcoord attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
    shader_minimap = LoadShader(vertexPath, fragmentPath);

    // Verifying the shader was correclty created
    if (shader_minimap == 0) {
        std::cerr << "Erreur: shader_minimap n'a pas pu �tre cr��!" << std::endl;
        return;
    }

    // Verifying the uniforms exist
    glUseProgram(shader_minimap);
    GLint modelLoc = glGetUniformLocation(shader_minimap, "modelMatrix");
    GLint texLoc = glGetUniformLocation(shader_minimap, "minimapTexture");

    if (modelLoc == -1) {
        std::cerr << "Attention: Uniform 'modelMatrix' non trouv� dans le shader" << std::endl;
    }
    if (texLoc == -1) {
        std::cerr << "Attention: Uniform 'minimapTexture' non trouv� dans le shader" << std::endl;
    }
    glUseProgram(0);
}


/*______________ Loading Texture ______________*/

void map::loadTexture(const char* path, GLuint& textureID, bool reverse) {
    if (textureID != 0)
        return;

    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    if (reverse) {
        stbi_set_flip_vertically_on_load(true);
    }
    else {
        stbi_set_flip_vertically_on_load(false);
    }
    unsigned char* data = stbi_load(path, &mapWidth, &mapHeight, &channels, 4);
    if (imgHeight == 0) { imgHeight = mapHeight ;}
    if (data) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, mapWidth, mapHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
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

/*______________ Conversion Mini Map to Reel Map ______________*/

ImVec2 map::worldToMinimap (const cgp::vec2& pos, float refX, float refY, float refAngle, ImVec2 center, float size, float zoom) const {
    // 1. Calculate object position relative to car
    float relX = pos.x - refX;
    float relY = pos.y - refY;

    // 2. Rotate coordinates based on car's orientation
    float cosA = std::cos(refAngle);
    float sinA = std::sin(refAngle);
    float rotX = relX * cosA + relY * sinA;
    float rotY = relX * sinA - relY * cosA;

    // 3. Convert to screen coordinates centered on minimap
    float coeff = (size * correct_scale * scale);
    float screenX = center.x + rotX * zoom * coeff;
    float screenY = center.y + rotY * zoom * coeff;

    //// Debug output
    //std::cout << "World pos: " << pos.x << "," << pos.y
    //    << " | Car pos: " << carX << "," << carY
    //    << " | Rel: " << relX << "," << relY
    //    << " | Rot: " << rotX << "," << rotY
    //    << " | Screen: " << screenX << "," << screenY << std::endl;

    return ImVec2(screenX, screenY);
    };

/*______________ Conversion Reel Map to Mini Map ______________*/

ImVec2 map::Minimaptoworld(const cgp::vec2& pos, float size) const {

    float coeff = size/(width/2);
    float screenX = (pos.x - width / 2) * coeff;
    float screenY = (pos.y - height / 2) * coeff;

    return ImVec2(screenX, screenY);
};

/*______________ Fill a tab of vec3 color of the mini map ______________*/

void map::fill_minimap_color()
{
    if (textureMap == 0) {
        std::cerr << "Erreur: textureMap n'est pas initialisée." << std::endl;
        return;
    }

    glBindTexture(GL_TEXTURE_2D, textureMap);

    // Getting size of the texture
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);

    minimap_color_matrix.resize(height);
    for (int y = 0; y < height; ++y) {
        minimap_color_matrix[y].resize(width);
    }

    width = minimap_color_matrix[0].size();
    height = minimap_color_matrix.size();

    // Make a temp tab to get the pixels (RGBA8 → 4 bytes/pixel)
    std::vector<unsigned char> pixel_data(width * height * 4);

    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixel_data.data());

    // Fill minimap_color_matrix with RGB colors
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            int index = 4 * (y * width + x);
            float r = pixel_data[index] / 255.0f;
            float g = pixel_data[index + 1] / 255.0f;
            float b = pixel_data[index + 2] / 255.0f;
            minimap_color_matrix[y][x] = { r, g, b };
        }
    }

    glBindTexture(GL_TEXTURE_2D, 0);
}

/*______________ Get the color in the Minimap from Real World coords ______________*/

cgp::vec3 map::get_minimap_color_at(const cgp::vec2& world_pos) const
{
    if (minimap_color_matrix.empty()) {
        std::cerr << "Erreur: minimap_color_matrix est vide ! As-tu bien appelé fill_minimap_color() avant ?" << std::endl;
        return { 0.0f, 0.0f, 0.0f };
    }

    ImVec2 center = ImVec2(0.0f, 0.0f);
    ImVec2 idx = worldToMinimap(world_pos,0.0f,0.0f,0.0f,center,1.0f,1.0f);
    int x = int(idx.x);
    int y = -int(idx.y);
    x += width / 2;
    y += height / 2;
    if (x < 0 || x >= width || y < 0 || y >= height) {
        return { 255.0f, 255.0f, 255.0f };
    }
    return minimap_color_matrix[y][x];
}

/*______________ Drawing the Map with Translation and Rotation ______________*/

void map::drawRotatedMap(int xcenter, int ycenter, int radius, float xfocus, float yfocus, float zangle, float scale, float zoom) {
    using namespace cgp;

    // Check that OpenGL objects are valid
    if (minimapVAO == 0 || shader_minimap == 0 || textureMap == 0) {
        std::cerr << "Error: OpenGL objects not initialized for minimap" << std::endl;
        return;
    }

    // Save current OpenGL state
    GLint lastProgram;
    glGetIntegerv(GL_CURRENT_PROGRAM, &lastProgram);

    // Define rendering region
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    glViewport(xcenter - radius, ycenter - radius, 2 * radius, 2 * radius);

    // Activate shader
    glUseProgram(shader_minimap);
    checkGLError("glUseProgram");

    // Calculate normalized position (-1 to 1 range)
    float normalizedX = 2.0f * (xfocus / mapWidth) - 1.0f;
    float normalizedY = -(2.0f * (yfocus / mapHeight) - 1.0f); // Flipped for OpenGL

    // Create transformation matrix
    affine T = affine();

    // Apply transformations in order:
    // 1. First translate to center (origin)
    // 2. Then rotate around origin
    // 3. Finally translate back to show car at center

    // Create a matrix that will:
    // - Position the map so the car is at the center of our viewport
    // - Rotate the map around the car's position
    T.set_scaling(1.0f); // Adjust zoom if needed
    T.set_rotation(rotation_transform::from_axis_angle({ 0, 0, 1 }, zangle));
    T.set_translation({ -normalizedX, -normalizedY, 0.0f });

    // Set uniform for matrix
    mat4 model = T.matrix();
    GLint modelLoc = glGetUniformLocation(shader_minimap, "modelMatrix");
    if (modelLoc != -1) {
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &model.data[0][0]);
        checkGLError("glUniformMatrix4fv");
    }
    GLint offsetLoc = glGetUniformLocation(shader_minimap, "texOffset");
    if (offsetLoc != -1) {
        float u = xfocus*scale;
        float v = yfocus*scale;
        glUniform2f(offsetLoc, u, v);
    }

    GLint zoomLoc = glGetUniformLocation(shader_minimap, "zoom");
    if (zoomLoc != -1) {
        glUniform1f(zoomLoc, zoom);
    }

    // Set up texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureMap);

    GLint texLoc = glGetUniformLocation(shader_minimap, "minimapTexture");
    if (texLoc != -1) {
        glUniform1i(texLoc, 0);
    }

    // Draw the quad
    glBindVertexArray(minimapVAO);

    // Enable alpha blending for the circle
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    // Restore OpenGL state
    glBindVertexArray(0);
    glUseProgram(lastProgram);
    glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
    glDisable(GL_BLEND);
}


/*______________ Final Mini Map Render ______________*/

void map::renderMinimap(float carX, float carY, float carAngle, int screenwidth, int screenheight) {


    static bool warmup = false;
    if (!warmup) {
        ImGui::SetNextWindowPos(ImVec2(-1000, -1000));
        ImGui::SetNextWindowSize(ImVec2(10, 10));
        ImGui::Begin("hidden", nullptr, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoDecoration);
        drawRotatedMap(-999, -999, 1, carX, carY, carAngle, scale, 1); // Valeurs bidon
        ImGui::End();
        warmup = true;
    }
    
    /*______________ Mini Map Caracteristics ______________*/

    // Position et taille
    ImVec2 minimapPos = ImVec2(10, screenheight-(160* screenheight/540)); //10, 380
    //std::cout <<"width: " << screenwidth << ", height: " << screenheight << std::endl; //Fenêtre réduite 960 540
    ImVec2 minimapSize = ImVec2(screenwidth*150/960, screenheight*150/510);
    //std::cout << "minimapSize: " << minimapSize.x <<","<<minimapSize.y<< std::endl;
    float radius = (minimapSize.x / 2)*0.95;
    ImVec2 center = ImVec2(minimapPos.x + minimapSize.x / 2, minimapPos.y + minimapSize.y / 2);
    float zoom = 4;

    /*______________ ImGUI window settings ______________*/
    
    ImGui::SetNextWindowPos(minimapPos);
    ImGui::SetNextWindowSize(minimapSize);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0,0,0,0));
    ImGui::Begin("Mini-map", nullptr, 
                 ImGuiWindowFlags_NoTitleBar | 
                 ImGuiWindowFlags_NoResize | 
                 ImGuiWindowFlags_NoScrollbar |
                 ImGuiWindowFlags_NoBackground |
                 ImGuiWindowFlags_NoInputs);
    
    /*______________ Warm-up textures (1st frame) ______________*/

    static bool texturesWarmedUp = false;
    if (!texturesWarmedUp) {
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        ImVec2 dummyPos = ImVec2(-1000, -1000);
        ImVec2 size = ImVec2(2, 2);
        ImVec2 dummyPosTranslated = ImVec2(dummyPos.x + size.x, dummyPos.y + size.y);

        for (entity& e : project::All_entities.List) {
            if (e.textureID != 0)
                drawList->AddImage((void*)(intptr_t)e.textureID, dummyPos, dummyPosTranslated);
        }
        if (pin_texture != 0)
            drawList->AddImage((void*)(intptr_t)pin_texture, dummyPos, dummyPosTranslated);
        if (textureCursor != 0)
            drawList->AddImage((void*)(intptr_t)textureCursor, dummyPos, dummyPosTranslated);

        texturesWarmedUp = true;
    }

    if (warmup) {
        glFinish(); // Force le GPU à finir tout le rendu immédiatement
    }

    /*______________ Drawing Rotating Mini Map ______________*/

    drawRotatedMap(10 + minimapSize.x / 2, (160 * screenheight / 540) - (minimapSize.y / 2), radius, carX, carY, carAngle, scale, zoom);
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    
    // Adding a border (circle shape)
    drawList->AddCircle(center, radius, IM_COL32(255, 255, 255, 255), 32, 2.0f);

    /*______________ Adding/Drawing other elements on the Mini Map ______________*/

    // Display all icon entities
    for (entity& e : project::All_entities.List) {

        ImVec2 pos = worldToMinimap(e.position, carX, carY, carAngle, center, minimapSize.x, imgHeight / 2 * scale * scale * zoom);
        float dx = pos.x - center.x;
        float dy = pos.y - center.y;
        float dist2 = dx * dx + dy * dy;

        if (dist2 <= radius * 0.90 * radius * 0.90) { // If inside of the circle
            ImVec2 imageSize = ImVec2(screenwidth * 16 / 960, screenheight * 16 / 510);
            ImGui::SetCursorScreenPos(ImVec2(pos.x - imageSize.x / 2, pos.y - imageSize.y / 2));
            ImGui::Image((void*)(intptr_t)e.textureID, ImVec2(screenwidth * 16 / 960, screenheight * 16 / 510));
        }
        //if (e.tracked) { std::cout << "aaaaaaah" << std::endl; }
        else if (dist2 > radius * 0.95 * radius * 0.95 && e.tracked) { // If supposedly outisde of the circle
            // Normalize the direction vector
            float dist = std::sqrt(dist2);
            float normalizedDx = dx / dist;
            float normalizedDy = dy / dist;

            // Place on the border by multiplying by the radius
            pos.x = center.x + normalizedDx * radius * 0.95;
            pos.y = center.y + normalizedDy * radius * 0.95;
            ImVec2 imageSize = ImVec2(screenwidth * 16 / 960, screenheight * 16 / 510);
            ImGui::SetCursorScreenPos(ImVec2(pos.x - imageSize.x / 2, pos.y - imageSize.y / 2));
            
            
            ImGui::Image((void*)(intptr_t)pin_texture, ImVec2(screenwidth * 16 / 960, screenheight * 16 / 510));
        }
    }
    // Drawing the centered cursor (must be done regardless of entities)
    ImVec2 imageSize = ImVec2(screenwidth * 16 / 960, screenheight * 16 / 510);
    ImGui::SetCursorScreenPos(ImVec2(center.x - imageSize.x / 2, center.y - imageSize.y / 2));
    ImGui::Image((void*)(intptr_t)textureCursor, imageSize);

    /*______________ End of Rendering ______________*/

    ImGui::PopStyleColor();
    ImGui::End();
    
}



/*
// Display all icon entities
    for (entity& e : project::All_entities.List) {

        if (e.textureID == 0) { // Only load if not already loaded
            std::string path_texture;
            if (e.tracked) { path_texture = project::path + "assets/mini_map/red_pin.png"; }
            else {
                if (e.type == "tree") { path_texture = project::path + "assets/mini_map/Tree_minimap.png"; }
                else if (e.type == "grass") { path_texture = project::path + "assets/mini_map/grass_minimap.png"; }
                else if (e.type == "checkpoint") { path_texture = project::path + "assets/mini_map/Checkpoint_mini_map.png"; }
                else { path_texture = project::path + "assets/mini_map/red_pin.png"; }
            }
            loadTexture(path_texture.c_str(), e.textureID);
        }

        ImVec2 pos = worldToMinimap(e.position, carX, carY, carAngle, center, minimapSize.x, imgHeight / 2 * scale * scale * zoom);
        float dx = pos.x - center.x;
        float dy = pos.y - center.y;
        float dist2 = dx * dx + dy * dy;

        if (dist2 <= radius * 0.90 * radius * 0.90) { // If inside of the circle
            ImVec2 imageSize = ImVec2(screenwidth * 16 / 960, screenheight * 16 / 510);
            ImGui::SetCursorScreenPos(ImVec2(pos.x - imageSize.x / 2, pos.y - imageSize.y / 2));
            ImGui::Image((void*)(intptr_t)e.textureID, ImVec2(screenwidth * 16 / 960, screenheight * 16 / 510));
        }

    }
    // Display tracked entities
    for (entity& e : project::Tracked_entities) {
        ImVec2 pos = worldToMinimap(e.position, carX, carY, carAngle, center, minimapSize.x, zoom*scale);
        float dx = pos.x - center.x;
        float dy = pos.y - center.y;
        float dist2 = dx * dx + dy * dy;

        if (dist2 > radius * 0.95 * radius * 0.95) { // If supposedly outisde of the circle
            // Normalize the direction vector
            float dist = std::sqrt(dist2);
            float normalizedDx = dx / dist;
            float normalizedDy = dy / dist;

            // Place on the border by multiplying by the radius
            pos.x = center.x + normalizedDx * radius * 0.95;
            pos.y = center.y + normalizedDy * radius * 0.95;
        }
        ImVec2 imageSize = ImVec2(screenwidth * 16 / 960, screenheight * 16 / 510);
        ImGui::SetCursorScreenPos(ImVec2(pos.x - imageSize.x / 2, pos.y - imageSize.y / 2));
        if (e.textureID == 0) { // Only load if not already loaded
            std::string path_texture = project::path + "assets/mini_map/red_pin.png";
            loadTexture(path_texture.c_str(), e.textureID);
        }
        ImGui::Image((void*)(intptr_t)e.textureID, ImVec2(screenwidth * 16 / 960, screenheight * 16 / 510));
    }

    // Drawing the centered cursor
    ImGui::SetCursorScreenPos(ImVec2(center.x - 8, center.y - 8));
    ImGui::Image((void*)(intptr_t)textureCursor, ImVec2(screenwidth * 16 / 960, screenheight * 16 / 510));

    /*______________ End of Rendering ______________

ImGui::PopStyleColor();
ImGui::End();

}
*/


