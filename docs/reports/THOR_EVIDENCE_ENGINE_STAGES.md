# THOR Evidence Engine — staged implementation plan

Дата: 2026-09-12. Статус: **план реализации; live V0/V1 ещё не выполнены**.
Архитектура, schema и fail-closed правила:
[THOR_EVIDENCE_ENGINE_ARCHITECTURE.md](THOR_EVIDENCE_ENGINE_ARCHITECTURE.md).

## Порядок и общие gates

```text
V0 architecture + storage contract + capability experiments
  → V1 narrow RAM/register slice + known canary
  → V2 general RAM lifetime/last-writer
  → V3 extended register/control provenance
  → V4 ROM/resource/DMA roots
  → V5 static enumeration / non-owning Carver bridge
  → V6 multi-scenario differential
  → V7 bounded scheduler
  → V8 held-out progressive expansion
  → V9 operational M12 integration
```

Минимальные RAM/register semantics входят в V1. V2/V3 расширяют проверенное
ядро; V1 не может ждать их, чтобы показать настоящую provenance цепочку.
V5 не означает SOURCE_OWNED promotion, V9 не означает production runtime.
Все файлы ниже, кроме двух design reports и governance links, **планируемые**.

Каждый stage завершается отдельным stage report со статусом PASS / PARTIAL /
BLOCKED, commit identity, command lines, hashes, machine profile, performance,
positive/negative controls и перечисленными непроверенными возможностями.
Переход разрешён только при выполненных обязательных acceptance clauses;
PARTIAL не переименовывается в PASS через снижение требований.

Общие hard STOP:

- ROM/state/core identity mismatch, непонятная callback phase, event drops,
  silent inference через неизвестную инструкцию, unresolved mandatory conflict.
- Любой baseline manifest mutation во время import/slice/plan/export.
- Изменение цели, начало M13/M14, dependency из native core на evidence engine.
- Необоснованный semantic label или ownership по runtime-only coverage.
- Необходимость расширить эксперимент за заранее зафиксированный budget.

STOP прекращает зависимый эксперимент/продвижение; диагностика и сохранение
отрицательного результата продолжаются в оставшемся бюджете. Не удалять
неудачные captures. Новая попытка допустима после конкретного исправления
capability/contract, а не с тем же планом и надеждой на другой результат.

Перед implementation commit: соответствующие tests, Debug/Release available
CI-equivalent checks, diff check, ≤500-line source scan. При CMake/portability
изменениях также GNU/Linux build/link; недоступность инструмента записывается
до push. Не менять существующие failures ради формального green. Для одной
только design documentation native build не нужен.

## V0 — архитектура, storage prototype и capability contract

**Exact goal:** сделать невозможными ложный duplicate edge, смешение epochs и
claim из неполного capture; установить, какие наблюдения реально предоставляет
установленный BizHawk 2.11.1/GPGX. Архитектурная часть V0 зафиксирована этой
задачей; executable storage/capture prototype остаётся следующим шагом.

**Scope:** DDL в отдельном `schema.sql`, sqlite3 importer synthetic JSONL,
canonical identity, seal validation, transactional import и deterministic export.
Не открывать неизвестные ROM frontiers. Сохранить существующий local harness
как baseline; новый Lua передавать через уже существующий `-LuaScript`.
Если нужен переносимый launcher, параметризовать копию проверенной логики,
сравнить её с baseline; не создавать второй несовместимый lifecycle.

**Artifacts/files:** `src/tools/thor_evidence/schema.sql`, `identity.py`,
`store.py`, `events.py`; `tests/thor_evidence_store_test.py`;
`capture/capabilities.lua`; local `build/thor-evidence/v0/` с capabilities.json,
environment.json, fixture DB, repeat/negative reports. Payload-free capability
fixtures допустимы в tests; private ROM/state/trace paths — local config.

**Acceptance tests:**

1. Импорт одного sealed trace дважды даёт те же events/relations/evidence;
   новый attempt не увеличивает independent lineage count.
