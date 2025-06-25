#pragma once

#include "cgp/cgp.hpp"
#include "deformable.hpp"
#include "environment.hpp"
#include "iostream"
#include <chrono>
#ifndef DRIVINGCAR_HPP
#define DRIVINGCAR_HPP

class terrain;
//-----Definition de la classe-----
class driving_car : public shape_deformable_structure{// 
    public:
    //-----Methodes-----
    void moove(cgp::vec3 diff_forward);
    void checkpoint_checker(cgp::vec3 checkpoint_collide);
    //-----Constructor-----
    driving_car();
    //-----Getters Setters-----
    std::string name_obj_getter() const;
    std::string name_texture_getter() const;
    float x_getter() const;
    float y_getter() const;
    float z_getter() const;
    void x_setter(float x_para);
    void y_setter(float y_para);
    void z_setter(float z_para);

    // Values
    terrain *t;
    cgp::vec3 movement_force;
    cgp::vec3 forward_flatten_previous;
    cgp::vec3 mean_velocity = { 0.0f,0.0f,0.0f };

    float z_angle = 0.01f;
    float flip_angle = 0.01f;
    float delta_z_angle = 0.01f;
    float rising_angle = 0.001f;
    int sens_rotate = 0;
    int avance = 0;
    cgp::vec3 current_checkpoint = {0,1,0};
    cgp::vec2 current_lap = { 1,3 }; // Current, Max
    int max_checkpoint = 0;
    bool finished_race = false;

    std::chrono::time_point<std::chrono::high_resolution_clock> checkpoint_time_display_start;
    bool bool_new_checkpoint = false;
    bool checkpoint_text_visible = false;
    std::vector<int> lap_times_ms;
    int last_checkpoint_time_ms = 0;


    float coeff_velocity_amort = 1.5f;
    float velocity_max = 200.0f*(11.81f/ 60.0f)/coeff_velocity_amort;
    int speed_counter = 1;
    float We = 70;
    float We_max = 7000;

    float W = 0;

    
    protected:
    std::string name_obj = "assets/car/car.obj";
    std::string name_texture = "assets/car/car_texture.png";
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;

};
#endif