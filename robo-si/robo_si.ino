//=====================================================================
// PROGRAMA MODELO PARA O ARDUINO DO ROBO DE SI
// Placa alvo: Arduino Mega 2560
//
// ALTERACAO NESTA VERSAO: sistema de LED RGB por ESTADOS
//   - Luz PASSIVA .... robo ligado, arma desarmada
//   - Luz de ATAQUE .. arma acionada
//
// O controle do LED saiu de dezenas de digitalWrite() espalhados pelo
// codigo e virou uma maquina de estados nao-bloqueante com PWM real
// (analogWrite). Os pinos 44, 45 e 46 do Mega sao PWM (Timer5), entao
// agora da para produzir qualquer cor, e nao so as 8 combinacoes de
// liga/desliga que o codigo antigo permitia.
//=====================================================================


//=====================================================================
// CONFIGURACAO DO LED  --  MEXA SO AQUI PARA AJUSTAR AS CORES
//=====================================================================

// Tipo do LED RGB:
//   0 = CATODO comum (HIGH acende)  <-- era o comportamento do codigo original
//   1 = ANODO  comum (LOW  acende)
// Se ao gravar as cores sairem invertidas (ex: passivo laranja e ataque
// ciano), troque este valor para 1. E o unico ajuste necessario.
#define LED_ANODO_COMUM   0

// Brilho maximo global (0..255). Use para calibrar sem mexer nas cores.
#define BRILHO_MAX        255

//---------------------------------------------------------------------
// CORES  (R, G, B)
//---------------------------------------------------------------------

// PASSIVA: robo ligado e arma desarmada.
// Ciano/azul. Escolhido de proposito: e a cor mais distante do vermelho
// no circulo cromatico, entao a troca passivo -> ataque e inconfundivel
// mesmo de longe ou para quem tem daltonismo vermelho-verde.
const byte COR_PASSIVA[3] = {   0, 140, 255 };

// ATAQUE: este e o ponto do seu problema.
// O vermelho puro (255, 0, 0) JA E o maximo do canal vermelho - nao ha
// como "aumentar" por software. O que resolve e mudar o matiz: os 45 de
// verde puxam a cor para laranja-vermelho, que tem luminancia percebida
// muito maior porque o olho humano e muito mais sensivel ao verde.
//   Quer mais forte / mais visivel -> suba o verde (45 -> 70 -> 100)
//   Quer mais vermelho puro         -> desca o verde (45 -> 20 -> 0)
const byte COR_ATAQUE[3]  = { 255,  45,   0 };

// ARMA ESTENDIDA: mesmo matiz do ataque (continua sendo "estou armado"),
// so muda a animacao. Ambar, um pouco mais claro.
const byte COR_ARMADA[3]  = { 255,  90,   0 };

// RECOLHENDO: magenta-vermelho, avisa que a arma esta voltando.
const byte COR_RECOLHE[3] = { 255,   0,  70 };

//---------------------------------------------------------------------
// TEMPOS DAS ANIMACOES (ms)
//---------------------------------------------------------------------
#define RESPIRA_PERIODO   2600   // ciclo completo do "respirar" passivo
#define RESPIRA_MINIMO      30   // nunca apaga de vez: robo sempre visivelmente ligado

#define STROBE_ON           60   // ataque: tempo aceso
#define STROBE_OFF          70   // ataque: tempo apagado  (~7,7 Hz)

#define PULSO_ON            90   // recolhimento: tempo no brilho alto
#define PULSO_OFF           90   // recolhimento: tempo no brilho baixo
#define PULSO_PISO          70   // brilho baixo do pulso (nao chega a apagar)

//---------------------------------------------------------------------
// MODOS DE ANIMACAO
//---------------------------------------------------------------------
#define MODO_SOLIDO   0
#define MODO_RESPIRA  1
#define MODO_STROBE   2
#define MODO_PULSO    3
//=====================================================================


//Variaveis Globais*******************************
//Portas usados para o controle do Driver de Motor
char DIR_Latch  = 12;                           //
char DIR_SER    = 8;                            //
char DIR_CLK    = 4;                            //
                                                //
char PWM_M1     =  11;                          //
char PWM_M2     =  3;                           //
char PWM_M3     =  5;                           //
char PWM_M4     =  6;                           //
//------------------------------------------------

