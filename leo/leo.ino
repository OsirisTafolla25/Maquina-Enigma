/**
 * ============================================================================
 * PROYECTO: ENIGMA RECEPTOR (LEONARDO)
 * DESCRIPCION: Estación de intercepción que recibe la configuración completa,
 * simula la máquina y verifica el mensaje.
 * ============================================================================
 */

// Variables dinámicas (se sobrescriben al recibir datos)
int rotorCount = 3; 
int rotorOrder[5]; 
int ringSettings[5]; 
int rotorPos[5];
String steckerPairs = "";
String currentReflector = "";

// Datos técnicos
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

// Funciones Auxiliares
int mod(int val, int m) { int r=val%m; return r<0?r+m:r; }

char aplicarPlugboard(char c){
  int index=steckerPairs.indexOf(c);
  if(index!=-1 && index%2==0) return steckerPairs[index+1];
  if(index!=-1 && index%2!=0) return steckerPairs[index-1]; return c;
}

char enc(char c){
  if(c<'A'||c>'Z') return c; 
  
  // 1. Plugboard IN
  c = aplicarPlugboard(c); 
  int idx = c-'A';

  int r=rotorCount-1; int m=rotorCount-2; 
  
  // 2. Movimiento Mecánico (Stepping)
  bool mM=(rotorPos[r]==(notch[rotorOrder[r]]-'A'));
  bool mL=(rotorCount>2 && rotorPos[m]==(notch[rotorOrder[m]]-'A'));
  
  if(mL) mM=true;

  rotorPos[r]=(rotorPos[r]+1)%26; 
  if(mM) rotorPos[m]=(rotorPos[m]+1)%26;
  if(mL) rotorPos[rotorCount-3]=(rotorPos[rotorCount-3]+1)%26;
  
  // 3. Rotores (Ida)
  for(int i=rotorCount-1; i>=0; i--){
    int shift=rotorPos[i]-ringSettings[rotorOrder[i]];
    idx=mod(idx+shift,26); 
    idx=(rotorWiring[rotorOrder[i]][idx]-'A'); 
    idx=mod(idx-shift,26);
  }
  
  // 4. Reflector
  if(currentReflector.length()>0) idx=currentReflector[idx]-'A';
  
  // 5. Rotores (Vuelta)
  for(int i=0; i<rotorCount; i++){
    int shift=rotorPos[i]-ringSettings[rotorOrder[i]];
    idx=mod(idx+shift,26); 
    idx=rotorWiring[rotorOrder[i]].indexOf((char)(idx+'A')); 
    idx=mod(idx-shift,26);
  }
  
  // 6. Plugboard OUT
  return aplicarPlugboard((char)(idx+'A'));
}

// Función para parsear el paquete 'Count|Ref|Order|Rings|Plugs|Start|Msg'
String getValue(String data, char separator, int index){
  int found = 0; int strIndex[] = {0, -1}; int maxIndex = data.length()-1;
  for(int i=0; i<=maxIndex && found<=index; i++){
    if(data.charAt(i)==separator || i==maxIndex){
        found++; strIndex[0] = strIndex[1]+1; strIndex[1] = (i == maxIndex) ? i+1 : i;
    }
  }
  return found>index ? data.substring(strIndex[0], strIndex[1]) : "";
}

void setup() {
  Serial.begin(9600); Serial1.begin(9600);
  Serial.println(F("=== ESTACION DE INTERCEPCION (LEONARDO) ==="));
  Serial.println(F("Sistema en espera de transmision segura..."));
}

void loop() {
  if (Serial1.available()) {
    String packet = Serial1.readStringUntil('\n');
    packet.trim();
    if(packet.indexOf('|') == -1) return; 

    Serial.println(F("\n\n>>> PAQUETE DE DATOS RECIBIDO <<<"));
    
    // 1. EXTRAER DATOS
    String sCount = getValue(packet, '|', 0);
    String sRef   = getValue(packet, '|', 1);
    String sOrder = getValue(packet, '|', 2);
    String sRings = getValue(packet, '|', 3);
    String sPlugs = getValue(packet, '|', 4);
    String sStart = getValue(packet, '|', 5);
    String sMsg   = getValue(packet, '|', 6);

    // 2. CONFIGURACIÓN VISUAL
    Serial.println(F("[CFG] Auto-configurando Hardware..."));
    rotorCount = sCount.toInt();
    currentReflector = (sRef == "B") ? refB : refC;
    
    // Rotores
    for(int i=0; i<rotorCount; i++){
      String digit = String(sOrder.charAt(i));
      rotorOrder[i] = digit.toInt();
    }
    // Anillos
    for(int i=0; i<rotorCount; i++) ringSettings[rotorOrder[i]] = sRings.charAt(i) - 'A';
    // Cables
    steckerPairs = (sPlugs.length() > 0) ? sPlugs : "";
    // Posición Inicial
    for(int i=0; i<rotorCount; i++) rotorPos[i] = sStart.charAt(i) - 'A';
    
    // 3. MOSTRAR INFO
    Serial.println(F("---------------------------------------"));
    Serial.print(F("CONF: ")); Serial.print(sCount); Serial.print("R | UKW-"); Serial.print(sRef);
    Serial.print(F(" | INI: ")); Serial.println(sStart);
    Serial.print(F("MSG : ")); Serial.println(sMsg);
    Serial.println(F("---------------------------------------"));
    
    // 4. DESCIFRADO
    Serial.print(F("Procesando"));
    String output = "";
    for(char c : sMsg){
      output += enc(c);
      Serial.print("."); delay(50); 
    }
    Serial.println(" OK.");
    
    Serial.print(F("\n>> CLARO: ")); Serial.println(output);
    
    // 5. RETORNO AL UNO
    Serial1.println(output);
    Serial.println(F("[TX] Confirmacion enviada."));
  }
}