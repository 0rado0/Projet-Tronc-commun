#include "simulation.hpp"
#include "terrain.hpp"
#include "physic.hpp"
#include "environment.hpp"
#include "third_party/src/eigen/Eigen/Core"
#include "third_party/src/eigen/Eigen/SVD"
#include <chrono>
#include <mutex>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

using namespace cgp;

// Compute the polar decomposition of the matrix M and return the rotation such that
//   M = R * S, where R is a rotation matrix and S is a positive semi-definite matrix
mat3 polar_decomposition(mat3 const& M);

// Compute the collision between the particles and the walls
void collision_with_walls(driving_car &car);

// Compute the collision between the particles to each other
void collision_between_particles(driving_car &car, simulation_parameter const& param, float extra_margin, float dynamic_r_threshold, float displacement);

// Compute the shape matching on all the deformable shapes
void shape_matching(driving_car &car, simulation_parameter const& param);


// Define InputAction
ShareVal InputAction = {0, 0, 0, 0};
int* is_input_run0 = get_input_run();
std::mutex* input_mutex0 = get_input_mutex();
ShareVal* share_val0 = get_input_shareval();
float Angle_roues = 0.0;
float Etat_acc = 0.0;
float Acceleration = 0.0;
int last_rapport = 0;

// Perform one simulation step (one numerical integration along the time step dt) using PPD + Shape Matching
void simulation_step(driving_car& car, simulation_parameter& param)
{
    /*___________________ Get_input _________________________*/


    if (*is_input_run0) {
        input_mutex0->lock();
        copyShareVal(share_val0, &InputAction);
        input_mutex0->unlock();
        Angle_roues = -InputAction.angle_roue;
        if (InputAction.frein > 0.5) {
            Etat_acc = -1;
        }
        else if (InputAction.acceleration > 0.5) {
            Etat_acc = 1;
        }
        Acceleration = InputAction.acceleration;
        if (last_rapport != InputAction.rapport) {
            car.speed_counter += InputAction.rapport;
            last_rapport = InputAction.rapport;
        }
    }
    if (car.sens_rotate != 0) {
        Angle_roues = car.sens_rotate;
    }
    if (car.avance != 0) {
        Etat_acc = car.avance;
        Acceleration = 1.0f;
    }

    if (Etat_acc == 1){
        car.moove({0.0f, 1.0f, 0.0f});
    } else if (Etat_acc == -1) {
        car.moove({0.0f, -1.0f, 0.0f});
    }

    /*______________ Registering Previous Reference vector for Angle Update ______________*/


    // The idea is to get the Reference Vector from the two most distant points and keep track of its
    // update to know the accurate additive angle to the car

    /*_____________________________*/
    // Find the vertex furthest from COM (one in the front and one in the back)
    // for tracking rotation, ONLY USED TO FIND THE INDEX, KEEP IT TO CHANGE
    /*
    int furthest_vertex_idx = 0;
    float max_distance = 0.0f;
    vec3 furthest_r_vec;
    int furthest_vertex_idx_neg = 0;
    float max_distance_neg = 0.0f;
    vec3 furthest_r_vec_neg;

    for (int k = 0; k < car.size(); ++k) {
        vec3 r_vec = car.position[k] - car.com;
        r_vec.z = 0; // Only consider XY plane
        float distance = norm(r_vec);
        
        int direction_factor = (dot(r_vec, forward_flatten) > 0) ? 1 : -1;
        if (distance > max_distance && direction_factor==1) {
            max_distance = distance;
            furthest_vertex_idx = k;
            furthest_r_vec = r_vec;
            std::cout << "ah"<< std::endl;
        }
        else if (distance > max_distance_neg && direction_factor == -1) {
            max_distance_neg = distance;
            furthest_vertex_idx_neg = k;
            furthest_r_vec_neg = r_vec;
        }
    }
    std::cout << "furthest_vertex_idx: " << furthest_vertex_idx << std::endl; // 688
    std::cout << "furthest_vertex_idx_neg: " << furthest_vertex_idx_neg << std::endl; // 2298

    */
    /*_____________________________*/

    // Store the original orientation vector
    vec3 r_vec_old_plus = car.position_predict[688] - car.com;
    vec3 r_vec_old_neg = car.position_predict[2298] - car.com;
    vec3 r_vec_old = r_vec_old_plus- r_vec_old_neg;

    /*______________ I. Force Application ______________*/

    /*___ Parameters and Forces Setting ___*/

    // Define of crutial landmarks for our model
    float dt = param.time_step;
    vec3 forward_flatten = car.movement_force;
    forward_flatten.z = 0;

    // Sum of forces calculation
    car.mean_velocity = car.coeff_velocity_amort *average(car.velocity);

    float zangle_adjust = car.z_angle + (float(M_PI / 2));

    int forward = (dot(car.mean_velocity, Etat_acc*forward_flatten) > -0.01) ? 1 : -1;
    if (car.speed_counter == 1 && forward == -1 && Etat_acc != 0 ) { car.speed_counter = -1; }
    if (car.speed_counter == -1 && Etat_acc == 1) { car.speed_counter = 1; }
    if (car.speed_counter == 0 && Etat_acc == 1) { car.speed_counter++; }
    if (car.speed_counter == 0 && Etat_acc ==-1) { car.speed_counter--; }
    if (car.speed_counter > 4) { car.speed_counter = 4; }
    if (car.speed_counter < -1) { car.speed_counter = -1; }

    if (car.speed_counter > 1 && Etat_acc == -1) { 
        if (forward == -1){
            Acceleration = 0.0f;
        }
        Etat_acc = -2;
    }

    float zangle = car.z_angle;
    vec3 sumforce = force_globale(car,0.0f,Angle_roues, Etat_acc, Acceleration, dt)/car.coeff_velocity_amort;
    sumforce = { -sumforce.y ,sumforce.x, 0.0f };

    // Other constant forces
    vec3 const gravity = vec3(0.0f, 0.0f, -9.81f);

    // Calculate the angle rotation force
    vec3 rotation_direction = { forward_flatten.y, -forward_flatten.x, 0.0f };
    if (norm(rotation_direction) > 1e-6f) {
        rotation_direction = rotation_direction / norm(rotation_direction);
    }
    // float dzangle = zangle-car.z_angle; // Base angular velocity factor
    float dzangle = car.delta_z_angle;
    //dzangle = 1.5f * car.sens_rotate;
    // dzangle = (car.speed_counter > 0) ? 1.5f * car.sens_rotate : -1.5f * car.sens_rotate;
    // vec3 rotation_force = rotation_direction * dzangle * (car.sens_rotate != 0 ? 1.0f : 0.0f);

    /*___ Applying Forces to each Vertex ___*/
    int N_vertex = car.position.size();

    // // // // // // // //
    // Pas super d'avoir mis ca la mais tant pis
    float L = 3.0f;      // longueur
    float l = 1.65f;     // largeur
    float m = 1500.0f;   // masse
    float J = m * (L * L + l * l) / 12.0f;
    // // // // // // // //

    if(car.W == 0) {
        for (int k = 0; k < N_vertex; ++k) {
            vec3 p = car.position[k];
            vec3 diff = p - car.com;
            diff.z = 0;
            float r = norm(diff);
            car.W += r * r;
        }
    } // On ne calcule qu'une fois car la voiture n'est pas censee se deformer
    
    for (int k = 0; k < N_vertex; ++k)
    {   
        // Important parameters of the applying forces
        vec3 p = car.position[k];
        vec3 diff = p - car.com;
        diff.z = 0;
        float r = norm(diff);

        if (r < 1e-6f) r = 1e-6f; // Pour eviter les divisions pas 0

        vec3 n = { diff.y / r, -diff.x / r, 0 };
        int rotation_factor = (car.speed_counter > 0) ? 1 : -1;

        // Coefficient of triggering forces
        car.height_ref[k] = car.t->get_height_interpo({p.x, p.y});
        int gravity_factor = (p.z > car.height_ref[k]) ? 1 : 0;
        int ground_contact = (p.z-0.4f < car.height_ref[k]) ? 1 : 0;
        gravity_factor = 1;
        ground_contact = 1;
        // int direction_factor = (dot(diff, forward_flatten) > 0) ? 1 : -1;
        
        
        // Euler Integration

        vec3 total_force = (sumforce * ground_contact + (gravity_factor * gravity) +
            1*rotation_factor*(n * (dzangle * J * r) / (car.W * dt * dt)) / car.coeff_velocity_amort);
        float coeff_frott = 1.0f;
        if (norm(car.mean_velocity)>0.0001f) { coeff_frott = dot(forward_flatten, car.mean_velocity / norm(car.mean_velocity)); }
        car.velocity[k] = car.velocity[k] * (1 - (dt * param.friction*( 1 +10*std::cos(coeff_frott*M_PI/2) ) ) ) + dt * total_force;

        car.position_predict[k] = car.position[k] + dt * car.velocity[k];
    }


    /*______________ II. Apply Colisions and Shape Matching ______________*/


    float displacement = norm(sumforce) * dt;
    float extra_margin = std::min(displacement * 1.0f, 0.5f);
    float dynamic_r_threshold = 1.0f + 0.5f * displacement;

    vec3 com_prev = car.com;
    for (int k = 0; k < param.collision_steps ; ++k) {
        collision_with_walls(car);

        collision_between_particles(car, param, extra_margin, dynamic_r_threshold, displacement);

        if (k % 200 == 0) {
            shape_matching(car, param);
        }
    }
    shape_matching(car, param);

    /*______________ III. Final velocity update and calculate orientation change ______________*/
    
    /*___ Calculating the Reference vector for Angle Update ___*/

    // Update the center of mass with predicted positions

    // Calculate the new r_vec for the furthest vertex
    vec3 r_vec_new_plus = car.position_predict[688] - car.com;
    vec3 r_vec_new_neg = car.position_predict[2298] - car.com;
    vec3 r_vec_new = r_vec_new_plus - r_vec_new_neg;
    r_vec_new.z = 0; // Keep in XY plane

    // Only update angle if we have valid vectors
    if (norm(r_vec_old) > 1e-6f && norm(r_vec_new) > 1e-6f) {
        // Normalize both vectors
        r_vec_old = r_vec_old / norm(r_vec_old);
        r_vec_new = r_vec_new / norm(r_vec_new);

        // Calculate angle between old and new r_vec using atan2 of both vectors
        float old_angle_z = std::atan2(r_vec_old.y, r_vec_old.x);
        float new_angle_z = std::atan2(r_vec_new.y, r_vec_new.x);
        float old_angle_flip = std::atan2(r_vec_old.z, std::sqrt(r_vec_old.x * r_vec_old.x + r_vec_old.y * r_vec_old.y));
        float new_angle_flip = std::atan2(r_vec_new.z, std::sqrt(r_vec_new.x * r_vec_new.x + r_vec_new.y * r_vec_new.y));

        // Calculate the angle difference (this handles wrapping around 2pi)
        float angle_diff_z = new_angle_z - old_angle_z;
        float angle_diff_flip = new_angle_flip - old_angle_flip;

        // Handle angle wrapping for shortest path
        if (angle_diff_z > M_PI) angle_diff_z -= 2 * M_PI;
        if (angle_diff_z < -M_PI) angle_diff_z += 2 * M_PI;
        if (angle_diff_flip > M_PI) angle_diff_flip -= 2 * M_PI;
        if (angle_diff_flip < -M_PI) angle_diff_flip += 2 * M_PI;

        // Update car's z_angle by adding the change
        car.z_angle += angle_diff_z;
        car.flip_angle += angle_diff_flip;

    }

    /*___ Update velocities and positions ___*/
    for (int k = 0; k < car.size(); ++k)
    {
        // Update velocity based on position change
        car.velocity[k] = (car.position_predict[k] - car.position[k]) / dt;

        // Update the vertex position
        car.position[k] = car.position_predict[k];
    }

    /*______________ Update & Reinitialize values ______________*/
    // Update values
    vec3 com = car.com;
    car.x_setter(com.x);
    car.y_setter(com.y);
    car.z_setter(com.z);

    // Update forward_flatten_previous adjusting by car.avance if not null
    if (norm(forward_flatten * Etat_acc) > 0.001) { 
        car.forward_flatten_previous = forward_flatten;
        
    }

    // Re-initialize values
    car.movement_force = { 0.0f, 0.0f, 0.0f };
    car.sens_rotate = 0;
    car.avance = 0;
    Angle_roues = 0.0;
    Etat_acc = 0.0;
    Acceleration = 0.0;
}