2. Одинаковые endpoints с новым evidence сохраняют один relation ID.
3. Event с missing epoch/FK, altered chunk hash, duplicate seq с иным payload,
   unknown schema либо отсутствующим footer отвергается.
4. Crash в import не оставляет наполовину опубликованный trace; повтор после
   rollback даёт тот же logical export. Порядок imports A/B и B/A эквивалентен.
5. Capability fixtures определяют exec pre/post phase, write callback PC,
   actual width, same-value write delivery, overlapping write filtering,
   register reads, callback ordering/reentrancy и effect-free debug peeks.
6. Baseline oracle через текущий runner повторён; расширенная instrumentation
   сохраняет oracle. Если нельзя проверить read/DMA/IRQ hook — capability
   `UNVERIFIED/ABSENT`, не PASS. V1 допускает лишь capabilities своего scope.

**Performance limits:** synthetic 1M events import ≤60 s, RSS ≤1 GiB;
каждый capability run ≤30 s; не более 4 новых live fixture runs плюс два baseline
repeats в initial batch. На отрицательном исходе остановить batch досрочно.

**STOP:** провал identity/seal/FK/dedup; невозможно однозначно сопоставить
нужный V1 exec/write; instrumentation меняет oracle. Отсутствие DMA само по себе
не блокирует storage/V1 structural-only branch, но запрещает live DMA claim.

**Evidence before next:** capability matrix с PASS/ABSENT/UNVERIFIED для каждого
поля, raw fixtures, два совпадающих baseline streams, import logs, metrics,
зафиксированный V1 contract и перечень supported instruction forms.

## V1 — узкий полноценный FF13CC canary

**Exact goal:** из raw observations автоматически восстановить известную
first-record ROM/register/address/write цепочку и известные counter writers.
Готовый static graph не подаётся engine как ответ.

**Scope:** один verified state, three-neutral settle + one measured frame,
selected dense producer window и sparse memory writes. Реализовать только
нужные instruction forms с operand-size/address-mode semantics, byte versions,
flags для выбранной ветки и opaque precapture roots. Extensible rules dispatch
по decoded form, а не special-case `if PC == A372: emit known graph`.

**Artifacts/files:** `capture/canary.lua`, `capture/events.lua`, `normalize.py`,
`memory_versions.py`, `m68k_rules.py`, `provenance.py`, `slice.py`;
`tests/thor_evidence_canary_test.py`, `tests/fixtures/thor_evidence/canary.json`
с payload-free assertions; local canary capture/DB/graph/reproducibility report.
При приближении rules к 500 строкам разделить по memory/ALU/control semantics.

**Acceptance tests — frozen mandatory contract:**

| ID | Обязательный результат | Не засчитывается |
| --- | --- | --- |
| C01 | State 2117, pre-window 2120, end 2121; valid sealed capture | Пустой trace либо успех только process exit |
| C02 | A342 entry, observed caller, 6 A372 instances с отдельными outputs | Один global PC node без instances |
| C03 | A372 writer у FF13CC, callback A374 сохранён отдельно | PC−2 как универсальная эвристика |
| C04 | FF1858 predicate выбирает A438, checked ROM source bits | A43A как base; A480 отмечен DYNAMIC без исполнения |
| C05 | D2 high24 из record, low8 из incremented D5; 00880901 oracle | Union всех ROM bits без kill low8 |
| C06 | FF188C sign-extended offset → A5 destination; FF188A → D5 | Только source value, потерян address expression |
| C07 | A42A/A430 0030/0006 → B000/B006 0060/000C → ACE6/ACEC 0080/0010 | Финальные 0080/0010 приписаны A342 |
| C08 | Writer completeness внутри declared window; precapture origins явны | Last observed автоматически назван last writer |
| C09 | Повторный import no-op; повторный run подтверждает reproduction | Repeats искусственно увеличили новые relations |
| C10 | Neutral и Right дают COMMON path; zero poll не создаёт INPUT_CAUSES | Right-only causal edge по установленной кнопке |
| C11 | Shadow SAT→DMA→D000 imported structural join явно отделён | Отчёт о новом live same-frame VRAM proof |
| C12 | Каждое derived relation раскрывается до raw witness + checked rule | Готовая цепочка скопирована из oracle |

