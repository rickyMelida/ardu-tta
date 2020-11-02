// Include Emon Library
#include "EmonLib.h"
#include <SPI.h>
#include <Ethernet.h>

#define RESET 9
#define STOP 8
#define C_GENERADOR 7 //SALIDA AL CONTACTOR DEL GENERADOR
#define C_ANDE 6 //SALIDA AL CONTACTOR DE ANDE
#define START 5

#define S_ANDE 4 //SEÑAL DE ANDE
#define S_GENERADOR 3//SEÑAL DE GENERADOR

// Crear una instancia EnergyMonitor
EnergyMonitor energyMonitor;

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };   //Direccion Fisica MAC
IPAddress ip(192, 168, 100, 177);                      // IP Local que usted debe configurar

EthernetServer server(80);                             //Se usa el puerto 80 del servidor

// Voltaje de nuestra red eléctrica
float voltajeRed = 220.0;
int cont_ande = 0;
int cont_generador = 0;

void setup() {
  pinMode(RESET, OUTPUT);
  pinMode(STOP, OUTPUT);
  pinMode(C_GENERADOR, OUTPUT);
  pinMode(C_ANDE, OUTPUT);
  pinMode(START, OUTPUT);

  pinMode(S_ANDE, INPUT);
  pinMode(S_GENERADOR, INPUT);


  // Iniciamos la clase indicando
  // Número de pin: donde tenemos conectado el SCT-013
  // Valor de calibración: valor obtenido de la calibración teórica
  energyMonitor.current(0, 2.63);

  Ethernet.begin(mac, ip); // Inicializa la conexion Ethernet y el servidor
  server.begin();

}

void loop()
{
  // Obtenemos el valor de la corriente eficaz
  // Pasamos el número de muestras que queremos tomar
  double Irms = energyMonitor.calcIrms(1500);

  // Calculamos la potencia aparente
  double potencia =  Irms * voltajeRed;

  //Variables donde se almacenan los estados de energia
  //Leemos las señales de entrada de Ande Y Generador
  boolean estado_ande = digitalRead(S_ANDE);
  boolean estado_generador = digitalRead(S_GENERADOR);

  // Crea una conexion Cliente
  EthernetClient client = server.available();


  if (client) {
    //Comprobamos si hay conexion con el cliente
    while (client.connected()) {
      //Comprobamos si hay byte para ser leidos
      if (client.available()) {
        String peticion = client.readString();


        client.println("HTTP/1.1 200 OK");
        client.println("Access-Control-Allow-Origin: *");
        client.println("Content-Type: text/html");
        client.println("Connection: close");
        client.println();

        //Reseteo del combustible
        reset(peticion, client);

        //Peticion para poder enviar el valor de la corriente
        recibirCorriente(peticion, client, Irms);

        // Si recibimos la señal de ande, le enviamos un verdadero a la aplicacion
        statusAnde(peticion, client, estado_ande);

        //Enviamos la señal del generador a la aplicacion
        statusGenerador(peticion, client, estado_generador);

        /*--------------------------------- SI TENEMOS ANDE --------------------*/
        //Si tenemos señal de ande
        if (estado_ande) {
          cont_generador = 0;
          digitalWrite(C_GENERADOR, LOW);

          if (cont_ande == 0) {

            //Desactivamos el contactor de generador
            digitalWrite(C_GENERADOR, LOW);
            delay(3000);

            //Activamos el contactor de la Ande
            digitalWrite(C_ANDE, HIGH);

            //Activamos el stop del generador
            digitalWrite(STOP, HIGH);
            cont_ande = cont_ande + 1;
          }
          /*--------------------------------- SI NO TENEMOS ANDE --------------------*/
        } else {
          cont_ande = 0;

          if (cont_generador == 0) {
            //Desactivamos el contactor de la Ande
            digitalWrite(C_ANDE, LOW);

            delay(3000);

            //Activamos el contactor de generador
            digitalWrite(C_GENERADOR, HIGH);

            //Desactivamos el stop del generador
            digitalWrite(STOP, LOW);
            cont_generador = cont_generador + 1;
          }

          //Si no arranca el generador el va seguir intentado
          if (!estado_generador) {
            //Peticion para poder encender el generador
            start_G(peticion);
          }
        }
        delay(50);
        client.stop();

      }
    }
  }
}



void start(int duracion) {
  digitalWrite(START, HIGH);
  delay(duracion);
  digitalWrite(START, LOW);
}

void start_G( String peticion) {
  //Peticion para poder encender el generador
  if (peticion.indexOf("start") >= 0) {
    int duracion = peticion.substring(peticion.indexOf("?") + 6, peticion.indexOf("?") + 10).toInt();
    start(duracion);
  }
}


void statusAnde( String peticion, EthernetClient client,  boolean data) {
  if (peticion.indexOf("estadoande") >= 0) {
    client.print(data);
  }
}

void statusGenerador( String peticion, EthernetClient client,  boolean data ) {
  if (peticion.indexOf("estadogenerador") >= 0) {
    client.print(data);
  }
}

void recibirCorriente(String peticion, EthernetClient client, double corriente) {
  if (peticion.indexOf("enviaDato") >= 0) {
    client.print(corriente);
  }
}

void stopG( String peticion ) {


}

void reset( String peticion, EthernetClient client ) {
  if (peticion.indexOf("reset") >= 0) {
    digitalWrite(RESET, HIGH);
    delay(500);
    digitalWrite(RESET, LOW);
  }
}
