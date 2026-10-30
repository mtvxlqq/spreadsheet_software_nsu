# PLAN.md — Табличный процессор (вариант Г), этап 1: ядро и бизнес-логика

> Этот файл — рабочее задание для Claude Code. Читай его целиком перед началом
> и возвращайся к нему перед каждой фазой. Источник правды по архитектуре — `DESIGN.md`;
> этот план его детализирует. Если план и `DESIGN.md` расходятся — см. раздел
> «Отклонения от DESIGN.md», а при любом новом расхождении **остановись и спроси**.

---

## 0. Контекст и рамки

**Что делаем:** цели CMake `core` (ядро: ячейки, значения, формулы, парсер, вычисление,
граф зависимостей, пересчёт, файлы) и `app` (бизнес-логика сеанса: курсор, режимы,
действия, клавиши, команды, фоновые задачи). Плюс тесты для обоих через `ctest`.

**Чего НЕ делаем в этом плане:**
- Слой TUI (`spreadsheet`, FTXUI) — только контракт, который `app` ему предоставляет (раздел 6).
  Реализация TUI — отдельный план.
- Функциональность этапа 2 (undo/redo, копирование со сдвигом ссылок, вставка строк,
  форматы чисел, даты, экспорт CSV). Но **архитектура должна позволять добавить её
  новыми классами** — места подготовки отмечены пометкой `[→ этап 2]`.

**Окружение:** Linux (Fedora 43, GCC 15), C++20, CMake ≥ 3.24, сборка одной командой.
Флаги `-Wall -Wextra -Werror` на всех своих целях. Тесты: GoogleTest через `FetchContent`.

**Важно про автора кода.** На защите студент без ИИ объясняет каждую строку и за 15–20 минут
дописывает расширение. Поэтому:
- код простой и прямолинейный: без шаблонного метапрограммирования, без CRTP, без
  «умных» трюков, если есть очевидный вариант;
- имена говорящие, функции короткие (ориентир ≤ 40 строк);
- в конце каждой фазы — короткое резюме в чате: какие классы появились, кто кем владеет,
  что будет при исключении, какой вопрос может задать преподаватель и как на него ответить;
- каждая фаза = 1–3 осмысленных коммита (история коммитов оценивается). Коммиты делай
  только когда пользователь попросит; иначе предложи текст сообщения коммита.

---

## 1. Правила SOLID — как они применяются в этом проекте

Это не декларация, а чек-лист. Каждый новый класс проверяется по нему.

### S — Single Responsibility
Один класс — одна причина для изменения.

| Класс | Его единственная ответственность | Чего он НЕ делает |
|---|---|---|
| `Lexer` | строка → токены | не знает про приоритеты и дерево |
| `Parser` | токены → дерево `Expr` | не вычисляет, не знает список функций (спрашивает `FunctionProvider`) |
| `CellInputParser` | сырой ввод (`"12"`, `"abc"`, `"=A1+1"`) → содержимое ячейки | не трогает лист |
| `Evaluator` | дерево → значение | не хранит ячейки |
| `FormulaPrinter` | дерево → текст | — |
| `RefCollector` | дерево → множество адресов | — |
| `Sheet` | хранение заполненных ячеек | не пересчитывает |
| `DependencyGraph` | кто от кого зависит, порядок пересчёта, циклы | не вычисляет значения |
| `Recalculator` | пересчёт затронутых ячеек в заданном порядке | не разбирает ввод |
| `Spreadsheet` | фасад: координирует всё выше + уведомляет подписчиков | сам не парсит, не обходит граф |
| `NativeFormat` / `CsvReader` | чтение/запись одного формата | не выбирает формат |
| `FormatRegistry` | выбор формата по расширению | не читает файлы |
| `Session` | состояние сеанса и маршрутизация клавиш в режим | не знает, что делает конкретная клавиша |
| `Keymap` | клавиша → имя действия, загрузка из конфига | не создаёт действия |
| `ActionRegistry` | имя → объект `Action` | не знает про клавиши |
| `TaskRunner` | запуск работы в фоне и доставка результата в главный поток | не знает про файлы |
| `CellPresenter` | ячейка → строка для отображения | не рисует |

### O — Open/Closed
Все пять точек расширения из задания добавляются **новым классом + одной строкой
регистрации в «корне композиции»** (`registerBuiltins`, `registerFormats`,
`registerActions`, `main`). Существующие классы не правятся. Это проверяется
«учебными расширениями» в фазе 13.

### L — Liskov Substitution
- Любой наследник `Value` корректно отвечает на все методы базы: `ErrorValue::asNumber()`
  возвращает `std::nullopt`, а не бросает исключение.
- Нельзя заводить методы-заглушки, бросающие `not implemented`. Пример: CSV на этапе 1 только
  читается, поэтому интерфейс формата **разделён** на `FileReader` и `FileWriter`
  (иначе `CsvFormat::save` был бы заглушкой — нарушение LSP и ISP).
- Любой `Mode` можно поставить в `Session`, любой `Function` — в реестр.

### I — Interface Segregation
Клиент зависит только от того, чем пользуется:
- `Function` видит только `EvalContext` (чтение значений и вычисление аргументов), а не весь `Spreadsheet`.
- `FileReader` видит только `CellSink` (куда класть ввод), `FileWriter` — только `CellSource`
  (что читать), а не весь фасад.
- `Parser` видит только `FunctionProvider` (`create(name)`), а не реестр с `add`.
- TUI видит `Session` через `SessionView` (только чтение) + `handleKey`/`resize`.

