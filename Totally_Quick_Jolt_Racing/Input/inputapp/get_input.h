#pragma once

#include <mutex>
#include "share_val.h"
#include "donnees.h"

/**
 * @brief Démarre la boucle de lecture asynchrone des entrées utilisateur (dans un thread séparé).
 * @return 1 si l'initialisation a réussi, 0 sinon.
 */
int start_input_reader();

/**
 * @brief Accès au mutex protégeant la structure partagée des entrées.
 * @return Pointeur vers le mutex utilisé pour protéger ShareVal.
 */
std::mutex* get_input_mutex();

/**
 * @brief Accès à la structure partagée contenant les dernières valeurs d'entrée.
 * @return Pointeur vers la structure ShareVal à jour.
 */
ShareVal* get_input_shareval();

/**
 * @brief Arrête proprement la boucle de lecture des entrées utilisateur.
 */
void stop_input_reader();

/**
 * @brief Copies the contents of a ShareVal structure from source to destination.
 *
 * This function duplicates all relevant data from the source ShareVal pointer
 * to the destination ShareVal pointer. Both pointers must be valid and non-null.
 *
 * @param source Pointer to the ShareVal structure to copy from.
 * @param destination Pointer to the ShareVal structure to copy to.
 */
void copyShareVal(ShareVal* source, ShareVal* destination);

/**
 * @brief Returns a pointer to the variable controlling the input reading loop.
 * @return Pointer to the variable controlling the input reading loop.
 */
int* get_input_run();