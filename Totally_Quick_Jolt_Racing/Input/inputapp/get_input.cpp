// file get_input.cpp
// Gestion de la recuperation asynchrone des entrees utilisateur volant pedales boutons via HID ou SDL
// Ce module lance une boucle de lecture des entrees dans un thread separe
// met a jour une structure partagee ShareVal protegee par un mutex
// et fournit des fonctions d acces a ces donnees pour le reste du programme

#include "share_val.h"
#include "donnees.h"
#include <stdio.h>
#include <stdlib.h>
#if Use_SDL
#include "input_SDL.c"
#else
#include "input_HID.c"
#endif

#include <mutex>
#include <iostream>
#include <thread>
#include <chrono>

#define PI 3.14159265358979323846

// Flag de controle du thread d entree
int is_input_run = 1;

// Mutex protegeant l acces a la structure partagee
std::mutex input_mutex;

// Structure contenant les valeurs partagees issues des entrees
ShareVal share_val;

// Arrete la boucle de lecture des entrees utilisateur
void stop_input_reader(){
    is_input_run = 0;
}

// Copie les valeurs d une structure ShareVal vers une autre
void copyShareVal(ShareVal* shareval, ShareVal* share_val0){
    share_val0 -> acceleration   = shareval -> acceleration;
    share_val0 -> angle_roue     = shareval -> angle_roue;
    share_val0 -> frein          = shareval -> frein;
    share_val0 -> rapport        = shareval -> rapport;
}

// Boucle principale de recuperation des entrees utilisateur
// Appellee dans un thread separe elle lit les entrees les convertit et met a jour la structure partagee
// param get_input Fonction de recuperation des donnees brutes HID ou SDL
void get_input_loop(){
    ShareVal temp;
    int null_count = 0;
    while (is_input_run){
        temp = {0,0,0,0};
        Donnees input = get_input();
        if (input.pedale_acc == 0 && input.pedale_fr == 0 && input.button == 0 && input.volant_1 == 0 && input.volant_2 == 0) {
            // Si aucune entree n est detectee pendant 10 iterations on arrete le lecteur d entree
            if (input.autre == -1){
                std::cout << "\nErreur dans la lecture de l'entree du controleur, arret du lecteur d'entree\n" << std::endl;
                is_input_run = 0;
                break;
            } else if (input.autre == 0){
                // Si aucune entree n est detectee on continue la boucle
                null_count++;
                if (null_count > 100){
                    std::cout << "\nPas d'entree utilisateur, arret du lecteur d entree\n" << std::endl;
                    is_input_run = 0;
                    break;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            } else {
                null_count = 0; // Reset le compteur si une entree est detectee
            }
        }

        int button = input.button;
        // Decodage des boutons pour determiner le rapport
        if (button >= 128) button -= 128;
        if (button >= 64)  button -= 64;
        if (button >= 32)  button -= 32;
        if (button >= 16)  button -= 16;
        if (button >= 8)   button -= 8;
        if (button >= 4)   button -= 4;
        if (button >= 2){
            button -= 2;
            temp.rapport -= 1;
        }
        if (button >= 1){
            temp.rapport += 1;
        }

        // Calcul de l angle du volant a partir des deux octets
        int angle_int = (input.volant_1<<8) + (input.volant_2);
        double angle = angle_int * PI / 2 / 65535 - PI / 4;
        temp.angle_roue = angle;

        // Normalisation des valeurs d acceleration et de frein
        temp.acceleration = input.pedale_acc / 255.0;
        temp.frein = input.pedale_fr / 255.0;

        // Mise a jour thread safe de la structure partagee
        if (input_mutex.try_lock()) {
            copyShareVal(&temp, &share_val);
            input_mutex.unlock();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

// Demarre la boucle de lecture des entrees utilisateur
// return 1 si l initialisation a reussi 0 sinon
int start_input_reader(){
    if (!init_input()){
        std::cout << "\nEntree initialisee avec succes\nInput avec controleur\n" << std::endl;
        get_input_loop();
        return 1;
    }
    std::cout << "\nEchec de l'initialisation de l'entree\nInput avec clavier seulement\n" << std::endl;
    return 0;
}

// Acces au mutex protegeant la structure partagee
// return Pointeur vers le mutex
std::mutex* get_input_mutex(){
    return &input_mutex;
}

// Acces a la structure partagee contenant les dernieres valeurs d entree
// return Pointeur vers la structure ShareVal
ShareVal* get_input_shareval(){
    return &share_val;
}

int* get_input_run(){
    return &is_input_run;
}
