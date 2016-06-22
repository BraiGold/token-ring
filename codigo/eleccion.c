#include <stdio.h>
#include "mpi.h"
#include "eleccion.h"
#include <stdbool.h>

#define TAG_TOKEN 123456 //defino un tag cualquiera para que no se pisen los mensajes con los de control
#define TAG_OK 231234
#define OK 111
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
 /* Completar ac� el algoritmo de inicio de la elecci�n.
   * Si no est� bien documentado, no aprueba.
  */
  bool respondio=false;
  t_pid siguiente=siguiente_pid(pid,es_ultimo);
  t_pid buff[3];
  //en el mensaje meto <id,id> como indica el protocolo
  buff[0]=pid;
  buff[1]=pid;
  buff[2]=pid;//este tercero es a quien devolverle el OK
  t_pid rta;
  //mientras no respondio el ack voy al "siguiente del siguiente" que es siguiente+1 porque como no soy el ultimo siempre hay alguien vivo entre el ultimo y yo. ( y si soy el ultimo la funcion siguiente_pid me da el primero y seguro hay alguien entre eso y yo o soy el unico )
  while(!respondio){
    MPI_Request r;

    MPI_Isend( &buff, 3, MPI_INT, siguiente, TAG_TOKEN, MPI_COMM_WORLD, &r);

    //hago polling si me llega un ack listo, sino hago hasta que se acabe el tiempo
    int llego=0;
    MPI_Status stat;
    double ahora= MPI_Wtime();
    double timeout= 1;
    double tiempo_maximo= ahora+timeout;
    while(llego==0 && ahora<tiempo_maximo){
      MPI_Iprobe(MPI_ANY_SOURCE,TAG_OK,MPI_COMM_WORLD,&llego,&stat);
      ahora= MPI_Wtime();
    }
    if(llego==1){
      respondio=true;
       MPI_Irecv(&rta, 1, MPI_INT, MPI_ANY_SOURCE, TAG_OK, MPI_COMM_WORLD, &r);
    }

    siguiente++;
  }

}

void eleccion_lider(t_pid pid, int es_ultimo, unsigned int timeout) {
 static t_status status= NO_LIDER;
 double ahora= MPI_Wtime();
 double tiempo_maximo= ahora+timeout;
 //t_pid proximo= siguiente_pid(pid, es_ultimo);

 while (ahora<tiempo_maximo) {
	 /* Completar ac� el algoritmo de elecci�n de l�der.
	  * Si no est� bien documentado, no aprueba.
          */
    t_pid entra[3]; //<i,cl>
    t_pid sale[3]; //<i,cl>
    t_pid ok=OK;
    MPI_Request r;
  /*  double ya= MPI_Wtime();
    double tout= 3*MPI_Wtime();
    double t_maximo= ya+tout;*/
    int llegoAlgo=0;
    MPI_Status stat;
    //espero que llegue el mensaje y lo recibo
    double ya=MPI_Wtime();
    while (llegoAlgo==0 && ya < tiempo_maximo) {
      MPI_Iprobe(MPI_ANY_SOURCE,TAG_TOKEN,MPI_COMM_WORLD,&llegoAlgo,&stat);
      ya = MPI_Wtime();
    }
    if(MPI_Wtime()>= tiempo_maximo){  break; } // si se acabo el tiempo, salir

    //si no se acabo el tiempo entonces llego algo.
    MPI_Irecv(&entra, 3, MPI_INT, MPI_ANY_SOURCE, TAG_TOKEN, MPI_COMM_WORLD, &r);//la tercer coordenada es el origen
    MPI_Isend( &ok, 1, MPI_INT, entra[2], TAG_OK, MPI_COMM_WORLD, &r);
    //comparaciones
    bool sigueGirando=true;
    t_pid i=entra[0];
    t_pid cl=entra[1];
    t_pid ID=pid;
    sale[0]=i;
    sale[1]=cl;
    sale[2]=pid;//le mando quien soy asi me sabe decir ok
    //if(cl==ID){ status =LIDER; }//

    if(i==ID){
      if(cl > ID){
        sigueGirando=true;
        sale[0]=cl;
        sale[1]=cl;
        status=NO_LIDER;
      }else{
        sigueGirando=false;
      }
    }else{
      if(ID >= cl){
        sigueGirando=true;
        status=LIDER;
        sale[0]=i;
        sale[1]=ID;//si el ID es mayor al lider actual soy yo el nuevo lider y el toquen gira con <i,id>
      }else{
        status=NO_LIDER;
        sigueGirando=true;
      }
    }

    //fin de comparaciones mando el msj
    if(sigueGirando){
      int siguiente = siguiente_pid(pid,es_ultimo);
      ya=MPI_Wtime();

      int respondio=0;
      while(respondio==0 && ya < tiempo_maximo){
        MPI_Request r;
        printf(" el siguiente es: %d\n",siguiente );
        MPI_Isend( &sale, 3, MPI_INT, siguiente, TAG_TOKEN, MPI_COMM_WORLD, &r);

        double t_respuesta_max =ya + (3);//le doy un timeout de 3
        while (respondio==0 && ya <t_respuesta_max) {
          MPI_Iprobe(MPI_ANY_SOURCE,TAG_OK,MPI_COMM_WORLD,&respondio,&stat);
          ya=MPI_Wtime();
        }
        if(respondio==1){
          MPI_Irecv(&ok, 1, MPI_INT, MPI_ANY_SOURCE, TAG_OK, MPI_COMM_WORLD, &r);
        }
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