### D — Dependency Inversion
Верхние модули зависят от абстракций, конкретику подставляет корень композиции:
- `Parser` → `FunctionProvider` (реализация `FunctionRegistry`).
- `Spreadsheet` → `CellObserver` (реализация `Session`).
- `Session` → `CellObserver`/`SessionObserver` наружу и `MainThreadDispatcher` внутрь
  (реализация в TUI через `screen.Post`, в тестах — ручная очередь).
- `Session` → `FormatRegistry`, `ActionRegistry`, `Keymap` получает через конструктор, а не создаёт сама.
- Никаких глобальных переменных и синглтонов. Реестры живут в `main` и передаются по ссылке.

---

## 2. Общие правила кода (проверяются на каждой фазе)

1. **Владение:** только `std::unique_ptr` / `std::shared_ptr` / значения. Сырые указатели и
   ссылки — только невладеющие (наблюдатели, параметры). Нет `new`/`delete` в коде.
2. Полиморфная база: `virtual ~Base() = default;` и запрет копирования/перемещения, если
   объект живёт через указатель (`Base(const Base&) = delete` и т.д.) — правило пяти.
   Остальные классы — правило нуля.
3. Все переопределения — `override` (`final` по желанию). Никаких `dynamic_cast`,
   `typeid`, `switch` по типу объекта — только виртуальные методы и Visitor.
4. Const-корректность: геттеры `const`, параметры по `const&` или `std::string_view`,
   посетители, которые ничего не меняют, принимают `const` узлы.
5. Исключения — только собственная иерархия (раздел 3.1) + `std::bad_alloc`.
   Деструкторы `noexcept`. Изменяющие операции дают **строгую гарантию**: сначала всё,
   что может бросить, потом коммит из `noexcept`-операций (или откат через scope-guard).
6. Ошибки **в формулах** — это значения (`ErrorValue`), а не исключения. Вычисление никогда
   не бросает доменных исключений.
7. В `core/` и `app/` запрещён `#include <ftxui/...>` и любой терминальный код. Проверяется
   CMake-скриптом (фаза 0).
8. В заголовках нет `using namespace`. Пространства имён: `ss::core`, `ss::app`.
9. Код — на английском, комментарии короткие, можно по-русски; комментарий объясняет «почему», а не «что».
10. После каждой фазы: сборка без предупреждений, `ctest` зелёный в обычной сборке
    и в пресете `asan` (ASan + UBSan).

---

## 3. Модель ядра — ключевые решения

### 3.1 Иерархия исключений (`core/include/core/errors.hpp`)

```
std::runtime_error
└── ss::core::Error                 // корень всех ошибок предметной области
    ├── AddressError                // "A0", "ZZZ99999999" — неверный адрес
    ├── ParseError                  // + size_t position() — ошибка разбора формулы
    ├── FileFormatError             // + size_t line() — битый файл таблицы/CSV
    └── LimitError                  // выход за размеры листа
ss::app::ConfigError : ss::core::Error   // битый конфиг клавиш (в app)
ss::app::CommandError : ss::core::Error  // неверная команда ":xyz"
```

`Session::handleKey` ловит `ss::core::Error` и пишет сообщение в строку состояния —
программа продолжает работу.

### 3.2 Адреса (`address.hpp`)
- `CellAddress { int row; int col; }` — 0-based, `operator<=>`, `std::hash`-специализация.
- Лимиты: `kMaxRows = 65536`, `kMaxCols = 702` (A…ZZ). Вне лимита → `AddressError`.
- `CellAddress::parse("B12")` / `toString()`; парсинг регистронезависимый.
- `CellRef { CellAddress addr; bool absRow; bool absCol; }` — лексер уже понимает `$A$1`,
  но на этапе 1 флаги ни на что не влияют `[→ этап 2: RefShifter]`.
- `Range { CellAddress from, to; }` — нормализуется (from ≤ to), `contains`, `forEach`, `size()`.

### 3.3 Значения (`value.hpp`)
- `using ValuePtr = std::shared_ptr<const Value>;` — значения неизменяемы, поэтому их можно
  безопасно разделять между ячейками.
- `Value`: `asNumber() -> std::optional<double>`, `toText() -> std::string`, `isError()`,
  `display(const NumberFormat&) -> std::string`.
- `EmptyValue`, `NumberValue`, `StringValue`, `ErrorValue{ErrorCode}`.
  `[→ этап 2: DateValue — новый наследник, без правки существующих]`
- `ErrorCode { DIV0, REF, NAME, CYCLE, VALUE }`, тексты: `#DIV/0!`, `#REF!`, `#NAME?`, `#CYCLE!`, `#VALUE!`.
- Фабричные функции: `makeNumber(double)`, `makeString(std::string)`, `makeError(ErrorCode)`,
  `emptyValue()` (один общий экземпляр).
- `NumberFormat` (интерфейс) + `GeneralFormat` — до 10 значащих цифр без хвостовых нулей
  (`std::format("{:.10g}", v)`). `[→ этап 2: FixedFormat, PercentFormat]`

**Как избежать `dynamic_cast` в операциях:** вычислитель работает только через
`isError()` и `asNumber()`/`toText()`. Для «это строка или число?» добавить в `Value`
виртуальный `ValueKind kind() const` **нельзя** (это switch по типу). Вместо этого:
- арифметика: `asNumber()`; `nullopt` у не-ошибки → `#VALUE!`; у `EmptyValue` `asNumber()` = 0;
- сравнения: виртуальный `std::optional<std::partial_ordering> compare(const Value& other) const`
  с двойной диспетчеризацией через `compareWithNumber(double)` / `compareWithString(const std::string&)`.
  `nullopt` → несравнимые типы → `#VALUE!`. Это стоит описать в DESIGN.md как сознательное решение.

