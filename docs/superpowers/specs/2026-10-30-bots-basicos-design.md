# Bots básicos com autoridade do servidor

## Objetivo e escopo aprovado
Tornar o ciclo atual de extração jogável contra inimigos: bots no mapa existente, patrulha com desvio de obstáculos, detecção por visão/distância, perseguição, disparos, vida/morte e loot simples. Dois jogadores devem observar os mesmos bots e resultados. O usuário autorizou iniciar esse bloco após o merge do PR #4; os detalhes abaixo são propostas do assistente para revisão antes da implementação.

## Abordagem
Recomendação: personagem e controlador de IA próprios em C++, com navegação padrão da Unreal e estados pequenos (patrulha, investigação, combate, morto) executados somente no servidor. Reutilizar assets humanoides/rifle já existentes e efeitos compatíveis com o protótipo. Blueprints ficam disponíveis para ajustar aparência e parâmetros.

O template já contém ShooterNPC/StateTree, mas usa CurrentHP, armas e projéteis de outro fluxo, separado do ledger e do combate autoritativo atual. Adaptá-lo exigiria conciliar dois modelos de vida e disparo. Uma IA tática completa acrescentaria decisões sem necessidade neste bloco. A implementação própria mantém as regras do Marau6 e evita depender do exemplo sem validar sua replicação.

## Comportamento proposto pelo assistente
- Três bots por rodada, em pontos identificados no mapa atual, fora da posição de entrada e da área de extração. Validar chão, navegação e espaço para a cápsula antes de criar cada bot. Falha de navegação nunca permite atravessar obstáculos ou teleportar até o jogador.
- Cada bot patrulha pontos próximos alcançáveis pela malha de navegação. Ao falhar um caminho, tenta outro destino com intervalo; não recalcula caminhos a cada quadro.
- Percepção inicial: até 20 metros, cone frontal de 120 graus e linha de visão desobstruída. Som, visão através de paredes e conhecimento automático dos jogadores ficam fora do bloco.
- Alvo: jogador vivo e participante ativo da arena, nunca alguém na base, extraído ou eliminado. Selecionar o alvo visível mais próximo; manter o atual enquanto válido para evitar alternância contínua. Bots não atacam outros bots.
- Ao perder a visão, investigar apenas a última posição realmente vista por até 5 segundos, depois voltar à patrulha. Não usar a posição oculta atual do jogador para mirar ou perseguir.
- 100 de vida por bot. Disparo semiautomático de 10 de dano, intervalo mínimo de 0,8 segundo e reação inicial de 1 segundo, alcance de 20 metros e dispersão de 3 graus. Estes números são valores iniciais ajustáveis propostos, não balanceamento definitivo.
- Antes de cada disparo, o servidor verifica estado, reação, cadência, alcance, linha de visão e trajetória do tiro. Um obstáculo ou outro personagem intercepta o tiro; não aplicar dano diretamente ao alvo selecionado ignorando o mundo. Projéteis balísticos não são necessários: usar hitscan, como o rifle atual.
- Para limitar o bloco, o rifle do bot é representação visual: não é uma instância transferível do inventário. O disparo do bot não usa a munição persistente dos jogadores; carregadores, recarga e arma saqueável dos bots ficam para evolução posterior. Não afirmar munição/recarga realista neste marco.
- Ao morrer: parar percepção, movimento e disparos, replicar o estado morto e remover o corpo após um intervalo curto. Sem ragdoll ou física de loot neste bloco.

## Combate, replicação e itens
O disparo atual dos jogadores só reconhece AMarau6Character. A implementação deverá permitir atingir os novos bots sem enfraquecer a validação atual de tiros e sem introduzir dano informado pelo cliente. Bots terão um único estado de vida, independente do inventário/base/perfis de jogadores; não criar participações persistentes para IA.

Servidor cria, movimenta, escolhe alvos, atira, aplica dano e resolve morte. Clientes recebem personagem, movimento, vida/estado necessário e efeitos cosméticos. Não executar IA nem conceder loot no cliente. Manter o movimento e os efeitos funcionando em servidor dedicado de PIE.

