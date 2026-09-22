# Robô de SI — firmware Arduino Mega

## Estados do LED RGB

| Estado do robô | `estado` | Cor | Animação |
|---|---|---|---|
| Parado, arma desarmada | 0 | Ciano `(0,140,255)` | Respiração lenta (2,6 s) |
| Andando, arma desarmada | 0 | Ciano `(0,140,255)` | Sólido |
| **Arma disparando** | 1 | **Laranja-vermelho `(255,45,0)`** | **Strobe 7,7 Hz** |
| Arma estendida (12,5 s) | 2 | Âmbar `(255,90,0)` | Sólido |
| Arma recolhendo | 3 | Magenta-vermelho `(255,0,70)` | Pulso |

O estado da arma **sempre** tem prioridade sobre a movimentação.

## Ajustes rápidos

Todos no bloco `CONFIGURAÇÃO DO LED`, no topo do `.ino`:

- `LED_ANODO_COMUM` — troque para `1` se as cores saírem invertidas
- `BRILHO_MAX` — brilho global (0..255)
- `COR_ATAQUE` — suba o canal verde para ficar mais visível, desça para ficar mais vermelho
- `STROBE_ON` / `STROBE_OFF` — velocidade do pisca de ataque

## Pinagem do LED

| Cor | Pino | Timer |
|---|---|---|
| Vermelho | 45 | Timer5 (OC5B) |
| Verde | 46 | Timer5 (OC5A) |
| Azul | 44 | Timer5 (OC5C) |

Os três são PWM no Mega 2560. O Timer5 não é usado pelo core do Arduino,
então o `analogWrite` nesses pinos não interfere no `millis()` nem no PWM
dos motores (pinos 11/3/5/6 → Timers 1, 3 e 4).