Фиксированный sink запроса C04–C06 — first four-byte record write. Полная routine
и все восемь bytes записи — дополнительные запросы с собственными obligations.
Нельзя выдавать успех first-record canary за полную реконструкцию producer.

**Negative controls:** same-value overwrite, partial Dn write с отличными high
bits, longword FF188A overlap, missing write event, unknown opcode, shifted
callback PC, false alternate root, dropped footer, lagged input; все должны
либо вернуть правильные зависимости, либо остановить affected proof.

**Performance limits:** на arm ≤250k events/64 MiB/30 s, ≤4 MiB Lua buffer,
≤64 exec sites/64 normalized memory intervals. Начальный batch: два neutral
repeats и один Right; ≤120 s суммарного live wall budget. Dense selected window
не расширяется на весь кадр без нового плана. Цель measured overhead ≤10x.

**STOP:** любая mandatory C01–C12 не выполнена; unknown в центральной
high24/low8/address цепочке; hard-coded expected edge; скрытая lossiness.
Нельзя масштабировать A372←UNKNOWN под названием successful V1.

**Evidence before next:** graph diff against frozen oracle, zero forbidden claims,
ordered writer witnesses, exact rule fixtures, reproducibility и bounded
non-interference reports, измеренные budgets. V1 success не требует открытия
нового ROM interval или увеличения ownership.

## V2 — общая RAM lifetime / last-writer provenance

**Exact goal:** корректно отвечать на arbitrary covered RAM byte/word/long seed,
включая overlap, aliases, same-value writes и нескольких consumers.

**Scope:** interval watch compiler, physical mapping normalization, watch
activation epochs, per-byte SSA, coverage validator, snapshot roots,
read-after-write slices, forward readers. Не расширять register ISA без нужды.

**Artifacts/files:** `watch.py`, `coverage.py`, `address_spaces.py`,
`memory_versions.py`; `tests/thor_evidence_memory_test.py`; synthetic alias/
overlap/IRQ fixtures, local V2 last-writer report.

**Acceptance tests:** byte→word, long→overlapping word, write at address−2,
two successive equal writes, reader before first capture write, alias and
mapping change, two restores to same RAM address. Все last-writer positives
требуют complete writer certificate. Unobserved actor/window выдаёт frontier.
Forward slice не перечисляет readers за концом trace как proven absent.
Canary C01–C12 остаётся green.

**Performance limits:** ≤256 normalized intervals и 64 KiB shadow RAM для
начального Genesis RAM scope; store импортирует 1M synthetic events в V0 budget;
slice ≤100k operations/5 s. Live proof один мотивированный canary replay ≤30 s.

**STOP:** неучтённый пересекающийся writer; stale version после watch gap;
address mirror без map certificate; flat RAM_ADDRESS graph вместо versions.

**Evidence before next:** независимые synthetic expected versions, overlapping
word/long live capability evidence, coverage rejection report и peak RSS.

## V3 — расширяемая register / control provenance

**Exact goal:** восстанавливать local instruction algorithms по supported
68000 forms и явно останавливаться на unsupported effects.

**Scope:** typed micro-op IR, byte/word/long Dn, An sign extension, shifts,
mask/ALU, CCR predicates, DBcc, addressing updates; MOVEM/stack/IRQ только
после отдельных tests. Reuse existing exact decoder, без дублирования decoder.
CALL/RETURN handling проверяет opcode, stack/version и continuation.

**Artifacts/files:** `m68k_alu.py`, `m68k_memory.py`, `m68k_control.py`,
`instruction_ir.py`, `proof_rules.py`; corresponding Python tests и synthetic
instruction programs. Сохранить supported-forms inventory в stage report.

