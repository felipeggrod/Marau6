# Munição e recarga — incremento validado

Regras aprovadas em 28/10/2026:
- Carregador de 10 tiros e reserva inicial de 30.
- R inicia uma recarga de 2 segundos.
- Servidor decide consumo, início e conclusão da recarga.
- Pedidos rejeitados pela cadência não gastam balas; tiros aceitos gastam mesmo sem acerto.
- Não há disparo vazio ou durante recarga.
- Transfere somente o necessário, respeitando capacidade e reserva.
- R repetido, carregador cheio e reserva vazia são ignorados.
- Morte e EndPlay limpam o timer. Morrer antes do fim não transfere balas.
- Estado de munição replicado apenas ao dono; HUD mostra os contadores e RECARREGANDO.

## Verificação automática

Build Development Editor / Win64 passou. Passaram Marau6.Combat.Health, Marau6.Combat.ServerFire e Marau6.Combat.AmmoReload.
AmmoReload verifica números iniciais, consumo/cadência, carregador cheio, transferência parcial, duração, R repetido, bloqueio vazio/recarregando, reserva esgotada e cancelamento por morte.
A automação usa um mundo local autoritativo e timer real com avanço de quadros sintéticos. A validação manual com dois clientes descrita abaixo complementa os testes locais.

## Validação manual confirmada pelo usuário

- Cada jogador inicia com 10 no carregador e 30 de reserva.
- Três tiros resultam em 7/30; R e cerca de2 segundos resultam em10/27.
- O outro jogador mantém seus próprios contadores.
- Não há disparo durante recarga nem com o carregador vazio.
- Após morrer, não é possível atirar nem iniciar recarga.

Interrupção de uma recarga já ativa pela morte foi verificada automaticamente. EndPlay limpa o timer no código, mas não teve teste dedicado neste incremento.

Branch: feat/server-ammo-reload. Este documento registra o estado validado para o commit do incremento.