### 3.4 Дерево формулы (`expr.hpp`) — Composite + Visitor
Узлы как в DESIGN.md: `NumberLit`, `StringLit`, `RefExpr`, `RangeExpr`, `UnaryExpr`,
`BinaryExpr`, `FunctionCallExpr`.
- `virtual void accept(ExprVisitor&) const = 0;` `virtual std::unique_ptr<Expr> clone() const = 0;`
- `ExprVisitor` принимает `const` узлы. Посетители: `Evaluator`, `FormulaPrinter`, `RefCollector`.
  `[→ этап 2: RefShifter строит НОВОЕ дерево, а не правит старое — так проще undo]`
- `UnaryOp { Neg, Plus }`, `BinaryOp { Add, Sub, Mul, Div, Eq, Ne, Lt, Le, Gt, Ge }`.
- `FunctionCallExpr` хранит `name` (как написано), `args` и `std::shared_ptr<const Function> fn`
  (`nullptr`, если функция неизвестна → при вычислении `#NAME?`).

### 3.5 Грамматика (рекурсивный спуск)

```
formula        := '=' comparison EOF
comparison     := additive ( ('=' | '<>' | '<' | '<=' | '>' | '>=') additive )?
additive       := multiplicative ( ('+' | '-') multiplicative )*
multiplicative := unary ( ('*' | '/') unary )*
unary          := ('-' | '+') unary | primary
primary        := NUMBER | STRING
                | REF ( ':' REF )?
                | IDENT '(' ( comparison ( ',' comparison )* )? ')'
                | '(' comparison ')'
```

- Токены: `Number` (`12`, `3.5`, `1e-3`), `String` (`"a""b"` → `a"b`), `Ref` (`A1`, `$B$2`),
  `Ident`, операторы, `(`, `)`, `,`, `:`, `End`. Пробелы пропускаются. Каждый токен хранит `pos`.
- Ошибка синтаксиса → `ParseError` с позицией. Ссылка за пределами листа (`A99999999`)
  разбирается в `RefExpr`, который при вычислении даёт `#REF!` (это не синтаксическая ошибка).
- Имя функции регистронезависимое (`sum` = `SUM`); `FormulaPrinter` печатает в верхнем регистре.

### 3.6 Функции — Factory через реестр
```cpp
class Function {
public:
    virtual ~Function() = default;
    virtual std::string_view name() const = 0;
    virtual ValuePtr call(std::span<const Expr* const> args, EvalContext& ctx) const = 0;
};
class FunctionProvider {           // то, что видит Parser (ISP + DIP)
public:
    virtual ~FunctionProvider() = default;
    virtual std::shared_ptr<const Function> create(std::string_view name) const = 0;
};
class FunctionRegistry final : public FunctionProvider {
public:
    using Factory = std::function<std::shared_ptr<const Function>()>;
    void add(std::string name, Factory f);   // повтор имени → ss::core::Error
    std::shared_ptr<const Function> create(std::string_view name) const override;
};
void registerBuiltins(FunctionRegistry&);   // builtins.cpp — корень композиции функций
```
- Аргументы передаются **невычисленными** (ради `IF`).
- `EvalContext`:
  - `ValuePtr valueAt(CellAddress) const` — кешированное значение ячейки;
  - `ValuePtr evaluate(const Expr&)` — скаляр (диапазон вне функции → `#VALUE!`);
  - `std::vector<ValuePtr> evaluateFlat(const Expr&)` — диапазон разворачивается в список значений,
    скаляр — в список из одного.
- Семантика встроенных функций:

| Функция | Правило |
|---|---|
| `SUM`, `MIN`, `MAX`, `AVERAGE` | в диапазонах пустые и строки пропускаются; скалярная строка-аргумент → `#VALUE!`; первая встреченная ошибка возвращается; `MIN`/`MAX` без чисел → 0; `AVERAGE` без чисел → `#DIV/0!`; 0 аргументов → `#VALUE!` |
| `IF(c, a [, b])` | вычисляется только нужная ветка; `c` — ошибка → она же; строка → `#VALUE!`; число ≠ 0 → истина; нет `b` и ложь → 0; не 2–3 аргумента → `#VALUE!` |

### 3.7 Семантика вычисления (`Evaluator`)
- Ссылка → `ctx.valueAt(addr)`; адрес вне листа → `#REF!`.
- Арифметика: ошибка в операнде (слева направо) → она же; `asNumber()==nullopt` → `#VALUE!`;
  деление на 0 → `#DIV/0!`; результат `inf`/`nan` → `#VALUE!`.
- Сравнения возвращают `1` или `0` (отдельного логического типа на этапе 1 нет — так в задании).
  Строки сравниваются без учёта регистра. Число vs строка → `#VALUE!`.
- Если итог формулы — `EmptyValue` (например `=A1`, где A1 пуста), ячейка показывает `0`.

### 3.8 Ячейка, лист, ввод
- `Cell { std::string raw; std::unique_ptr<Expr> formula; ValuePtr value; }`
  (`CellStyle` — `[→ этап 2]`, не добавлять сейчас поле-заглушку).
- `CellInputParser::parse(std::string_view raw) -> CellContent`:
  - `""` → очистка ячейки;
  - начинается с `=` → формула (может бросить `ParseError`);
  - начинается с `'` → строка без апострофа (способ ввести `'123` как текст);
  - полностью парсится как число (`std::from_chars`) → число;
  - иначе → строка.