//Porta usada para o Sensor-----------------------
char sensor     = 10;                            //
//------------------------------------------------

//Pinos do LED RGB (44, 45 e 46 sao PWM no Mega - Timer5)
const byte LED_VM_1 = 45;   // vermelho
const byte LED_VD_1 = 46;   // verde
const byte LED_AZ_1 = 44;   // azul

//Variaveis de Estado*****************************
char estado     = 0;                            //
char estado_mov = 0;                            //
//------------------------------------------------

//Variaveis de Temporizacao***********************
unsigned long  tempo_arma = 0;                  //
unsigned long  tempo_mov = 0;                   //
//------------------------------------------------

int valor_b = 0;

//Variaveis do motor de animacao do LED***********
byte          led_r     = 0;                    //
byte          led_g     = 0;                    //
byte          led_b     = 0;                    //
byte          led_modo  = MODO_SOLIDO;          //
unsigned long led_t0    = 0;                    //
//------------------------------------------------

//Variaveis de recebimento de dados serial********
int data_by_serial = 0;                         //
//------------------------------------------------


//=====================================================================
// CAMADA 1 - escrita fisica no LED
// Unico lugar do programa que fala com os pinos do LED. Trata a
// inversao de anodo/catodo comum e aplica o brilho maximo global.
//=====================================================================
void escreve_rgb(byte r, byte g, byte b){
  r = (byte)((unsigned int)r * BRILHO_MAX / 255);
  g = (byte)((unsigned int)g * BRILHO_MAX / 255);
  b = (byte)((unsigned int)b * BRILHO_MAX / 255);

#if LED_ANODO_COMUM
  analogWrite(LED_VM_1, 255 - r);
  analogWrite(LED_VD_1, 255 - g);
  analogWrite(LED_AZ_1, 255 - b);
#else
  analogWrite(LED_VM_1, r);
  analogWrite(LED_VD_1, g);
  analogWrite(LED_AZ_1, b);
#endif
}


//=====================================================================
// CAMADA 2 - selecao de cor e modo
// So reinicia a animacao quando a cor OU o modo realmente mudam. Sem
// isso, chamar a funcao dentro do loop() travaria a animacao no quadro
// zero para sempre.
//=====================================================================
void define_led(const byte cor[3], byte modo){
  if ((led_r == cor[0]) && (led_g == cor[1]) &&
      (led_b == cor[2]) && (led_modo == modo)){
    return;}

  led_r    = cor[0];
  led_g    = cor[1];
  led_b    = cor[2];
  led_modo = modo;
  led_t0   = millis();
}


//=====================================================================
// CAMADA 3 - motor de animacao (NAO-BLOQUEANTE)
// Calcula o brilho do quadro atual a partir de millis() e escreve no
// LED. Nao usa delay(), entao nunca atrasa o sensor da arma nem o
// bluetooth. Deve ser chamada a cada volta do loop().
//=====================================================================
void atualiza_led(void){
  unsigned long dt    = millis() - led_t0;
  unsigned int  nivel = 255;

  if (led_modo == MODO_RESPIRA){
    unsigned int f    = (unsigned int)(dt % RESPIRA_PERIODO);
    unsigned int meio = RESPIRA_PERIODO / 2;
    unsigned int tri;

    //Onda triangular: sobe ate o meio do periodo, desce depois
    if (f < meio){
      tri = (unsigned int)((unsigned long)f * 255UL / meio);}
        else{
          tri = (unsigned int)((unsigned long)(RESPIRA_PERIODO - f) * 255UL / meio);}

    //Correcao gama: o olho enxerga brilho de forma logaritmica. Sem esta
    //linha o fade parece "pular" perto do maximo e travar perto do minimo.
    nivel = (unsigned int)((unsigned long)tri * tri / 255UL);
    if (nivel < RESPIRA_MINIMO){
      nivel = RESPIRA_MINIMO;}}

  if (led_modo == MODO_STROBE){
    if ((dt % (STROBE_ON + STROBE_OFF)) < STROBE_ON){
      nivel = 255;}
        else{
          nivel = 0;}}

  if (led_modo == MODO_PULSO){
    if ((dt % (PULSO_ON + PULSO_OFF)) < PULSO_ON){
      nivel = 255;}
        else{
          nivel = PULSO_PISO;}}

  //MODO_SOLIDO cai aqui com nivel = 255

  escreve_rgb((byte)((unsigned int)led_r * nivel / 255),
              (byte)((unsigned int)led_g * nivel / 255),
              (byte)((unsigned int)led_b * nivel / 255));
}


