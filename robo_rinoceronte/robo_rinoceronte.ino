/*
 ============================================================
  FIAP RoboCup 2026 — Robô Rinoceronte
 ============================================================

  CONEXÕES:
  ─────────────────────────────────────────────────────────
  HC-06 Bluetooth:
    TXD → pino 19 (RX1 do MEGA)
    RXD → pino 18 (TX1 do MEGA)
    VCC → 5V | GND → GND

  Servo MG995 (chifre retrátil):
    Signal → pino 9
    VCC    → 5V (ou bateria separada para mais torque)
    GND    → GND

  Chaves fim de curso (NO — Normally Open):
    Um terminal → GND, outro terminal → pino abaixo
    Frente Esq → pino 22
    Frente Dir → pino 23
    Trás Esq   → pino 24
    Trás Dir   → pino 25

  LED RGB via transistor BD135 + resistor 4k7:
    Arduino pino → resistor 4k7 → Base do BD135
    Coletor BD135 → canal LED (R/G/B)
    Emissor BD135 → GND
    Canal R → pino 30
    Canal G → pino 31
    Canal B → pino 32

  Shield L293D:
    M1 → Motor Direito
    M2 → Motor Esquerdo
    (encaixado diretamente no Arduino MEGA)

  BIBLIOTECA NECESSÁRIA (instalar na Arduino IDE):
    Sketch > Gerenciar Bibliotecas > buscar "AFMotor"
    Instalar: "Adafruit Motor Shield library" by Adafruit

  COMANDOS BLUETOOTH (app Android "Bluetooth RC Car"):
    F = Frente       B = Ré
    L = Esquerda     R = Direita
    S = Parar        W = Disparar chifre
    w = Recolher chifre

  CORES DO LED:
    Azul   = aguardando    Verde = frente
    Amarelo = ré           Azul  = virando
    Vermelho = parado      Roxo  = chifre ativo
 ============================================================
*/

#include <AFMotor.h>
#include <Servo.h>

// ── Motores de movimentação ──────────────────────────────
AF_DCMotor motorDir(1);          // M1 no shield = Motor Direito
AF_DCMotor motorEsq(2);          // M2 no shield = Motor Esquerdo
const uint8_t VELOCIDADE = 220;  // 0–255 | ajuste se motores ficarem fracos

// ── Servo (Chifre retrátil) ──────────────────────────────
Servo chifre;
const uint8_t PINO_SERVO       = 9;
const uint8_t CHIFRE_RECOLHIDO = 10;  // graus | ajuste conforme mecanismo
const uint8_t CHIFRE_DISPARADO = 90;  // graus | ajuste conforme mecanismo

// ── Bluetooth HC-06 ──────────────────────────────────────
#define BT Serial1  // hardware serial do MEGA: TX1=18, RX1=19

// ── Sensores fim de curso ────────────────────────────────
const uint8_t SW_FRENTE_ESQ = 22;
const uint8_t SW_FRENTE_DIR = 23;
const uint8_t SW_TRAS_ESQ   = 24;
const uint8_t SW_TRAS_DIR   = 25;

// ── LED RGB ──────────────────────────────────────────────
const uint8_t LED_R = 30;
const uint8_t LED_G = 31;
const uint8_t LED_B = 32;

// ── Estado de movimento ──────────────────────────────────
enum Direcao { PARADO, INDO_FRENTE, INDO_RE, VIRANDO };
Direcao direcaoAtual = PARADO;

// ─────────────────────────────────────────────────────────

void setup() {
  Serial.begin(9600);  // monitor serial para debug
  BT.begin(9600);      // baud padrão do HC-06

  // Fim de curso: LOW = ativado (com INPUT_PULLUP)
  pinMode(SW_FRENTE_ESQ, INPUT_PULLUP);
  pinMode(SW_FRENTE_DIR, INPUT_PULLUP);
  pinMode(SW_TRAS_ESQ,   INPUT_PULLUP);
  pinMode(SW_TRAS_DIR,   INPUT_PULLUP);

  // LEDs
  pinMode(LED_R, OUTPUT);
  pinMode(LED_G, OUTPUT);
  pinMode(LED_B, OUTPUT);

  // Servo: começa recolhido
  chifre.attach(PINO_SERVO);
  chifre.write(CHIFRE_RECOLHIDO);

  // Motores: velocidade padrão, parados
  motorDir.setSpeed(VELOCIDADE);
  motorEsq.setSpeed(VELOCIDADE);
  parar();

  setLED(0, 0, 1);  // azul = pronto, aguardando
  Serial.println(F("Robo Rinoceronte pronto!"));
}

