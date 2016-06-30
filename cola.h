#ifndef COLA_H
#define COLA_H

/* ******************************************************************
 *                    PRIMITIVAS DE LA COLA
 * *****************************************************************/

// Crea una cola.
// Post: devuelve una nueva cola vac�a.
cola_t* cola_crear();

// Destruye la cola. 
// Pre: la cola fue creada. 
// Post: se eliminaron todos los elementos de la cola.
void destruir(cola_t *cola){

// Devuelve verdadero o falso, seg�n si la cola tiene o no elementos encolados.
// Pre: la cola fue creada.
bool vacia(const cola_t *cola);

// Agrega un nuevo elemento a la cola. Devuelve falso en caso de error.
// Pre: la cola fue creada.
// Post: se agreg� un nuevo elemento a la cola, valor se encuentra al final
// de la cola.
bool encolar(cola_t *cola, void* valor);

// Saca el primer elemento de la cola. Si la cola tiene elementos, se quita el
// primero de la cola, y se devuelve su valor, si est� vac�a, devuelve NULL.
// Pre: la cola fue creada.
// Post: se devolvi� el valor del primer elemento anterior, la cola
// contiene un elemento menos, si la cola no estaba vac�a.
void* desencolar(cola_t *cola);

// Obtiene el valor del primer elemento de la cola. Si la cola tiene
// elementos, se devuelve el valor del primero, si est� vac�a devuelve NULL.
// Pre: la cola fue creada.
// Post: se devolvi� el primer elemento de la cola, cuando no est� vac�a.
void* ver_primero(cola_t *cola);

#endif // COLA_H
