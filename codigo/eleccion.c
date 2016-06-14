#include <stdio.h>
#include "mpi.h"
#include "eleccion.h"

#define TAG_ELEC 123456

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
  t_pid buf[2];
  //en el mensaje meto <id,id> como indica el protocolo
  buff[0]=pid;
  buff[1]=pid;

  t_pid rta;
  //mientras no respondio el ack voy al "siguiente del siguiente" que es siguiente+1 porque como no soy el ultimo siempre hay alguien vivo entre el ultimo y yo. ( y si soy el ultimo la funcion siguiente_pid me da el primero y seguro hay alguien entre eso y yo o soy el unico )
  while(!respondio){
    MPI_Request r;

    MPI_Isend( &buf, 2, MPI_INT, siguiente, TAG_ELEC, MPI_COMM_WORLD, &r);

    //hago polling si me llega un ack listo, sino hago hasta que se acabe el tiempo
    int llego=0;
    MPI_Status status;
    double ahora= MPI_Wtime();
    double timeout= 3*MPI_Wtime();
    double tiempo_maximo= ahora+timeout;
    while(llego==0 || ahora<tiempo_maximo){
      MPI_Iprobe(MPI_ANY_SOURCE,TAG_ELEC,&llego,&status);
      ahora= MPI_Wtime();
    }
    if(llego==1){
      respondio=true;
       MPI_Irecv(&rta, 1, MPI_INT, MPI_ANY_SOURCE, TAG_ELEC, MPI_COMM_WORLD, &r);
    }

    siguiente++;
  }

}

void eleccion_lider(t_pid pid, int es_ultimo, unsigned int timeout) {
 static t_status status= NO_LIDER;
 double ahora= MPI_Wtime();
 double tiempo_maximo= ahora+timeout;
 t_pid proximo= siguiente_pid(pid, es_ultimo);

 while (ahora<tiempo_maximo) {
	 /* Completar ac� el algoritmo de elecci�n de l�der.
	  * Si no est� bien documentado, no aprueba.
          */
    t_pid rta[2]; //<i,cl>
    MPI_Request r;
    double ya= MPI_Wtime();
    double tout= 3*MPI_Wtime();
    double t_maximo= ya+tout;
    int llego=0;
    MPI_Status status;
    //espero que llegue el mensaje y lo recibo
    while (ya < t_maximo) {
      MPI_Iprobe(MPI_ANY_SOURCE,TAG_ELEC,&llego,&status);
      ya = MPI_Wtime();
    }
    MPI_Irecv(&rta, 2, MPI_INT, MPI_ANY_SOURCE, TAG_ELEC, MPI_COMM_WORLD, &r);

    //comparaciones
    t_pid buff[2];
    if (rta[0] == pid) {
      //cl es el lider
      if (rta[1] > pid) { // cl>pid el lider está más adelante y no sabe que ganó
          buff[0] = rta[0];
          buff[1] = rta[1];
          MPI_Isend( &buf, 2, MPI_INT, siguiente, TAG_ELEC, MPI_COMM_WORLD, &r);
      } else { 
        if (rta[1] == pid) { //cl==pid soy el lider
          status = LIDER;
          //no gira mas
        } 
      }
    } else {
      if(rta[1] < pid) {
        buff[0] = rta[0];
        buff[1] = pid;
        MPI_Isend( &buf, 2, MPI_INT, siguiente, TAG_ELEC, MPI_COMM_WORLD, &r);
      }
    }

    //fin de comparaciones mando el msj



	 /* Actualizo valor de la hora. */
	 ahora= MPI_Wtime();
	}

 /* Reporto mi status al final de la ronda. */
 printf("Proceso %u %s l�der.\n", pid, (status==LIDER ? "es" : "no es"));
}
