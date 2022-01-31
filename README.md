# Recoleccion_Datos_ServidorUDP
SCRIPTS DE LINUX

Sistema que recoge datos de distintas máquinas ubicadas en una planta industrial.
Cada máquina envia datos de su funcionamiento a un servidor UDP que escucha
en el puerto 2021, y los almacena en una memoria compartida.

Los datos llegan al servidor, este añadirá un campo con el instante de recepción
y se almacenan en un espacio de memoria compartida configurado como buffer ping-pong
con espacio para 128 estructuras en total.
Por otra parte otro programa se encarga de leer los datos que se reciban en el
buffer ping-pong y generar un archivo de texto que genere los comandos adecuados
para publicarlos en un servidor MQTT. Por ejemplo si se recibió desde la estación
con ID 123, corriente=10 A y tensión=223V, a las 10:23:14 del 24/09/2021, deberían
generarse tres líneas con el siguiente formato:
mosquitto_pub -h test.mosquitto.org -m 10 -t soyredes/GrupoX/123/Corriente
mosquitto_pub -h test.mosquitto.org -m 223 -t soyredes/GrupoX/123/Tension
mosquitto_pub -h test.mosquitto.org -m “2021/09/04 10:23:41” -t
soyredes/GrupoX/123/Tiempo
Por defecto se usará el servidor test.mosquitto.org.
Si se envía la señal SIGHUP al programa que lee los datos y genera el archivo, se
genera un proceso hijo, que lee el archivo, ejecuta las líneas generadas
hasta el momento como comando y luego termina.