**Acceptance tests:** независимые nonzero vectors для partial writes/sign
extension/carry/CCR.X, LEA versus memory read, PC-relative base, postincrement,
unknown instruction clobber, IRQ interruption, tail JMP versus call. Bit slices
must remove overwritten/constant-masked inputs. Replay computed output values
сравнивается с independent observations; own interpreter не единственный oracle.

**Performance limits:** expression DAG ≤100k operations/query, import RSS ≤1 GiB;
такие же capture caps как V1. Unsupported form не запускает автоматическую
реализацию всех 68000 инструкций; только фиксированный следующий form batch.

**STOP:** rule output mismatch, неизвестный effect выдан за identity copy,
semantic helper зашит по ROM PC или недоказанное universal control dependence.

**Evidence before next:** form-by-form tests, raw/core checkpoints, FIRST_DIVERGENCE
negative tests, rule hashes и explicit unsupported inventory.

## V4 — ROM roots, resource summaries и hardware boundary

**Exact goal:** получить воспроизводимые ROM/value/address roots для нескольких
seeds и сохранить правильную границу между producer, DMA и видимостью VDP.

**Scope:** demand ROM reads, mapping generation, constants, certified copy
summaries; один existing 0x3820 resource contract и один доказуемый DMA mode.
Token-level decoder provenance только для selected decoded outputs. Если
hardware hooks недостаточны, ROM-root часть может завершиться, hardware часть
остаётся PARTIAL; общий V4 PASS требует заранее указанного transfer gate.

**Artifacts/files:** `roots.py`, `resource_summary.py`, `dma_model.py`,
`capture/resources.lua`, `capture/vdp.lua`; resource/DMA tests; local root,
token witness и queued/start/complete/visibility reports.

**Acceptance tests:** ROM high bits vs literal bits, resource no-return,
backreference to earlier output, consumed extent vs allocation size,
DMA source overwritten after queueing, fill versus copy, wrap/auto-increment,
unknown command latch. SAT label требует table-base evidence. Hardware output
claim не строится только по FF13CC/D000 одинаковым buffers.

**Performance limits:** ≤2 selected calls/run, ≤2 measured frames после exact
state/trigger, ≤250k events/64 MiB/30 s; summaries expand только requested output
slice, ≤100k operations. Никакого глобального graphics capture.

**STOP:** unknown source mapping, guessed resource boundary, отсутствующее
transfer evidence при live DMA claim; необходимость заменить stock core без
отдельной capability validation.

**Evidence before next:** проверенные root/copy certificates, negative DMA race
test и чёткое разделение structural vs observed hardware edges. Primary V5
может работать с ROM-only certificates, не считая hardware PARTIAL закрытым.

## V5 — static enumeration и non-owning Carver bridge

**Exact goal:** преобразовать runtime frontier в проверяемый static request и
добавить результат в существующую IntervalDB без изменения ownership.

**Scope:** адаптеры к existing M12 analyzers, scoped proof certificates,
dependency invalidation, deterministic Carver export и explicit manifest pinning.
Первый finite-domain positive — существующий A438/A480 или selector 0..7;
negative — incomplete extent 03BDD8.

**Artifacts/files:** `static_bridge.py`, `certificates.py`, `carver_export.py`,
`manifest_links.py`; `tests/thor_evidence_static_bridge_test.py`; local static
request/certificate/rebuilt Carver view и manifest guard report.

**Acceptance tests:** positive finite domain independently checked;
observed subset не объявлен полным; invalid assumptions invalidate proof;
03BDD8 остаётся EXTENT_UNRESOLVED; identical export stable; RAM intervals не
передаются как ROM bounds. Старый screen-root-c count 1,475,346 отвергается при
ожидаемых 1,475,368. Import не создаёт SOURCE_OWNED delta и не меняет input hash.

**Performance limits:** ≤16 bounded static requests/batch, ≤60 s/request,
≤5 min batch; envelope declared до исполнения. Graph queries ≤5 s/100k ops.
Не перезапускать полный ROM scan ради одного finite consumer.