//=====================================================================
// CAMADA 4 - traducao do estado do robo em cor
// Prioridade: a arma SEMPRE ganha da movimentacao. Enquanto a arma nao
// estiver parada, o LED mostra ataque, aconteca o que acontecer no
// bluetooth. Isso elimina a classe de bug do codigo antigo, onde um
// comando de movimento podia apagar o LED de ataque.
//=====================================================================
void atualiza_estado_led(void){
  if (estado == 1){                              //arma disparando
    define_led(COR_ATAQUE,  MODO_STROBE);  return;}

  if (estado == 2){                              //arma estendida (segurando)
    define_led(COR_ARMADA,  MODO_SOLIDO);  return;}

  if (estado == 3){                              //arma recolhendo
    define_led(COR_RECOLHE, MODO_PULSO);   return;}

  //estado == 0 -> arma desarmada: luz PASSIVA.
  //Solida enquanto anda (confirma que o comando chegou), respirando parado.
  if (estado_mov == 1){
    define_led(COR_PASSIVA, MODO_SOLIDO);}
      else{
        define_led(COR_PASSIVA, MODO_RESPIRA);}
}


//Funcao para acionamento individual de motor***************
//Valor M: Especifica o motor, 1 = motor 1                //
//2 = motor 2, 3 = motor 3 e 4 = motor 4                  //
//Valor S: Especifica o sentido de rotacao, por ser       //
//1 ou 2                                                  //
//Valor M_P: Especifica a potencia do motor, seu          //
//valor pode ser de 0 (potencia minima) ate 255 (         //
//potencia maxima                                         //
void aciona_motor_b(int M, int S, int M_P){               //
  char i = 0;                                             //
  int valor_b_temp = 0;                                   //
                                                          //
  if (M == 1){                                            //
    analogWrite(PWM_M1, M_P);                             //
    valor_b = valor_b & 0b11110011;                       //
    if (S == 1){                                          //
      valor_b = valor_b | 0b00000100;}                    //
          else{                                           //
            if (S == 2){                                  //
            valor_b = valor_b | 0b00001000;}}}            //
                                                          //
  if (M == 2){                                            //
    analogWrite(PWM_M2, M_P);                             //
    valor_b = valor_b & 0b11101101;                       //
    if (S == 1){                                          //
      valor_b = valor_b | 0b00000010;}                    //
          else{                                           //
            if (S == 2){                                  //
            valor_b = valor_b | 0b00010000;}}}            //
                                                          //
  if (M == 3){                                            //
    analogWrite(PWM_M3, M_P);                             //
    valor_b = valor_b & 0b10111110;                       //
    if (S == 1){                                          //
      valor_b = valor_b | 0b00000001;}                    //
          else{                                           //
            if (S == 2){                                  //
            valor_b = valor_b | 0b01000000;}}}            //
                                                          // 
  if (M == 4){                                            //
    analogWrite(PWM_M4, M_P);                             //
    valor_b = valor_b & 0b01011111;                       //
    if (S == 1){                                          //
      valor_b = valor_b | 0b00100000;}                    //
          else{                                           //
            if (S == 2){                                  //
            valor_b = valor_b | 0b10000000;}}}            //
                                                          //
  valor_b_temp = valor_b;                                 //         
  for (i = 0; i <= 7; i++){                               //
    digitalWrite(DIR_SER, LOW);                           //<- era analogWrite
    if ((valor_b_temp & 0b10000000) == 128){              //
      digitalWrite(DIR_SER, HIGH);}                       //
      delay(1);                                           //
      digitalWrite(DIR_CLK, HIGH);                        //
      delay(1);                                           //
      digitalWrite(DIR_CLK, LOW);                         //
      valor_b_temp = valor_b_temp * 2;}                   //
                                                          //
  digitalWrite(DIR_Latch, HIGH);                          //
  delay(1);                                               //
  digitalWrite(DIR_Latch, LOW);}                          //