Cada bot concede uma instância de Peças ao morrer. Sem loot de rifle ou inventário completo de cadáver nesta etapa. Criar o item exclusivamente pelo ledger autoritativo, com GUID único e guarda contra morte repetida. A coleta usa as mesmas validações existentes de distância, mira, capacidade e custódia. A persistência mantém os itens efetivamente coletados/extraídos pelas regras atuais.

Se a gravação durável estiver indisponível, não reconhecer coleta/extração ou uma concessão duplicada: usar o bloqueio/recuperação do ledger e registrar a concessão com identidade estável. Se for necessário acrescentar uma API de concessão, o planejamento deverá definir rollback e repetição antes de escrever o código. Não recriar loot após reinício como compensação automática.

## Rodadas e disponibilidade
Os bots pertencem à rodada corrente. Podem patrulhar sem jogadores, mas só atacam participantes ativos e enquanto a ponte persistente permite ações de jogo. Não bloquear encerramento de rodada esperando bots morrerem: continuam valendo os dois resultados dos perfis.

Na nova rodada confirmada pelo backend, remover bots/controladores/corpos antigos e criar os três novos, uma única vez. Loot remanescente segue ResetRound já existente; UUIDs novos para a nova rodada. Fechar o PIE deve cancelar timers e referências sem acessar subsistemas já destruídos. Interrupção de servidor encerra a arena conforme a política atual de abandono; não persistir posição/vida de bots.

## Navegação e mapa
Usar a arena existente, preparar cobertura de NavMesh e posições de patrulha/spawn para tamanho real das cápsulas. Preservar pontos de loot, entrada dos jogadores e extração. Sem novo mapa ou assets externos. Validar caminhos reais contornando obstáculos, e não apenas movimento em linha reta em fixtures vazias. Detalhes de pontos e organização de arquivos são decisões de implementação, sujeitos à verificação de espaço/navegação.

## Fora do escopo
Facções, esquadrões, cobertura tática, audição, granadas, dificuldades múltiplas, animações elaboradas, ragdoll, física de caixas (FUT-001), inventário completo de bots, arma saqueável, munição/recarga de bots, autenticação de produção e alterações na política de extração/persistência.

## Verificações exigidas
- Regressões de combate, inventário, extração e persistência continuam aprovadas.
- Percepção: alcance/cone/parede; jogadores na base, mortos e extraídos não são alvos. Perda de visão não revela posição atual oculta.
- Combate: reação, cadência e dano somente no servidor; parede/ator intercepta tiro; bot morto não ataca; jogador pode matar bot com o rifle existente.
- Loot: morte repetida não duplica GUID, disputa entre jogadores tem um vencedor, falha durável não cria concessão extra, extração/reinício conserva uma única instância.
- Navegação real: bot nasce fora de geometria e alcança destino contornando obstáculo; sem malha/caminho não atravessa paredes.
- Dois clientes PIE: mesmos bots e movimentos, efeitos dos disparos e morte observáveis nos dois clientes, dano coerente e encerramento sem crash.
- Nova rodada recria três bots uma única vez, não repete loot da anterior nem muda kits persistidos.
- Dados de testes isolados. Nunca alterar o projeto Docker sdd-task-analyzer-docker-test ou seus recursos. Apenas repositório phinasdt/Marau6, conexão phinasdt.

## Entrega
Implementar em branch própria a partir da main do PR #4, mantendo o projeto local de teste utilizável. Compilar, executar testes e obter revisão independente antes de pedir teste manual. Não publicar PR/merge deste novo bloco sem autorização específica posterior. Ao usuário serão entregues controles/roteiro curto, resultado das verificações e limitações reais.
## Aprovação e validação
Desenho aprovado explicitamente, implementação e verificações automáticas concluídas. O usuário confirmou o teste manual e autorizou PR e merge do bloco de bots.
