
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>

// =====================================================
// LCD I2C
// SDA -> A4
// SCL -> A5
// =====================================================
LiquidCrystal_I2C lcd(0x27, 16, 2);

// =====================================================
// SERVO - SOMENTE ENTRADA
// =====================================================
Servo cancela;

const int PINO_SERVO = 6;
const int CANCELA_FECHADA = 0;
const int CANCELA_ABERTA = 90;

// =====================================================
// TEMPORIZADOR - 3 SEGUNDOS
// =====================================================
const unsigned long TEMPO_CANCELA = 3000UL;

unsigned long inicioTemporizador = 0;
bool cancelaEstaAberta = false;

// =====================================================
// DISTANCIA DOS SENSORES
// =====================================================
const float DISTANCIA_DETECCAO = 15.0;
const float DISTANCIA_LIBERACAO = 20.0;

// =====================================================
// SENSOR DE ENTRADA
// =====================================================
const int TRIG_ENTRADA = 9;
const int ECHO_ENTRADA = 10;

// =====================================================
// SENSOR DE SAIDA
// =====================================================
const int TRIG_SAIDA = 11;
const int ECHO_SAIDA = 12;

// =====================================================
// ESTACIONAMENTO
// =====================================================
const int TOTAL_VAGAS = 10;
int carrosEstacionados = 0;

// =====================================================
// CONTROLE DOS SENSORES
// =====================================================
bool carroNaEntrada = false;
bool carroNaSaida = false;

// =====================================================
// MEDIR DISTANCIA
// =====================================================
float medirDistancia(int trig, int echo)
{
  digitalWrite(trig, LOW);
  delayMicroseconds(2);

  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);

  unsigned long duracao = pulseIn(echo, HIGH, 30000UL);

  if (duracao == 0)
  {
    return 999.0;
  }

  return (duracao * 0.0343) / 2.0;
}

// =====================================================
// ATUALIZAR LCD
// =====================================================
void atualizarLCD()
{
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("ESTACIONAMENTO");

  lcd.setCursor(0, 1);

  if (carrosEstacionados >= TOTAL_VAGAS)
  {
    lcd.print("CHEIO!");
  }
  else
  {
    lcd.print("Vagas: ");
    lcd.print(TOTAL_VAGAS - carrosEstacionados);
  }
}

// =====================================================
// ABRIR CANCELA - SOMENTE ENTRADA
// =====================================================
void abrirCancela()
{
  cancela.write(CANCELA_ABERTA);

  cancelaEstaAberta = true;
  inicioTemporizador = millis();

  Serial.println("CANCELA ABERTA");
  Serial.println("TEMPORIZADOR: 3 SEGUNDOS");
}

// =====================================================
// FECHAR CANCELA
// =====================================================
void fecharCancela()
{
  cancela.write(CANCELA_FECHADA);

  cancelaEstaAberta = false;

  Serial.println("CANCELA FECHADA");
}

// =====================================================
// TEMPORIZADOR DA CANCELA
// =====================================================
void controlarTemporizador()
{
  if (cancelaEstaAberta)
  {
    unsigned long tempoDecorrido =
      millis() - inicioTemporizador;

    if (tempoDecorrido >= TEMPO_CANCELA)
    {
      // Fecha apos 3 segundos,
      // desde que a entrada esteja livre.
      if (!carroNaEntrada)
      {
        fecharCancela();
        Serial.println("TEMPO FINALIZADO");
      }
    }
  }
}

// =====================================================
// SETUP
// =====================================================
void setup()
{
  Serial.begin(9600);

  // Sensores
  pinMode(TRIG_ENTRADA, OUTPUT);
  pinMode(ECHO_ENTRADA, INPUT);

  pinMode(TRIG_SAIDA, OUTPUT);
  pinMode(ECHO_SAIDA, INPUT);

  // Servo
  cancela.attach(PINO_SERVO);
  fecharCancela();

  // LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("ESTACIONAMENTO");

  lcd.setCursor(0, 1);
  lcd.print("Iniciando...");

  delay(2000);

  atualizarLCD();

  Serial.println("========================");
  Serial.println(" SISTEMA ESTACIONAMENTO");
  Serial.println("========================");
  Serial.println("Sistema iniciado");
}

// =====================================================
// LOOP PRINCIPAL
// =====================================================
void loop()
{
  // Leitura do sensor de entrada
  float distanciaEntrada =
    medirDistancia(TRIG_ENTRADA, ECHO_ENTRADA);

  delay(60);

  // Leitura do sensor de saida
  float distanciaSaida =
    medirDistancia(TRIG_SAIDA, ECHO_SAIDA);

  // ===================================================
  // ENTRADA - ABRE O SERVO
  // ===================================================
  if (distanciaEntrada <= DISTANCIA_DETECCAO)
  {
    if (!carroNaEntrada)
    {
      carroNaEntrada = true;

      Serial.println("CARRO DETECTADO NA ENTRADA");

      if (carrosEstacionados < TOTAL_VAGAS)
      {
        carrosEstacionados++;

        // Abre a cancela somente na entrada
        abrirCancela();

        atualizarLCD();

        Serial.print("Carros estacionados: ");
        Serial.println(carrosEstacionados);

        Serial.print("Vagas disponiveis: ");
        Serial.println(
          TOTAL_VAGAS - carrosEstacionados
        );
      }
      else
      {
        Serial.println("ESTACIONAMENTO CHEIO");
        Serial.println("ENTRADA BLOQUEADA");

        atualizarLCD();
      }
    }
  }
  else if (distanciaEntrada >= DISTANCIA_LIBERACAO)
  {
    if (carroNaEntrada)
    {
      carroNaEntrada = false;

      Serial.println("CARRO DEIXOU A ENTRADA");
    }
  }

  // ===================================================
  // SAIDA - NAO MOVIMENTA O SERVO
  // ===================================================
  if (distanciaSaida <= DISTANCIA_DETECCAO)
  {
    if (!carroNaSaida)
    {
      carroNaSaida = true;

      Serial.println("CARRO DETECTADO NA SAIDA");

      if (carrosEstacionados > 0)
      {
        carrosEstacionados--;

        Serial.println("CARRO SAIU");

        Serial.print("Carros estacionados: ");
        Serial.println(carrosEstacionados);

        Serial.print("Vagas disponiveis: ");
        Serial.println(
          TOTAL_VAGAS - carrosEstacionados
        );

        // Apenas atualiza as vagas
        atualizarLCD();
      }
      else
      {
        Serial.println("ESTACIONAMENTO VAZIO");
      }
    }
  }
  else if (distanciaSaida >= DISTANCIA_LIBERACAO)
  {
    if (carroNaSaida)
    {
      carroNaSaida = false;

      Serial.println("CARRO DEIXOU A SAIDA");
    }
  }

  // Fechamento automatico apos 3 segundos
  controlarTemporizador();

  delay(100);
}
