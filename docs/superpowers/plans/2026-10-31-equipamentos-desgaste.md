# Equipamentos e desgaste Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [x]`) syntax for tracking.

**Goal:** Durabilidade individual: 100%, perda de 2,5 pontos por tiro, bloqueio em zero e rifles novos10/40.
**Architecture:** Ledger authoritative grava munição e condição juntas antes do dano. Campo inteiro opcional no backend mantém hashes legados; condição efetiva ausente=10000. Inventário replica estado por UUID; HUD apenas apresenta.
**Tech Stack:** UE5.8.4 C++, ASP.NET10, PostgreSQL.
**Spec:** ../specs/2026-10-31-equipamentos-desgaste-design.md

## Global Constraints
Somente phinasdt/Marau6; nunca conta Faculdade nem seus recursos Docker. Não resetar perfis, diários, identidades ou recibos. Não publicar sem autorização específica. Execução nativa no checkout E:/VisualStudio/Marau6, branch própria, conforme fluxo estabelecido, para não duplicar assets LFS nem mudar o caminho de teste do usuário.

## Review Focus
- Tiro40 é aceito, tiro41 é recusado mesmo depois de recarga: teste de combate.
- Último tiro não desconta duas vezes por Sync: comparar item e AmmoState a cada tiro.
- Comando antigo sem campo mantém hash de recibo: contrato com hash da representação anterior.
- Zero não volta a100% após mover, extrair ou reiniciar: transferência e contrato backend.
- Falha de journal não gasta condição nem causa dano: fixture persistente com caminho bloqueado.

### Task1: Contrato persistente e compatibilidade
Files: Backend/Marau6.Backend/{Models,Profiles,RaidService}.cs; Backend/Marau6.Backend.Tests/Program.cs; Scripts/Test-Marau6Backend.ps1.
Interfaces: Item.Durability int? omitido se null; EffectiveDurability=Durability??10000. Novos rifles10000 e10/40; não rifles null. Snapshot rejeita fora0..10000, aumento e resultados alterados. Campo ausente legítimo legado=10000.
- [x] Escrever contratos para condição nova, transferência, zero, rejeições e hash legado; executar e observar falha.
- [x] Implementar campo opcional, novas sementes e validação; executar contratos PostgreSQL isolados e reinício.

### Task2: Disparo durável e apresentação
Files: Source/Marau6/Marau6{InventoryTypes,InventoryComponent,ItemLedger,Character,Persistence,PrototypeHUD}.{h,cpp}; Tests/Marau6AmmoTests.cpp; testes existentes com reserva30.
Interfaces: FMarau6Item.Durability=10000; Inventory.SaveEquippedShot() -> bool; Ledger.SaveShot(Component) -> bool. SaveAmmo permanece para recarga sem desgaste. Character.GetRifleDurability() retorna unidades; persistência inclui durability apenas em Rifle e importa ausente como10000.
- [x] Escrever teste40 tiros com recargas, rejeições, munição restante e UUID; observar falha.
- [x] Implementar perda250 e uma bala atomicamente; Sync mantém AmmoState sem desconto duplicado; bloqueio servidor e input; HUD/base/inventário mostram percentual e aviso zero. Sementes10/40.
- [x] Acrescentar teste journal falho e transferência/extrair; ajustar expectativas antigas de reserva e rodar suite Marau6.

### Task3: Integração e entrega
Files: testes PIE/persistência existentes, docs/equipamentos-desgaste.md.
- [x] Estender transferência e extração entre2clientes com condição e zero; executar integração isolada e reinício backend.
- [x] Compilar, rodar regressões e revisão independente da branch; resolver problemas materiais com testes.
- [x] Atualizar apenas API Marau6 se necessário preservando banco; entregar roteiro de40 tiros. Não fazer push/PR/merge nesta autorização.
## Resultado
27 testes Unreal e28 contratos backend aprovados; teste PIE de40tiros,transferência,2rodadas e reinício aprovado. Regressão cycle/verify aprovada. Revisão independente sem P1/P2. Dados públicos preservados e API atualizada. Branch local aguarda teste manual; sem publicação.
