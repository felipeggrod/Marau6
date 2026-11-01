# Equipamentos e desgaste do rifle

Regra atual, atualizada em 01/11/2026: rifle novo com100% de condição,20 balas no carregador e40 na reserva. Disparo automático ao segurar o botão, cadência0,25s, desgaste de1 ponto percentual por tiro aceito. O tiro100 ainda causa dano e deixa a arma em0%; o tiro101 é recusado mesmo com munição. R recarrega sem reparar.

Condição e munição pertencem ao UUID do rifle. Trocar slot, guardar, descartar/coletar, mudar de dono, morrer ou extrair não restaura valores. Rifles antigos preservam sua munição e condição. Campo de durabilidade ausente em registros legados significa100%, sem alterar recibos antigos.

Caixas de munição20 podem abastecer a reserva de qualquer rifle por arraste, na base e em partida. Limite120 na reserva, consumo inteiro. Não repara a arma. Gravação de disparo e consumo é atômica; falha preserva munição/condição/caixa e não aplica dano não pago.

Roteiro e regras completas: [Ícones, munição e automático](icones-municao-automatico.md).

O marco anterior validado na PR7 usava10/40 e2,5 pontos por tiro. Os documentos históricos daquela implementação permanecem como registro; estes números foram substituídos pelo pedido de01/11/2026.
