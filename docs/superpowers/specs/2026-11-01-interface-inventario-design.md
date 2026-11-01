# Interface do inventário — desenho aprovado

## Intenção e decisões do usuário
Melhorar o uso do inventário e da stash: seleção por mouse, arrastar e soltar, informações dos itens e mensagens claras para ações recusadas. O usuário escolheu arrastar e soltar em vez de clicar origem/destino e pediu uma prévia visual. As regras abaixo complementam essa escolha como propostas do assistente, aprovadas pelo usuário em 01/11/2026.

## Abordagem e escopo
Usar UMG/Slate nativos da Unreal, com widgets C++ e drag-and-drop, integrados aos RPCs/ledger e backend existentes. Aproveitar o HUD Canvas como painel clicável foi considerado, mas exigiria recriar foco, hit testing e arraste manualmente. Uma reconstrução artística completa, ícones finais ou inventário por dimensões fica fora deste bloco. O usuário mantém os slots simples existentes.

Mesma linguagem visual na base e na partida: arma equipada, mochila de 8 slots, safe pocket de 2 slots e painel do item selecionado. A base também mostra os 32 slots da stash, os perfis locais e as ações de entrar/nova rodada. Na partida a stash não aparece e não pode ser acessada por arraste. Não expor UUIDs, revisões, chaves ou erros técnicos na interface. A prévia é ilustrativa e local, sem conexão com os dados reais do jogo; nela os botões Base/Partida servem apenas para comparar essas duas telas.

## Arrastar, selecionar e cancelar
- Clique seleciona o item e exibe seus detalhes; slot vazio pode ser selecionado, mas não inicia arraste. Arraste inicia após o limiar de movimento do mouse e identifica o item original, seu slot e o contexto do inventário.
- Arrastar não retira o item do inventário nem altera o ledger. Um indicador acompanha o cursor e a origem fica identificada. Após o ajuste solicitado pelo usuário, a origem fica cinza, destinos compatíveis mantêm sua aparência normal e os incompatíveis recebem um X diagonal neutro sobre toda a caixa; sem amarelo, verde ou vermelho durante o arraste.
- Soltar em slot vazio compatível move. Soltar em slot ocupado troca os dois itens somente se ambos couberem nos destinos. Arma aceita apenas rifle; safe não aceita rifle. Preservar todas as regras já existentes.
- Soltar na origem ou fora de um slot cancela, sem descartar o item no mundo. Não implementar descarte por soltar fora da janela. TAB fecha o inventário da partida e cancela o arraste. Escape cancela o arraste; não substituir o encerramento padrão do PIE quando não houver um arraste em curso.
- Após soltar em destino válido, mostrar estado aguardando confirmação e impedir nova movimentação concorrente daquele inventário. Não anunciar sucesso nem retirar definitivamente o item da origem antes de confirmar o estado autoritativo. Rejeição conserva/atualiza os itens e explica o motivo. O widget deve continuar responsivo para fechar e consultar informações enquanto aguarda.
- Qualquer mudança que invalide o item/contexto do arraste — slot/identidade, perfil, base/partida, morte, extração ou fim de sessão — cancela o arraste. Uma resposta atrasada não pode aplicar uma operação a um novo perfil, personagem ou rodada.
- Usar GUID e estado esperado da origem para a solicitação; na base capturar perfil e revisão correspondentes. Servidor revalida identidade, origem, dono, slot, revisão/contexto e ocupação, inclusive em RPCs enviados diretamente. A comparação visual de compatibilidade no cliente é somente uma ajuda. Não permitir que um slot alterado entre início e fim do arraste mova outro item.
- Uma mudança de munição/condição pode atualizar a apresentação sem trocar o item arrastado; quando a operação exigir revisão atualizada, recusar claramente a solicitação obsoleta. Não criar cópia de item ou conceder itens para corrigir uma ação recusada.

## Mouse, teclado e estado da partida
TAB abre/fecha na partida; base aparece automaticamente. Com a interface aberta, mostrar cursor e direcionar os cliques para ela: não mover a câmera, disparar ou pular por acidente. Proposta do assistente: bloquear também os comandos voluntários de movimentação enquanto o inventário estiver aberto, sem parar gravidade, empurrões, dano, bots, extração ou tempo de cura. O jogo não pausa. Ao fechar, devolver foco/captura ao jogo; não disparar pela liberação do clique usado na interface.