- `Sheet`: `std::unordered_map<CellAddress, Cell>`, хранит только непустые.
  `get` → `const Cell*` (невладеющий, `nullptr` если пусто), `set`, `remove`, `usedRange()`,
  `forEach(fn)` (в порядке строк/столбцов — для детерминированного сохранения).

### 3.9 Граф зависимостей и пересчёт
- `DependencyGraph`: `deps` (ячейка → от кого зависит) и `dependents` (обратное).
  - `setDependencies(addr, set)` — сначала строит новые множества (может бросить),
    потом подменяет через `swap` (`noexcept`) → строгая гарантия;
  - `removeCell(addr)`;
  - `RecalcPlan plan(const std::vector<CellAddress>& changed) const` →
    `{ std::vector<CellAddress> order; std::set<CellAddress> cyclic; }`:
    1. собрать все транзитивно зависимые от `changed` (включая их самих);
    2. алгоритм Кана по этому подграфу → `order`;
    3. всё, что не попало в `order`, лежит в цикле или ниже цикла → `cyclic`.
  - `planAll()` — то же для всех формул (после загрузки файла).
- Диапазон в формуле (`SUM(A1:A100)`) регистрируется как зависимость от каждой ячейки
  диапазона, **включая пустые** — иначе ввод в пустую ячейку не пересчитает сумму.
- `Recalculator::run(const RecalcPlan&, Sheet&, EvalContext&)`: ячейкам из `cyclic` ставит
  `#CYCLE!`, остальные вычисляет строго по `order` — к моменту вычисления ячейки все её
  зависимости уже свежие, поэтому никакой рекурсии между ячейками и двойных пересчётов.
- Возвращает список изменившихся адресов (для уведомления наблюдателей).

### 3.10 Фасад `Spreadsheet`
```cpp
class Spreadsheet final : public CellSink, public CellSource {
public:
    explicit Spreadsheet(const FunctionProvider& functions);
    void setInput(CellAddress, std::string_view raw);   // строгая гарантия
    void clear(CellAddress);
    ValuePtr value(CellAddress) const;                    // пусто → EmptyValue
    std::string raw(CellAddress) const;
    std::string formulaText(CellAddress) const;           // через FormulaPrinter
    Range usedRange() const;
    void recalculateAll();
    void subscribe(CellObserver*);   void unsubscribe(CellObserver*);
};
```
Алгоритм `setInput` (строгая гарантия — защищать на защите!):
1. `CellInputParser` разбирает ввод → может бросить `ParseError`, **ничего ещё не изменено**.
2. `RefCollector` собирает зависимости в новое множество.
3. Подготовить новую `Cell` (без значения).
4. Коммит: `graph.setDependencies` (строгая гарантия) → `sheet.set` (если бросит —
   откатить граф старыми зависимостями через scope-guard).
5. `graph.plan({addr})` → `Recalculator::run` → вычисление не бросает доменных исключений.
6. Уведомить наблюдателей одним вызовом `onCellsChanged(changed)`. Перед обходом
   скопировать список наблюдателей (наблюдатель может отписаться внутри уведомления).

`EvalContext` реализует **не** сам `Spreadsheet`, а внутренний `SheetEvalContext`
(читает `Sheet`, создаёт `Evaluator`) — чтобы `evaluate(...)` не торчал в публичном API фасада.

`CellSink { setInput, clear }` и `CellSource { usedRange, raw, value, forEachCell }` —
узкие интерфейсы для форматов файлов.

Наблюдатели хранятся как невладеющие `CellObserver*`. Контракт: наблюдатель обязан
отписаться до своего уничтожения (`Session` делает это в деструкторе и при смене документа).

### 3.11 Файлы — Strategy + реестр
- `FileReader { name(); extensions(); void load(std::istream&, CellSink&) const; }`
- `FileWriter { name(); extensions(); void save(std::ostream&, const CellSource&) const; }`
- `NativeFormat : FileReader, FileWriter` — расширение `.sst`:
  ```
  SST 1
  A1	42
  B2	=A1*2
  C3	text with \t tab and \\ backslash
  ```
  Первая строка — сигнатура и версия; далее `адрес<TAB>raw`; экранирование `\\ \t \n`.
  Неверная сигнатура, адрес или escape → `FileFormatError` с номером строки.
- `CsvReader : FileReader` — RFC 4180 (кавычки, `""`, переводы строк в кавычках, `,`),
  ячейки ставятся с A1. Значения импортируются **как литералы**: число или строка, формулы
  не исполняются (строка `=1+1` из CSV станет текстом — вписать в README).
  `[→ этап 2: CsvWriter — новый класс, регистрация одной строкой]`
- `FormatRegistry`: `addReader`, `addWriter`, `readerFor(path)`, `writerFor(path)`
  (по расширению, регистронезависимо; нет формата → `FileFormatError`).
- **Загрузка всегда в новый `Spreadsheet`**, затем `recalculateAll()`, затем замена старого
  документа целиком. Если файл битый — старый документ не тронут (строгая гарантия) и
  загрузку можно вести в фоновом потоке.

---

## 4. Модель слоя приложения (`app`)

### 4.1 Session
Хранит: `std::unique_ptr<Spreadsheet> doc`, `CellAddress cursor`, `Viewport`, `editBuffer`,
`Status { std::string text; enum Kind { Info, Error } kind; }`, `std::unique_ptr<Mode> mode`,
`std::optional<std::filesystem::path> currentPath`, `bool dirty`, `bool quitRequested`.
Зависимости через конструктор: `const FunctionProvider&`, `const FormatRegistry&`,
`const ActionRegistry&`, `Keymap`, `MainThreadDispatcher&`.

