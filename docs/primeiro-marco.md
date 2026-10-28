# Primeiro marco: combate multiplayer no editor

Data do registro: 28/10/2026 (America/Sao_Paulo).

## Resultado

Dois clientes no editor, em Play As Client, com servidor dedicado separado da perspectiva dos jogadores. Personagens se movimentam, pulam e enxergam as ações um do outro.

Disparo instantâneo no botão esquerdo, com dano uniforme de 25 e vida inicial 100. A autoridade fica no servidor, incluindo o limite de frequência. O HUD mostra a vida replicada e o estado de eliminado.

## Regras verificadas

- Quatro acertos eliminam.
- Eliminado não anda, pula ou causa dano; pode olhar.
- Vida permanece em zero ao receber mais disparos.
- Tiros não atravessam paredes/blocos sólidos.
- Testes automáticos Health e ServerFire passaram antes da validação manual.

## Limites desta entrega

- Tiro representado por linhas temporárias; sem arma visual, animação ou som específicos.
- Sem munição, recarga, reaparecimento ou persistência.
- Hitbox usa cápsula com dano uniforme; sem headshots ou balística.
- Sem compensação de latência ou solução de anticheat.
- Não houve teste de escala, exposição pública de servidor nem validação de executável dedicado empacotado.
- Variantes Shooter/Horror do template não foram migradas para as regras deste marco.

## Decisões

Câmera do personagem base presa à cápsula na altura dos olhos, para alinhar a mira à origem calculada pelo servidor sem depender da atualização de animações de ossos em um servidor sem renderização.

Git LFS rastreia assets Unreal antes do primeiro commit. Soluções do Visual Studio, caches, binários, logs e cópias de segurança locais não fazem parte do histórico.

## Próximo incremento proposto

Munição e recarga controladas pelo servidor, mantendo as regras já validadas. A forma de apresentação da arma será definida separadamente.

