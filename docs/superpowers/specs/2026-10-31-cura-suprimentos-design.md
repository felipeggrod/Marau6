# Cura e suprimentos — proposta para aprovação

## Objetivo e abordagem
Dar utilidade aos Suprimentos existentes, permitindo tratar dano durante a partida com tempo de uso, interrupção e consumo autoritativo. O usuário escolheu iniciar este bloco; os números e controles abaixo são propostas do assistente.

Recomendação: cada Suprimento individual funciona como um kit de uso único, sem criar categorias novas ou cargas parciais. Aproveita os itens já no stash/mapa. Vários tipos de medicamentos e kits com várias cargas são alternativas futuras; ampliariam inventário, balanceamento e persistência sem necessidade para este primeiro ciclo de cura.

## Regras propostas
- Um Suprimento recupera até40 de vida ao concluir4 segundos de uso; vida máxima continua100. Sem cura gradual nem cura adicional acima do limite. Com vida cheia, recusar sem consumir.
- TAB abre inventário; selecionar Suprimentos e pressionar H inicia o uso. Inventário fecha ao começar; HUD mostra progresso e H cancela. Não usar automaticamente outro kit após conclusão ou cancelamento.
- Pode usar um Suprimento da mochila ou safe pocket. Stash permanece acessível apenas na base. Peças e rifles não curam. Não criar kits extras nem repor os perfis existentes.
- Só pode começar vivo, em participação ativa na arena, parado e no chão, com backend/journal disponíveis. Não começar durante recarga, cura já em curso ou extração em contagem. Mensagem clara explica a recusa.
- Pode mover a câmera enquanto trata o dano. Tentar andar/pular cancela e permite o movimento. Dano efetivamente aplicado também cancela. Tolerância pequena a correções físicas de posição fica a cargo da implementação; não permitir caminhar para outro ponto durante a cura.
- Atirar e recarregar ficam bloqueados enquanto a cura estiver ativa; não disparar automaticamente depois. H cancela explicitamente. Começar uma extração cancela a cura.
- Mover, equipar/trocar, descartar ou perder o item usado cancela o tratamento antes de concluir; não transferir a ação para um item que ocupe o mesmo slot. Abrir o inventário não cura nem consome por si só.
- Morte, retorno à base, desconexão e fim da sessão cancelam. Cancelamento antes da conclusão conserva o kit pelas regras normais de inventário/morte/safe; não concede cura parcial. Dano letal não pode ser revertido pelo timer de cura.
- Conclusão válida consome exatamente o GUID selecionado e só depois aplica a cura, sem espaço para reutilizar a mesma instância. Uma cura iniciada com90HP, por exemplo, termina em100HP e consome um kit.

## Autoridade, concorrência e persistência
O cliente solicita uso/cancelamento de um GUID próprio. Quantidade de cura, duração, validação de posse/estado, conclusão e remoção do item são decisões do servidor. Vida e estado/progresso relevante replicam para os clientes; mensagens e HUD não decidem o resultado.

No início e na conclusão, revalidar o mesmo GUID, tipo, dono e custódia/slot. Solicitações repetidas não reiniciam o timer nem multiplicam a cura. Reserva/conclusão devem resistir a callbacks reentrantes e conflito com troca, descarte ou resultado da partida.

Hoje o backend exige que identidades permaneçam nos snapshots. Acrescentar custódia terminal consumed, em vez de apagar o registro: apenas Suprimento anteriormente em inventário de participante ativo pode passar a consumido. Não consumir itens do mundo, de outro jogador, stash ou resultado confirmado. Consumido não tem dono nem slot disponível, não aparece no inventário/loot nem retorna na extração/morte/abandono. É imutável e não pode ressuscitar em snapshots futuros, novas entradas ou novas rodadas. Identidades novas não podem nascer consumidas.

A transição de consumo deve estar gravada no diário antes de confirmar a cura. Se a gravação falhar, não aplicar vida nem confirmar consumo, bloquear conforme a política de erro existente e manter a identidade recuperável. Repetição após perda de resposta/reinício preserva consumo único. Publicar alteração do inventário e vida com cuidado para que callbacks de sincronização não consumam sem curar ou curem duas vezes.

Vida continua sendo estado da partida; este bloco não acrescenta saúde persistente na base. Recuperação de servidor segue a política de abandono atual, respeitando os consumos já registrados. Um kit consumido não reaparece no safe por recuperação de cópia antiga. Não mudar o significado nem a numeração dos tipos de itens já salvos, nem apagar dados existentes para atualizar o backend.

## Apresentação e limites
HUD mostra tratamento ativo, tempo restante e confirmação/interrupção/recusa legível. Mostrar aos dois clientes a vida resultante do jogador. Animações médicas, som próprio, sangramento, membros corporais, curar terceiros, fome/sede, vários medicamentos, cura na base e cargas parciais ficam fora deste bloco.

## Verificações
- Cura40/max100 após4 segundos; sem efeito antecipado; vida cheia/tipo errado/slot vazio recusados sem consumo.
- Cancelamentos por H, movimento/pulo, dano, extração, morte, descarte/troca do item e encerramento conservam item quando aplicável; nenhum timer cura mortos.
- Tiro/recarga bloqueados durante uso e liberados ao encerrar, sem acionamento automático.
- Servidor rejeita GUID alheio, item consumido, replay, uso simultâneo e substituição no slot; origem no safe funciona sem recriar kit posteriormente.
- Falha de diário, callbacks durante conclusão e perda de resposta não duplicam vida nem itens.
- Backend valida consumed terminal, dono de origem, tipo, ausência de slot/dono final e imutabilidade; snapshots não podem omitir nem restaurar o registro.
- Dois clientes PIE com dados isolados: dano de bot, uso/cancelamento, vida replicada, uma instância consumida, extração/morte e reinício sem devolução do kit.
- Regressões de bots, combate, inventário, extração e persistência continuam aprovadas. Atualização preserva perfis reais e operações anteriores.

## Entrega e restrições
Somente phinasdt/Marau6 e conexão phinasdt. Não alterar recursos do Docker sdd-task-analyzer-docker-test ou configurações globais. Implementar/testar em branch própria a partir da main do PR#5, sem instalar dependências externas desnecessárias. Compilar, obter revisão independente e entregar roteiro curto ao usuário. PR/merge do novo bloco exigem autorização específica posterior.

## Estado
Regras e controles aprovados explicitamente pelo usuário em31/10/2026. Implementação e verificações automáticas concluídas conforme docs/cura-suprimentos.md; validação manual confirmada pelo usuário, com autorização específica de PR e merge.