Preservar setas/Enter para selecionar e mover, e os atalhos F/G/P/H já existentes nos contextos válidos. Seleção por mouse e teclado usa o mesmo item selecionado; H deve usar o suprimento selecionado pelo mouse. As ações existentes continuam sob as regras atuais, inclusive interrupção de recarga ao trocar arma e interrupção da cura ao mudar seu item de origem/equipamento. Não acrescentar cura na base, acesso remoto à stash ou funcionalidades de reparo/crafting.

Itens mantidos após morte/extração são somente leitura, até a transição confirmada para a base. Encerramento do PIE, troca de posse ou destruição do widget limpa foco/arraste sem acessar objetos já destruídos. Interface só existe para o jogador local; servidor dedicado não cria widgets.

## Informações e mensagens
Painel selecionado mostra nome, tipo e localização. Rifle: munição do carregador/reserva, durabilidade individual e aviso de bloqueio em 0%; explicar que recarga não repara. Suprimentos: até 40 de vida, uso em 4 segundos, uma utilização e condições/interrupções atuais. Peças: material coletável sem uso de crafting neste protótipo. Nenhum valor fictício de peso, raridade, preço ou bônus.

Mensagens visíveis junto do inventário distinguem pedido pendente, sucesso e recusa. Exemplos: “Rifles não cabem no safe pocket”, “O espaço de arma aceita somente rifles”, “Troca incompatível”, “O item mudou de posição; tente novamente”, “Aguarde a transferência”, “Mochila cheia”, “Inventário indisponível neste estado”, “Conexão indisponível; aguardando confirmação”. Só afirmar uma causa específica quando ela tiver sido validada; erro desconhecido recebe mensagem honesta e atualização do estado. Preservar o motivo real após atualizar a base, em vez de apagá-lo imediatamente com “Base pronta”.

Resultados das ações devem chegar ao jogador dono por canal autoritativo, com correlação suficiente para diferenciar solicitações e descartar respostas antigas. Detalhes técnicos dos tipos/identificadores de correlação ficam para a implementação. Reutilizar os contratos HTTP e recibos existentes quando possível; se um campo de comando precisar mudar, manter compatibilidade de dados e recibos anteriores, como no bloco de desgaste.

## Apresentação e verificação
Prévia: layout de três áreas na base e duas na partida, legível em janelas dos dois clientes; jogo deve ajustar escala/organização para não cortar slots nem informações. Usar rótulos, textos e espaços reservados simples antes de arte final. Não sobrepor o painel antigo do inventário ao novo. Vida/munição/cura/extração continuam disponíveis no HUD quando a janela estiver fechada.

Testar: drag/drop vazio e ocupado, swaps compatíveis/incompatíveis, safe/arma/stash, cancelar fora/origem/TAB/Escape, clique sem arrastar e limiar, teclado e seleção compartilhada, interação com cura/recarga/durabilidade zero, inventário fechado sem cursor ou bloqueio de input residual. Verificar morte/extração durante arraste, perfil alterado, item movido antes do drop, resposta atrasada, pedido duplicado, conexão indisponível e rejeição com motivo preservado. Nenhuma UI pode alterar os itens de outro jogador, duplicar GUIDs, reparar condição ou repor munição.

Automação unitária das regras/controladores e testes com dois clientes reais para replicação, pedidos concorrentes e transições. Fazer também verificação visual e teste manual do arraste em duas janelas, incluindo janela pequena; não tratar um teste sem renderização como prova de apresentação/foco. Rodar regressões de combate, inventário, bots, cura, desgaste e persistência.

## Entrega e restrições
Somente phinasdt/Marau6 e conexão phinasdt. Nunca tocar na conta Faculdade/eduardotrm ou nos seus repositórios e recursos Docker sdd-task-analyzer-docker-test. Preservar dados reais e configurações globais. Sem instalação de ferramentas prevista. Implementar em branch própria depois da aprovação, compilar, revisar e entregar roteiro de teste manual. PR/merge serão autorizados separadamente após a validação, conforme o fluxo já estabelecido.

## Estado
Desenho e comportamento de input aprovados pelo usuário em 01/11/2026. Implementação local concluída, compilada e verificada; roteiro e resultados em docs/interface-inventario.md. Validação manual do usuário e publicação pendentes.