// Compute the shape matching on all the deformable shapes
void shape_matching(driving_car &car, simulation_parameter const& param)
{
    // Arguments:
    //   car: deformable shape
    //   Each deformable shape structure contains 
    //        - the predicted position [deformable].position_predicted
    //        - the center of mass [deformable].com
    //        - the reference position [deformable].position_reference
    //        - the center of mass from the reference position [deformable].com_reference


    // Algorithm:
    //   - For all deformable shapes
    //     - Update the com (center of mass) from the predicted position
    //     - Compute the best rotation R such that p_predicter - com = R (p_reference-com_reference)
    //        - Compute the matrix M = \sum r r_ref^T
    //            with r  = p_predicted - com
    //                 r_ref = p_reference - com_reference
    //        - Compute R as the polar decomposition of M
    //     - Set the new predicted position as p_predicted = R (p_reference-com_reference) + com
    // 
    
    car.com = average(car.position_predict);
    mat3 T = mat3::build_zero();
    for (int i = 0; i < car.position_predict.size();i++) {
        vec3 r = car.position_predict[i] - car.com;
        vec3 r_ref = car.position_reference[i] - car.com_reference;
        T += tensor_product(r, r_ref);
    }
    mat3 R = polar_decomposition(T);
    float alpha = 0.5f;
    for (size_t i = 0; i < car.position_predict.size(); ++i) {
        vec3 goal = R * (car.position_reference[i] - car.com_reference) + car.com;
        car.position_predict[i] += alpha * (goal - car.position_predict[i]);
    }
    
}




