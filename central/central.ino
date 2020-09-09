
// ---------------------------------------------------------------------------
// Escornabot: Control por gestos mediante acelerometro para Escornabot - v1.0 - 08/09/2020
// 
// La idea principal de este proyecto es que personas con movilidad reducida puedan utilizar el robot Escornabot mediante 4 simples movimientos:  
// inclinando la cabeza hacia adelante, inclinando la cabeza hacia atrás, inclinando la cabeza hacia la izquierda e inclinando la cabeza hacia la derecha. 
//
// Aunque la idea principal es controlar el robot con los movimientos de la cabeza 
// también lo podremos controlar con las manos como si fuese un mando.
//
// AUTOR:
// Creado por Angel Villanueva - @avilmaru
//
// LINKS:
// Blog: http://www.mecatronicalab.es
//
//
// HISTORICO:
// 08/09/2020 v1.0 - Release inicial.
//
// ---------------------------------------------------------------------------
  

#include <ArduinoBLE.h>
#include <Arduino_LSM9DS1.h>
#include "DFRobotDFPlayerMini.h"

DFRobotDFPlayerMini myDFPlayer;

const int R_LED_PIN = 22;
const int G_LED_PIN = 23;
const int B_LED_PIN = 24;

const char* deviceServiceUuid = "ffe0";
const char* deviceServiceCharacteristicUuid = "ffe1";

String command = "";
String ultimaPosicion ="";
bool movimientosEnviados = false;
bool bloqueo = true;
String modo = "ESCRITURA";

unsigned long startTime = millis(); 
unsigned long timeInterval = 0; 
unsigned long LIMIT1 = 500;
unsigned long LIMIT2= 3000;

void setup() {

  /* Todas las trazas se encuentran comentadas */
  
  //Serial.begin(9600);
  //while (!Serial);

  Serial1.begin(9600);
  while (!Serial1);
  
  if (!IMU.begin()) {
    //Serial.println("Failed to initialize IMU!");
    while (1);
  }
  
  // begin ble initialization
  if (!BLE.begin()) {
    //Serial.println("starting BLE failed!");
    while (1);
  }

  if (!myDFPlayer.begin(Serial1)) {
    //Serial.println("Failed to initialize DFPlayer!");
    while (1);
  }
  
  //Serial.println(F("DFPlayer Mini online."));
  myDFPlayer.volume(20);


  pinMode(R_LED_PIN, OUTPUT);
  pinMode(G_LED_PIN, OUTPUT);
  pinMode(B_LED_PIN, OUTPUT);

  setColor("BLACK");

}

void loop() {
  
     connectToPeripheral();
}


void connectToPeripheral(){

  BLEDevice peripheral;

  do
  {
    // start scanning for peripherals
    BLE.scanForUuid(deviceServiceUuid);
    peripheral = BLE.available();
    
  } while (!peripheral);

  
  if (peripheral) {
    // discovered a peripheral, print out address, local name, and advertised service
    //Serial.print("Found  ");
    //Serial.print(peripheral.address());
    //Serial.print(" '");
    //Serial.print(peripheral.localName());
    //Serial.print("' ");
    //Serial.print(peripheral.advertisedServiceUuid());
    //Serial.println();
  
    // stop scanning
    BLE.stopScan();
  
    controlPeripheral(peripheral);
   
  }
  
}

void controlPeripheral(BLEDevice peripheral) {

  
  // connect to the peripheral
  //Serial.println("Connecting ...");

  if (peripheral.connect()) {
    //Serial.println("Connected");
  } else {
    //Serial.println("Failed to connect!");
    return;
  }

  // discover peripheral attributes
  //Serial.println("Discovering attributes ...");
  if (peripheral.discoverAttributes()) {
    //Serial.println("Attributes discovered");
  } else {
    //Serial.println("Attribute discovery failed!");
    peripheral.disconnect();
    return;
  }

  BLECharacteristic gestureCharacteristic = peripheral.characteristic(deviceServiceCharacteristicUuid);
    
  if (!gestureCharacteristic) {
    //Serial.println("Peripheral does not have gesture characteristic!");
    peripheral.disconnect();
    return;
  } 
  
  if (!gestureCharacteristic.canWrite()) {
    //Serial.println("Peripheral does not have a writable gesture characteristic!");
    peripheral.disconnect();
    return;
  } 
  
  if (!gestureCharacteristic.canRead()) {
    //Serial.println("Peripheral does not have a read gesture characteristic!");
    peripheral.disconnect();
    return;
  }

  
  while (peripheral.connected()) {

    if (modo == "LECTURA")
    {
      setColor("RED");
      if (gestureCharacteristic.valueUpdated()) 
      {
      
        byte value;
        gestureCharacteristic.readValue(value);
        lecturaComandos(value);
        
      }
      
    }else if (modo == "ESCRITURA")
    {
      
      command = sendInstruction();
      if (command == "")
        continue;
        
      int n = command.length(); 
      // declaring character array 
      char char_array[n + 1]; 
      // copying the contents of the string to char array 
      strcpy(char_array, command.c_str()); 
      gestureCharacteristic.writeValue((char*)char_array);
      bloqueo = true;

      if (command == "g\n")
        modo = "LECTURA";
      
    }
   
  }

  //Serial.println("Peripheral disconnected!");

}

 
String calculoAngulos()
{
    float ax, ay, az;
    float gx, gy, gz;
    float pitch,roll;
    String posicion = "";
    
    if (IMU.accelerationAvailable()) {
      IMU.readAcceleration(ax, ay, az);
    }
    
    // Calcular los ángulos con el acelerometro
    pitch = atan2(ax, sqrt(ay*ay + az*az));
    roll =  atan2(ay, sqrt(ax*ax + az*az));

    // Convertimos de radianes a grados
    pitch *= (180.0 / PI);
    roll  *= (180.0 / PI);  
      
   //Serial.print(F("valor en X:  "));
   //Serial.print(pitch);
   //Serial.print(F("\t Valor en Y: "));
   //Serial.println(roll);
   
    posicion =  String(pitch, DEC) + "," + String(roll, DEC);
    delay(50);
    return posicion;
}


