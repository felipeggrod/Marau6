# Inventário e loot por slots

## Comportamento

O servidor cria uma instância única de rifle por jogador e as instâncias de loot do mapa. Cada item tem GUID próprio. Inventário: mochila8, safe pocket2 e arma1, sem empilhamento. Rifles mantêm seu carregador/reserva ao equipar, trocar, descartar, coletar ou mudar de dono; trocar/descartar a arma cancela recarga pendente.

Tipos provisórios: rifle equipado do template, suprimentos e peças. Suprimentos/peças são loot, sem efeito de consumo nesta etapa. Somente esses itens pequenos podem ocupar o safe pocket. Quatorze atores de loot estão em trechos livres da arena, principalmente nas laterais. A arte e as dimensões são provisórias.

Controles: E coleta sob a mira, TAB abre/fecha a lista, setas selecionam, F equipa rifle, G descarta e P move entre mochila e safe. O inventário pode ser consultado em movimento; feche-o para disparar. Sem arma, tiro e recarga ficam bloqueados.

Morte finaliza a custódia do jogador, disponibilizando mochila e rifle como loot e preservando o safe pocket no resultado. Extração preserva todos os itens no resultado, indisponíveis para transferências posteriores. A tela mostra itens mantidos/perdidos. Remover o personagem não apaga esse resultado na sessão. Desconexão que destrói um personagem ainda ativo segue a finalização por morte.

## Custódia e transferências

UMarau6ItemLedger, um UTickableWorldSubsystem, é a fonte autoritativa da identidade e da custódia: Inventory, World, Result, Recovery ou Transfer. Snapshot replicado não é outra propriedade do item: é uma vista dos registros do servidor. UMarau6InventoryComponent envia a lista somente ao dono; o GUID da arma equipada replica aos demais para atualizar a apresentação.

RPCs do inventário recebem referência/identidade esperada e destino, nunca dados de item ou munição fornecidos pelo cliente. Servidor valida posse, estado, capacidade, compatibilidade do slot, alcance de2,5m, linha de visão e alvo sob a mira. Coletas concorrentes são serializadas no game thread; somente a primeira transferência válida consome o ator disponível.

Antes de criar um ator de loot, o ledger reserva a identidade, o slot e o inventário de origem. Callbacks de criação não podem repetir a transferência nem ocupar o slot reservado. Código não mantém referências/iteradores de TMap através desses callbacks: usa cópias e volta a consultar pelo GUID. A identidade é atribuída ao ator antes dos callbacks de spawn. Finalização percorre uma lista estável de identidades. Falha na criação restaura o estado de origem ou mantém custódia de resultado/recuperação conforme o estado do jogador.

Recovery preserva dados quando um ator desaparece inesperadamente ou falha a disponibilização do loot. O servidor tenta recriar o ator a cada segundo mantendo a mesma identidade, sem depender da existência do personagem. O encerramento da sessão remove todo esse estado transitório; não existe persistência em disco/backend neste bloco.

## Verificação

Development Editor / Win64 compilado. Treze testes Marau6 aprovados: sete regressões de combate/extração e seis casos de inventário (Transfers, WeaponsAndResults, RecoveryAndUnarmed, ReentrantDrop, FinalizationCallbacks e RollbackReservation).

Casos incluem identidade/capacidade, coleta repetida/segundo jogador, item fora de alcance/atrás de parede/fora da mira, seleção antiga, munição após troca de dono, recarga pendente, safe pocket, morte/extração e preservação após Destroy + coleta de lixo. Callbacks que realocam o ledger e tentam transferir de novo reproduziram falhas antes das correções; callbacks de falha de spawn também exercitam rollback e ocupação de slots.

Prévia local conferida com os controles reais: E coletou suprimentos, P moveu para safe, setas selecionaram arma, G descartou e E recuperou o rifle. Nenhum estado/configuração dessa prévia foi salvo.

Esses testes usam um mundo autoritativo local. O usuário confirmou o funcionamento do teste integrado entre dois clientes em 29/10/2026, com a ressalva de empilhamento descrita abaixo. A validação não constitui uma garantia geral contra todos os exploits.

## Teste integrado

1. Abra o mapa com dois clientes, como nos blocos anteriores. Use E mirando nos itens da arena e TAB para acompanhar o inventário. Tente pegar o mesmo item com os dois jogadores: deve aparecer somente em um inventário.
2. Proteja um suprimento com P e mantenha outro na mochila. Atire algumas vezes, descarte o rifle e faça o outro jogador coletá-lo/equipá-lo. Confira os mesmos valores de munição, sem recarga gratuita. Troque de rifle durante uma recarga e confirme seu cancelamento.
3. Elimine um jogador carregando itens: o outro pode pegar mochila/arma, enquanto o eliminado vê somente o safe pocket entre os itens mantidos. Reinicie o Play.
4. Colete itens e extraia: a lista de resultado mantém todos eles e nenhuma transferência é permitida após extrair. Confira que o outro jogador continua.
5. Tente coletar com mochila cheia ou através de parede: o loot deve permanecer disponível, sem sumir nem aparecer em dois lugares.

O comando exclusivo do editor Marau6.PreviewInventory prepara uma prévia local de interação, sem alterar o mapa. O stash, a base e a gravação entre partidas são o próximo bloco. Desgaste de equipamentos continua fora da mecânica ativa.

## Validação do usuário e limitação conhecida

Em 29/10/2026, o usuário confirmou o funcionamento do bloco no teste integrado. Relatou que descartes consecutivos podem se empilhar, dificultando a coleta do item inferior; ao coletar o apoio, itens superiores podem permanecer flutuando. Por decisão explícita do usuário, o comportamento atual foi mantido. A possível adoção de física e gravidade foi registrada como melhoria futura FUT-001 em [melhorias-futuras.md](melhorias-futuras.md), sem aprovação de implementação.