void collision_between_particles(driving_car& car, simulation_parameter const& param, float extra_margin, float dynamic_r_threshold, float displacement)
{
    float r = param.collision_radius; // radius of colliding sphere

    bounding_box bp;
    bp.initialize(car.position_predict);
    bp.extends(r + extra_margin);

    // Interaction with static meshes
    float rebounce_factor = 5.5f + 0.5f * norm(average(car.velocity));
    for (entity &e: project::All_entities){
        if (!e.collision)
            continue;
        if (bounding_box::collide(bp, e.box))
        {   
            if (e.type == "checkpoint") {
                vec3 prev_check = car.current_checkpoint;
                car.checkpoint_checker(e.extra_data[3]);
                if (prev_check.x != car.current_checkpoint.x) {
                    int ID_next = e.extra_data[3].x + 1;
                    e.tracked = false;
                    for (entity &e_next : project::All_entities) {
                        if (e_next.type == "checkpoint" && e_next.extra_data[3].x == ID_next) {
                            e_next.tracked = true;
                        }
                    }
                }

            }

            for (int pi = 0; pi < car.position_predict.size(); ++pi) {
                vec3& p = car.position_predict[pi];

                vec3 diff;
                float d2;
                vec3 reflection_dir;
                vec3 correction;


                // Normal AAB Collision
                vec3 p_clamped = {
                    clamp(p.x, e.box.p_min.x, e.box.p_max.x),
                    clamp(p.y, e.box.p_min.y, e.box.p_max.y),
                    clamp(p.z, e.box.p_min.z, e.box.p_max.z)
                };
                diff = p - p_clamped;
                d2 = dot(diff, diff);


                if (d2 < (r + extra_margin) * (r + extra_margin)) { // If collision
                    float d = std::sqrt(d2);
                    vec3 dir;
                    vec3 correction;

                    if (r < dynamic_r_threshold && e.type == "tree") {
                        dir = diff;

                        reflection_dir = dir;

                        // Normalize reflection_dir
                        float norm_reflection = norm(reflection_dir);
                        if (norm_reflection > 1e-6f) {
                            reflection_dir = reflection_dir / norm_reflection; // Secured Normalization
                        }
                        else {
                            reflection_dir = -car.forward_flatten_previous / norm(car.forward_flatten_previous);
                        }

                        // Amplification of the correction
                        correction = rebounce_factor * std::abs(r - d) * reflection_dir;
                        
                    }
                    else if (e.type == "checkpoint") {
                        dir = -car.movement_force * Etat_acc;

                        reflection_dir = dir;

                        // Normalize reflection_dir
                        float norm_reflection = norm(reflection_dir);
                        if (norm_reflection > 1e-6f) {
                            reflection_dir = reflection_dir / norm_reflection; // Secured Normalization
                        }
                        else {
                            reflection_dir = -car.forward_flatten_previous / norm(car.forward_flatten_previous);
                        }

                        // Ramener tout � z=0 pour les tests 2D
                        vec3 center1 = e.extra_data[0];
                        vec3 center2 = e.extra_data[1];

                        vec3 pos_flat = { p.x, p.y, 0.0f };

                        float checkpoint_radius = 0.8f;

                        bool collided = false;
                        vec3 total_correction = { 0, 0, 0 };

                        for (vec3 const& c : { center1, center2 }) {
                            vec3 diff2d = pos_flat - c;
                            float d = norm(diff2d);
                            //std::cout << "dist to checkpoint circle: " << d << " (r+ checkpoint_radius =" << r + checkpoint_radius << ")\n";
                            if (d < r + checkpoint_radius) {
                                vec3 dir = (d > 1e-6f) ? diff2d / d : vec3{ 1.0f, 0.0f, 0.0f }; // s�curit�
                                float penetration = std::abs(r + checkpoint_radius - d);
                                total_correction += reflection_dir *penetration* rebounce_factor;
                                collided = true;
                            }
                        }

                        if (collided) { // If touching poles of arch
                            correction = total_correction;
                            
                        }

                        else { // If going through arch
                            correction = { 0.0f,0.0f,0.0f };
                        }
                    }
                    else { // Other collisions
                        correction = diff;
                    }

                    // Verify if the correction is too weak and increase it
                    float max_correction_magnitude = std::max(0.2f, displacement * 0.5f); // Correction floor
                    while (norm(correction) > max_correction_magnitude) {
                        correction /= 4.0f;
                    }

                    p += correction;
                    // For Dynamics objets we'd do
                    /*
                    if (dist2 < param.collision_radius * param.collision_radius && dist2 > 1e-8f) { // avoid division by zero
                        float dist = std::sqrt(dist2);
                        vec3 direction = diff / dist;
                        vec3 correction = 0.5f * (param.collision_radius - dist) * direction;

                        p_i += correction;
                        p_j -= correction;
                        */
                }
            }
        }
    } 
}


