/*
Programa servidor:
Este programa recibe una estructura de datos enviada por el programa cliente, los copia en una  
estructura similar (con la diferencia de que agrega un campo de fecha y hora) y los guarda en
un buffer tipo ping-pong en memoria compartida*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>    
#include <sys/socket.h>  
#include <netinet/in.h> 
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <time.h>
#include <signal.h>

/****************DEFINO SEÑAL PARA FINALIZAR****************/

void finalizar(int sig);
int fin=0;

/********************DEFINE SERVIDOR********************/

#define  PORT_NUM           2021  // Numero de Port usado
#define  IP_ADDR "127.0.0.1" //  Dirección IP 
#define SOCKET_PROTOCOL 0

/********************DEFINE SEMAFORO********************/

#define ENTRAR(OP) ((OP).sem_op = -1)
#define SALIR(OP) ((OP).sem_op = +1)

/********************STRUCT SEMAFORO********************/
union semun {
	int              val;			/* Value for SETVAL */
	struct semid_ds *buf;			/* Buffer for IPC_STAT, IPC_SET */
	unsigned short  *array;			/* Array for GETALL, SETALL */
	struct seminfo  *__buf;			/* Buffer for IPC_INFO (Linux-specific) */
};

/********************STRUCT DATOS********************/
struct datos{
	int						ID; 						//	Identificacion de maquina
	int						corriente;					// Valor de corriente multiplicado por 10
	unsigned				tension_alimentacion;		// Valor de tension de alimentacion multiplicado por 10
	char					fechayhora[100];			// Momento en el que llega el datagrama
};

