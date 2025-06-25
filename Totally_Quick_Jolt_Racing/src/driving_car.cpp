#include "cgp/cgp.hpp"
#include "iostream"
#include "driving_car.hpp"
#include "terrain.hpp"
#include "environment.hpp"

using namespace cgp;

//Methodes
void driving_car::moove(cgp::vec3 diff_forward){
    rotation_transform ZAxis = rotation_transform::from_axis_angle({ 0,0,1 }, z_angle);
    movement_force = ZAxis * diff_forward;

}

void driving_car::checkpoint_checker(vec3 checkpoint_collide) {
    if (checkpoint_collide.x == current_checkpoint.x + 1 || current_checkpoint.z&&checkpoint_collide.y) { // Update if collide is next checkpoint or if currently at last checkpoint, hitting the first one
        current_checkpoint = checkpoint_collide;
        bool_new_checkpoint = true;
        current_lap.x += current_checkpoint.y; // Incremente lap if new checkpoint ID is 0
        if (current_lap.x > current_lap.y) {
            finished_race = true;
            current_lap.x = current_lap.y;
        }
        std::cout << "Checkpoint: " << int(current_checkpoint.x) << "/"<<max_checkpoint<<", Lap: " << current_lap.x<<"/"<<current_lap.y << std::endl;
        if (finished_race) { std::cout << "Race Finished!!!" << std::endl; }
    }
}


//Construcor
driving_car::driving_car(){
    x = 0.0f;
    y = 0.0f;
    z = 0.0f;
}

//Setters getters


std::string driving_car::name_obj_getter() const{
    return name_obj;
}
std::string driving_car::name_texture_getter() const{
    return name_texture;
}
float driving_car::x_getter() const{
    return x;
}
float driving_car::y_getter() const{
    return y;
}
float driving_car::z_getter() const{
    return z;
}
void driving_car::x_setter(float x_para){
    x = x_para;
}
void driving_car::y_setter(float y_para){
    y = y_para;
}
void driving_car::z_setter(float z_para){
    z = z_para;
}