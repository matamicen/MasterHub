#include <stdlibm.h> 
#define bool short

typedef struct{
   void* siguiente;
   void* dato;
} nodo_t;

typedef struct{
   void* primero;
   void* ultimo;
   int16 largo;
} cola_t;

/* ******************************************************************
*                        IMPLEMENTACION
* *****************************************************************/
  
// Crea una cola.
// Post: devuelve una nueva cola vacía.
cola_t* cola_crear(){
 
    cola_t* cola = malloc(sizeof(cola_t));
    if (cola == NULL) {
      return NULL;
    }
 
    cola->primero=NULL;
    cola->ultimo=NULL;
    cola->largo=0;
 
    return cola;
 
}

// Devuelve verdadero o falso, según si la cola tiene o no elementos encolados.
// Pre: la cola fue creada.
bool vacia(cola_t * cola){
 
    if (cola->primero== NULL && cola->ultimo==NULL){
        return true;
    }
     return false;
}
 
// Agrega un nuevo elemento a la cola. Devuelve falso en caso de error.
// Pre: la cola fue creada.
// Post: se agregó un nuevo elemento a la cola, valor se encuentra al final
// de la cola.
bool encolar(cola_t *cola, void* valor){
   
    nodo_t* nodo_nuevo = malloc(sizeof(nodo_t));
 
    if (nodo_nuevo == NULL){
        return false;
    }
   
    nodo_nuevo->dato=valor;
    nodo_nuevo->siguiente=NULL;
       
    if (vacia(cola)){
        cola->primero = nodo_nuevo;
        cola->largo=1;
    }
    else{
        nodo_t* ultimo = cola->ultimo;
        ultimo->siguiente=nodo_nuevo;
        int l = cola->largo;
        cola->largo=l+1;
    }
    cola->ultimo = nodo_nuevo;
   
    return true;
}
 
// Saca el primer elemento de la cola. Si la cola tiene elementos, se quita el
// primero de la cola, y se devuelve su valor, si está vacía, devuelve NULL.
// Pre: la cola fue creada.
// Post: se devolvió el valor del primer elemento anterior, la cola
// contiene un elemento menos, si la cola no estaba vacía.
void* desencolar(cola_t *cola){
 
    if (vacia(cola)){      
        return NULL;
    }
 
    if (cola->primero == cola->ultimo){
        cola->ultimo = NULL;
    }
   
    int l = cola->largo;
    nodo_t *nodo = cola->primero;
    void *dato = nodo->dato;
    cola->primero = nodo->siguiente;
    free(nodo);
    cola->largo=l-1;
 
    return dato;
   
}

// Destruye la cola.
// Pre: la cola fue creada.
// Post: se eliminaron todos los elementos de la cola.
void destruir(cola_t* cola){
   
    while (!vacia(cola)){
            desencolar(cola);
    }
    free(cola);
}

// Obtiene el valor del primer elemento de la cola. Si la cola tiene
// elementos, se devuelve el valor del primero, si está vacía devuelve NULL.
// Pre: la cola fue creada.
// Post: se devolvió el primer elemento de la cola, cuando no está vacía.
void* ver_primero(cola_t *cola){
   
    if (vacia(cola)){      
        return NULL;
    }
   
    nodo_t* nodo = cola->primero;
   
    return nodo->dato;
 
}
