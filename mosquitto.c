/*SOYR 2021. Trabajo Practico 6.
GRUPO 11
Sagarra Santiago 66606/5
Ferreras Joaquin 67103/0
Quintana Alberto 55640/8 

Programa moquitto:
Este programa lee los datos de memoria compartida y crea un archivo con las sentencias de bash 
para subir esta información en un servidor MQTT. Cuando recibe la señal SIGHUP, se ejecuta el scrip
y cargan los datos en test.mosquitto.org*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h> 
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>

void handler(int sig);
void finalizar(int sig);

pid_t pid_n;
int flag=0, fin=0;

/**************** UBICACION DEL SCRIPT ****************/
char mosquitto[]="/home/soyredes/Descargas/TP6/mosquitto";
char *args[]={"/home/soyredes/Descargas/TP6/mosquitto",NULL};

/******************** DEFINO SEMAFORO ********************/
#define ENTRAR(OP) ((OP).sem_op = -1)
#define SALIR(OP) ((OP).sem_op = +1)

/********************STRUCT SEMAFORO********************/
union semun {
	int					val;						/* Value for SETVAL */
	struct semid_ds *	buf;						/* Buffer for IPC_STAT, IPC_SET */
	unsigned short  *	array;						/* Array for GETALL, SETALL */
	struct seminfo  *	__buf;						/* Buffer for IPC_INFO (Linux-specific) */
};

/********************STRUCT DATOS********************/
struct datos{
	int	ID; 										//	Identificacion de maquina
	int	corriente;									//	Valor de corriente multiplicado por 10
	unsigned tension_alimentacion;					//	Valor de tension de alimentacion multiplicado por 10
	char fechayhora[100];							//	Momento en el que llega el datagrama
};

/********************COMIENZO********************/
int main(int argc, char *argv[]){
	key_t ClaveMem,ClaveSem;
	struct datos *Memoria = NULL;
	int Id_Memoria,Id_Semaforo;	
	struct datos info;  							//	Estructura de datos sacados de memoria compartida
	struct sembuf Operacion;
	union semun arg;
	int i=0, j=0;

	//*********MANEJADOR DE SEÑALES*********
	signal(SIGHUP,handler);					
	signal(SIGTERM,finalizar);

	//*********MEMORIA COMPARTIDA*********
	ClaveMem = ftok ("/bin/ls", 33);
	if (ClaveMem == -1){
		printf("No consigo clave para memoria compartida\n");
		exit(0);
	}
	
	Id_Memoria = shmget (ClaveMem, sizeof(struct datos)*4, 0660 | IPC_CREAT);
	if (Id_Memoria == -1){
		printf("No consigo Id para memoria compartida\n");
		exit (0);
	}
	
	Memoria = (struct datos *)shmat (Id_Memoria, (struct datos *)0, 0);
    if (Memoria == NULL){
        printf("No consigo memoria compartida\n");
        exit (0);
    }
	
	//*********SEMAFOROS*********
	ClaveSem = ftok ("/bin/ls", 33);
	if (ClaveSem == (key_t)-1){
		printf("No puedo conseguir clave de semáforo\n");
		exit(0);
	}       

	Id_Semaforo = semget (ClaveSem, 2, 0660 | IPC_CREAT);		//	Array de semaforo
	if (Id_Semaforo == -1){
		printf("No puedo crear semáforo\n");
		exit (0);
	}
	Operacion.sem_num = 0;					
	Operacion.sem_flg = 0;					

	//********APERTURA DEL ARCHIVO********
	FILE * archivo = fopen("mosquitto", "w");
	fprintf(archivo, "#!/bin/bash\n");
	printf("Ejecutando programa mosquittoc\n");
	do{
		if(i%2 == 0){
			arg.val =0;											//	Inicialiazo semaforo de la parte de memoria A en ROJO
			semctl (Id_Semaforo, 1, SETVAL, arg);										
			Operacion.sem_num = 1;								//	Semaforo 1 parte A de memoria						
			ENTRAR(Operacion);						
			semop (Id_Semaforo, &Operacion, 1);	
			printf("Se ingreso a la parte A de memoria compartida\n");
			printf("Semaforo 1 en rojo\n");		
			for (j=0; j<2; j++){							
				info = Memoria[j];
	    		fprintf(archivo, "mosquitto_pub -h test.mosquitto.org -m %d -t soyredes/Grupo11/%d/Corriente\n", info.corriente, info.ID);
				fprintf(archivo, "mosquitto_pub -h test.mosquitto.org -m %d -t soyredes/Grupo11/%d/Tension\n", info.tension_alimentacion, info.ID);
				fprintf(archivo, "mosquitto_pub -h test.mosquitto.org -m %s -t soyredes/Grupo11/%d/Tiempo\n", info.fechayhora, info.ID);	
			}
			SALIR(Operacion);
			semop (Id_Semaforo, &Operacion, 1);
			printf("\nSemaforo 1 en verde\n");
		}
		else{
			arg.val =0;											//	Inicialiazo semaforo de la parte de memoria B en ROJO
			semctl (Id_Semaforo, 0, SETVAL, arg);
			Operacion.sem_num = 0;								//	Semaforo 0 parte B de memoria									
			ENTRAR(Operacion);							
			semop (Id_Semaforo, &Operacion, 1);	
			printf("Se ingreso a la parte B de memoria compartida\n");
			printf("Semaforo 0 en rojo\n");			
			for (j=2; j<4; j++){
				info = Memoria[j];
        		fprintf(archivo, "mosquitto_pub -h test.mosquitto.org -m %d -t soyredes/Grupo11/%d/Corriente\n", info.corriente, info.ID);
				fprintf(archivo, "mosquitto_pub -h test.mosquitto.org -m %d -t soyredes/Grupo11/%d/Tension\n", info.tension_alimentacion, info.ID);
				fprintf(archivo, "mosquitto_pub -h test.mosquitto.org -m %s -t soyredes/Grupo11/%d/Tiempo\n", info.fechayhora, info.ID);;
			}
			SALIR(Operacion);
			semop (Id_Semaforo, &Operacion, 1);
			printf("\nSemaforo 0 en verde\n");
		}
		i++;
		if(flag == 1){
			flag = 0;
			fclose(archivo);
			if((pid_n=fork())==0){
				execv (mosquitto, args);
				kill(getpid(),SIGKILL);				
			}
			else{
				printf("Soy el padre con PID %d y derivo a mi hijo de PID %d \n", getpid(), pid_n);	
			}
			sleep(10);													//	Para dar tiempo a ejecutarse el script
			FILE * archivo = fopen("mosquitto", "w");					//	Limpio el script y lo vuelvo a abrir
			fprintf(archivo, "#!/bin/bash\n"); 			
		}
		
	}while (fin == 0);	
	semctl(Id_Semaforo, 0, IPC_RMID);
	shmdt ((struct datos *)Memoria);
	shmctl (Id_Memoria, IPC_RMID, (struct shmid_ds *)NULL);
	return 0;
}


void finalizar(int sig){
	fin = 1;
}
void handler(int sig){
	flag = 1;
}
