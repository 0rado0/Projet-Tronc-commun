#include "terrain.hpp"
#include "cmath"



using namespace cgp;


terrain::terrain(int para_Nu, int para_Nv, float para_x_min, float para_x_max, float para_y_min, float para_y_max, map* para_mini_map){

    // Settings values
    Nu = para_Nu;
    Nv = para_Nv;
    x_min = para_x_min;
    x_max = para_x_max;
    y_min = para_y_min;
    y_max = para_y_max;
    mini_map = para_mini_map;

    // Generating the map
	for(int kv=0 ; kv<Nv ; ++kv)
    {
        for(int ku=0 ; ku<Nu ; ++ku)
        {  
            float z = 0;

            position.push_back({(float(ku)/Nu)*(x_max-x_min)+x_min,(float(kv)/Nv)*(y_max-y_min)+y_min,z});
            // Ca peut t'aider je pense
            cgp::vec2 p_world = { (float(ku) / Nu) * (x_max - x_min) + x_min, (float(kv) / Nv) * (y_max - y_min) + y_min };

            cgp::vec3 color_at_pos = mini_map->get_minimap_color_at(p_world); // C'est ta couleur

            color.push_back(color_at_pos);
            /*if(color_at_pos.x==0&& color_at_pos.y == 0&& color_at_pos.z == 0){
                color.push_back({ 0.0f,0.0f,0.0f });
            }
            else {
                color.push_back({ 30.0 / 256,200.0 / 256,23.0 / 256 });
            }*/
			uv.push_back({0,0});
		}
    }
    for(int kv=0 ; kv<Nv-1 ; ++kv)
    {
        for(int ku=0 ; ku<Nu-1 ; ++ku)
        {
            connectivity.push_back({kv*Nu+ku,kv*Nu+ku+1,(kv+1)*Nu+ku});
            connectivity.push_back({(kv+1)*Nu+ku+1,(kv+1)*Nu+ku,kv*Nu+ku+1});
        }
    }
    normal = normal_per_vertex(position, connectivity);
}

void terrain::add_nose(){
    int vmax_pos = 5;
    int vmax_col = 2;
    for(int k = 0; k < position.size(); k++){
        if(color(k)[0] != 0.0f)
            {
                position(k)[2] =  get_height({position(k)[0],position(k)[1]});
                float nose_color = get_color({position(k)[0],position(k)[1],position(k)[2]});
                // Le bruit varie autour de 1.0 pour ne pas "�teindre" la couleur mais juste moduler
                float intensity = 0.8f + 0.4f * nose_color; // dans [0.8 ; 1.2] si noise appartient � [0 ; 1]
                color(k) = color(k) * intensity;
            }
        int v = 0; 
        vec3 closet_v = {float(vmax_pos+1),0.0f,0.0f};
        std::vector<vec2> voisin_dir= {{1,0},{-1,0},{0,1},{0,-1} } ;
        bool v_find = false;
        while(v_find == false && v < vmax_pos) {
            v++;
            for(vec2 v_dir:voisin_dir){
                if(0 < k+v_dir.x*v+Nu*v_dir.y*v && k+v_dir.x*v+Nu*v_dir.y*v < Nu*Nv && color(k+v_dir.x*v+Nu*v_dir.y*v)[0] == 0.0f){
                    v_find = true;
                    closet_v = {v,v_dir.x,v_dir.y};
                    continue;
                }
            }
        }
        if(v_find){
            float v = closet_v.x;
            float dir_x = closet_v.y;
            float dir_y = closet_v.z;
            float coeff_pos = (float(v) * sqrt(std::abs(dir_x) + std::abs(dir_y)) / vmax_pos);
            float x_rel = position( k - dir_x * (vmax_pos-v) - Nu * dir_y * (vmax_pos-v) )[0];
            float y_rel = position( k - dir_x * (vmax_pos-v) - Nu * dir_y * (vmax_pos-v) )[1];
            float h_ref =  get_height({x_rel,y_rel});
            position(k)[2] =   coeff_pos*coeff_pos * h_ref;

            if (v < vmax_col) {
                float coeff_col = (float(v) * sqrt(std::abs(dir_x) + std::abs(dir_y)) / vmax_col);
                vec3 color_ref = color(k - dir_x * (vmax_col - v) - Nu * dir_y * (vmax_col - v));
                color(k) = coeff_col * coeff_col * color_ref;
            }
        }
        if(color(k)[0] == 0.0f){
            position(k)[2] = 0.0f;
        }
        
        
    }
}

