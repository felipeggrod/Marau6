# Destruição controlada por seções

Desenho aprovado pelo usuário em01/11/2026. Três barreiras internas, com três painéis verticais independentes por barreira. Dimensão aproximada de cada painel:120x240cm,20cm de espessura.

## Regras
- Painéis azuis, com faixas claras e identificação DESTRUTIVEL, contrastam com a arena cinza/amarela.
- Cada seção começa com200 de resistência. O rifle atual causa25: oito tiros atingindo a mesma seção abrem a passagem.
- Dano acumula por seção; vizinhas não recebem dano. Qualquer dano positivo finito aplicado pelo servidor é aceito. Só o servidor altera resistência, quebra e restauração.
- Rachaduras simples indicam dano. A quebra produz fragmentos cosméticos breves, sem colisão, navegação, dano ou loot.
- O disparo que quebra o painel termina naquele impacto. Disparos posteriores atravessam a abertura. Jogadores e bots também podem passar.
- Painéis intactos bloqueiam tiro, visão, coleta e passagem. O servidor atualiza a navegação quando um painel abre ou é restaurado.
- Um cliente que entra depois recebe o estado atual. A quebra antiga não é reproduzida como evento novo.
- Nova rodada restaura as seções. A liberação da rodada aguarda a atualização de navegação. Reiniciar o mundo começa intacto; paredes não são persistidas no PostgreSQL.
- Destruir todas as três seções remove uma barreira inteira. Não há colapso estrutural ou painel superior suspenso.

As barreiras são criadas em tempo de execução sobre áreas navegáveis livres, sem substituir o mapa original, mantendo pontos de entrada, extração e loot acessíveis. As coordenadas são escolhidas de forma determinística entre posições candidatas e conferidas por colisão.

## Teste manual
1. Inicie o Marau6 normalmente e abra2 clientes Play As Client. Entre na partida com os dois perfis.
2. Procure as três barreiras azuis com faixas claras. Compare com paredes comuns que continuam indestrutíveis.
3. Atire4 vezes no mesmo painel: ele deve mostrar dano, continuar bloqueando e permanecer igual nas duas janelas. As seções ao lado ficam intactas.
4. Complete8 acertos nesse painel. A seção desaparece, deixando uma passagem; as vizinhas continuam em pé.
5. Atravesse a abertura, observe o outro jogador passar e atire através dela. Confira que não há destroços bloqueando o chão.
6. Destrua as seções restantes para remover a barreira inteira.
7. Termine a rodada com os dois jogadores e abra a próxima: todos os painéis devem voltar intactos, bloqueando a passagem e os caminhos dos bots.

Explosões e materiais que exigem tipos específicos de dano estão registrados como FUT-003 e FUT-004 em melhorias-futuras.md; não foram implementados neste bloco.

## Verificação
Compilação aprovada e38 testes Unreal aprovados, incluindo dano de seção, tiros/munição/desgaste, passagem, recusa de dano inválido e por cliente, e restauração.30 contratos backend e replays após reinício aprovados em esquema isolado. Integração renderizada confirmou8 tiros na seção correta, vizinhas intactas, bloqueio do tiro de quebra, remoção da colisão, passagem real de jogador, cliente tardio com estado correto, caminho inicialmente contornando a parede e bot atravessando a abertura. A nova rodada restaura os nove painéis e o desvio de navegação antes de liberar entrada. Revisão independente apontou a necessidade dessa espera, corrigida com verificação específica. Aparência intacta/danificada/quebrada inspecionada em capturas reais.

Decisão de implementação: a geração de navegação é Dynamic, reconstruindo a geometria dos trechos afetados, com modificadores de bloqueio por painel. O modo inicialmente avaliado DynamicModifiersOnly perdeu caminhos ao atualizar a navegação pré-existente; o teste real identificou o problema e a alternativa dinâmica prevista no desenho foi adotada. Acertos que apenas diminuem resistência não solicitam reconstrução.

Verificação final renderizada aprovada, incluindo cliente tardio sem repetir efeitos antigos. Ciclo Unreal/API/PostgreSQL e recuperação em novo processo aprovados. Validação manual confirmada pelo usuário em01/11/2026; PR e merge autorizados na sequência.