Публичный API (минимальный, все изменения документа — через `applyEdit`):
```cpp
void handleKey(std::string_view key);          // ловит ss::core::Error → status
void resize(int visibleRows, int visibleCols); // от TUI
void moveCursor(int dRow, int dCol);           // с ограничением по краям листа
void setMode(std::unique_ptr<Mode>);           // ОТЛОЖЕННАЯ смена, см. ниже
void applyEdit(std::unique_ptr<EditCommand>);  // выполнить; [→ этап 2: положить в UndoStack]
void replaceDocument(std::unique_ptr<Spreadsheet>, std::filesystem::path);
void setStatus(std::string, Status::Kind);
void runInBackground(TaskRunner::Work);
void subscribe(SessionObserver*); void unsubscribe(SessionObserver*);
const SessionView& view() const;               // только чтение для TUI
```

**Ловушка, которую надо обработать:** `setMode` вызывается изнутри `mode->handleKey(...)`.
Если сразу заменить `unique_ptr`, текущий объект режима уничтожится, пока выполняется его
метод → use-after-free (ASan поймает). Решение: `setMode` кладёт режим в `pendingMode_`,
а `Session::handleKey` применяет его **после** возврата из `mode->handleKey`.
Обязательно тест на это под ASan.

### 4.2 Режимы — State
`Mode { name(); handleKey(Session&, std::string_view key); }`
- `NormalMode` — спрашивает `Keymap`, создаёт `Action` через `ActionRegistry`, выполняет.
  Печатный символ без привязки → начать редактирование, буфер = этот символ.
- `EditMode` — `enter` → `applyEdit(SetCellCommand)` и курсор вниз; `esc` → отмена;
  `backspace`, `left`/`right` внутри буфера (курсор в строке формулы), печатные символы — вставка.
  `ParseError` → остаёмся в `EditMode`, ввод не теряется, ошибка в статусе.
- `CommandMode` — ввод после `:`; `enter` → разбор `имя арг...` → действие по имени
  (с алиасами: `w`→`save`, `e`→`open`, `q`→`quit`, `q!`→`force-quit`, `import`→`import-csv`, `help`→`show-help`).
- `HelpMode` — любая клавиша возвращает в `NormalMode`. Текст справки строится из `Keymap`
  + `Action::description()`, т.е. новое действие автоматически попадает в справку.

### 4.3 Действия — Command + Factory
```cpp
class Action {
public:
    virtual ~Action() = default;
    virtual std::string_view name() const = 0;
    virtual std::string_view description() const = 0;
    virtual void execute(Session&, std::span<const std::string> args) const = 0;
};
```
Список этапа 1: `move-up/down/left/right`, `page-up/page-down`, `go-home` (столбец A),
`go-top` (A1), `start-edit`, `clear-cell`, `command-mode`, `show-help`, `save` (`[path]`),
`open` (`path`), `import-csv` (`path`), `quit`, `force-quit`.
`quit` при `dirty` → сообщение «есть несохранённые изменения, :q! для выхода».

`ActionRegistry`: `add(name, Factory)`, `addAlias(alias, name)`, `create(name)` (нет → `CommandError`),
`names()` (для справки и валидации конфига). Регистрация — `registerActions(ActionRegistry&)`.

`EditCommand { apply(Spreadsheet&); revert(Spreadsheet&); }` и `SetCellCommand`
(в `apply` запоминает старый `raw`) вводятся уже на этапе 1: все изменения идут через них,
поэтому undo на этапе 2 — это только `UndoStack` и два действия, без правки остального.

### 4.4 Keymap
- Формат конфига (`~/.config/spreadsheet/keys.conf`, путь можно передать аргументом):
  ```
  # клавиша = действие
  ctrl+s = save
  h = move-left
  ```
- `Keymap::defaults()`; `Keymap::load(path, const ActionRegistry&) -> LoadResult{Keymap, std::vector<std::string> warnings}`:
  неизвестное действие, строка без `=`, пустая клавиша → предупреждение (с номером строки),
  строка пропускается, для этой клавиши остаётся значение по умолчанию. Нет файла → тихо
  берутся значения по умолчанию. Предупреждения показываются в строке состояния при старте.
- Имена клавиш — строки: `up down left right enter esc backspace delete tab home end
  pageup pagedown f1..f12`, `ctrl+<x>`, одиночный UTF-8 символ.

Клавиши по умолчанию (попадут в README): стрелки и `hjkl` — перемещение; `enter`/`f2` — правка;
`delete` — очистить; `:` — команда; `?`/`f1` — справка; `ctrl+s` — сохранить; `ctrl+q` — выйти;
`pageup`/`pagedown`; `home` — столбец A; `ctrl+home` — A1.

### 4.5 Viewport
`{ firstRow, firstCol, visibleRows, visibleCols }`, `ensureVisible(CellAddress)` —
минимальная прокрутка так, чтобы курсор был виден; `resize` пересчитывает и снова вызывает
`ensureVisible`. Ширина столбцов на этапе 1 фиксированная. `[→ этап 2: ширины столбцов]`

### 4.6 Фоновые задачи
```cpp
class MainThreadDispatcher {            // реализует TUI (screen.Post), в тестах — очередь
public:
    virtual ~MainThreadDispatcher() = default;
    virtual void post(std::function<void()>) = 0;
};
class TaskRunner {
public:
    // Work выполняется в фоне и возвращает продолжение, которое выполнится в главном потоке.
    using Work = std::function<std::function<void()>(std::stop_token)>;
    explicit TaskRunner(MainThreadDispatcher&);
    void submit(Work);
    ~TaskRunner();   // request_stop + join всех потоков (std::jthread)
};
```
- Исключение в фоне ловится, превращается в продолжение «показать ошибку в статусе».
- `open`/`import-csv`: в фоне читается файл в новый `Spreadsheet` (только чтение реестров —
  потокобезопасно), продолжение вызывает `replaceDocument`. Пока идёт загрузка — статус
  «Загрузка…», интерфейс работает.
