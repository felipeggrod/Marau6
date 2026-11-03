# Som provisório de disparo

O tiro usa `/Game/Marau6/Audio/S_RifleFire` como único som (0,86s, -21 LUFS), convertido do clipe fornecido `rifle_fire_sfx.mp3` para WAV mono 16-bit 44,1kHz:

`ffmpeg -i rifle_fire_sfx.mp3 -ac 1 -ar 44100 -c:a pcm_s16le Saved/SourceAudio/S_RifleFire.wav`, depois `Marau6.SetupShotFX` com o editor fechado.

Substituiu em 02/11/2026 as duas camadas anteriores (Uzi `S_Shot_Uzi` e `S_RifleShot` sintetizado em `AMarau6ShotFX`), que não tocam mais. Volume 1,8 dono / 2,1 outros (cerca de +6 dB sobre o nível da antiga dupla, a pedido: o tiro deve dominar); passos subiram para 0,8 / 1,4 para não sumirem.

AMarau6ShotEffect::Present toca uma única alternativa de áudio: PlaySound2D para o dono (volume 0,85, não marcado como som de interface), ou PlaySoundAtLocation na boca do rifle em terceira pessoa para outros clientes (volume 1). Pitch aleatório 0,95–1,05 a cada tiro, como no TogetherWEB. Atenuação linear esférica: volume estável nos primeiros3m e queda até silêncio em80m. Esses valores são provisórios. Não há oclusão por paredes nesta etapa.

O som nasce apenas pelo multicast de tiro aceito já usado nos efeitos. Não há novo RPC ou mudança em dano/cadência/munição. Servidor dedicado não toca áudio. Reprodução independente do ator visual: remover o clarão/impacto não corta o som.

## Validação

Build Development Editor / Win64 e seis testes Marau6.Combat aprovados. ShotAudio verifica referência do asset, duração curta e sem loop, espacialização e curva de volume com a distância. Os testes executados sem áudio não comprovam a experiência auditiva; essa parte requer teste manual.

Abra duas janelas Play As Client. Atire com cada jogador e ouça sua arma e a do outro. Compare o tiro do outro de perto e de longe; confira ausência de som quando não há disparo aceito (vazio, recarga ou morte). O rifle e os efeitos visuais devem permanecer funcionando.

Se só uma janela tiver som, confira nas configurações de Play: Enable Game Sound ativo e Solo Audio in First PIE Client desativado. Create Audio Device for Every Player permite dispositivos separados por jogador, com custo adicional de CPU; para dois jogadores a engine já pode criar dois dispositivos com essa opção desmarcada. Nenhuma configuração pessoal do editor foi alterada automaticamente.

Validação manual confirmada pelo usuário em 28/10/2026, com dois jogadores.

## Passos

`UMarau6Footsteps` (jogadores e bots) reproduz `S_Footstep_01`/`S_Footstep_02` alternados, também vindos do TogetherWEB (`footstep1.mp3`, `footstep2.mp3`). Um passo a cada 1,6m percorridos no chão, acima de 0,5m/s, sem agachar, apenas com vida. Pitch 0,9–1,1. Dono ouve em 2D (volume 0,25); os outros ouvem nos pés do personagem (volume 0,5), estável até 2m e silêncio em 25m. Calculado localmente em cada cliente a partir do movimento replicado: sem RPC novo; servidor dedicado não toca. O mesmo componente toca `S_Land` (TogetherWEB `land.mp3`) para todos, volume 0,5, ao tocar o chão caindo a mais de 4m/s. No pulo, só o dono ouve o mesmo `S_Land` em 2D, volume 0,4 e pitch 1,3. Teste: `Marau6.Combat.Footsteps`.

## Reverberação ambiente

Cada cliente ativa, no PlayerController local, um reverb curto e quente (`Marau6Audio::ConfigureAmbientReverb`: decaimento 1,1s, agudos amortecidos, ganho 0,32, volume 0,5) com a tag `Marau6Ambient`. É desativado no EndPlay para não ficar ativo no editor depois do PIE. Sem AudioVolume no mapa; um AudioVolume com prioridade maior pode substituí-lo por área no futuro.

