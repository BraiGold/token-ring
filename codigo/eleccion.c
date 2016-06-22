#include <stdio.h>
#include "mpi.h"
#include "eleccion.h"
#include <stdbool.h>

//cada vez que mandemos mensajes por el anillo tambien mandaremos el origen en la tercer coord
#define TAG_TOKEN 123456 //defino un tag Para el "token" osea el <i,cl,id>
#define TAG_OK 231234 // el tag para mandar la senial de recibido o OK
#define OK 111 // contante del OK
//#define true 1
static t_pid siguiente_pid(t_pid pid, int es_ultimo)
{
 t_pid res= 0; /* Para silenciar el warning del compilador. */

 if (es_ultimo)
	res= 1;
 else
	res= pid+1;

 return res;
}

void iniciar_eleccion(t_pid pid, int es_ultimo)
{
  bool respondio=false;
  t_pid siguiente=siguiente_pid(pid,es_ultimo);
  t_pid buff[3];//aca el mensaje <i,cl,id> que en este caso sera <id,id,id> , la tercera coord es para saber a quien devolver el OK
  //en el mensaje meto <id,id> como indica el protocolo
  buff[0]=pid;
  buff[1]=pid;
  buff[2]=pid;
  t_pid rta;
  //mientras no respondio el OK voy al "siguiente del siguiente" que es siguiente+1 porque como no soy el ultimo siempre hay alguien vivo entre el ultimo y yo. ( y si soy el ultimo la funcion siguiente_pid me da el primero y seguro hay alguien entre eso y yo o soy el unico )
  while(!respondio){
    MPI_Request r;

    MPI_Isend( &buff, 3, MPI_INT, siguiente, TAG_TOKEN, MPI_COMM_WORLD, &r);

    //hago polling si me llega un ack listo, sino hago hasta que se acabe el tiempo
    int llego=0;
    MPI_Status stat;
    double ahora= MPI_Wtime();
    double timeout= 1;//time out arbitrario
    double tiempo_maximo= ahora+timeout;
    while(llego==0 && ahora<tiempo_maximo){// hago el polling a ver si me llego el OK
      MPI_Iprobe(MPI_ANY_SOURCE,TAG_OK,MPI_COMM_WORLD,&llego,&stat);
      ahora= MPI_Wtime();
    }
    if(llego==1){//si llego lo leo para "limpiar el canal" y termina (el respondio=true corta el while)
      respondio=true;
       MPI_Irecv(&rta, 1, MPI_INT, MPI_ANY_SOURCE, TAG_OK, MPI_COMM_WORLD, &r);
    }

    siguiente++; //avanzo al siguiente, al cual le mandare el mensaje si no respondio
  }

}

void eleccion_lider(t_pid pid, int es_ultimo, unsigned int timeout) {
 static t_status status= NO_LIDER;
 double ahora= MPI_Wtime();
 double tiempo_maximo= ahora+timeout;
 //t_pid proximo= siguiente_pid(pid, es_ultimo);

 while (ahora<tiempo_maximo) {
   //organizo 2 buffers uno que "entra" y uno que 'sale' osea el que se recibe y el que se enviara eventualmente
    t_pid entra[3]; //<i,cl,quienLoEnvio>
    t_pid sale[3]; //<i,cl,ID>
    t_pid ok=OK;//el mensaje de OK
    MPI_Request r;
  /*  double ya= MPI_Wtime();
    double tout= 3*MPI_Wtime();
    double t_maximo= ya+tout;*/
    int llegoAlgo=0;
    MPI_Status stat;
    //espero que llegue el mensaje y lo recibo
    double ya=MPI_Wtime();
    //espero a que llegue algun mensaje o a que se acabe el tiempo.
    while (llegoAlgo==0 && ya < tiempo_maximo) {
      MPI_Iprobe(MPI_ANY_SOURCE,TAG_TOKEN,MPI_COMM_WORLD,&llegoAlgo,&stat);
      ya = MPI_Wtime();
    }
    if(MPI_Wtime()>= tiempo_maximo){  break; } // si se acabo el tiempo, salir

    //si no se acabo el tiempo entonces llego un mensaje.
    MPI_Irecv(&entra, 3, MPI_INT, MPI_ANY_SOURCE, TAG_TOKEN, MPI_COMM_WORLD, &r);//recibio el token ,la tercer coordenada es el origen
    MPI_Isend( &ok, 1, MPI_INT, entra[2], TAG_OK, MPI_COMM_WORLD, &r);//mando el OK de que lo recibi
    //comparaciones
    bool sigueGirando=true;//tengo que mandar el mensaje de salida ("sale")?
    //nombre las variables como el enunciado
    t_pid i=entra[0];
    t_pid cl=entra[1];
    t_pid ID=pid;
    sale[0]=i;
    sale[1]=cl;
    sale[2]=pid;//le mando quien soy asi me sabe decir ok
    //if(cl==ID){ status =LIDER; }//

    //efectuo el algoritmo descrito en el enunciado
    if(i==ID){
      if(cl > ID){//todavia el lider no se entero que es lider
        sigueGirando=true;
        sale[0]=cl;
        sale[1]=cl;
        status=NO_LIDER;
      }else{//el lider ya sabe que lo es
        sigueGirando=false;
      }
    }else{// i!=ID
      if(ID >= cl){//soy mayor que el lider?
        sigueGirando=true;
        status=LIDER;//soy el lider
        sale[0]=i;
        sale[1]=ID;//si el ID es mayor al lider actual soy yo el nuevo lider y el toquen gira con <i,id>
      }else{//soy menor que el lider
        status=NO_LIDER;//no soy el lider
        sigueGirando=true;
      }
    }

    //fin de comparaciones mando el msj si hay que mandarlo osea si "sigueGirando"
    if(sigueGirando){
      int siguiente = siguiente_pid(pid,es_ultimo);
      ya=MPI_Wtime();

      //mientras no me respondio el OK y todavia queda tiempo mando el mensaje al siguiente
      int respondio=0;
      while(respondio==0 && ya < tiempo_maximo){
        MPI_Request r;
        printf(" el siguiente es: %d\n",siguiente );
        MPI_Isend( &sale, 3, MPI_INT, siguiente, TAG_TOKEN, MPI_COMM_WORLD, &r);//mando el mensaje de salida "sale"

        double t_respuesta_max =ya + (3);//le doy un timeout de 3 arbitrario
        while (respondio==0 && ya <t_respuesta_max) {// hago polling esperando hasta que me responda o se acabe el tiempo
          MPI_Iprobe(MPI_ANY_SOURCE,TAG_OK,MPI_COMM_WORLD,&respondio,&stat);
          ya=MPI_Wtime();
        }
        if(respondio==1){//si respondio listo
          MPI_Irecv(&ok, 1, MPI_INT, MPI_ANY_SOURCE, TAG_OK, MPI_COMM_WORLD, &r);
        }//si no respondio avanzo al siguiente del siguiente que es siguiente ++ pues el problema del ultimo no se da como se explico en "iniciar eleccion"
        ya=MPI_Wtime();
        siguiente++;
      }
    }


	 /* Actualizo valor de la hora. */
	 ahora= MPI_Wtime();
	}

 /* Reporto mi status al final de la ronda. */
 printf("Proceso %u %s l�der.\n", pid, (status==LIDER ? "es" : "no es"));
}
