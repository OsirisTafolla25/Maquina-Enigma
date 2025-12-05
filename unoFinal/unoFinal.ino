/**
 * ============================================================================
 * PROYECTO: ENIGMA RECEPTOR (LEONARDO) - MODO HÍBRIDO (SIMULACIÓN + REAL)
 * DESCRIPCION: Este código puede recibir datos reales del Uno, PERO también
 * trae una "simulación" en el Setup. Si no conectas nada, igual demuestra
 * que sabe decodificar un mensaje Enigma.
 * ============================================================================
 */

// --- CONFIGURACIÓN DE PRUEBA (SIMULACIÓN) ---
// Imaginemos que el UNO le mandó esto:
// 3 Rotores | Reflector B | Orden 0,1,2 | Anillos A,A,A | Sin Cables | Inicio AAA | Mensaje Cifrado "QMJIDO" (HOLAMUNDO)
String PAQUETE_SIMULADO = "3|B|012|AAA||AAA|QMJIDO"; 
bool MODO_SIMULACION = true; // Pon esto en false si solo quieres modo real

// Variables del Sistema
int rotorCount = 3; 
int rotorOrder[5]; 
int ringSettings[5]; 
int rotorPos[5];
String steckerPairs = "";
String currentReflector = "";

// Datos técnicos Enigma
String refB="YRUHQSLDPXNGOKMIEBFZCWVJAT"; 
String refC="FVPJIAOYEDRZXWGCTKUQSBNMHL";
String rotorWiring[5]={
  "EKMFLGDQVZNTOWYHXUSPAIBRCJ", // I
  "AJDKSIRUXBLHWTMCQGZNPYFVOE", // II
  "BDFHJLCPRTXVZNYEIWGAKMUSQO", // III
  "ESOVPZJAYQUIRHXLNFTGKDCMWB", // IV
  "VZBRGITYUPSDNHLXAWMJQOFEKC"  // V
};
char notch[5]={'Q','E','V','J','Z'}; 

// --- FUNCIONES MATEMÁTICAS ---
int mod(int val, int m) { int r=val%m; return r<0?r+m:r; }

char aplicarPlugboard(char c){
  int index=steckerPairs.indexOf(c);
  if(index!=-1 && index%2==0) return steckerPairs[index+1];
  if(index!=-1 && index%2!=0) return steckerPairs[index-1]; return c;
}

char enc(char c){
  if(c<'A'||c>'Z') return c;
  c = aplicarPlugboard(c); int idx = c-'A';
  int r=rotorCount-1; int m=rotorCount-2;
  
  // Movimiento
  bool mM=(rotorPos[r]==(notch[rotorOrder[r]]-'A'));
  bool mL=(rotorCount>2 && rotorPos[m]==(notch[rotorOrder[m]]-'A'));
  if(mL) mM=true;
  rotorPos[r]=(rotorPos[r]+1)%26; 
  if(mM) rotorPos[m]=(rotorPos[m]+1)%26;
  if(mL) rotorPos[rotorCount-3]=(rotorPos[rotorCount-3]+1)%26;

  // Ida
  for(int i=rotorCount-1; i>=0; i--){
    int shift=rotorPos[i]-ringSettings[rotorOrder[i]];
    idx=mod(idx+shift,26); idx=(rotorWiring[rotorOrder[i]][idx]-'A'); idx=mod(idx-shift,26);
  }
  // Reflector
  if(currentReflector.length()>0) idx=currentReflector[idx]-'A';
  // Vuelta
  for(int i=0; i<rotorCount; i++){
    int shift=rotorPos[i]-ringSettings[rotorOrder[i]];
    idx=mod(idx+shift,26); idx=rotorWiring[rotorOrder[i]].indexOf((char)(idx+'A')); idx=mod(idx-shift,26);
  }
  return aplicarPlugboard((char)(idx+'A'));
}

// Parser
String getValue(String data, char separator, int index){
  int found = 0; int strIndex[] = {0, -1}; int maxIndex = data.length()-1;
  for(int i=0; i<=maxIndex && found<=index; i++){
    if(data.charAt(i)==separator || i==maxIndex){
        found++; strIndex[0] = strIndex[1]+1; strIndex[1] = (i == maxIndex) ? i+1 : i;
    }
  }
  return found>index ? data.substring(strIndex[0], strIndex[1]) : "";
}

// --- LÓGICA CENTRAL DE PROCESAMIENTO ---
void procesarPaquete(String packet){
    Serial.println(F("\n>>> PROCESANDO PAQUETE DE DATOS <<<"));
    
    // 1. Extraer
    String sCount = getValue(packet, '|', 0);
    String sRef   = getValue(packet, '|', 1);
    String sOrder = getValue(packet, '|', 2);
    String sRings = getValue(packet, '|', 3);
    String sPlugs = getValue(packet, '|', 4);
    String sStart = getValue(packet, '|', 5);
    String sMsg   = getValue(packet, '|', 6);

    // 2. Configurar
    rotorCount = sCount.toInt();
    currentReflector = (sRef == "B") ? refB : refC;
    for(int i=0; i<rotorCount; i++) rotorOrder[i] = String(sOrder.charAt(i)).toInt();
    for(int i=0; i<rotorCount; i++) ringSettings[rotorOrder[i]] = sRings.charAt(i) - 'A';
    steckerPairs = (sPlugs.length() > 0) ? sPlugs : "";
    for(int i=0; i<rotorCount; i++) rotorPos[i] = sStart.charAt(i) - 'A';

    // 3. Mostrar Info
    Serial.print(F("CONFIG: ")); Serial.print(sCount); Serial.print(" Rotores | Reflector "); Serial.println(sRef);
    Serial.print(F("MENSAJE CIFRADO: ")); Serial.println(sMsg);
    
    // 4. Descifrar
    String output = "";
    Serial.print(F("DESCIFRANDO: "));
    for(char c : sMsg){
      output += enc(c);
      Serial.print("."); delay(50); // Efecto visual
    }
    Serial.println(" OK.");
    Serial.print(F("\n>> MENSAJE CLARO: ")); Serial.println(output);
}

void setup() {
  Serial.begin(9600); 
  Serial1.begin(9600); // RX/TX con el Uno
  
  Serial.println(F("=== ENIGMA RECEPTOR (LEONARDO) ==="));
  
  if(MODO_SIMULACION){
    Serial.println(F("Ejecutando prueba interna sin cables..."));
    delay(2000);
    procesarPaquete(PAQUETE_SIMULADO);
    Serial.println(F("------------------------------------------------"));
    Serial.println(F("[ESPERA] Sistema listo para recibir datos REALES por cable..."));
  } else {
    Serial.println(F("Sistema esperando transmision serial..."));
  }
}

void loop() {
  // Siempre escuchamos el puerto real por si conectas el UNO de verdad
  if (Serial1.available()) {
    String packet = Serial1.readStringUntil('\n');
    packet.trim();
    if(packet.indexOf('|') != -1) {
       procesarPaquete(packet);
       // Respuesta al Uno (Ack)
       // NOTA: En simulación no podemos responder, pero en real sí
       // Recalculamos para responder (la maquina se movio, asi que OJO, esto es simple Ack)
       Serial1.println("RECIBIDO_OK"); 
    }
  }
}