**STOP:** adapters повышают OBSERVED до full-domain без certificate; manifest
hash/count изменился; proof зависит только от того же generated assertion.

**Evidence before next:** проверяемые positive/negative certificates,
diff двух Carver logical exports, unchanged manifest/hash и proof invalidation test.
Promotion dry-run допускается позже по existing workflow, не как побочный
эффект evidence import.

## V6 — multi-scenario differential и process reuse

**Exact goal:** получать честные COMMON/ARM_ONLY результаты и сокращать runs
без загрязнения состояния между arms.

**Scope:** сначала separate-process arms; затем same-process restore epochs,
stateful collector reset, scenario differ и corpus minimization. Входной набор
не обязан включать все направления/кнопки. Input experiment требует poll-capable
window; текущий lagged canary используется как negative causal control.

**Artifacts/files:** `scenario.py`, `scenario_diff.py`, `corpus.py`,
`capture/epochs.lua`; isolation/differ tests; local neutral/Right/neutral reports.

**Acceptance tests:** separate-process A/B/A совпадает с same-process A/B/A
по каждому arm, не появляется duplicate hooks, state digest и call/taint maps
сброшены. Different watch coverage выдаёт UNKNOWN вместо ONLY. Известный lagged
canary не порождает causal edge. Корпус после minimization сохраняет все selected
obligation witnesses и raw refs.

**Performance limits:** ≤8 arms / 120 s / 256 MiB на batch, каждый arm ≤30 s;
initial windows ≤4 measured frames, extension требует нового budget plan.

**STOP:** reset divergence, no polls для causal input test, hidden external
input/movie influence, combinatorial enumeration без predicted obligation.

**Evidence before next:** separate/same-process cross-check, C10 negative control,
scope-aware graph diff, corpus before/after cost и preserved witness inventory.

## V7 — автоматический frontier scheduler

**Exact goal:** автоматически выбрать и выполнить небольшой следующий batch
только из допустимых, информативных proof obligations.

**Scope:** deterministic priority queue, prerequisites, gain/cost estimates,
negative-result memoization, compatible-plan packing, explainable selection,
campaign budgets и stop persistence. Никакого autonomous code generation,
непроверенного solver или новых capture capabilities из планировщика.

**Artifacts/files:** `frontier.py`, `planner.py`, `scheduler.py`,
`campaign.py`; scheduler tests; local plan.json/attempt ledger/gain reports.

**Acceptance tests:** fixed DB+config → identical plan; forbidden capabilities
не выбираются; конфликт блокирует dependents; duplicate prerequisites не
удваивают score; overlapping ROM gains считаются union; два identical no-gain
attempts выключают план до evidence/contract change. Kill/resume сохраняет
расход budget и completed jobs, не выполняет arm второй раз случайно.

**Performance limits:** ≤10k frontiers scored ≤5 s, ≤3 execution plans и
≤120 s emulator wall time/campaign; static batch имеет отдельный V5 cap.
Доля exploratory выбора ≤10% бюджета и только среди допустимых планов.

**STOP:** цикл без marginal gain, exceeded budget, план требует wider
instrumentation без оценки cost, scheduler сам изменил proof threshold.

**Evidence before next:** воспроизводимый ranking rationale, fixtures
reachable/high-gain vs unreachable/expensive, budget-resume/negative tests,
один реальный bounded canary campaign с корректным no-novelty результатом.

## V8 — held-out progressive graph expansion

**Exact goal:** доказать переносимость метода за пределы вручную известного
canary и полезность progressive watch sets.

**Scope:** выбрать один frontier 03BDA6/03BDD8 либо B730 с новым конкретным
вопросом. Разведочный capture → missing writer/read → focused replay → join
или проверяемый negative result. Не зашивать найденные ответы в Lua.
Optional concolic experiment — отдельная подзадача только после успешного
основного stage: ≤200 instructions, ≤8 input variables, ≤2 s solve/query,
RAM-only verified semantics и обязательный actual replay solver model.

