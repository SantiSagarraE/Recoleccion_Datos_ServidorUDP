/*
Programa clienteUDP:
Este programa completa los 3 campos de una estructura de datos y se los envia 
al servidor. Para ejecutarlo es necesario el nombre del programa y la IP del 
servidor*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>    
#include <sys/socket.h>  
#include <netinet/in.h> 
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h> 
#include <time.h>

/******************** MANEJADOR DE SEÑALES ********************/
void finalizar(int sig);
int fin=0;

/******************** DEFINE SERVIDOR ********************/
#define PORT_NUM 2021				
#define IP_ADDR "127.0.0.1"	
#define SOCKET_PROTOCOL 0

/******************** STRUCT DATOS ********************/
struct datos{
    int					ID;							//identificacion de maquina
    int					corriente;					// Valor de corriente multiplicado por 10
    unsigned			tension_alimentacion;		// Valor de tension de alimentacion multiplicado por 10
};

/******************** COMIENZO ********************/
int main(int argc, char *argv[]){
    unsigned int         client_s;					// Descriptor del socket
    struct sockaddr_in   server_addr;				// Estructura con los datos del servidor
    struct sockaddr_in   client_addr;				// Estructura con los datos del cliente
    int                  addr_len;					// Tamaño de las estructuras
    struct datos         buf_tx;
    char                 ipserver[16];
    int                  bytesaenviar, bytestx;		// Contadores

    signal(SIGTERM,finalizar);

    srand(time(NULL));

    if (argc!=2){									// Leo argumentos desde la linea de comandos
        printf("uso: clienteUDP www.xxx.yyy.zzz\n");
        return -1; 
    }
    strncpy(ipserver,argv[1],16);

	/**************** INICIACIÓN DEL SOCKET ****************/
	
    client_s = socket(AF_INET, SOCK_DGRAM, SOCKET_PROTOCOL);
    if (client_s==-1){
        perror("socket");
        return 2;
    }
    printf("Cree el descriptor del socket %d\n",client_s);

    server_addr.sin_family      = AF_INET;            // Familia TCP/IP
    server_addr.sin_port        = htons(PORT_NUM);    // Número de Port, htons() lo convierte al orden de la red

    if (inet_aton(ipserver, &server_addr.sin_addr)==0){
        printf ("La dirección  IP_ADDR no es una dirección IP válida. Programa terminado\n"); 
        return 3;
    }
   
    addr_len = sizeof(server_addr);

    while(1){  
        printf("Datos desde el proceso %d:\n", getpid()); 
        buf_tx.ID= (rand()%10001);                    // Se agrega un numero random entre  0 y 10000
        buf_tx.corriente=(rand()%11);                 // Se agrega un numero random entre  0 y 10
        buf_tx.tension_alimentacion= (rand() %21);    // Se agrega un numero random entre  0 y 20

        printf("PID: %d\n", buf_tx.ID);
        printf("Corriente: %i\n", buf_tx.corriente);
        printf("Tension: %i\n", buf_tx.tension_alimentacion); 

        bytesaenviar=sizeof(buf_tx);

        printf("******************************\n");
        printf("      Mensaje Enviado\n");
        printf("******************************\n");

        bytestx=sendto(client_s, (char*)&buf_tx, bytesaenviar, 0, (struct sockaddr *)&server_addr, sizeof(server_addr));

        if (fin == 1){
            printf("Se finalizo el cliente\n");
            close(client_s);
            return 0;
        }
        sleep(3);
    }
}

void finalizar(int sig){
    fin = 1;
}