- Продолжение не должно обратиться к уже уничтоженной `Session`: `TaskRunner` — **последний**
  член `Session` (уничтожается первым и дожидается потоков), а продолжения держат
  `std::weak_ptr<Alive>`-токен сеанса и ничего не делают, если он истёк.
- Состояние `Session` меняется **только** в главном потоке. Никаких мьютексов вокруг документа.

### 4.7 Что получает TUI (контракт для следующего плана)
- `SessionView` (только чтение): `cursor()`, `viewport()`, `modeName()`, `status()`,
  `editBuffer()`, `formulaBarText()`, `helpLines()`, `quitRequested()`.
- `CellPresenter::text(const Spreadsheet&, CellAddress, const NumberFormat&) -> std::string`
  и выравнивание (числа вправо, строки влево) — вся «предметная» логика отображения здесь,
  а не в TUI.
- `SessionObserver::onSessionChanged()` — сигнал «перерисуйся».
- Вход: `handleKey(std::string_view)` и `resize(rows, cols)`.

---

## 5. Структура репозитория

```
CMakeLists.txt
CMakePresets.json            # debug, release, asan (ASan+UBSan)
cmake/
  Warnings.cmake             # ss_set_warnings(target)
  Sanitizers.cmake
  CheckLayering.cmake        # проверка, что core/app не линкуются с ftxui
core/
  CMakeLists.txt             # add_library(ss_core STATIC ...)
  include/core/
    errors.hpp address.hpp value.hpp number_format.hpp
    token.hpp lexer.hpp expr.hpp expr_visitor.hpp parser.hpp
    function.hpp eval_context.hpp function_registry.hpp builtins.hpp
    evaluator.hpp formula_printer.hpp ref_collector.hpp
    cell.hpp cell_input.hpp sheet.hpp dependency_graph.hpp recalculator.hpp
    cell_observer.hpp cell_io.hpp spreadsheet.hpp
    io/file_reader.hpp io/file_writer.hpp io/native_format.hpp
    io/csv_reader.hpp io/format_registry.hpp
  src/ ...                   # по .cpp на заголовок; builtins/ — по файлу на функцию
app/
  CMakeLists.txt             # add_library(ss_app STATIC ...) PUBLIC ss_core
  include/app/
    errors.hpp session.hpp session_view.hpp session_observer.hpp status.hpp viewport.hpp
    mode.hpp modes/{normal,edit,command,help}_mode.hpp
    action.hpp action_registry.hpp actions/*.hpp register_actions.hpp
    edit_command.hpp set_cell_command.hpp
    keymap.hpp task_runner.hpp main_thread_dispatcher.hpp cell_presenter.hpp
  src/ ...
tests/
  CMakeLists.txt
  core/  test_address.cpp test_value.cpp test_lexer.cpp test_parser.cpp
         test_printer.cpp test_evaluator.cpp test_functions.cpp test_graph.cpp
         test_spreadsheet.cpp test_native_format.cpp test_csv.cpp test_extensions.cpp
  app/   test_keymap.cpp test_viewport.cpp test_session_keys.cpp
         test_actions.cpp test_task_runner.cpp
  support/ manual_dispatcher.hpp
testdata/
  tables/   simple.sst  chain.sst  cycle.sst  broken_signature.sst  broken_escape.sst
  csv/      plain.csv  quoted.csv  multiline.csv  broken_quote.csv  empty.csv
  config/   keys_ok.conf  keys_broken.conf
config/keys.conf             # пример с клавишами по умолчанию
```

---

## 6. Фазы работы

Каждая фаза: **сделать → тесты → сборка `-Werror` → `ctest` (обычный и `asan`) → резюме в чате**.
Не переходить к следующей фазе с красными тестами.

### Фаза 0. Каркас сборки
- Корневой `CMakeLists.txt`: C++20, `CMAKE_CXX_STANDARD_REQUIRED ON`, `CMAKE_CXX_EXTENSIONS OFF`,
  `enable_testing()`, подкаталоги `core`, `app`, `tests`.
- `ss_set_warnings(target)`: `-Wall -Wextra -Werror` (+ `-Wpedantic -Wshadow -Wconversion`
  по желанию; если включены — держать чистыми). Применять только к своим целям, не к GoogleTest.
- Пресет `asan`: `-fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=all`.
- GoogleTest через `FetchContent` (зафиксированный тег), `gtest_discover_tests`.
- `CheckLayering.cmake`: после объявления целей прочитать `LINK_LIBRARIES` у `ss_core` и `ss_app`,
  и если там встречается `ftxui` — `message(FATAL_ERROR ...)`.
- Пустой тест-заглушка проходит. Сборка одной командой:
  `cmake --preset debug && cmake --build --preset debug && ctest --preset debug`.
- **Готово, когда:** сборка и `ctest` проходят в `debug` и `asan`.

### Фаза 1. Ошибки, адреса, значения
- `errors.hpp`, `address.hpp/.cpp`, `value.hpp/.cpp`, `number_format.hpp/.cpp`.
- Тесты: `A1`↔(0,0), `Z1`, `AA1`, `ZZ65536`, нижний регистр, `A0`/`1A`/`AAA1`/пусто → `AddressError`;
  нормализация `Range(B2:A1)`, `contains`, `size`; `display` для чисел (`3`, `0.1`, `1e+20`,
  `-0` → `0`), ошибок, строк; `compare` для пар число/строка/пусто.

