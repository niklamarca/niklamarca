// ================================================================
// FIAP RoboCup 2026 - Robô Rinoceronte
// Microcontrolador : Arduino MEGA 2560
// Shield de Motor  : L293D (encaixe direto, biblioteca AFMotor)
// Comunicação      : Bluetooth HC-06 via Serial1
//
// PROTOCOLO BLUETOOTH (compatível com "Bluetooth RC Car" Android):
//   F = Frente        B = Trás
//   L = Girar Esq     R = Girar Dir
//   G = Frente+Esq    I = Frente+Dir
//   H = Trás+Esq      J = Trás+Dir
//   S = Parar
//   W = Disparar chifre    w = Recolher chifre
//   0-9 = Velocidade (0=mínimo  9=máximo)
// ================================================================

#include "config.h"
#include <AFMotor.h>
#include <Servo.h>

#ifdef LED_WS2812B
  #include <FastLED.h>
  CRGB leds[NUM_LEDS];
#endif

// ================================================================
// OBJETOS
// ================================================================
AF_DCMotor motorEsq(MOTOR_ESQ_NUM);
AF_DCMotor motorDir(MOTOR_DIR_NUM);
Servo      hornServo;

// ================================================================
// ESTADO GLOBAL
// ================================================================
int           velocidade        = VEL_PADRAO;
bool          chifreFora        = false;
unsigned long chifreDispParaoEm = 0;     // millis() em que deve recolher (0 = manual)
char          ultimoCmd         = 'S';

// ================================================================
// MOTORES
// ================================================================

void motoresSetup() {
  motorEsq.setSpeed(velocidade);
  motorDir.setSpeed(velocidade);
  motorEsq.run(RELEASE);
  motorDir.run(RELEASE);
}

void parar() {
  motorEsq.run(RELEASE);
  motorDir.run(RELEASE);
}

void moverFrente(int vel) {
  motorEsq.setSpeed(vel);
  motorDir.setSpeed(vel);
  motorEsq.run(FORWARD);
  motorDir.run(FORWARD);
}

void moverTras(int vel) {
  motorEsq.setSpeed(vel);
  motorDir.setSpeed(vel);
  motorEsq.run(BACKWARD);
  motorDir.run(BACKWARD);
}

void girarEsquerda(int vel) {
  motorEsq.setSpeed(vel);
  motorDir.setSpeed(vel);
  motorEsq.run(BACKWARD);
  motorDir.run(FORWARD);
}

void girarDireita(int vel) {
  motorEsq.setSpeed(vel);
  motorDir.setSpeed(vel);
  motorEsq.run(FORWARD);
  motorDir.run(BACKWARD);
}

void diagonalFrenteEsq(int vel) {
  motorEsq.setSpeed(vel / 2);
  motorDir.setSpeed(vel);
  motorEsq.run(FORWARD);
  motorDir.run(FORWARD);
}

void diagonalFrenteDir(int vel) {
  motorEsq.setSpeed(vel);
  motorDir.setSpeed(vel / 2);
  motorEsq.run(FORWARD);
  motorDir.run(FORWARD);
}

void diagonalTrasEsq(int vel) {
  motorEsq.setSpeed(vel / 2);
  motorDir.setSpeed(vel);
  motorEsq.run(BACKWARD);
  motorDir.run(BACKWARD);
}

void diagonalTrasDir(int vel) {
  motorEsq.setSpeed(vel);
  motorDir.setSpeed(vel / 2);
  motorEsq.run(BACKWARD);
  motorDir.run(BACKWARD);
}

// ================================================================
// CHAVES DE FIM DE CURSO
// ================================================================

void sensoresSetup() {
  pinMode(SW_FRONT_1, INPUT_PULLUP);
  pinMode(SW_FRONT_2, INPUT_PULLUP);
  pinMode(SW_BACK_1,  INPUT_PULLUP);
  pinMode(SW_BACK_2,  INPUT_PULLUP);
}

bool frenteBloqueada() {
  return digitalRead(SW_FRONT_1) == LOW || digitalRead(SW_FRONT_2) == LOW;
}

bool trasBloqueada() {
  return digitalRead(SW_BACK_1) == LOW || digitalRead(SW_BACK_2) == LOW;
}

// ================================================================
// CHIFRE - SERVO MG995
// ================================================================

void chifreSetup() {
  hornServo.attach(SERVO_PIN);
  hornServo.write(HORN_RETRACTED_DEG);
  chifreFora = false;
}

void dispararChifre() {
  hornServo.write(HORN_EXTENDED_DEG);
  chifreFora = true;
  if (HORN_ATTACK_DELAY > 0) {
    chifreDispParaoEm = millis() + HORN_ATTACK_DELAY;
  }
}

void recolherChifre() {
  hornServo.write(HORN_RETRACTED_DEG);
  chifreFora = false;
  chifreDispParaoEm = 0;
}

