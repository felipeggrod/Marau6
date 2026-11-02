# Destruição controlada por seções — desenho

Data: 01/11/2026. Estado: desenho aprovado pelo usuário em01/11/2026; implementação autorizada.

## Intenção e escolhas confirmadas
O usuário escolheu destruição por seções independentes: é possível destruir a parede inteira destruindo todas as suas seções. Cada seção aceita qualquer dano válido aplicado pelo servidor, incluindo tiros do rifle comum. Paredes destrutíveis precisam de cor diferente das indestrutíveis. Explosões e materiais que exigem dano específico são ideias futuras, não funcionalidades deste bloco.

## Abordagens
Escolhida pelo usuário: painéis predefinidos com resistência e estado próprios. Alternativa considerada: parede inteira como uma peça; recusada por perder a abertura localizada. Fragmentação física livre não atende ao escopo inicial de seções predefinidas e acrescentaria custo de sincronização e navegação.

## Comportamento proposto pelo implementador
Arena de teste com três barreiras internas em locais que não bloqueiem nascimento, extração ou loot existente. Cada barreira terá três painéis verticais adjacentes, de aproximadamente120cm de largura e240cm de altura, com espessura20cm; destruir um painel abre passagem para a cápsula do jogador e do bot. Sem painéis superiores que fiquem suspensos e sem colapso estrutural automático. Posições exatas ficam a cargo da implementação, verificadas no mapa.
Cada painel começa com200 de resistência: oito tiros de25 do rifle. Dano inferior acumula; dano maior pode quebrar de uma vez. Só o painel atingido recebe dano neste bloco. A resistência nunca fica negativa. Dano inválido, negativo, não finito ou aplicado por cliente não altera o estado. Painel já quebrado não recebe novos efeitos de quebra.
Aparência proposta: azul com faixas claras e marcação discreta DESTRUTIVEL, distinta do cinza e amarelo existentes e do verde de extração. Mantém a identificação mesmo sem depender somente de cor. Os painéis mostram estágios simples de dano; ao quebrar, efeito breve de partículas/fragmentos cosméticos, sem colisão, dano ou loot. Sem física autoritativa de destroços.

## Servidor, tiros e replicação
Estado de cada painel (resistência/quebrado) pertence ao servidor e é replicado. Acertos usam o impacto e componente calculados pelo servidor, nunca seleção de painel ou valor de dano enviado pelo cliente. Pode-se representar cada seção por ator próprio para reutilizar TakeDamage e evitar ambiguidades de roteamento. O caminho de dano deve ser genérico, sem restringir a classe ou tipo de arma; tiros válidos de jogadores e bots podem atingir painéis. Não fazer bots atirarem deliberadamente em paredes neste bloco: eles continuam seu comportamento atual, podendo acertar uma parede por dispersão.
O tiro que quebra um painel termina naquele impacto; não atravessa e causa dano atrás no mesmo disparo. Disparos posteriores atravessam a abertura. Painel intacto bloqueia visão, tiro, pickup e passagem; quebrado perde colisão em todos esses usos. Estado atual é enviado também a cliente que conecta ou reconecta depois, sem repetir efeitos antigos de quebra.

## Bots e rodada
A navegação do servidor precisa refletir o bloqueio e a abertura, não apenas a colisão visual. Usar obstrução dinâmica sobre navegação existente ou geração dinâmica conforme avaliação local; testar rota real pela passagem. Bots não atravessam painéis intactos e podem usar o caminho depois de aberto, após atualização de navegação. Restaurar painéis e obstruções em cada nova rodada antes de liberar jogadores; reinicialização do mundo começa intacta. Não adicionar persistência de paredes ao PostgreSQL: cenário é temporário por rodada, diferente dos itens. Não alterar inventário, ledger ou recibos de itens.

## Limites e restrições
Somente repositório phinasdt/Marau6 e conexão phinasdt. Preservar dados atuais, itens e contas/projetos da faculdade. Não instalar dependências, não implementar servidor empacotado, explosivos, reparos, propagação de dano radial, materiais seletivos ou colapso físico. Sem publicar PR/merge antes de teste manual. Azul,200HP, número/dimensões das barreiras são propostas do assistente, ajustáveis na aprovação.

## Verificação e entrega
Testes de resistência acumulada e seção vizinha preservada; oito tiros e gasto normal de munição/desgaste; dano de outra origem autoritativa; rejeição de dano inválido; quebra única; colisão/visão e tiros através da abertura; passagem real de jogador e bot; replicação para dois clientes e estado tardio; restauração ao mudar rodada. Rodar regressões Unreal/backend e integração isolada, inspecionar aparência renderizada e entregar roteiro manual. Guardar as duas ideias futuras na lista existente. Um desenho visual pode ser mostrado antes da implementação se o usuário desejar.
