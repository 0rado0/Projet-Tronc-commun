#ifndef PHYSIQUE_HPP
#define PHYSIQUE_HPP

#include <cmath> // /!\ ajouter -lm à la compilation
#include "cgp/cgp.hpp"
#include "driving_car.hpp"

using cgp::vec3;

// Constantes physiques
constexpr float g = 9.81f; // Accélération de la pesanteur (m/s²)
constexpr float rho = 1.225f; // Masse volumique de l'air (kg/m³)

// Constantes du véhicule
constexpr float m = 1500.0f; // Masse du véhicule (kg)
constexpr float A = 2.5f; // Surface frontale (m²)
constexpr float Rwheel = 0.2032f; // Rayon de la roue (m) -- 16 pouces de diamètre
constexpr float L = 3.0f; // Empattement (m)
constexpr float G = 3.5f; // Rapport du différentiel final
constexpr float Cd = 0.3f; // Coefficient de traînée
constexpr float Cr = 0.01f; // Coefficient de frottement roulant

// Constantes des forces
constexpr float Cdrag = 0.5f * Cd * rho * A; // (kg/m)
constexpr float Crr = Cr * m * g; // (kg/s)
constexpr float Cbrake = 13500.0f; // (N)

/**
 * @brief Calcul le bilan des forces appliquées à TOUTE la voiture (SAUF la réaction du sol)
 *
 * @param PX Position en X (m) -- repère monde
 * @param PY Position en Y (m) -- repère monde
 * @param VX Vélocité en X (m/s) -- repère monde
 * @param VY Vélocité en Y (m/s) -- repère monde
 * @param yaw Angle de lacet (radians) -- repère monde -- Passage par référence
 * @param slope Inclinaison locale de la route dans le sens de la voiture (radians) -- descendante si positif
 * @param steer Angle de braquage (radians) -- positif si à droite
 * @param accel Accélération -- 1 si accélération, 0 si freinage
 * @param Caccel Coefficient d'accélération -- 0 si pas d'accélération, 1 si pied au plancher
 * @param gear Vitesse de la boîte de vitesse -- 1 à 4
 * @return (vec3) Bilan des forces appliquées à TOUTE la voiture
 */
vec3 force_globale(driving_car& car, float slope, float steer, float accel, float Caccel, float dt);

#endif // PHYSIQUE_HPP