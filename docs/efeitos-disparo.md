# Efeitos provisórios de disparo

O servidor mantém a decisão de disparar e acertar. Apenas após aceitar o disparo, envia um multicast cosmético não confiável com ponto, normal e existência de impacto. ServerFire continua sem parâmetros enviados pelo cliente.

Cada cliente cria um AMarau6ShotEffect local, sem replicação, colisão ou atualização por frame. O clarão dura 0,065s e acompanha o socket Muzzle do rifle em primeira pessoa para o dono e em terceira pessoa para outros. O impacto dura até 0,22s, deslocado da superfície pela normal. Amarelo indica superfície; alaranjado indica personagem vivo atingido. Não há impacto no limite do alcance quando o tiro não encontra nada.

O material emissivo M_ShotGlow pertence ao projeto em Content/Marau6/Effects. Usa geometria simples para esta primeira apresentação; nenhum asset original é modificado. As linhas de depuração foram substituídas pelos efeitos. Não há som ou nova animação nesta etapa.

## Validação

Development Editor / Win64 compilado. Cinco testes Marau6.Combat aprovados: Health, ServerFire, AmmoReload, WeaponVisuals e ShotEffects. O teste novo verifica disparo aceito, rejeição por cadência/recarga/morte, ausência de colisão/replicação, socket da boca, impacto apenas em acerto e destruição temporal. Esses testes usam mundo local; a observação entre clientes reais complementa a verificação.

Teste manual realizado com sucesso pelo usuário: em duas janelas Play As Client, observe o clarão da própria arma e da arma do outro jogador. Atire em parede, chão e personagem e confira o brilho no impacto. Atire para o céu: clarão sem impacto flutuante. Repita durante recarga e vazio: não deve haver efeito de tiro. Vida, dano e munição devem manter o comportamento validado.

Validação manual confirmada pelo usuário em 28/10/2026, com dois jogadores.