void loop() {
  verificarFimDeCurso();

  if (BT.available()) {
    char cmd = (char)BT.read();
    Serial.print(F("CMD: "));
    Serial.println(cmd);
    processarComando(cmd);
  }
}

// ─────────────────────────────────────────────────────────
// COMANDOS
// ─────────────────────────────────────────────────────────

void processarComando(char cmd) {
  switch (cmd) {
    case 'F': frente();         break;
    case 'B': re();             break;
    case 'L': esquerda();       break;
    case 'R': direita();        break;
    case 'S': parar();          break;
    case 'W': dispararChifre(); break;
    case 'w': recolherChifre(); break;
    default: break;
  }
}

// ─────────────────────────────────────────────────────────
// MOVIMENTAÇÃO
// ─────────────────────────────────────────────────────────

void frente() {
  if (sensorFrenteAtivado()) {
    parar();
    return;
  }
  direcaoAtual = INDO_FRENTE;
  motorDir.run(FORWARD);
  motorEsq.run(FORWARD);
  setLED(0, 1, 0);  // verde
}

void re() {
  if (sensorTrasAtivado()) {
    parar();
    return;
  }
  direcaoAtual = INDO_RE;
  motorDir.run(BACKWARD);
  motorEsq.run(BACKWARD);
  setLED(1, 1, 0);  // amarelo
}

void esquerda() {
  // giro no próprio eixo: roda dir vai frente, roda esq vai atrás
  direcaoAtual = VIRANDO;
  motorDir.run(FORWARD);
  motorEsq.run(BACKWARD);
  setLED(0, 0, 1);  // azul
}

void direita() {
  // giro no próprio eixo: roda esq vai frente, roda dir vai atrás
  direcaoAtual = VIRANDO;
  motorDir.run(BACKWARD);
  motorEsq.run(FORWARD);
  setLED(0, 0, 1);  // azul
}

void parar() {
  direcaoAtual = PARADO;
  motorDir.run(RELEASE);
  motorEsq.run(RELEASE);
  setLED(1, 0, 0);  // vermelho
}

// ─────────────────────────────────────────────────────────
// ARMA — CHIFRE RETRÁTIL
// ─────────────────────────────────────────────────────────

void dispararChifre() {
  chifre.write(CHIFRE_DISPARADO);
  setLED(1, 0, 1);  // roxo = arma ativa
}

void recolherChifre() {
  chifre.write(CHIFRE_RECOLHIDO);
  // restaura cor do LED conforme estado atual
  switch (direcaoAtual) {
    case INDO_FRENTE: setLED(0, 1, 0); break;
    case INDO_RE:     setLED(1, 1, 0); break;
    case VIRANDO:     setLED(0, 0, 1); break;
    default:          setLED(1, 0, 0); break;
  }
}

// ─────────────────────────────────────────────────────────
// SENSORES FIM DE CURSO
// ─────────────────────────────────────────────────────────

bool sensorFrenteAtivado() {
  // LOW = ativado por causa do INPUT_PULLUP
  return (!digitalRead(SW_FRENTE_ESQ) || !digitalRead(SW_FRENTE_DIR));
}

bool sensorTrasAtivado() {
  return (!digitalRead(SW_TRAS_ESQ) || !digitalRead(SW_TRAS_DIR));
}

void verificarFimDeCurso() {
  // Só interrompe o motor se estiver indo na direção do sensor ativado
  if (direcaoAtual == INDO_FRENTE && sensorFrenteAtivado()) {
    parar();
  }
  if (direcaoAtual == INDO_RE && sensorTrasAtivado()) {
    parar();
  }
}

// ─────────────────────────────────────────────────────────
// LED RGB
// ─────────────────────────────────────────────────────────

void setLED(int r, int g, int b) {
  digitalWrite(LED_R, r ? HIGH : LOW);
  digitalWrite(LED_G, g ? HIGH : LOW);
  digitalWrite(LED_B, b ? HIGH : LOW);
}
