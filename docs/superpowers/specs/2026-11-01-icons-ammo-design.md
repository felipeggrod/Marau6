# Ícones e rifle automático com munição
Pedido de execução autônoma do usuário em 01/11/2026, baseado na proposta apresentada. Não requer nova pausa de aprovação.

## Escopo
Ícones vetoriais nativos para rifle, suprimentos, peças e munição, com a mesma figura no arraste; nome ao passar o mouse e detalhes ao selecionar. Conservar a organização, cores e X de destinos incompatíveis.
Rifle automático ao segurar o botão, cadência atual de 0,25s validada pelo servidor, carregador 20, reserva inicial 40, desgaste de 100 unidades (1%) por disparo aceito. Zero impede disparar. Rifles existentes não são reabastecidos nem reparados.
Caixa individual de munição (kind=3) contém sempre 20 balas, aparece no mapa a cada rodada e pode ser extraída/guardada. Arrastar caixa para rifle em qualquer slot consome a identidade e acrescenta 20 à reserva, na base ou partida. Limite proposto pelo implementador: 120 de reserva; sem consumo parcial. Operação rejeitada se não couber inteira, estado inválido ou rifle equipado recarregando/em tratamento. Sem troca de slots nesse gesto específico. Outros gestos mantêm troca normal.

## Abordagem
Ícones desenhados com Slate: sem dependências ou arte externa, escaláveis, substituíveis por arte futura. Alternativa texturas adicionaria arquivos/importações desnecessários neste marco. Automático usa input contínuo com limitação local e validação de cada disparo no servidor; sem fila de tiros após soltar. Consumo de munição usa o protocolo correlacionado existente de movimentação e gravação atômica no ledger/backend; identidades consumidas permanecem registradas.
## Restrições
Somente phinasdt/Marau6; preservar perfis e banco principal. Não tocar projeto Docker da faculdade. Sem servidor empacotado, destruição, reparos, mercado ou crafting. Sem PR/merge antes da validação manual.
## Verificação
Testes de capacidade, 1%/tiro, disparo contínuo, interrupções, consumo único, repetição, falha de gravação, limites, transferência, extração, reinício e inventário visual em dois clientes. Documentar valores antigos substituídos e roteiro manual.