**Artifacts/files:** преимущественно declarative seeds/plans, минимальные
добавления существующих adapters; `tests/thor_evidence_expansion_test.py`;
local held-out graph, watcher evolution, attempts и closure report.

**Acceptance tests:** хотя бы один новый scoped relation/contract join с raw
witness либо точный новый blocker, которого не было в исходном frontier.
Исследование общего участка повторно использует certified summary, а новые
preconditions корректно его переоткрывают. 03BDD8 не закрывается повтором
старого sentinel search. UNKNOWN не исчезает при time budget exhaustion.

**Performance limits:** ≤3 progressive passes, ≤8 total arms, ≤120 s emulator,
≤256 MiB raw; каждый pass заранее имеет watch diff и expected gain. Поиск
недостижимого state не расширяется в бесконечное gameplay exploration.

**STOP:** только canary-specific code работает; новое evidence дублирует старый
класс; frontier требует неизвестного state/input contract; solver без replay.

**Evidence before next:** held-out outcome с независимым manual spot-check,
before/after obligations, exact chain/expression witness и measured cost.
Honest BLOCKED валиден как результат RE, но не является доказательством
готовности универсального automatic expansion.

## V9 — operational M12 integration

**Exact goal:** сделать engine воспроизводимым штатным developer tool M12,
который сохраняет общую базу между задачами и ускоряет existing proof workflow.

**Scope:** versioned CLI/config, schema migrations, backup/restore, artifact
retention, dependency invalidation, CI synthetic tests, documentation, pinned
baseline policy. Existing promoters remain separate; можно подготовить одну
promotion proposal, если V5/V8 реально получили полный независимый contract.
Отсутствие eligible bytes не компенсируется ослаблением acceptance.

**Artifacts/files:** `src/tools/re_thor_evidence.py`, package entrypoint,
`migrations/`, focused CMake test registration, `docs/FILE_MAP.md`,
`docs/ARCHITECTURE.md`, `docs/WORKLOG.md`, tool usage report; payload-free
CI fixtures. DB, traces, savestates, ROM и decoded outputs остаются ignored/local.

**Acceptance tests:** install/use на чистом checkout с local prerequisites;
backup→restore identical logical export; schema N→N+1 migration с fixtures;
удалённый artifact invalidates replayability; existing Carver tests pass;
import/plan/slice/export не меняют manifest. Если проводится actual promotion,
соблюдены exact rebuild/source ownership и все existing pre-push gates.
No dependency из oasis_core/platform/runtime на tools package.

**Performance limits:** fixture DB с 1M events сохраняет V0 import и V2 query
budgets; retention limits конфигурируемые, reference artifacts не удаляются
автоматически. Публикуется measured profile, а не обещание работы на любой машине.

**STOP:** integration меняет reconstruction output, шлёт copyrighted payload
в Git, ломает independent proof lineage, скрывает migration/invalidation errors.

**Evidence before completion:** regression suite, reproducible install/restore,
final dependency/diff/source-limit review, quantitative campaign comparison на
одинаковых seeds и budgets, exact SOURCE_OWNED delta или честный zero delta.
Это готовность M12 инструмента; ASM completion и runtime parity отдельно.

## Ближайший исполнимый шаг

Начать V0 storage/identity/coverage fixtures и capability validation вокруг
существующего runner. Не начинать V1 collectors, пока не известны реальные
write width/phase/overlap guarantees. Не запускать 03BDD8 до successful canary
и не строить UI до появления полезных `explain`/`slice` результатов.

Критерий продолжения инвестиции: V1 восстановил known chain без hard-coded
edges, V5 выдаёт проверяемые static obligations, V8 показывает пользу вне
canary при ограниченном бюджете. Если система лишь накапливает trace, оставить
storage как evidence index и пересмотреть provenance capture, не масштабировать
неработающий inference.
