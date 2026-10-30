# Base, stash e persistência — desenho aprovado

## Objetivo e decisões já aprovadas

Fechar o ciclo preparar equipamento → entrar na partida → morrer/extrair → retornar à base, conservando os dados após encerrar e reabrir o jogo. Manter UE5 C++/Blueprints, servidor autoritativo, backend C# ASP.NET Core modular e PostgreSQL. O usuário escolheu dois perfis locais de teste, sem cadastro ou senha nesta etapa. O bloco de inventário anterior está integrado à main pelo PR #3.

## Abordagens consideradas

1. Perfis locais e backend PostgreSQL: escolhida pelo usuário; permite testar custódia persistente antes de autenticação real.
2. Cadastro e login completos: adiados; ampliariam o escopo sem serem necessários ao teste local.
3. Arquivo de save no cliente: descartado como fonte de verdade, pois não cumpre a autoridade e a custódia central previstas.

## Proposta de experiência e limites

Base em menu simples na Unreal, sem novo cenário. Dois perfis de desenvolvimento, Teste 1 e Teste 2, com identificadores estáveis selecionáveis. Um perfil só pode ter uma sessão ativa. A seleção não é autenticação de produção e só fica disponível no ambiente local de desenvolvimento.

Proposta do assistente: stash de 32 slots individuais, mochila de 8, safe de 2 e arma equipada de 1. Sem agrupamento, peso, compra, crafting, cura, desgaste ativo, física de loot ou Steam. Transferências na base preservam GUID e munição e são confirmadas pelo backend antes de aparecer como concluídas. Oferecer seleção de slots e troca entre stash e equipamento; compatibilidade do safe e da arma continua igual.

Cada perfil recebe uma única concessão inicial de itens de teste ao ser criado, persistida na mesma transação do perfil. Não gerar novo rifle a cada entrada nem repor itens automaticamente após morte. Uma ferramenta explícita de desenvolvimento pode restaurar somente os perfis Marau6, mediante ação deliberada e confirmação; nunca na inicialização normal.

## Custódia e transações

Backend em .NET 10; uma aplicação com módulos de perfis, inventário e partidas. PostgreSQL registra perfis, instâncias de itens, localização/slot, participação em partida e recibos das operações. Constraints impedem GUID duplicado e duas ocupações do mesmo slot válido. Transações serializam mudanças por perfil/participação. Comandos têm identificadores idempotentes: repetir uma requisição após timeout retorna seu resultado anterior; reutilizar a mesma chave com conteúdo diferente é rejeitado. Revisões rejeitam alterações feitas sobre estado antigo.

Ao entrar, o backend bloqueia o equipamento na participação da partida, sem deixá-lo disponível na base. Só após confirmação o servidor importa os itens no ledger da Unreal; ele deixa de criar o rifle inicial nesse fluxo. Stash permanece separado da custódia da partida. Erros antes da confirmação impedem a entrada; depois de timeout, consultar a operação em vez de iniciar outra reserva.

O servidor de jogo é o único que pode enviar resultados e transferências da partida. O cliente solicita ações por RPCs já validados, nunca envia uma lista de itens a salvar. Segredo de desenvolvimento existe somente no servidor/backend, fora do Git e da replicação. HTTP, banco e seleção de perfis limitados ao computador local; portas publicadas somente em 127.0.0.1. Cliente não acessa PostgreSQL.

O ledger continua responsável pelos itens durante a partida, inclusive trocas de dono e loot do mapa. Cada execução da arena tem RaidId próprio; loot semeado ganha IDs associados a essa execução. Backend aceita saídas exclusivamente de participantes e identidades dessa partida, reconciliando transferências entre jogadores: um item perdido por A e extraído por B passa a B uma vez, independentemente da ordem de envio dos resultados. Um resultado de morte não apaga uma instância que outro participante já extraiu. Itens em uso na arena não voltam ao stash por uma falha de chamada HTTP.

Morte mantém safe e deixa mochila/arma no mundo; extração mantém todos. Itens mantidos retornam aos slots de equipamento da base, sem depender de espaço no stash. O resultado só é mostrado como salvo quando houver recibo do backend. Se o backend cair, mostrar pendência e bloquear nova entrada, preservando o resultado para nova tentativa.

