# Equipamentos e desgaste — desenho aprovado

## Objetivo e abordagem
Registrar a condição individual dos rifles e mostrá-la ao jogador, preservando-a em todas as transferências e reinícios. O usuário aceitou seguir com este bloco após o PR#6; os parâmetros abaixo incorporam as alterações aprovadas pelo usuário.

Recomendação: condição linear por disparo, guardada na própria instância UUID do rifle. Aproveitar o ledger e os snapshots existentes, com munição e desgaste numa única gravação durável. Simulação por peças internas, uso por tempo e falhas probabilísticas são alternativas futuras; exigiriam novas decisões e ficariam fora deste marco.

## Regras aprovadas
- Rifle novo começa em100% de condição. Cada disparo efetivamente aceito pelo servidor reduz2,5 pontos percentuais, limitado a0%. Dez tiros reduzem25 pontos;40 tiros levam um rifle novo a0%.
- Disparos contra paredes ou que não acertam nada também desgastam. Cliques recusados por cadência, carregador vazio, recarga, cura, morte ou outro estado inválido não gastam condição.
- Recarregar, movimentar, equipar, guardar, descartar ou coletar não desgastam nem recuperam condição.
- Em0% o rifle não dispara mais, mesmo com munição disponível. O quadragésimo tiro de um rifle cheio é aceito e o seguinte é bloqueado. Dano, cadência e precisão permanecem iguais antes de zero; não há reparo ou recuperação. Recarregar não recupera condição.
- Suprimentos e Peças não recebem durabilidade. Rifle visual de bot continua sem instância de equipamento persistente e não participa do desgaste; FUT-002 permanece adiado.
- Novos rifles de loot/kit inicial começam em100%, sem conceder equipamentos novos aos perfis existentes. Rifles antigos que ainda não têm o campo serão interpretados como100%, uma única compatibilidade de dados antigos; nunca substituir um valor já salvo por100%.

## Autoridade e persistência
O cliente solicita disparo, nunca informa quanto desgaste aplicar. O servidor revalida arma equipada, munição, cadência e estado, e calcula a perda no rifle correto pelo GUID. Nenhum rifle guardado ou de outro jogador pode receber desgaste no lugar do equipado.

Munição e condição do disparo precisam ser confirmadas na mesma transação local/journal antes de aplicar dano e efeitos aceitos. Falha de gravação conserva os valores anteriores e não confirma o disparo. Repetição, sincronização e callbacks não aplicam desgaste duas vezes. Recarga continua atualizando munição sem alterar condição.

Usar representação inteira de precisão fixa (por exemplo10000 unidades=100%,250 unidades por tiro) para evitar erros acumulados. A UI converte para percentual. Esta é proposta técnica do assistente; detalhes dos nomes de campos/classes ficam a cargo da implementação.

Carregar, equipar, trocar slots/donos, descartar, coletar, morrer, extrair, retornar à base e abrir uma nova rodada devem preservar a condição da instância existente. O rifle transferido de A para B mantém seu próprio desgaste e munição; finalizar A depois não pode restaurar uma versão anterior da arma. Resultados finalizados continuam imutáveis. Saúde do jogador, cura e custódia consumed não mudam.

Backend valida tipo, faixa e não aumento da condição de rifles já conhecidos nos snapshots; novas instâncias válidas nascem em condição cheia. Não criar um endpoint de reparo nem aceitar condição ditada pelo cliente de jogo. Preservar os controles de identidade global, propriedade, slot, revisão e recibos idempotentes.

## Compatibilidade obrigatória
Preservar perfis, stash, resultados antigos, itens consumidos, diário pendente e recibos já existentes. Não apagar tabelas nem resetar dados. Tratar campo ausente em rifle antigo como condição cheia; campo presente válido, inclusive0, deve ser mantido exatamente.

A serialização de comandos legados exige cuidado: acrescentar um campo não pode invalidar o hash de um recibo antigo reproduzido após atualizar o backend. Planejar formato opcional/normalização que preserve o replay de comandos anteriores e a comparação imutável de resultados. Suprimentos/Peças legados não devem receber condição por efeito de um valor padrão indiscriminado. Uma gravação posterior pode materializar a condição normalizada, sem novos GUIDs ou concessões.

## Apresentação
Mostrar condição do rifle no inventário da partida, no equipamento/stash da base e no HUD da arma equipada. Mostrar percentual com uma casa decimal para que a perda de2,5 por tiro seja visível. Mostrar DURABILIDADE: 0% - ARMA SEM CONDICAO, indicando bloqueio de disparo. Preservar espaço para vida, munição, cura e extração.

## Verificação necessária
- Rifle novo/antigo sem campo inicia100%; valor existente, inclusive0, sobrevive à leitura e ao reinício.
- Um tiro aceito reduz exatamente250 unidades e uma bala; dez reduzem2500 unidades, sem mudar outros rifles. Rejeições não desgastam. Zero não vira negativo e bloqueia novos disparos. O tiro que chega a zero ainda causa dano normalmente.
- Recarga não altera condição. Falha de journal reverte munição/desgaste e não aplica dano; replay e callbacks não duplicam perda.
- Trocas, stash, descarte/coleta, transferência entre jogadores, morte/safe/extração e rodada seguinte preservam GUID, condição e munição.
- Backend rejeita condição fora de faixa, aumentos indevidos, tipos errados e alteração de resultados; aceita atualização compatível de dados antigos.
- Testar replay de recibos/comandos realmente gerados no formato anterior, e não apenas replay de comandos já no formato novo.
- Dois clientes PIE com dados isolados: disparar, transferir rifle, extrair, reiniciar API/servidor e conferir a mesma condição uma única vez.
- Regressões atuais de combate, inventário, bots, cura, extração e persistência continuam aprovadas.

## Entrega e restrições
Somente phinasdt/Marau6 e conexão phinasdt. Nenhuma alteração na conta Faculdade/eduardotrm, em seus repositórios ou nos recursos Docker sdd-task-analyzer-docker-test. Não alterar configurações globais nem instalar ferramentas sem necessidade. Implementar em branch própria a partir da main do PR#6, compilar, obter revisão independente e preservar os dados reais ao atualizar apenas a API do Marau6. Entregar roteiro curto de teste manual; publicação do bloco dependerá de autorização específica posterior.

## Estado
Desenho preparado a partir do código atual. Não há código de produto alterado para desgaste. Aprovado pelo usuário com alterações: 2,5 pontos por tiro, bloqueio em zero e rifle novo10/40.
## Munição de teste aprovada
Rifles novos: carregador10 e reserva40. Rifles já salvos preservam a munição gasta; não há reabastecimento em toda leitura ou entrada. Os rifles de loot do mapa permitem iniciar o teste com10/40 sem resetar perfis. Após40 tiros, recarregar permite conferir que ainda há10 balas mas o rifle não dispara.
