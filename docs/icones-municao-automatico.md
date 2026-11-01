# Ícones, rifle automático e caixas de munição

Implementação solicitada em 01/11/2026. Arte inicial vetorial nativa: rifle, caixa de munição, kit de suprimentos e peças. O mesmo desenho acompanha o cursor ao arrastar. Passe o mouse para ver o nome e selecione para ler os detalhes. Origem cinza e X em destinos incompatíveis permanecem.

Rifles novos começam com 20 balas no carregador e 40 de reserva. Segurar o botão esquerdo dispara a cada 0,25s; soltar interrompe. Cada tiro aceito perde exatamente 1 ponto percentual de durabilidade. Com 0%, não dispara. Morte, cura e inventário bloqueiam disparo; após fechar o inventário é preciso soltar e pressionar novamente. A recarga com R continua levando 2s, sem reparar a arma.

Caixas individuais de 20 balas aparecem perto dos rifles do mapa e reaparecem com novas identidades na próxima rodada. E coleta; arrastar a caixa sobre um rifle em qualquer slot abastece a reserva desse rifle. Funciona na base e durante a partida. A caixa inteira precisa caber: limite120 de reserva. Sem consumo parcial. Arrastar munição para slot vazio apenas move a caixa. A arma equipada não pode receber caixa durante recarga; cura também impede o consumo. Aumento da reserva não altera carregador nem durabilidade. Itens antigos conservam seus valores gravados, sem reposição nem reparo automático.

A gravação da caixa consumida e das20 balas ocorre em uma única operação durável. A identidade da caixa permanece no histórico para impedir reutilização. Falha de gravação preserva ambos. Pedidos repetidos não entregam balas novamente. O backend valida aumentos da reserva contra consumo correspondente de munição.

## Teste manual
1. Inicie com Iniciar-Marau6.cmd e abra2 clientes Play As Client.
2. Confira ícones na base, nome ao passar o mouse, descrição ao clicar e desenho acompanhando o cursor. Movimentação normal e X continuam funcionando.
3. Entre na partida e equipe um rifle novo do mapa para verificar20/40 e100%. Segure o botão esquerdo; confira disparos contínuos, diminuição de1% por tiro e parada ao soltar.
4. Colete uma caixa perto dos rifles. Abra TAB e arraste a caixa sobre o rifle: caixa desaparece e reserva aumenta20. Carregador e condição não mudam. R recarrega até20.
5. Arraste uma caixa para um slot vazio, depois para o safe. Ela continua como item, sem ser consumida. Também pode ser descartada e coletada pelo outro jogador.
6. Confira recusa durante recarga e com reserva acima100. A caixa deve permanecer intacta. Ao extrair, munição da arma e caixas não usadas devem voltar à base.
7. Para testar quebra, obtenha munição suficiente:100 tiros aceitos levam um rifle novo a0%. Novas caixas não reparam a arma.

## Verificação
Compilação Unreal aprovada.35 testes automatizados Unreal aprovados, incluindo consumo único,1%/tiro,100 disparos, rollback de caixa e restrições de entrada.30 contratos HTTP/PostgreSQL aprovados, mais consumo na base, limites e identidades históricas e replays após reinício. Integração renderizada com2 clientes: ícones e arraste real, input contínuo com4 tiros, parada ao soltar, consumo de caixa,2 rodadas e API reiniciada. Integração de desgaste:100 tiros, zero bloqueado com20 balas carregadas, transferência,2 rodadas e reinício. Ciclo e recuperação em novo processo aprovados. Revisão independente sem P1/P2. Backend principal atualizado; hash dos dados principais idêntico antes/depois. Arte inicial simples; validação manual do usuário ainda pendente. Sem PR/merge deste bloco.