### Фаза 2. Лексер
- `token.hpp`, `lexer.hpp/.cpp`.
- Тесты: числа (`1`, `1.5`, `.5`, `1e3`, `1E-3`), строки с `""`, незакрытая строка → `ParseError`
  с позицией, ссылки `A1`/`$A$1`/`A$1`/`$A1`, идентификаторы, все операторы включая `<=`, `<>`,
  `>=`, неизвестный символ → `ParseError`.

### Фаза 3. Дерево и «пассивные» посетители
- `expr.hpp`, `expr_visitor.hpp`, `formula_printer`, `ref_collector`, `clone()`.
- Тесты собирают деревья руками (парсера ещё нет): печать с минимально нужными скобками
  (`(1+2)*3`, но `1+2*3`), `clone` даёт независимую копию, `RefCollector` для ссылки, диапазона
  (разворачивается), вложенных функций.

### Фаза 4. Парсер
- `parser.hpp/.cpp`, зависит от `FunctionProvider&`.
- Тесты: приоритеты и ассоциативность (`1-2-3` = `-4`, `2*3+4`), унарный минус (`--1`, `-A1*2`),
  скобки, сравнения, функции с 0/1/N аргументами, диапазоны, неизвестная функция → узел с
  `fn == nullptr`, ошибки (`=1+`, `=(1`, `=SUM(1,)`, `=1 2`, `=A1:`) → `ParseError` с позицией.
  **Свойство-тест:** `parse(print(parse(s)))` печатается так же, как `print(parse(s))`, на ~30 формулах.
  Для тестов — фейковый `FunctionProvider` (проверка DIP: парсер работает без реального реестра).

### Фаза 5. Функции и вычисление
- `function.hpp`, `eval_context.hpp`, `function_registry`, `builtins/` (по файлу на функцию),
  `evaluator`.
- Тесты на фейковом `EvalContext` (map адрес→значение): вся таблица из 3.6 и 3.7,
  распространение ошибок, `IF` не вычисляет невыбранную ветку (`=IF(1, 1, 1/0)` → 1),
  `#NAME?` для неизвестной функции, `#VALUE!` для диапазона вне функции, дубль имени в
  реестре → исключение, регистронезависимость имён.

### Фаза 6. Лист и граф зависимостей
- `cell`, `cell_input`, `sheet`, `dependency_graph`, `recalculator`.
- Тесты графа без формул: цепочка A1←B1←C1 → порядок `A1,B1,C1`; ромб (D1 зависит от B1 и C1,
  оба от A1) → D1 ровно один раз и последним; самоссылка; цикл из 3; ячейка, зависящая от цикла,
  попадает в `cyclic`; разрыв цикла → `plan` снова без `cyclic`; `setDependencies` заменяет
  старые рёбра (нет «висячих» `dependents`).
- `CellInputParser`: `"12"`, `" 12 "` (решить и задокументировать: пробелы обрезаются),
  `"12abc"` → строка, `"'12"` → строка `12`, `"=..."` → формула, `""` → очистка.

### Фаза 7. Фасад Spreadsheet
- `spreadsheet`, `SheetEvalContext`, `cell_observer`, `cell_io` (`CellSink`, `CellSource`).
- Тесты (главные тесты ядра — их покажешь на защите):
  - ввод и чтение чисел/строк/формул, `raw` возвращает исходный текст;
  - пересчёт цепочки после изменения начала;
  - **пересчитываются только затронутые:** тестовый наблюдатель получает ровно ожидаемый набор
    адресов; несвязанная ячейка не пересчитывается (счётчик вычислений через тестовую функцию
    `COUNTCALLS`, зарегистрированную только в тесте — заодно проверка OCP);
  - ввод в пустую ячейку внутри диапазона пересчитывает `SUM`;
  - цикл: `A1=B1`, `B1=A1` → обе `#CYCLE!`; `C1=A1+1` → тоже ошибка; разрыв цикла → нормальные значения;
  - **строгая гарантия:** `setInput(A1, "=1+")` бросает `ParseError`, после чего `raw`, `value`,
    граф и уведомления — как до вызова;
  - очистка ячейки, от которой зависят другие, → зависимые видят пусто/0;
  - наблюдатель, отписывающийся внутри `onCellsChanged`, не ломает обход.

### Фаза 8. Файлы
- `io/*`, `registerFormats(FormatRegistry&)`.
- Тесты: сохранить → загрузить → те же `raw` и значения (включая escape-последовательности и
  юникод); каждый битый файл из `testdata/` → `FileFormatError` с верным номером строки;
  CSV: кавычки, `""`, перевод строки в кавычках, строки разной длины, пустой файл, незакрытая
  кавычка → ошибка; выбор формата по расширению (`.SST`, `.csv`), неизвестное → ошибка;
  формула из CSV остаётся текстом.

### Фаза 9. Keymap и ошибки app
- `app/errors.hpp`, `keymap`, `config/keys.conf`.
- Тесты: значения по умолчанию; загрузка корректного файла; битый файл → нужные предупреждения
  с номерами строк и сохранённые умолчания; отсутствующий файл → умолчания без предупреждений;
  комментарии и пустые строки.

### Фаза 10. Session, Viewport, режимы
- `session`, `session_view`, `status`, `viewport`, `mode`, `modes/*`, `edit_command`,
  `set_cell_command`, `cell_presenter`, `tests/support/manual_dispatcher.hpp`.
