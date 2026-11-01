# Equipamentos e desgaste do rifle

Rifle novo: 100% de durabilidade,10 balas no carregador e 40 na reserva. Cada disparo aceito pelo servidor custa 2,5 pontos percentuais. O tiro 40 ainda causa dano e deixa a condição em 0%; a partir daí, o servidor recusa disparos mesmo com munição. Recarregar não conserta a arma. Sem reparos neste marco.

A condição pertence ao UUID do rifle: trocar slots, equipar, guardar, descartar/coletar, mudar de dono, morrer ou extrair não restaura durabilidade. Munição e desgaste são confirmados na mesma gravação antes de dano/efeitos. Falha de gravação conserva os dois e não aplica dano.

Rifles antigos sem esse campo são lidos como 100%. Sua munição já gasta é preservada. Não há reposição automática de equipamentos nem reset dos perfis. Para começar o teste manual completo, use um rifle novo do loot do mapa, que terá10/40 e100%.

## Teste manual
1. Inicie pelo Iniciar-Marau6.cmd e abra o mapa em2 clientes, Play As Client.
2. Entre na partida; colete um rifle novo e equipe. Confirme10/40 e100% no HUD/inventário.
3. Dispare10 vezes:0no carregador,75% de condição. Recarregue com R e repita até40tiros aceitos:0%,carregador0,reserva10.
4. Recarregue:carregador10,reserva0,condição0%. Tente atirar: munição e vida do alvo devem permanecer iguais.
5. Descarte a arma; o outro jogador coleta e equipa. Ela deve manter0%,10/0 e o bloqueio.
6. Extraia, volte à base e entre em outra rodada. A arma mantém0% e não dispara.

## Verificação automatizada
- Backend: contratos HTTP/PostgreSQL isolados, condições inválidas/aumento, transferências, zero, recibo do formato anterior e reinício da API.
- Unreal: combate 40tiros, arma independente, bloqueio, recarga, exportação, mudança de dono, falha de journal e regressões.
- Integração: scripts/Test-Marau6Integration.ps1 -PIE -Wear testa2 clientes reais, transferência,2rodadas e reinício da API.

Estado: implementação local compilada e revisada, com27 testes Unreal e28 contratos backend aprovados. PIE com2 clientes,40tiros,transferência,2rodadas e reinício da API aprovado. Ciclo de persistência/recuperação em novo processo aprovado. API local atualizada com impressão do banco idêntica antes/depois. Validação manual confirmada pelo usuário em 01/11/2026; PR e merge autorizados na mesma data.