// Recolhe automaticamente o chifre depois de HORN_ATTACK_DELAY ms
void atualizarChifre() {
  if (chifreFora && chifreDispParaoEm != 0 && millis() >= chifreDispParaoEm) {
    recolherChifre();
    ledParado();
  }
}

// ================================================================
// LEDs
// ================================================================

void ledSetup() {
#ifdef LED_WS2812B
  FastLED.addLeds<WS2812B, LED_DATA_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(LED_BRILHO);
  fill_solid(leds, NUM_LEDS, CRGB::Blue);
  FastLED.show();
#endif
#ifdef LED_RGB5050
  pinMode(LED_R_PIN, OUTPUT);
  pinMode(LED_G_PIN, OUTPUT);
  pinMode(LED_B_PIN, OUTPUT);
  analogWrite(LED_R_PIN, 0);
  analogWrite(LED_G_PIN, 0);
  analogWrite(LED_B_PIN, 255);
#endif
}

void setLed(uint8_t r, uint8_t g, uint8_t b) {
#ifdef LED_WS2812B
  fill_solid(leds, NUM_LEDS, CRGB(r, g, b));
  FastLED.show();
#endif
#ifdef LED_RGB5050
  analogWrite(LED_R_PIN, r);
  analogWrite(LED_G_PIN, g);
  analogWrite(LED_B_PIN, b);
#endif
}

// Cores por estado
void ledParado()  { setLed(0,   0,   255); }  // Azul
void ledFrente()  { setLed(0,   255,  0 ); }  // Verde
void ledTras()    { setLed(255, 0,    0 ); }  // Vermelho
void ledGiro()    { setLed(255, 200,  0 ); }  // Amarelo
void ledAtaque()  { setLed(255, 50,   0 ); }  // Laranja

// ================================================================
// PROCESSAMENTO DE COMANDOS BLUETOOTH
// ================================================================

void processarComando(char cmd) {
  // Controle de velocidade: dígito '0' a '9'
  if (cmd >= '0' && cmd <= '9') {
    velocidade = map(cmd - '0', 0, 9, VEL_MINIMA, VEL_MAXIMA);
    return;
  }

  ultimoCmd = cmd;

  switch (cmd) {

    case 'F':
      if (!frenteBloqueada()) { moverFrente(velocidade); ledFrente(); }
      else                    { parar(); ledParado(); }
      break;

    case 'B':
      if (!trasBloqueada()) { moverTras(velocidade); ledTras(); }
      else                  { parar(); ledParado(); }
      break;

    case 'L':
      girarEsquerda(VEL_GIRO); ledGiro();
      break;

    case 'R':
      girarDireita(VEL_GIRO); ledGiro();
      break;

    case 'G':
      if (!frenteBloqueada()) { diagonalFrenteEsq(velocidade); ledFrente(); }
      else                    { parar(); ledParado(); }
      break;

    case 'I':
      if (!frenteBloqueada()) { diagonalFrenteDir(velocidade); ledFrente(); }
      else                    { parar(); ledParado(); }
      break;

    case 'H':
      if (!trasBloqueada()) { diagonalTrasEsq(velocidade); ledTras(); }
      else                  { parar(); ledParado(); }
      break;

    case 'J':
      if (!trasBloqueada()) { diagonalTrasDir(velocidade); ledTras(); }
      else                  { parar(); ledParado(); }
      break;

    case 'S':
      parar(); ledParado();
      break;

    case 'W':
      dispararChifre(); ledAtaque();
      break;

    case 'w':
      recolherChifre(); ledParado();
      break;
  }
}

// ================================================================
// PROTEÇÃO CONTÍNUA DOS SENSORES (loop)
// ================================================================

void verificarSensores() {
  if (frenteBloqueada() && ultimoCmd == 'F') {
    parar();
    ledParado();
    BT_SERIAL.print('X');
    Serial.println("BLOQUEADO: frente");
    ultimoCmd = 'S';
  }
  if (trasBloqueada() && ultimoCmd == 'B') {
    parar();
    ledParado();
    BT_SERIAL.print('X');
    Serial.println("BLOQUEADO: tras");
    ultimoCmd = 'S';
  }
}

// ================================================================
// SETUP & LOOP
// ================================================================

void setup() {
  Serial.begin(DBG_BAUD);
  BT_SERIAL.begin(BT_BAUD);

  motoresSetup();
  sensoresSetup();
  chifreSetup();
  ledSetup();

  Serial.println("=== Robo Rinoceronte - FIAP RoboCup 2026 ===");
  Serial.println("Aguardando conexao Bluetooth...");
}

void loop() {
  if (BT_SERIAL.available()) {
    char cmd = (char)BT_SERIAL.read();
    processarComando(cmd);
    Serial.print("[CMD] ");
    Serial.println(cmd);
  }

  verificarSensores();
  atualizarChifre();
}