- Тесты-сценарии нажатиями (`session.handleKey(...)` по списку строк):
  - `"1","2","enter"` → A1 = 12, курсор на A2;
  - `"enter"`, `"=","1","+","esc"` → ячейка не изменилась, режим Normal;
  - ввод `=1+` и `enter` → режим Edit сохранён, статус Error, буфер не потерян;
  - курсор не уходит за A1 и за границы листа; `Viewport` прокручивается при выходе курсора за
    видимую область и после `resize`;
  - смена режима изнутри режима — под ASan без ошибок;
  - `?` → Help, любая клавиша → Normal.

### Фаза 11. Действия и командная строка
- `action`, `action_registry`, `actions/*`, `register_actions`.
- Тесты: каждое действие по имени; `:w path` сохраняет, `:e path` открывает, `:import file.csv`;
  `:q` при `dirty` отказывает, `:q!` выходит; неизвестная команда → статус Error, программа живёт;
  справка содержит описание каждого зарегистрированного действия.

### Фаза 12. Фоновая загрузка
- `task_runner`, интеграция в `open`/`import-csv`.
- Тесты с `ManualDispatcher`: после `submit` документ ещё старый, после завершения работы и
  `drain()` — новый; битый файл → старый документ на месте, статус Error; уничтожение `Session`
  с незавершённой задачей — без утечек и обращений к освобождённой памяти (ASan/LSan).

### Фаза 13. Учебные расширения и документация
Проверка OCP — каждое расширение делается **только новым файлом + строкой регистрации**.
Делать в тестовом коде или отдельной веткой, чтобы показать на защите (и потренироваться самому
без ИИ — защита потребует ровно этого):
1. функция `COUNT` (новый `Function`);
2. формат `.tsv` для чтения (новый `FileReader`);
3. действие `go-end` — к последней заполненной строке (новый `Action` + клавиша в конфиге);
4. формат чисел `FixedFormat(2)` (новый `NumberFormat`) — подготовка к этапу 2.

Если для любого из них пришлось менять существующий класс — это дефект архитектуры, исправить.

Обновить документы:
- `DESIGN.md`: внести отклонения из раздела 7, описать `compare` с двойной диспетчеризацией,
  отложенную смену режима, строгую гарантию `setInput`, модель потоков; диаграммы привести к коду.
- `README.md`: сборка одной командой, запуск, клавиши по умолчанию, формат `.sst`, особенности
  импорта CSV.

---

## 7. Отклонения от DESIGN.md (согласовать с автором до начала работы)

| Было в DESIGN.md | Стало в плане | Почему (какой принцип) |
|---|---|---|
| `FileFormat { load; save }` | `FileReader` + `FileWriter` | CSV на этапе 1 только читается — иначе `save` была бы заглушкой (LSP, ISP) |
| `load(istream, Spreadsheet)` | `load(istream, CellSink&)` / `save(ostream, const CellSource&)` | формату не нужен весь фасад (ISP) |
| `Parser` → `FunctionRegistry&` | `Parser` → `FunctionProvider&` | парсер зависит от абстракции, тестируется с фейком (DIP) |
| `FunctionRegistry::withBuiltins()` | свободная `registerBuiltins(FunctionRegistry&)` | реестр не должен знать конкретные функции (OCP, DIP) |
| `FunctionCallExpr` владеет `unique_ptr<Function>` | `shared_ptr<const Function>` | функции без состояния, а `Expr::clone()` должен копировать узел без доступа к реестру |
| `EvalContext { valueAt, evaluate }` | + `evaluateFlat(const Expr&)` | `SUM(A1:A3, 5)` нужно развернуть диапазон, не зная типа узла (без `dynamic_cast`) |
| `Spreadsheet ..|> EvalContext` | внутренний `SheetEvalContext` | `evaluate` не должен быть публичным методом фасада (ISP, SRP) |
| нет | `CellInputParser`, `Recalculator` | вынесены из фасада (SRP) |
| `DependencyGraph::affectedInOrder` + `cycleThrough` | `plan(changed) -> {order, cyclic}` | один обход графа вместо двух, нет рассинхронизации |
| `Action::execute(Session)` | `execute(Session&, span<const string> args)` | командам `:w file`, `:e file` нужны аргументы |
| `EditCommand` — этап 2 | интерфейс и `SetCellCommand` уже на этапе 1 | все изменения сразу идут через команды → undo на этапе 2 без правки старого кода (OCP) |
| нет | `MainThreadDispatcher`, `SessionView`, `CellPresenter` | `app` не зависит от FTXUI (DIP), TUI не содержит логики |

---

## 8. Definition of Done для этого плана

- [ ] `cmake --preset debug && cmake --build --preset debug && ctest --preset debug` — зелёное, без предупреждений.
- [ ] То же для пресета `asan` — ни одного отчёта ASan/UBSan/LSan.
- [ ] `ss_core` и `ss_app` не линкуются с FTXUI (проверка CMake) и не включают его заголовки (`grep` пуст).
- [ ] Нет `new`/`delete`, `dynamic_cast`, `typeid`, синглтонов, глобальных изменяемых переменных (`grep`).
- [ ] У всех полиморфных баз виртуальный деструктор, у переопределений `override`.
- [ ] Все требования этапа 1 из задания покрыты тестами ядра: разбор, вычисление, пересчёт только затронутых, циклы, файлы, CSV, некорректный ввод.
- [ ] Четыре учебных расширения из фазы 13 добавлены без правки существующих классов.
- [ ] `DESIGN.md` и `README.md` соответствуют коду.
- [ ] Пользователь может своими словами объяснить: владение `Expr`/`Value`/`Mode`, строгую гарантию `setInput`, порядок пересчёта, почему Visitor, почему отложенная смена режима.
