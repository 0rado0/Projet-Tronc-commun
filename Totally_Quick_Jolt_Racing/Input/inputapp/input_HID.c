/**
 * @file input_HID.c
 * @brief Gestion des périphériques HID via HIDAPI pour la récupération des entrées utilisateur.
 *
 * Ce module permet d'initialiser la connexion à un périphérique HID spécifique (par exemple, un volant ou une manette),
 * de lire les rapports HID pour extraire les données utilisateur dans une structure Donnees, et de fermer proprement la connexion.
 *
 * Fonctions principales :
 * - int init_input(void) : Initialise la bibliothèque HIDAPI et recherche le périphérique cible.
 * - Donnees get_input(void) : Lit un rapport HID depuis le périphérique et extrait les données.
 * - void exit_input(void) : Ferme la connexion et libère les ressources HIDAPI.
 *
 * Dépendances :
 * - hidapi.h : Accès aux fonctions HIDAPI.
 * - share_val.h : Définition de la structure Donnees.
 *
 * Utilisation typique :
 * 1. Appeler init_input() pour initialiser la connexion au périphérique.
 * 2. Appeler get_input() à chaque boucle pour lire les données utilisateur.
 * 3. Appeler exit_input() à la fermeture du programme pour libérer les ressources.
 */

#include <hidapi.h>      // Pour l'utilisation de la bibliothèque HIDAPI
#include "donnees.h"   // Pour la structure Donnees
#include <string.h>
#include <stdio.h>

// Variable globale pour stocker le chemin du périphérique HID cible
static hid_device *handle = NULL;

/**
 * @brief Lit un rapport HID depuis le périphérique et extrait les données utilisateur.
 * @return Une structure Donnees remplie avec les valeurs extraites du rapport HID.
 *
 * Ouvre le périphérique HID à partir du chemin global, lit un rapport, affiche les octets lus,
 * puis extrait les informations dans une structure Donnees (à compléter selon le mapping du périphérique).
 */
Donnees get_input(){
    unsigned char buf[256];
    int res;
    
    Donnees donnees = {0,0,0,0,0,0}; // Initialise la structure Donnees
    if (handle) {
        // Réinitialise le buffer
        memset(buf, 0, sizeof(buf));
        buf[0] = 0x00; // ID du rapport (0x00 pour les périphériques sans ID de rapport)

        // Lecture d'un rapport depuis le périphérique
        res = hid_read(handle, buf, sizeof(buf));

        if (res > 0) {
            donnees.pedale_acc  = buf[1];
            donnees.pedale_fr   = buf[0];
            donnees.button      = buf[2];
            donnees.volant_1    = buf[4];
            donnees.volant_2    = buf[5];
            donnees.autre       = 1;
        } else if (res == 0) {
            donnees.autre = 0;
        } else {
            donnees.autre = -1;
        }
    }
    return donnees;
}

/**
 * @brief Initialise la bibliothèque HIDAPI et recherche le périphérique cible.
 * @return 0 en cas de succès, 1 en cas d'échec.
 *
 * Parcourt la liste des périphériques HID connectés et cherche un périphérique
 * avec les chaînes fabricant/produit "Pamoua"/"heho". Si trouvé, stocke le chemin
 * dans la variable globale pour une utilisation ultérieure.
 */
int init_input(){
    struct hid_device_info *devs, *cur_dev;

    if (hid_init()) {
        fprintf(stderr, "Echec de l'initialisation de hidapi.\n");
        return 1;
    }

    devs = hid_enumerate(0x0, 0x0);
    if (!devs) {
        fprintf(stderr, "Aucun périphérique HID trouvé.\n");
        hid_exit();
        return 1;
    }

    cur_dev = devs;
    while (cur_dev) {
        if (wcscmp(cur_dev->manufacturer_string, L"Totally Quick Jolt Racing Company") == 0 &&
            wcscmp(cur_dev->product_string, L"TQJR_A") == 0) {
            handle = hid_open_path(cur_dev->path);
            hid_free_enumeration(devs);
            return 0;
        }
        cur_dev = cur_dev->next;
    }
    hid_free_enumeration(devs);
    hid_exit();
    return 1;
}

/**
 * @brief Ferme la connexion HIDAPI et libère les ressources.
 */
void exit_input(){
    hid_exit();
}