// Compute the collision between the particles and the walls
// Note: This function is already pre-coded
void collision_with_walls(driving_car &car)
{
    int N_vertex = car.size();
    for(int k=0; k<N_vertex; ++k)
    {
        vec3& p = car.position_predict[k];

        if (p.x > car.t->x_max-2.0f) { p.x = car.t->x_max - 2.0f; }
        if (p.x < car.t->x_min + 2.0f) { p.x = car.t->x_min + 2.0f; }
        if (p.y > car.t->y_max - 2.0f) { p.y = car.t->y_max - 2.0f; }
        if (p.y < car.t->y_min + 2.0f) { p.y = car.t->y_min + 2.0f; }

        // Collision with the ground
        float zmin = car.height_ref[k];
        //zmin = 0.0f;
        if(p.z< zmin) {
            p.z = zmin+0.2f;

        }
    }
}


// Compute the polar decomposition of the matrix M and return the rotation such that
//   M = R * S, where R is a rotation matrix and S is a positive semi-definite matrix
mat3 polar_decomposition(mat3 const& M) 
{
    // The function uses Eigen to compute the SVD of the matrix M
    //  Give: SVD(M) = U Sigma V^tr
    //  We have: R = U V^tr, and S = V Sigma V^tr
	Eigen::Matrix3f A; A << M(0,0),M(0,1),M(0,2), M(1,0),M(1,1),M(1,2), M(2,0),M(2,1),M(2,2);
	Eigen::JacobiSVD<Eigen::Matrix3f> svd(A, Eigen::ComputeThinU | Eigen::ComputeThinV);
	Eigen::Matrix3f const R = svd.matrixU()* (svd.matrixV().transpose());

	return {R(0,0),R(0,1),R(0,2), R(1,0),R(1,1),R(1,2), R(2,0),R(2,1),R(2,2)};
}