String determinarPosicion() {

  String posicion = "indeterminada";

  String str = calculoAngulos();
  
  if (str.length() == 0)
    return posicion;
    
  // split 
  int pos = str.indexOf(",");
  int strLength = str.length();

  int valorX = str.substring(0,pos).toInt(); 
  int valorY = str.substring(pos+1,strLength).toInt(); 
  
  //Serial.print(F("valor en X:  "));
  //Serial.print(valorX);
  //Serial.print(F("\t valor en Y: "));
  //Serial.println(valorY);

  if ((valorY >= 20) && (valorX >= -10) && (valorX <= 10)) 
      posicion = "adelante";
  else if ((valorY <= -20) && (valorX >= -10) && (valorX <= 10)) 
      posicion = "atras";
  else if ((valorX >= 15) && (valorY >= -10) && (valorY <= 10)) 
      posicion = "izquierda";
  else if ((valorX <= -15) && (valorY >= -10) && (valorY <= 10)) 
      posicion = "derecha";
  else if ((valorX <= 10) && (valorX >= -10) && (valorY <= 10) && (valorY >= -10))
      posicion = "centro";
  else
      posicion = "indeterminada";    

  if (posicion == "indeterminada")
    setColor("RED");
  else
    setColor("GREEN");  

  return posicion;
  
}

bool validaPosicion(String posicion, unsigned long LIMIT)
{
   bool esValida = false;

  if ((bloqueo) && (posicion != "centro"))
  {
     ultimaPosicion = posicion;
     return false;
  }
   
  if (ultimaPosicion != posicion)
      startTime = millis();
      
    timeInterval = millis() - startTime;  
    if (timeInterval >= LIMIT)
      esValida = true;
    
    ultimaPosicion = posicion;
  
    return esValida;

}

String Tiporespuesta()
{
  
  bool esValida = false;
  String respuesta ="INDETERMINADA";
  
  String posicion = determinarPosicion();
  if (posicion == "adelante")
  {   
      esValida = validaPosicion(posicion,LIMIT1);
      if (esValida)
         respuesta = "SI";
  }
  else if (posicion == "atras")
  {
      esValida = validaPosicion(posicion,LIMIT1);
      if (esValida)
        respuesta = "NO";
      
  }else
      respuesta ="INDETERMINADA";

 
  return respuesta;
  
}

void obligarSituarseCentro(unsigned long LIMIT)
{

  bool esValida = false;
  String posicion = "";
  
  // situarse en el centro
  do{
    
    posicion = determinarPosicion();
    if (posicion == "centro")
      esValida = validaPosicion(posicion,LIMIT);
      
  }while (posicion != "centro" && esValida == false);
  
}

String sendInstruction() {

  String _sendCommand = "";
  String posicion = "";
  String respuesta = "";
  bool esValida = false;
    
  posicion = determinarPosicion();

  if (posicion == "adelante")
  {  
      esValida = validaPosicion(posicion,LIMIT1);
      if (esValida)
      {
         setColor("BLUE");
        _sendCommand = "n\n";
         decirFrase("avanzar");
         movimientosEnviados = true;
      }
  }
  else if (posicion == "atras")
  {
      esValida = validaPosicion(posicion,LIMIT1);
      if (esValida)
      {
         setColor("BLUE");
        _sendCommand = "s\n";
         decirFrase("retroceder");
         movimientosEnviados = true;
      }
  }
  else if (posicion == "izquierda")
  {
      esValida = validaPosicion(posicion,LIMIT1);
      if (esValida)
      {
         setColor("BLUE");
        _sendCommand = "w\n";
         decirFrase("izquierda");
         movimientosEnviados = true;
      }
  }
  else if (posicion == "derecha")
  {
      esValida = validaPosicion(posicion,LIMIT1);
      if (esValida)
      {
         setColor("BLUE");
        _sendCommand = "e\n";
         decirFrase("derecha");
         movimientosEnviados = true;
      }
 
  }
  else if (posicion == "centro")
  {
      if (bloqueo)
      {
         // permanecer en el centro 'x' tiempo para desbloquear
         esValida = validaPosicion(posicion,LIMIT1);
         if (esValida)
          bloqueo = false;
         else
          return "";          
      }

      // si se permanece en el centro "y" tiempo perdir acciones
      esValida = validaPosicion(posicion,LIMIT2);
      if (movimientosEnviados && esValida)
      {   
        _sendCommand = respuestaAccionesRequeridas();
        movimientosEnviados = false;
      }
      
  }else
  {
       ultimaPosicion = "indeterminada";  
  }

  
  return _sendCommand; 
  
}

