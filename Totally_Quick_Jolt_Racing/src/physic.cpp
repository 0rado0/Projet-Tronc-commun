#include <cmath> // /!\ ajouter -lm à la compilation
#include "physic.hpp"
#include "cgp/cgp.hpp"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

using cgp::vec3;

// Constantes physiques
#define g 9.81 // Accélération de la pesanteur (m/s²)
#define rho 1.225 // Masse volumique de l'air (kg/m³)

// Constantes du véhicule
#define m 1500 // Masse du véhicule (kg)
#define A 2.5 // Surface frontale (m²)
#define Rwheel 0.2032 // Rayon de la roue (m) -- 16 pouces de diamètre
#define L 3 // Empattement (m)
#define G 3.5 // Rapport du différentiel final
#define Cd 0.3 // Coefficient de traînée
#define Cr 0.01 // Coefficient de frottement roulant -- ?????

// Constantes des forces
#define Cdrag (1/2) * Cd * rho * A // (kg/m)
#define Crr 176.6 // Cr * m * g (kg/s)
#define Cbrake 5000 // (N)

/**
 * @brief Calcul le bilan des forces appliquées à TOUTE la voiture (SAUF la réaction du sol)
 *
 * @param car mesh deformable de la voiture qui se déplace dans le monde
 * @param slope Inclinaison locale de la route dans le sens de la voiture (radians) -- descendante si positif
 * @param steer Angle de braquage (radians) -- positif si à droite
 * @param accel Accélération -- 1 si accélération, 0 si freinage, -1 pour freinage
 * @param Caccel Coefficient d'accélération -- 0 si pas d'accélération, 1 si pied au plancher
 * @return (Vec3) Bilan des forces appliquées à TOUTE la voiture
 */
vec3 force_globale(driving_car &car, float slope, float steer, float accel, float Caccel, float dt) {

    float PX = car.com.x; // PX Position en X (m) -- repère monde
    float PY = car.com.y; // PY Position en Y (m) -- repère monde
    float VX = car.mean_velocity.x; // VX Vélocité en X (m/s) -- repère monde
    float VY = car.mean_velocity.y; // VY Vélocité en Y (m/s) -- repère monde
    float yaw = car.z_angle; // yaw Angle de lacet (radians) -- repère monde -- Passage par référence
    int gear = car.speed_counter; // gear Vitesse de la boîte de vitesse -- 1 à 4

    steer = steer / 10;
    
    // Courbe de couple moteur
    float rpm[] = {800, 1000, 1500, 2000, 2500, 3000, 3500, 4000, 4500, 5000, 5500, 6000, 6500, 7000}; // (tr/min)
    float torque[] = {80, 120, 180, 250, 300, 340, 360, 370, 375, 370, 360, 340, 310, 270}; // (Nm)

    // Boîte de vitesse
    float ratios[] = {3.36, 2.09, 1.48, 1.12};
    float gk = 0.0f;
    if (gear > 0 ){
        gk = ratios[gear - 1]; // Rapport de boîte de vitesse
    } else {
        gk = ratios[0]; // Marche arrière ou point mort
    }

    // Passage en repère local
    float Vx = cosf(yaw) * VX + sinf(yaw) * VY;
    float Vy = -sinf(yaw) * VX + cosf(yaw) * VY;

    // Calcul de Twheel
    float We = (sqrtf(Vx * Vx + Vy * Vy) * 60 * gk * G) / (2 * M_PI * Rwheel); // Vitesse du moteur (tr/min)
    car.We = We;

    float Te = 0.0f; // Couple moteur associé à We
    if (We > 7000) {
        Te = 0; // On coupe le moteur si sa vitesse dépasse une valeur réaliste
        if (We > 7200) {
            We = 7200;
            car.We = We;
        }
    } else {
        // Interpolation linéaire pour trouver Te
        for (int i = 0; i < sizeof(rpm) / sizeof(rpm[0]) - 1; i++) {
            if (We >= rpm[i] && We <= rpm[i + 1]) {
                Te = torque[i] + (torque[i + 1] - torque[i]) * (We - rpm[i]) / (rpm[i + 1] - rpm[i]);
                break;
            }
        }
        if (Te < 80) {
            Te = 80;
        }
    }

    // std::cout << "We : " << We << " Te : " << Te << std::endl;

    float Twheel = Te * gk * G;

    // Définition des forces

    vec3 Fdrag = {
        -Cdrag * Vx * std::abs(Vx),
        -Cdrag * Vy * std::abs(Vy),
        0
    }; // Force de traînée

    vec3 Frr = {
        -Crr * Vx,
        -Crr * Vy,
        0
    }; // Force de frottement roulant

    vec3 Fg = {
        m * g * sinf(slope),
        0,
        -m * g * cosf(slope)*0.001
    }; // Gravité avec pente dans le sens du déplacement

    
    vec3 Ftraction = {
        (Twheel / Rwheel) * cosf(steer) * Caccel,
        (Twheel / Rwheel) * sinf(steer) * Caccel,
        0
    }; // Force de traction des pneus

    vec3 Fbrake = {
        -Cbrake*5,
        0,
        0
    }; // Force de freinage

    vec3 Freverse = {
        -Cbrake,
        0,
        0
    }; // Force de freinage

    vec3 Fcentripete = {
        -(m * tanf(steer) * sqrtf(Vx * Vx + Vy * Vy) / L) * Vy,
        (m * tanf(steer) * sqrtf(Vx * Vx + Vy * Vy) / L) * Vx,
        0
    }; // Force centripète

    // Deuxième loi de Newton
    vec3 Fglob = {0, 0, 0}; // Force globale appliquée à toute la voiture
    if (accel == 1) {
        Fglob = Fdrag + Frr + Fg + Ftraction; // Cas d'accélération
    } else if (accel == -2) {
        Fglob = Fdrag + Frr + Fg + Fbrake * Caccel; // Cas de freinage
    } else if (accel == -1) {
        Fglob = Fdrag + Frr + Fg + Freverse; // Cas de marche arrière
    } else {
        Fglob = Fdrag + Frr + Fg; // Cas ni freinage ni accélération
    }

    // Passage de la force en repère monde
    float Fglob_x_world = cosf(yaw) * Fglob.x - sinf(yaw) * Fglob.y;
    float Fglob_y_world = sinf(yaw) * Fglob.x + cosf(yaw) * Fglob.y;
    Fglob.x = Fglob_x_world;
    Fglob.y = Fglob_y_world;


    // Calcul de la rotation du véhicule
    float w = (tanf(steer) / L) * sqrtf(Vx * Vx + Vy * Vy);
    car.delta_z_angle = w * dt; // new_yaw = yaw + w * dt --- delta_yaw = new_yaw - yaw = w * dt

    return Fglob / m;
}