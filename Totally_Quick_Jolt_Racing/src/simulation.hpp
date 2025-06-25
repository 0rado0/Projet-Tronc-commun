#pragma once
#include <vector>
#include "driving_car.hpp"
#include "cgp/cgp.hpp"
#include "../Input/inputapp/get_input.h"

struct simulation_parameter
{
    // Radius around each vertex considered as a colliding sphere
    float collision_radius = 0.03f;

    // Ratio considered for plastic deformation on the reference shape \in [0,1]	
	float plasticity = 0.0f;
    // Ratio considered for elastic deformation
    float elasticity = 0.0f;
    // Velocity reduction at each time step (* dt);
    float friction = 0.10f;
    // Numer of collision handling step for each numerical integration
    int collision_steps = 10;


    // Time step of the numerical time integration
	float time_step = 0.005f;
};

extern ShareVal InputAction;
extern ShareVal* share_val0;
extern int* is_input_run0;
extern std::mutex* input_mutex0;

void simulation_step(driving_car &car, simulation_parameter &param);

