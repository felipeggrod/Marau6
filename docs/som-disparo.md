# Som provisório de disparo

Reutiliza o SoundWave FirstPersonTemplateWeaponFire02 presente no template, como som provisório do rifle. Nenhum download ou asset original modificado.

AMarau6ShotEffect::Present toca uma única alternativa de áudio: PlaySound2D para o dono (volume0.6, não marcado como som de interface), ou PlaySoundAtLocation na boca do rifle em terceira pessoa para outros clientes. Atenuação linear esférica: volume estável nos primeiros3m e queda até silêncio em80m. Esses valores são provisórios. Não há oclusão por paredes nesta etapa.

O som nasce apenas pelo multicast de tiro aceito já usado nos efeitos. Não há novo RPC ou mudança em dano/cadência/munição. Servidor dedicado não toca áudio. Reprodução independente do ator visual: remover o clarão/impacto não corta o som.

## Validação

Build Development Editor / Win64 e seis testes Marau6.Combat aprovados. ShotAudio verifica referência do asset, duração curta e sem loop, espacialização e curva de volume com a distância. Os testes executados sem áudio não comprovam a experiência auditiva; essa parte requer teste manual.

Abra duas janelas Play As Client. Atire com cada jogador e ouça sua arma e a do outro. Compare o tiro do outro de perto e de longe; confira ausência de som quando não há disparo aceito (vazio, recarga ou morte). O rifle e os efeitos visuais devem permanecer funcionando.

Se só uma janela tiver som, confira nas configurações de Play: Enable Game Sound ativo e Solo Audio in First PIE Client desativado. Create Audio Device for Every Player permite dispositivos separados por jogador, com custo adicional de CPU; para dois jogadores a engine já pode criar dois dispositivos com essa opção desmarcada. Nenhuma configuração pessoal do editor foi alterada automaticamente.

Validação manual confirmada pelo usuário em 28/10/2026, com dois jogadores.