/********************COMIENZO********************/
int main(int argc, char *argv[]){
	unsigned int         	server_s;					// Descriptor del socket
	struct sockaddr_in  	server_addr;				// Estructura con los datos del servidor
	struct sockaddr_in   	client_addr;				// Estructura con los datos del cliente
	int                  	addr_len;					// Tamaño de las estructuras
	struct datos		 	buf_rx;
	int             		bytesrecibidos,bytesaenviar, bytestx;          // Contadores
	pid_t pid_n;
	key_t ClaveMem,ClaveSem;
	int Id_Memoria,Id_Semaforo;
	struct datos *Memoria = NULL;
	int i=0,j=0;
	struct sembuf Operacion;
	union semun arg;
	time_t	tiempo;	
	struct tm *tm;

	//*********MANEJADOR DE SEÑALES*********
	
	signal(SIGTERM,finalizar);

	//*********MEMORIA COMPARTIDA***********
	ClaveMem = ftok ("/bin/ls", 33);
	if (ClaveMem == -1){
		printf("No consigo clave para memoria compartida\n");
		exit(0);
	}
	
	Id_Memoria = shmget (ClaveMem, sizeof(struct datos)*4, 0660 | IPC_CREAT);  // Creo 4 espacios de memoria compartida
	if (Id_Memoria == -1){
		printf("No consigo Id para memoria compartida\n");
		exit (0);
	}
	
	Memoria = (struct datos *)shmat (Id_Memoria, (struct datos *)0, 0);
	if (Memoria == NULL){
		printf("No consigo memoria compartida\n");
		exit (0);
	}
	
	//***********SEMAFOROS***********
	ClaveSem = ftok ("/bin/ls", 33);
	if (ClaveSem == (key_t)-1){
		printf("No puedo conseguir clave de semáforo\n");
		exit(0);
	}       

	Id_Semaforo = semget (ClaveSem, 2, 0660 | IPC_CREAT);		// Array de semaforo
	if (Id_Semaforo == -1){
		printf("No puedo crear semáforo\n");
		exit (0);
	}
	
	Operacion.sem_num = 0;				
	Operacion.sem_flg = 0;				
	
	//*********INICIO DEL SERVIDOR*********
	server_s = socket(AF_INET, SOCK_DGRAM, SOCKET_PROTOCOL);
	if (server_s==-1){
	perror("socket");
	return 1;
	}
	printf("Cree el descriptor del socket %d\n",server_s);


	server_addr.sin_family      = AF_INET;            // Familia UDP
	server_addr.sin_port        = htons(PORT_NUM);    // Número de Port, htons() lo convierte al orden de la red
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);  // INADDR_ANY = cualquier direccion IP, htonl() lo convierte al orden de la red
  
	bind(server_s, (struct sockaddr *)&server_addr, sizeof(server_addr));
	  
	printf("Asocie el descriptor %u con el port %u acepta conexiones desde %u\n", server_s,PORT_NUM, INADDR_ANY) ;

	client_addr.sin_family      = AF_INET;            
	client_addr.sin_port        = htons(PORT_NUM);    

	if(inet_aton(IP_ADDR, &client_addr.sin_addr)==0){
		printf ("La dirección  IP_ADDR no es una dirección IP válida. Programa terminado\n"); 
		return 2;
	}

	addr_len = sizeof(client_addr);
	//if((pid_n=fork())==0){
	//	execve (mosquittoc, args, 0);
	//}
	//else{
	//	printf("Soy el padre con PID %d y derivo a mi hijo de PID %d el programa mosquittoc\n", getpid(), pid_n);
	//}
	do{  
		if(i%2 == 0){
			arg.val =1;										//	Inicializo el semaforo para la parte A de memoria en VERDE
			semctl (Id_Semaforo, 1, SETVAL, arg);							
			Operacion.sem_num = 1;							//	Semaforo 1 parte A de memoria				
			ENTRAR(Operacion);						
			semop (Id_Semaforo, &Operacion, 1);
			
			printf("\nSe ingreso a la parte A de memoria compartida\n");
			printf("Semaforo 1 en rojo\n");
				for(j=0; j<2; j++){							//	Parte A de memoria, posiciones 0 y 1
					bytesrecibidos = 0;								
					do{										//	Mientras no recibo bytes por parte del cliente
						printf( "Estoy esperando una conexion entrante\n");
						bytesrecibidos=recvfrom(server_s, (char*)&buf_rx, sizeof(buf_rx), 0,(struct sockaddr *)&client_addr, &addr_len);
						if (bytesrecibidos==-1){
							perror ("recvfrom");
							return 3;
						}
					}while(bytesrecibidos == 0);				
					tiempo=time(NULL);		
					tm=localtime(&tiempo);					//	Defino la hora de llegada del mensaje					
					printf("Recibi el siguiente mensaje: '%d' \n", buf_rx.ID);
					printf("%i\n", buf_rx.corriente);
					printf("%i\n", buf_rx.tension_alimentacion);
			        strftime(buf_rx.fechayhora, 100, "%X", tm);
			        printf("%s\n", buf_rx.fechayhora);
					printf("El IP del cliente es: %s y el port del cliente es %hu \n",inet_ntoa(client_addr.sin_addr),ntohs(client_addr.sin_port));
					Memoria[j] = buf_rx;					//	Guardo la estructura de datos recibida en la memoria compartida
				}
				
			SALIR(Operacion);
			semop (Id_Semaforo, &Operacion, 1);
			printf("Semaforo 1 en verde\n");
		}
		else{
			arg.val =1;						//	Inicializo el semaforo para la parte B de memoria en VERDE
			semctl (Id_Semaforo, 0, SETVAL, arg);		
			Operacion.sem_num = 0;			//	Semaforo 0 parte B de memoria										
			ENTRAR(Operacion);							
			semop (Id_Semaforo, &Operacion, 1);

			printf("\nSe ingreso a la parte B de memoria compartida\n");
			printf("Semaforo 0 en rojo\n");		
				for (j=2; j<4; j++){		//	Parte B de memoria, posiciones 2 y 3
					bytesrecibidos = 0;				
					do{						
						printf( "Estoy esperando una conexion entrante\n");
						bytesrecibidos=recvfrom(server_s, (char*)&buf_rx, sizeof(buf_rx), 0,(struct sockaddr *)&client_addr, &addr_len);
						if (bytesrecibidos==-1){
							perror ("recvfrom");
							return 3;
						}
					}while(bytesrecibidos == 0);
					tiempo=time(NULL);
					tm=localtime(&tiempo);					
					printf("Recibi el siguiente mensaje: '%d' \n", buf_rx.ID);
					printf("%i\n", buf_rx.corriente);
					printf("%i\n", buf_rx.tension_alimentacion);
			        strftime(buf_rx.fechayhora, 100, "%X", tm);
			        printf("%s\n", buf_rx.fechayhora);
					printf("El IP del cliente es: %s y el port del cliente es %hu \n",inet_ntoa(client_addr.sin_addr),ntohs(client_addr.sin_port));
					Memoria[j] = buf_rx;
				}
			SALIR(Operacion);
			semop (Id_Semaforo, &Operacion, 1);
			printf("Semaforo 0 en verde\n");
		}
		i++;	
	}while (fin==0); 
	semctl(Id_Semaforo, 0, IPC_RMID);
	shmdt ((struct datos *)Memoria);
	shmctl (Id_Memoria, IPC_RMID, (struct shmid_ds *)NULL);
	close(server_s);
	return 0;
}

void finalizar(int sig){
	fin=1;
}