Todos os sons alimentam esse reverb. Os do próprio jogador (tiro, passos, pulo, aterrissagem) deixaram de usar PlaySound2D: tocam com atenuação sem espacialização nem queda por distância e envio constante (tiro 0,3; passos 0,15), porque sons 2D não enviam ao reverb. Sons de outros jogadores e bots ficam mais molhados com a distância: tiro 0,3→0,8 entre 3m e 80m, passos 0,2→0,6 entre 2m e 25m. Valores provisórios, para ajuste de ouvido.

## Mixagem (03/11/2026)

(Histórico: substituído por `S_RifleFire`, ver topo.) A Uzi e `S_RifleShot` (efeitos do disparo) tocavam no mesmo volume: as duas fontes têm a mesma sonoridade (-18 LUFS). Volumes em `Marau6Audio.h`: cada camada 0,45 dono / 0,54 outros (juntas, o mesmo nível de antes), mesma queda com a distância (3 m a 80 m); passos 0,5 / 1,0; aterrissagem 0,8; pulo 0,6. Os passos são ~15 dB mais baixos na origem que a Uzi, por isso ficam perto do volume cheio. Tiros de outros jogadores caem em dB com a distância (NaturalSound, -60 dB no limite): tiroteio a 20 m fica bem abaixo do volume cheio e não cobre os passos próximos. A camada do rifle do dono também passou a alimentar o reverb.

## Descompressão na entrada da partida (03/11/2026)

`S_Decompression` é um jato de gás sintetizado ("pfvvshhhh": estalo da vedação, sopro grave que sobe para um chiado agudo, cauda de ~2,4s), gerado por `scripts/Generate-Decompression.py` em `Saved/SourceAudio` e importado por `Marau6.SetupShotFX`. Toca só para o jogador local, quando `bInRaid` passa a verdadeiro (entrada na partida pela base; B). Não toca ao iniciar o jogo nem ao voltar para a base. Volume 0,5, pitch 1,15, sem espacialização, com envio 0,3 ao reverb (`Marau6Audio::GasVolume`/`GasPitch`). Mapas sem base persistente não têm entrada na partida e, portanto, não tocam o som.

## Recarga (02/11/2026)

Quatro sons curtos sintetizados para a recarga do rifle, mono 16-bit 44,1kHz, pico -1 dBFS e cerca de -19 LUFS (perto dos passos, bem abaixo do tiro), em `/Game/Marau6/Audio`:

- `S_Reload_Cloth` (0,35s, -18,9 LUFS): manga e equipamento se mexendo junto com os braços — sopro de ruído filtrado, estalinhos de tecido, rangido leve de couro e toque de fivela.
- `S_Reload_MagOut` (0,40s, -19,6 LUFS): clique do botão de liberação, batida surda do carregador soltando e o metal deslizando para fora com raspagem, terminando num tique ao sair do poço.
- `S_Reload_MagIn` (0,40s, -19,2 LUFS): carregador novo subindo com raspagem crescente e "clack" metálico firme ao travar (estalo, baque grave e parciais de aço), com a trava fechando logo depois.
- `S_Reload_Charge` (0,45s, -19,1 LUFS): alavanca de manejo puxada (deslize com catraca de cliques que se espaçam), batente traseiro e retorno com estalo seco e corpo metálico.

Gerados por `scripts/Generate-ReloadSounds.py` (semente fixa, numpy + scipy, um fluxo aleatório por som; cada um com drive de saturação próprio e normalizado a -1 dBFS) e importados com o commandlet ImportAssets (JSON com caminhos absolutos, `DestinationPath` `/Game/Marau6/Audio`, `bReplaceExisting` verdadeiro; o erro "Invalid Destination Path ()" de um grupo vazio extra é inofensivo):

`python -I scripts/Generate-ReloadSounds.py Saved/SourceAudio`

`"D:\Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "D:\UnityProjects\marau6\Marau6.uproject" -run=ImportAssets -importsettings=<json> -nosourcecontrol -unattended -nopause -stdout`

Medição: `ffmpeg -i f.wav -af "apad=pad_dur=1,ebur128=peak=true" -f null -` (o preenchimento com silêncio é necessário porque o ebur128 integra em blocos de 400ms) e `volumedetect`. Sem verificação auditiva até aqui; ligação aos eventos da recarga fica no código C++.