Proposta do assistente para interrupção: desconexão individual encerra a participação como morte (safe conservado). Para recuperar de encerramento abrupto do servidor/editor, usar registro durável no servidor dos comandos/resultados pendentes, com IDs estáveis e escrita atômica. Na retomada, reenviar os resultados já registrados; participações abertas sem resultado confirmado são encerradas como abandono, mantendo apenas o safe registrado. Uma concessão de extração não registrada duravelmente não pode ser prometida após crash. Não devolver automaticamente toda a mochila de participações interrompidas. Essa política exige aprovação do usuário.

### Esclarecimentos após revisão

O safe conservado após interrupção é o último estado confirmado duravelmente, não a cópia da entrada. O servidor mantém diário ordenado por RaidId de mudanças de custódia, slot e safe, incluindo origem, destino, GUID, sequência e identificador da operação. Uma mudança de custódia só é confirmada ao cliente após gravação durável; falha de gravação rejeita a operação e conserva a origem. Eventos duplicados são idempotentes, e eventos antigos não podem desfazer estados posteriores. Recuperação reproduz o diário antes de encerrar participações abertas como abandono. Um item que saiu do safe não reaparece nele por recuperação; itens ainda na mochila ou no mundo ao encerrar uma arena interrompida são perdidos, sem concessão a antigos donos. O diário local não é um save editável pelo cliente: pertence ao processo de servidor e fica fora do versionamento.

Proposta de ciclo para este bloco: uma entrada por perfil por RaidId. Quem morre ou extrai pode organizar a base, mas aguarda o fim da rodada para reentrar. A rodada termina quando os dois perfis encerram suas participações, ou por encerramento explícito da sessão de desenvolvimento, que aplica a política de abandono. Perfil que ainda não entrou não é encerrado só porque o outro terminou. Nova rodada recebe novo RaidId e restaura os 14 pontos de loot com novas identidades, sem recriar itens de equipamento dos perfis. Não acrescentar duração máxima de partida neste bloco.

## Integração e funcionamento local

Desenvolver e testar no mesmo mapa de arena com dois clientes PIE e servidor do editor. Cada cliente começa na base/menu; preparação e entrada são individuais. Extrair ou morrer retorna ao menu após confirmação do resultado, sem reiniciar a partida do outro jogador. Encerrada uma execução da arena, a próxima usa outro RaidId. Reiniciar Play não concede equipamento nem elimina dados persistidos.

Preservar testes existentes por fixtures explícitas para o modo de combate isolado; esse modo não participa de perfis persistentes. Fluxo normal do protótipo exige backend e informa indisponibilidade com clareza, sem cair silenciosamente em inventário temporário.

Docker Compose exclusivo com nome de projeto marau6 e volume, rede e container próprios. Conferir conflitos de nomes/portas antes da criação e escolher portas locais livres. NÃO alterar, iniciar, parar, remover ou recriar sdd-task-analyzer-docker-test nem seus containers, volumes, imagens, redes ou configurações. NÃO usar limpeza global, prune ou reset do Docker. Scripts sempre apontam para o Compose e projeto específicos do Marau6. Não alterar configurações globais do Docker/WSL. Recursos existentes da faculdade não serão reutilizados.

## Validação necessária

- Testes de backend contra PostgreSQL real: concessão inicial única, slots/compatibilidade, reserva, concorrência, idempotência, revisões antigas, morte, extração, abandono, resultados fora de ordem e item transferido entre jogadores.
- Testar perda de resposta após commit, indisponibilidade e reinício de backend/servidor, reenvio de comandos e recuperação do registro durável.
- Compilar Unreal e rodar regressões de combate, extração e inventário; acrescentar cobertura da ponte com backend e bloqueios de estado.
- Teste integrado com dois clientes: preparação, transferência do mesmo rifle com munição, morte/safe, extração, retorno individual à base e persistência após fechar e reabrir.
- Verificar que nenhuma credencial entrou no Git e que scripts operam somente recursos marau6.

## Entrega e decisões deixadas ao implementador

Escolha de controles provisórios, organização interna dos arquivos, biblioteca PostgreSQL, portas locais livres e detalhes da UI ficam a cargo do implementador. Documentar instalação, início/parada apenas do Marau6, limitações e roteiro de teste. Testes e revisão precedem a validação manual. Push/PR/merge deste novo bloco ficam para autorização específica ao final.

## Aprovação e execução

Desenho aprovado explicitamente pelo usuário em 30/10/2026, incluindo stash de32 slots e a política de interrupção. A implementação local e seus testes foram concluídos; a validação manual do usuário com dois clientes permanece pendente.