String respuestaAccionesRequeridas()
{

  String _sendCommand = "";
  String respuesta = "";
  
  setColor("GREEN");

  // situarse en el centro
  obligarSituarseCentro(LIMIT1);
  
  // ¿Ejecutar las instrucciones?
  decirFrase("preguntar_ejecutar");
  delay(2200);
    
  do{
     respuesta = Tiporespuesta();
  }while (respuesta == "INDETERMINADA");
   
  if (respuesta == "SI")
  { 
      setColor("BLUE");
      decirFrase("si");
      delay(1200);
      _sendCommand = "g\n";
      decirFrase("ejecutar");
      delay(2000);
    
  }else{
  
      setColor("BLUE"); 
      decirFrase("no"); 
      delay(1200);

      setColor("GREEN");

      // situarse en el centro
      obligarSituarseCentro(LIMIT1);
      
      // ¿Resetear las instrucciones?
      decirFrase("preguntar_resetear");
      delay(2200);
  
      do{
        respuesta = Tiporespuesta();
      }while (respuesta == "INDETERMINADA");
  
      if (respuesta == "SI")
      {
        
        setColor("BLUE");
        decirFrase("si");
        delay(1500);
        _sendCommand = "r\n";
        decirFrase("resetear");
        
      }else{
  
         setColor("BLUE");
         decirFrase("no");
         delay(1200);
         decirFrase("continuar");
         delay(2200);
         bloqueo = true;
      }

  }

  return _sendCommand;
  

}

void lecturaComandos(byte comando)
{
      
  if (comando == 78)        // ascii: N 
    decirFrase("avanzando");
  else if (comando == 83)  // ascii:  S
    decirFrase("retrocediendo");
  else if (comando == 87)  // ascii : W
    decirFrase("izquierda");
  else if (comando == 69)  // ascii : E
    decirFrase("derecha");
  else if (comando == 70)  // ascii : F
  {
    decirFrase("fin");
    delay(3000);
    // inicializar los valores
    command = "";
    ultimaPosicion ="";
    movimientosEnviados = false;
    bloqueo = true;
    modo = "ESCRITURA";  
  }
 
}

void setColor(String color)
{

  if (color == "RED")
  {
    digitalWrite(R_LED_PIN, LOW);  // High values -> lower brightness
    digitalWrite(G_LED_PIN, HIGH);
    digitalWrite(B_LED_PIN, HIGH);
    
  }else if (color == "GREEN")
  {
    digitalWrite(R_LED_PIN, HIGH);  // High values -> lower brightness
    digitalWrite(G_LED_PIN, LOW);
    digitalWrite(B_LED_PIN, HIGH);
    
  }else if (color == "BLUE")
  {
    digitalWrite(R_LED_PIN, HIGH);  // High values -> lower brightness
    digitalWrite(G_LED_PIN, HIGH);
    digitalWrite(B_LED_PIN, LOW);
    
  }else if (color == "BLACK")
  {
    digitalWrite(R_LED_PIN, HIGH);  // High values -> lower brightness
    digitalWrite(G_LED_PIN, HIGH);
    digitalWrite(B_LED_PIN, HIGH);
      
  }else if (color == "YELLOW")
  {
    digitalWrite(R_LED_PIN, LOW);  // High values -> lower brightness
    digitalWrite(G_LED_PIN, LOW);
    digitalWrite(B_LED_PIN, HIGH);
  }            

}

void decirFrase(String instruccion)
{

  byte indice = 0;
 
  if (instruccion == "izquierda")
      indice=1;
  else if (instruccion == "derecha")
      indice=2;
  else if (instruccion == "avanzar")
      indice=3;
  else if (instruccion == "retroceder")
      indice=4;
  else if (instruccion == "ejecutar")
      indice=5;
  else if (instruccion == "resetear")
      indice=6;
  else if (instruccion == "preguntar_ejecutar") // ¿quieres ejecutar las instrucciones?
      indice=7;
  else if (instruccion == "preguntar_resetear") //  ¿quieres borrar las instrucciones?
      indice=8;        
  else if (instruccion == "si")
      indice=9;  
  else if (instruccion == "no")
      indice=10;
  else if (instruccion == "continuar")
      indice=11;
  else if (instruccion == "avanzando")
      indice=12;
  else if (instruccion == "retrocediendo")
      indice=13;
  else if (instruccion == "girando_izquierda")
      indice=14;
  else if (instruccion == "girando_derecha")
      indice=15;
  else if (instruccion == "fin")
      indice=16;

          
  if (indice > 0)
    myDFPlayer.play(indice);
       
}