float terrain::get_height(const vec2& p) const
{
    int octave = 1; float persitency = 0.5f;float frequency_gain = 0.5f;
    return scale_perlin * noise_perlin(0.05*p, octave , persitency, frequency_gain);
    
}

float terrain::get_height_interpo(const vec2& p) const
{
    // Compute dx and dy (size of a grid cell)
    float dx = (x_max - x_min) / (Nu - 1);
    float dy = (y_max - y_min) / (Nv - 1);

    // Compute indices ku, kv from the point p
    int ku = int((p.x - x_min) / dx);
    int kv = int((p.y - y_min) / dy);

    // Clamp to grid bounds to avoid overflow
    if (ku < 0) ku = 0;
    if (ku >= Nu - 1) ku = Nu - 2;
    if (kv < 0) kv = 0;
    if (kv >= Nv - 1) kv = Nv - 2;

    // Get bottom-left corner of the cell
    float x0 = x_min + ku * dx;
    float y0 = y_min + kv * dy;

    // Local coordinates within the cell (between 0 and 1)
    float u = (p.x - x0) / dx;
    float v = (p.y - y0) / dy;

    // Fetch the 4 surrounding heights
    float z00 = position[ku + Nu * kv].z;
    float z10 = position[ku + 1 + Nu * kv].z;
    float z01 = position[ku + Nu * (kv + 1)].z;
    float z11 = position[ku + 1 + Nu * (kv + 1)].z;

    

    // Use triangle (z00, z10, z11) if u + v > 1 (top triangle)
    // Otherwise use triangle (z00, z01, z11) (bottom triangle)
    vec3 p0, p1, p2;

    if (u + v <= 1.0f)
    {
        // Lower triangle: p0 = (x0,y0), p1 = (x0+dx,y0), p2 = (x0,y0+dy)
        p0 = { x0,       y0,       z00 };
        p1 = { x0 + dx,  y0,       z10 };
        p2 = { x0,       y0 + dy,  z01 };
    }
    else
    {
        // Upper triangle: p0 = (x0+dx,y0+dy), p1 = (x0,y0+dy), p2 = (x0+dx,y0)
        p0 = { x0 + dx,  y0 + dy,  z11 };
        p1 = { x0,       y0 + dy,  z01 };
        p2 = { x0 + dx,  y0,       z10 };
    }

    // Compute normal of the plane defined by (p0, p1, p2)
    vec3 v1 = p1 - p0;
    vec3 v2 = p2 - p0;
    vec3 n = cross(v1, v2); // normal = v1 x v2

    if (std::abs(n.z) < 1e-5f)
        return p0.z; // avoid division by zero if plane is vertical

    // Solve plane equation a*x + b*y + c*z + d = 0 for z
    float a = n.x;
    float b = n.y;
    float c = n.z;
    float d = -dot(n, p0);

    float z = (-a * p.x - b * p.y - d) / c;

    return z;

     //return (1 - u) * (1 - v) * z00 + u * (1 - v) * z10 + (1 - u) * v * z01 + u * v * z11;
}


float terrain::get_color(const vec3& p) const
{
    int octave = 1; float persitency = 0.5f;float frequency_gain = 0.5f;
    return noise_perlin(0.1*p, octave , persitency, frequency_gain);
}


float terrain::sign(float x1,float x2) const
{
    if(x1 - x2>0){
        return 1.0f;
    }
    if(x1 - x2<0){
        return -1.0f;
    }
    if(x1 - x2==0){
        return 1.0f;
    }
    return 0.0f;
}

float terrain::z_getter(int k) const{
    if (k > position.size()) { k = position.size() - 1; }
    return position(k)[2];
}