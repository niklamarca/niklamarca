/*
 * ROBÔ RINOCERONTE - FIAP Robocup
 * --------------------------------
 * Hardware:
 *   - Arduino Uno/Mega
 *   - Driver de motor L298N  (2 motores DC)
 *   - Servo SG90/MG996R      (chifre retrátil)
 *   - Módulo Bluetooth HC-05 (controle remoto)
 *
 * Comandos via Bluetooth (app Serial):
 *   F = Frente       B = Ré
 *   L = Girar Esq    R = Girar Dir
 *   Q = Estr. Esq    E = Estr. Dir   <- (rodas mecanum, se houver)
 *   A = ATACAR (estender chifre)
 *   X = RECOLHER chifre
 *   S = Parar
 *   + = Aumentar velocidade
 *   - = Diminuir velocidade
 */

#include <Servo.h>
#include <SoftwareSerial.h>

// ── Pinos Motor A (Esquerda) ──────────────────────────────────────
#define ENA  5   // PWM
#define IN1  2
#define IN2  3

// ── Pinos Motor B (Direita) ───────────────────────────────────────
#define ENB  6   // PWM
#define IN3  4
#define IN4  7

// ── Servo do Chifre ───────────────────────────────────────────────
#define SERVO_PIN     9
#define HORN_RECOLHIDO  0    // graus = chifre dentro
#define HORN_ESTENDIDO  90   // graus = chifre fora (ajuste conforme mecânica)

// ── Bluetooth (SoftwareSerial para liberar Serial0 para debug) ────
#define BT_RX  10
#define BT_TX  11

// ─────────────────────────────────────────────────────────────────

Servo servoChifre;
SoftwareSerial bluetooth(BT_RX, BT_TX);

int velocidade = 180;          // 0–255
bool chifreEstendido = false;

// ── Funções dos Motores ──────────────────────────────────────────

void motorEsquerda(int vel) {
  // vel positivo = frente, negativo = ré, 0 = parado
  if (vel > 0) {
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
  } else if (vel < 0) {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH);
  } else {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
  }
  analogWrite(ENA, abs(vel));
}

void motorDireita(int vel) {
  if (vel > 0) {
    digitalWrite(IN3, HIGH);
    digitalWrite(IN4, LOW);
  } else if (vel < 0) {
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, HIGH);
  } else {
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, LOW);
  }
  analogWrite(ENB, abs(vel));
}

void parar() {
  motorEsquerda(0);
  motorDireita(0);
}

void frente() {
  motorEsquerda(velocidade);
  motorDireita(velocidade);
}

void re() {
  motorEsquerda(-velocidade);
  motorDireita(-velocidade);
}

// Giro no próprio eixo
void girarEsquerda() {
  motorEsquerda(-velocidade);
  motorDireita(velocidade);
}

void girarDireita() {
  motorEsquerda(velocidade);
  motorDireita(-velocidade);
}

// Deslocamento lateral (requer rodas mecanum/omnidirecional)
// Com rodas normais, equivale a uma curva suave
void estrafaEsquerda() {
  motorEsquerda(velocidade / 2);
  motorDireita(velocidade);
}

void estrafaDireita() {
  motorEsquerda(velocidade);
  motorDireita(velocidade / 2);
}

// ── Chifre ────────────────────────────────────────────────────────

void estenderChifre() {
  servoChifre.write(HORN_ESTENDIDO);
  chifreEstendido = true;
  bluetooth.println("CHIFRE: ATACANDO!");
}

void recolherChifre() {
  servoChifre.write(HORN_RECOLHIDO);
  chifreEstendido = false;
  bluetooth.println("CHIFRE: recolhido.");
}

// Ataque em carga: avança e estende o chifre ao mesmo tempo
void cargarRinoceronte() {
  estenderChifre();
  frente();
  bluetooth.println(">> CARGA DO RINOCERONTE! <<");
}

// ── Setup ─────────────────────────────────────────────────────────

void setup() {
  // Motores
  pinMode(ENA, OUTPUT); pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(ENB, OUTPUT); pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);

  // Servo
  servoChifre.attach(SERVO_PIN);
  recolherChifre();

  // Serial / Bluetooth
  Serial.begin(9600);
  bluetooth.begin(9600);

  parar();

  bluetooth.println("=== ROBO RINOCERONTE PRONTO ===");
  bluetooth.println("F=Frente B=Re L=Esq R=Dir Q=EstrafaE E=EstrafaD");
  bluetooth.println("A=ATACAR X=Recolher S=Parar  +/- Velocidade");
}

// ── Loop ──────────────────────────────────────────────────────────

void loop() {
  if (bluetooth.available()) {
    char cmd = (char)bluetooth.read();
    executarComando(cmd);
  }
  if (Serial.available()) {
    char cmd = (char)Serial.read();
    executarComando(cmd);
  }
}

void executarComando(char cmd) {
  switch (cmd) {
    case 'F': case 'f': frente();          bluetooth.println("Frente");        break;
    case 'B': case 'b': re();              bluetooth.println("Re");             break;
    case 'L': case 'l': girarEsquerda();   bluetooth.println("Girando Esq");   break;
    case 'R': case 'r': girarDireita();    bluetooth.println("Girando Dir");   break;
    case 'Q': case 'q': estrafaEsquerda(); bluetooth.println("Estrafa Esq");   break;
    case 'E': case 'e': estrafaDireita();  bluetooth.println("Estrafa Dir");   break;
    case 'S': case 's': parar();           bluetooth.println("Parado");        break;
    case 'A': case 'a': cargarRinoceronte();                                    break;
    case 'X': case 'x': recolherChifre();  parar();                             break;
    case '+':
      velocidade = min(255, velocidade + 20);
      bluetooth.print("Velocidade: "); bluetooth.println(velocidade);
      break;
    case '-':
      velocidade = max(60, velocidade - 20);
      bluetooth.print("Velocidade: "); bluetooth.println(velocidade);
      break;
    default: break;
  }
}