//----------------------------------------------------------


//Funcao para tratamento de dado recebido pelo bluetooth**********
//NAO mexe mais no LED. Quem decide a cor e atualiza_estado_led(),
//chamada uma unica vez por volta do loop().
void Trata_serial(void){                                        //
                                                                //
  if (data_by_serial == 'w'){  //Se o dado recebido e w         //
    estado_mov = 1;                                             //
    aciona_motor_b(1, 1, 225); //Aciona motor 1 para direita    //
    aciona_motor_b(3, 1, 225);}//Aciona motor 2 para direita    //
                                                                //
  if (data_by_serial == 's'){  //Se o dado recebido e s         //
    estado_mov = 1;                                             //
    aciona_motor_b(1, 2, 225); //Aciona motor 1 para esquerda   //
    aciona_motor_b(3, 2, 225);}//Aciona motor 2 para esquerda   //
                                                                //
  if (data_by_serial == 'a'){                                   //
    estado_mov = 1;                                             //
    aciona_motor_b(1, 2, 225);                                  //
    aciona_motor_b(3, 1, 225);}                                 //
                                                                //
  if (data_by_serial == 'd'){                                   //
    estado_mov = 1;                                             //
    aciona_motor_b(1, 1, 225);                                  //
    aciona_motor_b(3, 2, 225);}                                 //
                                                                //
  if (data_by_serial == 'q'){                                   //
    //Parada explicita: estado_mov = 0. No codigo antigo o 'q'   //
    //marcava o robo como "em movimento" por mais 400 ms.        //
    estado_mov = 0;                                             //
    aciona_motor_b(1, 0, 0); //Para motor 1                     //
    aciona_motor_b(3, 0, 0);}//Para motor 2                     //
                                                                //
  tempo_mov = millis();}                                        //
//----------------------------------------------------------------


//=====================================================================
// Teste de LED na inicializacao.
// Serve para conferir a fiacao (se o vermelho acender no lugar do azul,
// os pinos estao trocados) e para ver as duas cores de trabalho lado a
// lado antes de entrar em combate.
//=====================================================================
void teste_led(void){
  escreve_rgb(255,   0,   0); delay(400);   //vermelho puro (o "fraco")
  escreve_rgb(  0, 255,   0); delay(400);   //verde
  escreve_rgb(  0,   0, 255); delay(400);   //azul
  escreve_rgb(  0,   0,   0); delay(200);

  //Preview das cores reais de trabalho
  escreve_rgb(COR_PASSIVA[0], COR_PASSIVA[1], COR_PASSIVA[2]); delay(700);
  escreve_rgb(COR_ATAQUE[0],  COR_ATAQUE[1],  COR_ATAQUE[2]);  delay(700);
  escreve_rgb(  0,   0,   0); delay(200);
}


//Funcao para configuracao inicial do microcontrolador*************
void setup(){

  pinMode(LED_VM_1, OUTPUT);
  pinMode(LED_VD_1, OUTPUT);
  pinMode(LED_AZ_1, OUTPUT);
  escreve_rgb(0, 0, 0);

  pinMode(7, OUTPUT);
  digitalWrite(7, LOW);

  pinMode(PWM_M1, OUTPUT);  //Define a porta 11, pois, PWM_M1 = 11, como saida
  pinMode(PWM_M2, OUTPUT);
  pinMode(PWM_M3, OUTPUT);
  pinMode(PWM_M4, OUTPUT);

  analogWrite(PWM_M1, 0);  //Escreve "0" na saida PWM_M1, esta saida define a potencia aplicada ao motor 1, logo motor inicializa parado
  analogWrite(PWM_M2, 0);
  analogWrite(PWM_M3, 0);
  analogWrite(PWM_M4, 0);

  pinMode(DIR_SER, OUTPUT);
  pinMode(DIR_CLK, OUTPUT);
  pinMode(DIR_Latch, OUTPUT);

  digitalWrite(DIR_SER, LOW);    //Inicializa a saida DIR_SER, porta "8" do arduino, pois, DIR_SER = 8 em 0 (tambem dito nivel logico baixo ou 0)
  digitalWrite(DIR_CLK, LOW);
  digitalWrite(DIR_Latch, LOW);

  pinMode(sensor, INPUT_PULLUP);  //Define a porta 10, pois, sensor = 10, como entrada

  teste_led();

  //Entra ja no estado passivo, com a animacao sincronizada a partir daqui
  led_t0 = millis();
  atualiza_estado_led();

  Serial3.begin(9600);  //Configura a comunicacao serial com o modulo Bluetooth
  Serial.begin(9600);
  Serial.println("INICIO");
}
//----------------------------------------------------------------


