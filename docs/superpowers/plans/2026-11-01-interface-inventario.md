# Interface de inventário Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [x]`) syntax for tracking.

**Goal:** Interface aprovada com arrastar/soltar, detalhes, recusas claras e confirmação autoritativa na base e partida.
**Architecture:** UUserWidget C++ com composição Slate e slots com drag/drop; visualização local lê estado autoritativo. Controller faz correlação e contexto dos pedidos, usando ledger existente na partida e bridge/backend existente na base. Sem novos itens ou reset de dados.
**Tech Stack:** Unreal5.8.4 C++ UMG/Slate; ASP.NET/PostgreSQL atuais.
**Spec:** ../specs/2026-11-01-interface-inventario-design.md (aprovada pelo usuário em01/11/2026).

## Global Constraints
Somente phinasdt/Marau6 e conexão phinasdt. Nunca conta Faculdade/eduardotrm, seus repositórios ou recursos Docker sdd-task-analyzer-docker-test. Preservar GUID, munição, condição, perfis, diários e recibos. Sem instalações/configurações globais. Execução nativa no checkout E:/VisualStudio/Marau6 em branch própria, preservando o caminho de teste já usado pelo usuário; PR/merge somente com autorização posterior.

## Review Focus
- Resposta RPC anterior à replicação: só concluir o pedido após revisão do estado alcançada.
- Perfil/rodada/posse alterados durante arraste/pedido: token de contexto invalida pedidos antigos.
- Origem e destino mudaram: validar GUIDs e slots esperados, não apenas índices.
- Destruição da UI/fim PIE: cancelar só o arraste próprio e devolver input sem acessar UObjects destruídos.
- Recusa HTTP seguida por Refresh: preservar motivo e não substituir por Base pronta.

### Task1: Contrato de interação e confirmações
Files: criar Source/Marau6/Marau6InventoryUIModel.{h,cpp}, Marau6InventoryUIController.cpp; modificar Marau6PlayerController.{h,cpp}, Marau6InventoryTypes.h, Marau6InventoryComponent.cpp, Marau6Persistence.{h,cpp}; testes Source/Marau6/Tests/Marau6InventoryTests.cpp.
Interfaces: FMarau6InventoryUIRequest USTRUCT com RequestId,Context,Item,DestinationItem,Source,Destination,Profile,Revision. FMarau6InventoryUIModel::MoveReason(Items,Source,Destination,bBase) retorna motivo vazio se válido; Details(Item) e SlotName(Slot). Controller GetUIItems/IsInventoryVisible/IsInventoryAtBase/IsInventoryReadOnly, CaptureInventoryDrag(Source,OutRequest), SubmitInventoryDrop(Request,Destination), ServerInventoryMoveUI(Request), ClientInventoryResult(RequestId,Context,bSuccess,Reason,Revision). Snapshot.Revision int32 incrementa a cada ApplySnapshot; não participa dos snapshots HTTP.
- [x] Teste reflexivo do novo RPC confiável e existência do widget; executar e observar falha por funcionalidade ausente.
- [x] Implementar modelo de compatibilidade/informação e contexto por controller, mudanças de perfil/modo/posse/rodada/estado final invalidam arraste.
- [x] Servidor valida origem/destino/identidades/contexto/revisão. Cache limitado de recibos de UI evita aplicar pedido duplicado. Um pedido pendente por controller; base usa bBaseBusy e callback durável; partida chama ledger sem alterar regras.
- [x] Cliente aguarda confirmação e revisão replicada antes de liberar nova transferência; não promete sucesso em estado antigo. Bridge mantém erro de movimento após atualizar perfil e responde callback sem alterar wire format dos comandos antigos.
- [x] Testes de movimento, swap, safe, slot inválido, estados finais, duplicação, revisão obsoleta, contexto e confirmação anterior à replicação.

### Task2: Janela UMG/Slate, mouse e foco
Files: criar Marau6InventoryWidget.{h,cpp}, Marau6InventorySlot.{h,cpp}; modificar Marau6PlayerController.cpp, Marau6Character.{h,cpp}, Marau6PrototypeHUD.cpp.
Interfaces: UMarau6InventoryWidget::RebuildWidget/Refresh/NativeOnPreviewKeyDown; FMarau6InventoryDragOperation e SMarau6InventorySlot usam Controller.CaptureInventoryDrag/SubmitInventoryDrop. Controller.UpdateInventoryUI, HandleInventoryKey, SelectInventorySlot, CancelInventoryDrag.
- [x] Testar bloqueio de movimento/câmera/pulo, seleção compartilhada e abrir/fechar sem cursor/foco residual; observar falha antes das mudanças.
- [x] Criar layout com arma/mochila8/safe2/stash32, painel de detalhes e mensagem; mostrar stash só na base e resultado como somente leitura. Slots usam rótulos e contraste, foco/seleção, destinos válidos/inválidos e indicador de arraste.
- [x] Arraste não altera itens; fora/origem/TAB/Escape cancela. Não remover/reconstruir widgets durante arraste. Com interface aberta bloquear input voluntário de movimento/câmera/tiro/pulo sem pausar mundo; fechamento devolve foco sem disparo.
- [x] Manter setas/Enter e F/G/P/H nos contextos válidos. Seleção de mouse atualiza o mesmo slot usado pela cura. Mostrar controles de perfil/entrada/rodada na base. Canvas antigo não duplica UI nova; HUD de combate mantido.
- [x] Testar ciclo de vida, cancelar após morte/extração/perfil, clique sem arrastar, partes de UI e revisão de dados. Compilar e suíte Marau6 verde.

### Task3: Integração, revisão e entrega
Files: criar testes PIE/UI e roteiro docs/interface-inventario.md; estender scripts/Test-Marau6Integration.ps1 com modo UI isolado.
- [x] Dois clientes reais: mover base/stash, entrar, mover partida, recusar origem/destino obsoletos e safe, resultado/read-only e retorno à base. Preservar identidades/munição/condição após APIrestart. Confirmar recebimento de mensagens corretas.
- [x] Verificação com renderização: janela normal e pequena, drag/drop via Slate input, cancelamento e restauração de foco; guardar evidências. Testes NullRHI não provam apresentação.
- [x] Rodar regressões de backend e Unreal; revisão independente da branch e correção dos achados materiais com testes.
- [x] Entregar implementação local pronta para teste manual; não publicar. Documentar resultados e limitações reais.

## Rulings de execução
- Preservar o fluxo nativo de implementação e teste pelo usuário, já autorizado neste projeto, sem novo pedido de confirmação sobre o plano.
- Usar UUserWidget com Slate nativo para composição e drag/drop, sem arte binária nova; continua UMG/Slate previsto no desenho.
- Não alterar esquemas SQL nem formato HTTP de comandos; contexto e recibos da interface pertencem ao servidor da partida/controller.

## Resultado da execução
Implementação nativa concluída na branch feat/inventory-interface. 32 testes Unreal e 28 contratos backend aprovados; dois clientes reais, arraste Slate com renderização, captura/foco, janela pequena, mensagens, duas rodadas e reinício da API aprovados. Revisão independente identificou aviso antigo encobrindo recusa; correção reproduzida por teste RED e revisada. Contexto do servidor dedicado atualizado em TickActor; PlayerTick remoto não executava. Fontes mantêm tamanho físico e áreas se reorganizam/rolam em janela pequena. Sem alteração do backend ou dados reais. Validação manual e publicação pendentes.
