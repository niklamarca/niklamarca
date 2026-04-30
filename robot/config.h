#pragma once

// ================================================================
// FIAP RoboCup 2026 - Configurações do Robô Rinoceronte
// Arduino MEGA 2560
//
// LIGAÇÕES FÍSICAS:
//
// [HC-06 Bluetooth]
//   VCC  → 5V do Arduino
//   GND  → GND do Arduino
//   TX   → Pin 19 (RX1) do MEGA
//   RX   → Pin 18 (TX1) do MEGA  (use divisor de tensão!)
//
// [Servo MG995 - Chifre]
//   Laranja (Sinal) → Pin 44 do MEGA
//   Vermelho (VCC)  → 5V (ou alimentação separada para carga alta)
//   Marrom  (GND)   → GND
//
// [Chaves Fim de Curso - conecte COM ao GND e NO ao pino]
//   Frente Esquerda → Pin 30
//   Frente Direita  → Pin 31
//   Trás Esquerda   → Pin 32
//   Trás Direita    → Pin 33
//
// [LED WS2812B]
//   Dados → Pin 46
//   5V    → 5V Arduino (ou fonte separada para muitos LEDs)
//   GND   → GND
//
// [LED RGB 5050 com BD135]
//   Base R → Resistor 4k7 → Pin 11
//   Base G → Resistor 4k7 → Pin 12
//   Base B → Resistor 4k7 → Pin 13
//   Emissor de cada BD135 → GND
//   Coletor R → fio vermelho da fita
//   Coletor G → fio verde da fita
//   Coletor B → fio azul da fita
//   VCC fita  → 12V ou fonte adequada
//
// [Shield L293D]
//   Encaixar diretamente no Arduino MEGA
//   Motor Esquerdo → bornes M1 do shield
//   Motor Direito  → bornes M2 do shield
// ================================================================

// ================================================================
// ESCOLHA O TIPO DE LED (descomente apenas UM)
// ================================================================
#define LED_WS2812B
// #define LED_RGB5050

// ================================================================
// SERVO - CHIFRE RETRÁTIL
// ================================================================
#define SERVO_PIN            44
#define HORN_RETRACTED_DEG   10    // graus: chifre totalmente recolhido
#define HORN_EXTENDED_DEG   160    // graus: chifre totalmente disparado
#define HORN_ATTACK_DELAY   400    // ms: tempo até recolher automaticamente (0 = manual)

// ================================================================
// CHAVES FIM DE CURSO (INPUT_PULLUP: LOW = acionado)
// ================================================================
#define SW_FRONT_1   30
#define SW_FRONT_2   31
#define SW_BACK_1    32
#define SW_BACK_2    33

// ================================================================
// LED WS2812B
// ================================================================
#ifdef LED_WS2812B
  #define LED_DATA_PIN   46
  #define NUM_LEDS       30     // ajuste ao tamanho do seu robô
  #define LED_BRILHO     80     // 0-255
#endif

// ================================================================
// LED RGB 5050 via Transistores BD135
// ================================================================
#ifdef LED_RGB5050
  #define LED_R_PIN   11   // PWM
  #define LED_G_PIN   12   // PWM
  #define LED_B_PIN   13   // PWM
#endif

// ================================================================
// VELOCIDADE (0-255)
// ================================================================
#define VEL_PADRAO   200
#define VEL_GIRO     180
#define VEL_MINIMA    80
#define VEL_MAXIMA   255

// ================================================================
// BLUETOOTH (Serial1 = pinos 18/19 do MEGA)
// ================================================================
#define BT_SERIAL   Serial1
#define BT_BAUD     9600
#define DBG_BAUD    9600

// ================================================================
// SHIELD DE MOTOR - Números dos motores (biblioteca AFMotor)
// M1 = Motor Esquerdo, M2 = Motor Direito
// ================================================================
#define MOTOR_ESQ_NUM   1
#define MOTOR_DIR_NUM   2