//Funcao principal do programa*************
void loop (){

//Este trecho verifica se algum dado foi recebido pela serial
  if( Serial3.available()){    //Se algum dado foi recebido pela serial
    data_by_serial = Serial3.read();  //Salva o dado na variavel data_by_serial
    Serial3.write(data_by_serial);  //Devolve o dado recebido
    if ((data_by_serial == 'a') || (data_by_serial == 'd') || (data_by_serial == 'w') || (data_by_serial == 's') || (data_by_serial == 'q')){  //Se o dado recebido corresponde a: a, d, w, s ou q
      Trata_serial ();}}  //Aciona os motores para executar o movimento associado ao dado 

//Este trecho interrompe o acionamento dos motores de movimentacao apos 400 ms sem recebimento de um novo dado pelo bluetooth
  if ((estado_mov == 1) && ((millis() - tempo_mov) >= 400)){  //Se os motores estao se movimentando devido a um dado recebido (estado_mov == 1) e se este movimento ja esta acontecendo ha mais de 400 milissegundos
     aciona_motor_b(1, 0, 0);//Para motor 1
     aciona_motor_b(3, 0, 0);//Para motor 2
     estado_mov = 0;}  //Desfaz a sinalizacao que indica que um movimento foi ordenado

//-------------------------------------------------------------------
//Inicia acionamento da arma
  if ((digitalRead(sensor) == 0 )&& (estado == 0)){  //Se o sensor da arma foi acionado (digitalRead(sensor) == 0) e se a arma nao estava acionada nem sendo acionada (estado == 1) nem esperando para ser desacionada (estado == 2) nem sendo desacionada (estado == 3)
    Serial.println("ATAQUE");
    estado = 1;  //Indica que a arma esta sendo acionada (estado == 1);
    tempo_arma = millis();  //Reseta o tempo de acionamento da arma
    aciona_motor_b(2, 1, 255);  //Aciona motor 3 para direita
    aciona_motor_b(4, 2, 255);} //Aciona motor 4 para direita

//Finaliza acionamento da arma
  if ((estado == 1) && ((millis() - tempo_arma) >= 600)){  //Se a arma esta sendo acionada (estado == 1) e o tempo de acionamento e maior ou igual a 600ms
    estado = 2;  //Indica que a arma esta estendida, esperando para ser recolhida (estado == 2);
    tempo_arma = millis();  //Reseta o tempo de acionamento da arma
    aciona_motor_b(2, 0, 0);  //Para motor 3
    aciona_motor_b(4, 0, 0);} //Para motor 4

//Inicia recolhimento da arma
  if ((estado == 2) && ((millis() - tempo_arma) >= 12500)){  //Se a arma esta esperando para ser desacionada (estado == 2) e o tempo de acionamento e maior ou igual a 12500ms
    estado = 3;  //Indica que a arma esta sendo desacionada (estado == 3);
    tempo_arma = millis();  //Reseta o tempo de acionamento da arma
    aciona_motor_b(2, 2, 200);  //Aciona motor 3 para esquerda
    aciona_motor_b(4, 1, 200);}  //Aciona motor 4 para esquerda

//Finaliza recolhimento da arma
  if ((estado == 3) && ((millis() - tempo_arma) >= 550)){  //Se a arma esta sendo desacionada (estado == 3) e o tempo de desacionamento e maior ou igual a 550ms
    estado = 0;  //Indica que a arma esta parada e pronta para ser acionada novamente (estado == 0);
    aciona_motor_b(2, 0, 0);  //Para motor 3
    aciona_motor_b(4, 0, 0);} //Para motor 4
//-------------------------------------------------------------------

//Controle do LED: decide a cor pelo estado atual e desenha o quadro.
//Nao bloqueia - so le millis() e escreve nos tres pinos PWM.
  atualiza_estado_led();
  atualiza_led();
}
