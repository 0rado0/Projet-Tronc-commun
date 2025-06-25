#pragma once

#include <iostream>
#include <unordered_map>
#include "cgp/cgp.hpp"
#include "map.hpp"

// This definitions allow to use the structures: mesh, mesh_drawable, etc. without mentionning explicitly cgp::
using cgp::mesh;
using cgp::mesh_drawable;
using cgp::vec3;
using cgp::vec2;
using cgp::numarray;
using cgp::timer_basic;


#ifndef TERRAIN_HPP
#define TERRAIN_HPP
// Custom hash function for std::pair<float, float>


//-----Definition de la classe-----
class terrain : public cgp::mesh{
    public:
    //-----Attributs-----

        int Nu;
        int Nv;
        float x_min;
        float x_max;
        float y_min;
        float y_max;
        float scale_perlin = 1.0f;
        map* mini_map;

    //-----Methodes-----
    
    void add_nose();
    float get_height(const vec2& p) const;
    float get_height_interpo(const vec2& p) const;//,int Nu, int Nv, float x_min, float x_max, float y_min, float y_max
    float get_color(const vec3& p) const;
    void fill_terrain_map();
    bool almost_equal(float a, float b, float epsilon);
    //Fonction signe a bouger maybe
    float sign(float x1,float x2) const;
    //-----Constructor-----
    terrain(int para_Nu, int para_Nv, float para_x_min, float para_x_max, float para_y_min, float para_y_max, map* para_mini_map);
    terrain() = default;
    //-----Getters Setters-----
    //Exemples
    // cgp::mesh mesh_getter() const;
    // cgp::vec3& deplacement_setter();
    float z_getter(int k) const;
    
    
    
    protected:

};
#endif